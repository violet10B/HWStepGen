// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

namespace hwsg {

constexpr int8_t kNoPin = -1;
constexpr int8_t kAutoChannel = -1;

/// Result of every fallible operation. The library never throws.
enum class Error : uint8_t {
  Ok = 0,
  InvalidStepPin,
  InvalidDirPin,
  InvalidEnablePin,
  /// Two of STEP/DIR/ENABLE refer to the same GPIO.
  PinConflict,
  InvalidStepsPerRev,
  InvalidMicrosteps,
  /// Every LEDC timer is taken, or the requested channel is in use.
  NoFreeChannel,
  AlreadyStarted,
  NotStarted,
  /// The step frequency is outside what the LEDC peripheral can produce
  /// on this chip. See Stepper::minRpm() / maxRpm().
  FrequencyOutOfRange,
  /// The Arduino core rejected the LEDC configuration.
  BackendFailure,
  BankFull,
  AlreadyInBank,
  /// The hardware is running at a different frequency than was commanded,
  /// which means another LEDC user reconfigured this stepper's timer.
  /// Reported only by Stepper::checkTimer().
  TimerConflict,
};

/// Name of the error, for logging. Never null.
const char *toString(Error error);

enum class Direction : uint8_t {
  Forward = 0,
  Reverse = 1,
};

/// Level on the driver's ENABLE pin that energises the coils.
/// A4988, DRV8825 and TMC drivers are active-low.
enum class EnablePolarity : uint8_t {
  ActiveLow = 0,
  ActiveHigh = 1,
};

struct StepperConfig {
  int8_t stepPin = kNoPin;
  int8_t dirPin = kNoPin;
  /// kNoPin if ENABLE is tied off in hardware.
  int8_t enablePin = kNoPin;

  /// Full steps per revolution: 200 for 1.8°, 400 for 0.9° motors.
  uint16_t stepsPerRev = 200;

  /// Microstepping factor set on the driver; used for speed conversion.
  uint8_t microsteps = 1;

  /// Swap the meaning of Direction::Forward and Reverse.
  bool invertDirection = false;

  EnablePolarity enablePolarity = EnablePolarity::ActiveLow;

  /// Energise the driver at the end of begin().
  bool startEnabled = true;

  /// Specific LEDC channel, or kAutoChannel to allocate one. Only needed
  /// when coexisting with other LEDC users (analogWrite, servo libs).
  int8_t ledcChannel = kAutoChannel;

  /// Pause after a DIR change before pulses resume. DRV8825 needs 650 ns,
  /// A4988 200 ns; the default leaves margin.
  uint16_t dirSetupTimeUs = 5;
};

}  // namespace hwsg
