// SPDX-License-Identifier: MIT

#include "hwsg/Stepper.h"

#include <Arduino.h>
#include <math.h>

#include "hwsg/detail/SocCaps.h"

namespace hwsg {

namespace {

bool pinsCollide(int8_t a, int8_t b) { return a != kNoPin && a == b; }

float stepsPerSecondToRpm(float hz, uint32_t stepsPerRevolution) {
  return (hz * 60.0f) / static_cast<float>(stepsPerRevolution);
}

}  // namespace

Stepper::~Stepper() { end(); }

Error Stepper::begin(const StepperConfig &config) {
  if (started_) {
    return Error::AlreadyStarted;
  }

  if (!detail::isValidOutputPin(config.stepPin)) {
    return Error::InvalidStepPin;
  }
  if (!detail::isValidOutputPin(config.dirPin)) {
    return Error::InvalidDirPin;
  }
  if (config.enablePin != kNoPin &&
      !detail::isValidOutputPin(config.enablePin)) {
    return Error::InvalidEnablePin;
  }
  if (pinsCollide(config.stepPin, config.dirPin) ||
      pinsCollide(config.stepPin, config.enablePin) ||
      pinsCollide(config.dirPin, config.enablePin)) {
    return Error::PinConflict;
  }
  if (config.stepsPerRev == 0) {
    return Error::InvalidStepsPerRev;
  }
  if (config.microsteps == 0) {
    return Error::InvalidMicrosteps;
  }

  config_ = config;
  requestedHz_ = 0.0f;
  actualHz_ = 0.0f;

  pinMode(static_cast<uint8_t>(config_.dirPin), OUTPUT);
  writeDirection(Direction::Forward);

  if (config_.enablePin != kNoPin) {
    pinMode(static_cast<uint8_t>(config_.enablePin), OUTPUT);
    // Keep the driver disabled until the LEDC channel is known-good, so a
    // failed begin() cannot leave the coils energised.
    writeEnable(false);
  } else {
    enabled_ = true;
  }

  const Error attached = backend_.attach(static_cast<uint8_t>(config_.stepPin),
                                         config_.ledcChannel);
  if (attached != Error::Ok) {
    return attached;
  }

  if (config_.enablePin != kNoPin) {
    writeEnable(config_.startEnabled);
  }

  started_ = true;
  return Error::Ok;
}

void Stepper::end() {
  if (!started_) {
    return;
  }
  backend_.detach();
  if (config_.enablePin != kNoPin) {
    writeEnable(false);
  }
  requestedHz_ = 0.0f;
  actualHz_ = 0.0f;
  started_ = false;
}

Error Stepper::setStepFrequency(float hz) {
  if (!started_) {
    return Error::NotStarted;
  }
  if (isnan(hz)) {
    return Error::FrequencyOutOfRange;
  }

  sequenceDirectionChange(hz < 0.0f ? Direction::Reverse : Direction::Forward);

  const float magnitude = fabsf(hz);
  float achieved = 0.0f;
  const Error result = backend_.setFrequency(magnitude, achieved);
  if (result != Error::Ok) {
    // Stop rather than run at whatever the peripheral clamped the request to.
    backend_.setFrequency(0.0f, achieved);
    requestedHz_ = 0.0f;
    actualHz_ = 0.0f;
    return result;
  }

  actualHz_ = achieved;
  requestedHz_ = achieved > 0.0f ? magnitude : 0.0f;
  return Error::Ok;
}

Error Stepper::setSpeed(float rpm) {
  if (!started_) {
    return Error::NotStarted;
  }
  const float hz = (rpm * static_cast<float>(stepsPerRevolution())) / 60.0f;
  return setStepFrequency(hz);
}

void Stepper::stop() {
  if (!started_) {
    return;
  }
  float achieved = 0.0f;
  backend_.setFrequency(0.0f, achieved);
  requestedHz_ = 0.0f;
  actualHz_ = 0.0f;
}

Error Stepper::setDirection(Direction direction) {
  if (!started_) {
    return Error::NotStarted;
  }
  if (direction == direction_) {
    return Error::Ok;
  }

  const float resumeHz = requestedHz_;
  sequenceDirectionChange(direction);
  if (resumeHz > 0.0f) {
    return setStepFrequency(direction == Direction::Reverse ? -resumeHz
                                                            : resumeHz);
  }
  return Error::Ok;
}

void Stepper::setEnabled(bool enabled) {
  if (config_.enablePin == kNoPin) {
    return;
  }
  writeEnable(enabled);
}

float Stepper::speedRpm() const {
  const float rpm = stepsPerSecondToRpm(requestedHz_, stepsPerRevolution());
  return direction_ == Direction::Reverse ? -rpm : rpm;
}

float Stepper::actualSpeedRpm() const {
  const float rpm = stepsPerSecondToRpm(actualHz_, stepsPerRevolution());
  return direction_ == Direction::Reverse ? -rpm : rpm;
}

uint32_t Stepper::stepsPerRevolution() const {
  const uint32_t steps =
      static_cast<uint32_t>(config_.stepsPerRev) * config_.microsteps;
  // Guard the not-yet-begun state; begin() rejects zero for both factors.
  return steps == 0 ? 1 : steps;
}

float Stepper::minStepFrequencyHz() { return detail::minStepFrequencyHz(); }

float Stepper::maxStepFrequencyHz() { return detail::maxStepFrequencyHz(); }

float Stepper::minRpm() const {
  return stepsPerSecondToRpm(minStepFrequencyHz(), stepsPerRevolution());
}

float Stepper::maxRpm() const {
  return stepsPerSecondToRpm(maxStepFrequencyHz(), stepsPerRevolution());
}

uint8_t Stepper::availableChannels() {
  return detail::StepBackend::availableChannels();
}

void Stepper::writeDirection(Direction direction) {
  const bool level =
      (direction == Direction::Forward) != config_.invertDirection;
  digitalWrite(static_cast<uint8_t>(config_.dirPin), level ? HIGH : LOW);
  direction_ = direction;
}

// DIR must be stable across the active STEP edge, or the driver loses steps.
// Halt pulses, flip DIR, wait out the driver's setup time, resume.
void Stepper::sequenceDirectionChange(Direction direction) {
  if (direction == direction_) {
    return;
  }
  const bool wasRunning = isRunning();
  if (wasRunning) {
    float achieved = 0.0f;
    backend_.setFrequency(0.0f, achieved);
  }
  writeDirection(direction);
  if (wasRunning && config_.dirSetupTimeUs > 0) {
    delayMicroseconds(config_.dirSetupTimeUs);
  }
}

void Stepper::writeEnable(bool enabled) {
  const bool level =
      (config_.enablePolarity == EnablePolarity::ActiveHigh) == enabled;
  digitalWrite(static_cast<uint8_t>(config_.enablePin), level ? HIGH : LOW);
  enabled_ = enabled;
}

}  // namespace hwsg
