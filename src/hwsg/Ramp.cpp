// SPDX-License-Identifier: MIT

#include "hwsg/Ramp.h"

#include <Arduino.h>
#include <math.h>

namespace hwsg {

namespace {

float clamp01(float t) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  return t >= 1.0f ? 1.0f : t;
}

}  // namespace

float ease(Easing easing, float t) {
  t = clamp01(t);
  switch (easing) {
    case Easing::Linear:
      return t;
    case Easing::SmoothStep:
      return t * t * (3.0f - 2.0f * t);
    case Easing::SmootherStep:
      return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    case Easing::Sinusoidal:
      return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * t));
  }
  return t;
}

void Ramp::attach(Stepper &stepper) {
  stepper_ = &stepper;
  currentRpm_ = stepper.speedRpm();
  targetRpm_ = currentRpm_;
  segmentStartRpm_ = currentRpm_;
  segmentDurationMs_ = 0;
  ramping_ = false;
  lastError_ = Error::Ok;
}

void Ramp::detach() {
  stepper_ = nullptr;
  ramping_ = false;
}

void Ramp::setAcceleration(float rpmPerSecond) {
  if (rpmPerSecond > 0.0f) {
    acceleration_ = rpmPerSecond;
  }
}

void Ramp::setEasing(Easing easing) {
  if (easing == easing_) {
    return;
  }
  easing_ = easing;
  if (ramping_) {
    restartSegment(millis());
  }
}

void Ramp::setTargetSpeed(float rpm) {
  targetRpm_ = rpm;
  restartSegment(millis());
}

void Ramp::emergencyStop() {
  targetRpm_ = 0.0f;
  currentRpm_ = 0.0f;
  segmentStartRpm_ = 0.0f;
  segmentDurationMs_ = 0;
  ramping_ = false;
  if (stepper_ != nullptr) {
    stepper_->stop();
  }
}

bool Ramp::update() { return update(millis()); }

bool Ramp::update(uint32_t nowMs) {
  if (stepper_ == nullptr || !ramping_) {
    return false;
  }

  // Unsigned subtraction stays correct across the millis() rollover.
  const uint32_t elapsedMs = nowMs - segmentStartMs_;

  float rpm;
  if (segmentDurationMs_ == 0 || elapsedMs >= segmentDurationMs_) {
    rpm = targetRpm_;
    ramping_ = false;
  } else {
    const float progress =
        static_cast<float>(elapsedMs) / static_cast<float>(segmentDurationMs_);
    rpm = segmentStartRpm_ +
          (targetRpm_ - segmentStartRpm_) * ease(easing_, progress);
  }

  currentRpm_ = rpm;
  lastError_ = stepper_->setSpeed(rpm);
  return ramping_;
}

void Ramp::restartSegment(uint32_t nowMs) {
  segmentStartRpm_ = currentRpm_;
  segmentStartMs_ = nowMs;

  const float delta = fabsf(targetRpm_ - segmentStartRpm_);
  if (delta <= 0.0f) {
    segmentDurationMs_ = 0;
    ramping_ = false;
    return;
  }

  const float milliseconds = (delta / acceleration_) * 1000.0f;
  segmentDurationMs_ =
      milliseconds < 1.0f ? 0u : static_cast<uint32_t>(milliseconds + 0.5f);
  ramping_ = true;
}

}  // namespace hwsg
