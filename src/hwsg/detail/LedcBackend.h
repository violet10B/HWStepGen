// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

#include "hwsg/Types.h"
#include "hwsg/detail/SocCaps.h"

namespace hwsg {
namespace detail {

// Hands out LEDC channels such that no two steppers share a timer (channels
// on the same timer are forced to the same frequency). Thread-safe.
class ChannelAllocator {
 public:
  // Returns the channel, or -1 when every timer is taken.
  static int8_t acquire();

  // Reserve a specific channel; fails if it or its timer is in use.
  static bool reserve(uint8_t channel);

  static void release(uint8_t channel);

  // Number of steppers that could still be started.
  static uint8_t available();

 private:
  static uint32_t usedChannels_;
  static uint32_t usedTimers_;
};

// LEDC-based square-wave generator for one STEP pin.
class LedcBackend {
 public:
  LedcBackend() = default;
  ~LedcBackend() { detach(); }

  LedcBackend(const LedcBackend &) = delete;
  LedcBackend &operator=(const LedcBackend &) = delete;

  // Claim a channel (kAutoChannel allocates) and route it to stepPin,
  // leaving the output idle low.
  Error attach(uint8_t stepPin, int8_t requestedChannel);

  // Stop the pulse train, release the channel, park the pin low.
  void detach();

  // 0 Hz stops the train. actualHz receives what the hardware settled on.
  Error setFrequency(float hz, float &actualHz);

  // Frequency the timer is producing right now, read back from the
  // peripheral. False when not attached.
  bool readFrequency(float &hz) const;

  bool isAttached() const { return attached_; }
  int8_t channel() const {
    return attached_ ? static_cast<int8_t>(channel_) : kAutoChannel;
  }

  static uint8_t availableChannels() { return ChannelAllocator::available(); }

 private:
  // Timer frequency between attach() and the first setFrequency(); the duty
  // stays at 0 so no pulses are emitted.
  static constexpr uint32_t kIdleFrequencyHz = 1000;

  uint8_t pin_ = 0;
  uint8_t channel_ = 0;
  bool attached_ = false;
};

}  // namespace detail
}  // namespace hwsg
