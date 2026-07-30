// SPDX-License-Identifier: MIT
#pragma once

// Per-SoC capability detection, derived from the ESP-IDF soc_caps.h that
// ships with the Arduino core. New ESP32 variants are picked up without
// changes here. Every macro can be overridden from the build system.

#include <stdint.h>

#if defined(__has_include)
// Include the version header directly: this file cannot rely on Arduino.h
// having been seen first, and without ESP_ARDUINO_VERSION_MAJOR the core-3.x
// detection below would silently answer "2.x".
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#if __has_include(<soc/soc_caps.h>)
#include <soc/soc_caps.h>
#endif
#if __has_include(<driver/gpio.h>)
#include <driver/gpio.h>
#endif
#else
#include <soc/soc_caps.h>
#endif

// Core 3.0 replaced the channel-oriented LEDC API with a pin-oriented one.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define HWSG_ARDUINO_CORE_3 1
#else
#define HWSG_ARDUINO_CORE_3 0
#endif

// Mirrors LEDC_CHANNELS in esp32-hal-ledc.c: the classic ESP32 has two speed
// groups of SOC_LEDC_CHANNEL_NUM channels, later variants a single group.
#ifndef HWSG_LEDC_CHANNEL_COUNT
#if defined(SOC_LEDC_CHANNEL_NUM)
#if defined(SOC_LEDC_SUPPORT_HS_MODE) && SOC_LEDC_SUPPORT_HS_MODE
#define HWSG_LEDC_CHANNEL_COUNT (SOC_LEDC_CHANNEL_NUM * 2)
#else
#define HWSG_LEDC_CHANNEL_COUNT (SOC_LEDC_CHANNEL_NUM)
#endif
#else
#define HWSG_LEDC_CHANNEL_COUNT 6  // smallest of any shipping variant
#endif
#endif

// ledcWriteTone() hard-codes 10-bit duty resolution in both core 2.x and
// 3.x, which is why the library exposes no resolution setting: it would be
// overwritten on the first speed change.
#ifndef HWSG_TONE_RESOLUTION_BITS
#define HWSG_TONE_RESOLUTION_BITS 10
#endif

// Clock feeding the LEDC timers. Only used for the advisory limits reported
// by Stepper::minStepFrequencyHz()/maxStepFrequencyHz(); setSpeed() verifies
// the achieved frequency against the hardware at runtime. The Arduino core
// picks XTAL where supported and APB (80 MHz) on the classic ESP32.
#ifndef HWSG_LEDC_SOURCE_CLOCK_HZ
#if defined(SOC_LEDC_SUPPORT_XTAL_CLOCK) && SOC_LEDC_SUPPORT_XTAL_CLOCK
#if defined(CONFIG_IDF_TARGET_ESP32H2)
#define HWSG_LEDC_SOURCE_CLOCK_HZ 32000000UL
#else
#define HWSG_LEDC_SOURCE_CLOCK_HZ 40000000UL
#endif
#else
#define HWSG_LEDC_SOURCE_CLOCK_HZ 80000000UL
#endif
#endif

// One stepper needs one LEDC timer, and the core pairs channels onto timers
// two at a time, so half the channel count is the ceiling.
#ifndef HWSG_MAX_STEPPERS
#define HWSG_MAX_STEPPERS (HWSG_LEDC_CHANNEL_COUNT / 2)
#endif

// Relative deviation between requested and achieved step frequency accepted
// before Error::FrequencyOutOfRange is reported.
#ifndef HWSG_FREQUENCY_TOLERANCE
#define HWSG_FREQUENCY_TOLERANCE 0.05f
#endif

namespace hwsg {
namespace detail {

// Channels pair onto timers: 0+1 -> timer 0, 2+3 -> timer 1, ... Channels on
// the same timer share a frequency, so the timer is the scarce resource.
constexpr uint8_t timerIndexForChannel(uint8_t channel) { return channel / 2; }

constexpr uint8_t kLedcChannelCount = HWSG_LEDC_CHANNEL_COUNT;
constexpr uint8_t kLedcTimerCount = HWSG_LEDC_CHANNEL_COUNT / 2;

constexpr float maxStepFrequencyHz() {
  return static_cast<float>(HWSG_LEDC_SOURCE_CLOCK_HZ) /
         static_cast<float>(1UL << HWSG_TONE_RESOLUTION_BITS);
}

// The timer divider is 10 integer bits, so the slowest rate is max / 1024.
constexpr float minStepFrequencyHz() { return maxStepFrequencyHz() / 1024.0f; }

inline bool isValidOutputPin(int8_t pin) {
  if (pin < 0) {
    return false;
  }
#if defined(GPIO_IS_VALID_OUTPUT_GPIO)
  // Covers per-variant quirks such as the ESP32's input-only GPIO34-39.
  return GPIO_IS_VALID_OUTPUT_GPIO(pin) != 0;
#elif defined(SOC_GPIO_PIN_COUNT)
  return pin < SOC_GPIO_PIN_COUNT;
#else
  return true;
#endif
}

}  // namespace detail
}  // namespace hwsg
