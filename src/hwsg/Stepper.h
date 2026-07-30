// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

#include "hwsg/Types.h"
#include "hwsg/detail/Backend.h"

namespace hwsg {

/// One step/dir driver channel (A4988, DRV8825, TMC2208/2209, TB6600, ...).
///
/// The STEP pulse train comes from the LEDC peripheral, so once a speed is
/// set the CPU is no longer involved. This is a velocity interface: there is
/// no step counting and no moveTo(). See docs/architecture.md for why.
///
/// A Stepper owns an LEDC channel and is not copyable or movable. Declare it
/// as a global, a static, or a member of a long-lived object.
class Stepper {
 public:
  Stepper() = default;
  ~Stepper();

  Stepper(const Stepper &) = delete;
  Stepper &operator=(const Stepper &) = delete;
  Stepper(Stepper &&) = delete;
  Stepper &operator=(Stepper &&) = delete;

  /// Claim the GPIOs and an LEDC channel. The motor stays stopped; DIR is
  /// set to Forward, and ENABLE (if configured) follows config.startEnabled.
  /// On failure nothing is left claimed and the driver is left disabled.
  Error begin(const StepperConfig &config);

  /// Stop, de-energise (if an ENABLE pin is configured), release the LEDC
  /// channel and park STEP low. Also called by the destructor.
  void end();

  /// Signed speed in rpm; negative runs in reverse. A magnitude below one
  /// step per second stops the motor.
  ///
  /// A sign change halts pulses, flips DIR, waits config.dirSetupTimeUs and
  /// resumes. It does not decelerate first — ramp down before reversing at
  /// speed, or the motor will skip steps.
  Error setSpeed(float rpm);

  /// Signed step frequency in Hz. Same semantics as setSpeed().
  Error setStepFrequency(float hz);

  /// Halt pulses. The driver stays energised and holds position.
  void stop();

  /// Force a direction regardless of the last speed's sign.
  Error setDirection(Direction direction);

  /// Drive the ENABLE pin. Disabled drivers stop holding torque.
  void setEnabled(bool enabled);

  bool isStarted() const { return started_; }
  bool isRunning() const { return requestedHz_ > 0.0f; }
  bool isEnabled() const { return enabled_; }
  Direction direction() const { return direction_; }

  /// Last commanded speed, negative for reverse.
  float speedRpm() const;

  /// Speed the hardware settled on after timer quantisation.
  float actualSpeedRpm() const;

  float stepFrequencyHz() const { return requestedHz_; }
  float actualStepFrequencyHz() const { return actualHz_; }

  /// stepsPerRev * microsteps.
  uint32_t stepsPerRevolution() const;

  const StepperConfig &config() const { return config_; }

  /// LEDC channel in use, or kAutoChannel when not started.
  int8_t channel() const { return backend_.channel(); }

  static float minStepFrequencyHz();
  static float maxStepFrequencyHz();

  /// Slowest non-zero speed for this motor's gearing.
  float minRpm() const;

  /// Fastest speed the peripheral can produce for this gearing. Usually far
  /// beyond what the motor can follow.
  float maxRpm() const;

  /// How many more steppers can be started on this chip.
  static uint8_t availableChannels();

  /// Read the step frequency back from the hardware and compare it with the
  /// commanded one.
  ///
  /// Other LEDC users (analogWrite(), servo and LED libraries) can take over
  /// the timer this stepper owns, which changes the motor's speed with no
  /// error anywhere. Calling this occasionally is how that gets noticed.
  ///
  /// Returns Ok while stopped or when the frequency still matches,
  /// NotStarted before begin(), TimerConflict on a mismatch.
  Error checkTimer() const;

 private:
  void writeDirection(Direction direction);
  void sequenceDirectionChange(Direction direction);
  void writeEnable(bool enabled);

  StepperConfig config_{};
  detail::StepBackend backend_{};
  float requestedHz_ = 0.0f;
  float actualHz_ = 0.0f;
  Direction direction_ = Direction::Forward;
  bool enabled_ = true;
  bool started_ = false;
};

}  // namespace hwsg
