// SPDX-License-Identifier: MIT

#include "hwsg/detail/LedcBackend.h"

#include <Arduino.h>
#include <math.h>

#include "hwsg/detail/Ledc.h"

namespace hwsg {
namespace detail {

namespace {

portMUX_TYPE g_allocatorMux = portMUX_INITIALIZER_UNLOCKED;

// Arduino.h defines bit() as a macro, hence the name.
constexpr uint32_t maskFor(uint8_t index) { return 1UL << index; }

}  // namespace

uint32_t ChannelAllocator::usedChannels_ = 0;
uint32_t ChannelAllocator::usedTimers_ = 0;

int8_t ChannelAllocator::acquire() {
  int8_t acquired = -1;
  portENTER_CRITICAL(&g_allocatorMux);
  for (uint8_t channel = 0; channel < kLedcChannelCount; ++channel) {
    const uint8_t timer = timerIndexForChannel(channel);
    if ((usedChannels_ & maskFor(channel)) || (usedTimers_ & maskFor(timer))) {
      continue;
    }
    usedChannels_ |= maskFor(channel);
    usedTimers_ |= maskFor(timer);
    acquired = static_cast<int8_t>(channel);
    break;
  }
  portEXIT_CRITICAL(&g_allocatorMux);
  return acquired;
}

bool ChannelAllocator::reserve(uint8_t channel) {
  if (channel >= kLedcChannelCount) {
    return false;
  }
  const uint8_t timer = timerIndexForChannel(channel);
  bool reserved = false;
  portENTER_CRITICAL(&g_allocatorMux);
  if (!(usedChannels_ & maskFor(channel)) && !(usedTimers_ & maskFor(timer))) {
    usedChannels_ |= maskFor(channel);
    usedTimers_ |= maskFor(timer);
    reserved = true;
  }
  portEXIT_CRITICAL(&g_allocatorMux);
  return reserved;
}

void ChannelAllocator::release(uint8_t channel) {
  if (channel >= kLedcChannelCount) {
    return;
  }
  portENTER_CRITICAL(&g_allocatorMux);
  usedChannels_ &= ~maskFor(channel);
  usedTimers_ &= ~maskFor(timerIndexForChannel(channel));
  portEXIT_CRITICAL(&g_allocatorMux);
}

uint8_t ChannelAllocator::available() {
  uint8_t free = 0;
  portENTER_CRITICAL(&g_allocatorMux);
  const uint32_t timers = usedTimers_;
  portEXIT_CRITICAL(&g_allocatorMux);
  for (uint8_t timer = 0; timer < kLedcTimerCount; ++timer) {
    if (!(timers & maskFor(timer))) {
      ++free;
    }
  }
  return free;
}

Error LedcBackend::attach(uint8_t stepPin, int8_t requestedChannel) {
  if (attached_) {
    return Error::AlreadyStarted;
  }

  int8_t channel = requestedChannel;
  if (requestedChannel == kAutoChannel) {
    channel = ChannelAllocator::acquire();
    if (channel < 0) {
      return Error::NoFreeChannel;
    }
  } else if (!ChannelAllocator::reserve(
                 static_cast<uint8_t>(requestedChannel))) {
    return Error::NoFreeChannel;
  }

  if (!ledcInit(stepPin, static_cast<uint8_t>(channel), kIdleFrequencyHz,
                HWSG_TONE_RESOLUTION_BITS)) {
    ChannelAllocator::release(static_cast<uint8_t>(channel));
    return Error::BackendFailure;
  }

  // Make sure no pulses run before the sketch asks for them.
  ledcTone(stepPin, static_cast<uint8_t>(channel), 0);

  pin_ = stepPin;
  channel_ = static_cast<uint8_t>(channel);
  attached_ = true;
  return Error::Ok;
}

void LedcBackend::detach() {
  if (!attached_) {
    return;
  }
  ledcTone(pin_, channel_, 0);
  ledcRelease(pin_, channel_);
  ChannelAllocator::release(channel_);
  attached_ = false;
}

Error LedcBackend::setFrequency(float hz, float &actualHz) {
  if (!attached_) {
    return Error::NotStarted;
  }

  // ledcWriteTone() takes integer Hz; below 0.5 rounds to stopped.
  const uint32_t requested =
      (hz < 0.5f) ? 0u : static_cast<uint32_t>(hz + 0.5f);

  if (requested == 0) {
    ledcTone(pin_, channel_, 0);
    actualHz = 0.0f;
    return Error::Ok;
  }

  const uint32_t achieved = ledcTone(pin_, channel_, requested);
  if (achieved == 0) {
    return Error::FrequencyOutOfRange;
  }

  actualHz = static_cast<float>(achieved);

  // Small deviations are timer-divider quantisation; a large one means the
  // request was clamped rather than honoured.
  const float deviation = fabsf(actualHz - static_cast<float>(requested));
  if (deviation > static_cast<float>(requested) * HWSG_FREQUENCY_TOLERANCE) {
    return Error::FrequencyOutOfRange;
  }
  return Error::Ok;
}

}  // namespace detail
}  // namespace hwsg
