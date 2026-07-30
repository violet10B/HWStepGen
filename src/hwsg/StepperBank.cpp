// SPDX-License-Identifier: MIT

#include "hwsg/StepperBank.h"

namespace hwsg {

Error StepperBank::add(Stepper &stepper) {
  for (uint8_t i = 0; i < count_; ++i) {
    if (steppers_[i] == &stepper) {
      return Error::AlreadyInBank;
    }
  }
  if (count_ >= kCapacity) {
    return Error::BankFull;
  }
  steppers_[count_++] = &stepper;
  return Error::Ok;
}

bool StepperBank::remove(Stepper &stepper) {
  for (uint8_t i = 0; i < count_; ++i) {
    if (steppers_[i] != &stepper) {
      continue;
    }
    for (uint8_t j = i + 1; j < count_; ++j) {
      steppers_[j - 1] = steppers_[j];
    }
    steppers_[--count_] = nullptr;
    return true;
  }
  return false;
}

void StepperBank::stopAll() {
  for (uint8_t i = 0; i < count_; ++i) {
    steppers_[i]->stop();
  }
}

Error StepperBank::setAllSpeed(float rpm) {
  Error first = Error::Ok;
  for (uint8_t i = 0; i < count_; ++i) {
    const Error result = steppers_[i]->setSpeed(rpm);
    if (result != Error::Ok && first == Error::Ok) {
      first = result;
    }
  }
  return first;
}

Error StepperBank::setAllDirection(Direction direction) {
  Error first = Error::Ok;
  for (uint8_t i = 0; i < count_; ++i) {
    const Error result = steppers_[i]->setDirection(direction);
    if (result != Error::Ok && first == Error::Ok) {
      first = result;
    }
  }
  return first;
}

void StepperBank::setAllEnabled(bool enabled) {
  for (uint8_t i = 0; i < count_; ++i) {
    steppers_[i]->setEnabled(enabled);
  }
}

bool StepperBank::anyRunning() const {
  for (uint8_t i = 0; i < count_; ++i) {
    if (steppers_[i]->isRunning()) {
      return true;
    }
  }
  return false;
}

}  // namespace hwsg
