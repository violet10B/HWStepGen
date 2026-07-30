// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

#include "hwsg/Stepper.h"
#include "hwsg/Types.h"

namespace hwsg {

/// Shape of the velocity ramp. All curves cover the same speed change in the
/// same time; the value passed to Ramp::setAcceleration() is the average, so
/// peak acceleration differs per curve.
enum class Easing : uint8_t {
  /// Constant acceleration (trapezoidal profile), stepping on and off at the
  /// ends. Peak = 1.0x.
  Linear = 0,
  /// S-curve 3t^2 - 2t^3. Acceleration reaches zero at both ends, jerk does
  /// not. Peak = 1.5x.
  SmoothStep,
  /// S-curve 6t^5 - 15t^4 + 10t^3. Acceleration and jerk both reach zero at
  /// both ends. Peak = 1.875x.
  SmootherStep,
  /// Half cosine. Same endpoint behaviour as SmoothStep. Peak = pi/2.
  Sinusoidal,
};

/// Evaluate an easing curve; t is clamped to [0, 1].
float ease(Easing easing, float t);

/// Walks a Stepper's commanded speed towards a target over time. Steppers
/// stall when asked to jump straight to a high speed; this provides the
/// acceleration without blocking or interrupts. Call update() from loop()
/// every 5-20 ms.
///
/// Speed changes reconfigure the LEDC timer, which can clip the pulse in
/// flight, so a ramp may lose the odd step. Fine for a conveyor or pump;
/// not suitable for anything that tracks position.
class Ramp {
 public:
  Ramp() = default;

  /// Bind to a stepper (not owned; must outlive the ramp). Resets ramp state.
  void attach(Stepper &stepper);

  /// Unbind. The stepper keeps its current speed.
  void detach();

  bool isAttached() const { return stepper_ != nullptr; }

  /// Average acceleration in rpm/s. Values <= 0 are ignored.
  void setAcceleration(float rpmPerSecond);
  float acceleration() const { return acceleration_; }

  /// Takes effect immediately, re-anchored at the current speed.
  void setEasing(Easing easing);
  Easing easing() const { return easing_; }

  /// Ramp towards rpm; negative runs in reverse. The ramp passes through
  /// zero, so the DIR flip happens at a standstill. Calling this mid-ramp
  /// retargets from the current speed.
  void setTargetSpeed(float rpm);
  float targetSpeed() const { return targetRpm_; }

  void stop() { setTargetSpeed(0.0f); }

  /// Cut the pulse train now, skipping the ramp. For end stops.
  void emergencyStop();

  /// Advance the ramp. Returns true while still ramping.
  bool update();

  /// Same, with an explicit timestamp (useful in tests).
  bool update(uint32_t nowMs);

  float currentSpeed() const { return currentRpm_; }
  bool isRamping() const { return ramping_; }

  /// Result of the last Stepper::setSpeed() the ramp performed.
  Error lastError() const { return lastError_; }

 private:
  void restartSegment(uint32_t nowMs);

  Stepper *stepper_ = nullptr;
  Easing easing_ = Easing::SmoothStep;
  Error lastError_ = Error::Ok;

  float acceleration_ = 200.0f;  // rpm/s
  float currentRpm_ = 0.0f;
  float targetRpm_ = 0.0f;
  float segmentStartRpm_ = 0.0f;

  uint32_t segmentStartMs_ = 0;
  uint32_t segmentDurationMs_ = 0;
  bool ramping_ = false;
};

}  // namespace hwsg
