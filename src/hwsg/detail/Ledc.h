// SPDX-License-Identifier: MIT
#pragma once

// Compatibility layer over the Arduino core's LEDC API. Core 3.0 removed
// ledcSetup()/ledcAttachPin()/ledcDetachPin() and moved from channels to
// pins; these wrappers take both so call sites compile against either.

#include <Arduino.h>

#include "hwsg/detail/SocCaps.h"

namespace hwsg {
namespace detail {

inline bool ledcInit(uint8_t pin, uint8_t channel, uint32_t freqHz,
                     uint8_t resolutionBits) {
#if HWSG_ARDUINO_CORE_3
  return ledcAttachChannel(pin, freqHz, resolutionBits, channel);
#else
  if (ledcSetup(channel, freqHz, resolutionBits) == 0) {
    return false;
  }
  ledcAttachPin(pin, channel);
  return true;
#endif
}

// Drive the pulse train at freqHz (0 stops it). Returns the frequency the
// hardware settled on, or 0 on failure.
inline uint32_t ledcTone(uint8_t pin, uint8_t channel, uint32_t freqHz) {
#if HWSG_ARDUINO_CORE_3
  (void)channel;
  if (freqHz == 0) {
    ledcWrite(pin, 0);
    return 0;
  }
  return ledcWriteTone(pin, freqHz);
#else
  (void)pin;
  if (freqHz == 0) {
    ledcWrite(channel, 0);
    return 0;
  }
  return static_cast<uint32_t>(ledcWriteTone(channel, freqHz));
#endif
}

// Frequency the timer is producing right now, straight from the hardware.
// Returns 0 when the channel's duty is 0, so only meaningful while running.
inline uint32_t ledcCurrentFreq(uint8_t pin, uint8_t channel) {
#if HWSG_ARDUINO_CORE_3
  (void)channel;
  return ledcReadFreq(pin);
#else
  (void)pin;
  return ledcReadFreq(channel);
#endif
}

// Release the pin from LEDC and park it driven low, so the driver's STEP
// input never floats.
inline void ledcRelease(uint8_t pin, uint8_t channel) {
  (void)channel;
#if HWSG_ARDUINO_CORE_3
  ledcDetach(pin);
#else
  ledcDetachPin(pin);
#endif
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

}  // namespace detail
}  // namespace hwsg
