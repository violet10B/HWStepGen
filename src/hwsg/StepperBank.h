// SPDX-License-Identifier: MIT
#pragma once

#include <stdint.h>

#include "hwsg/Stepper.h"
#include "hwsg/Types.h"

namespace hwsg {

/// Non-owning group of steppers, mainly so an emergency-stop path can hit
/// every axis with one call. Stores pointers; the steppers live wherever
/// they were declared and must outlive the bank.
class StepperBank {
 public:
  static constexpr uint8_t kCapacity = HWSG_MAX_STEPPERS;

  StepperBank() = default;

  Error add(Stepper &stepper);
  bool remove(Stepper &stepper);
  void clear() { count_ = 0; }

  uint8_t size() const { return count_; }
  bool empty() const { return count_ == 0; }

  /// nullptr when out of range.
  Stepper *at(uint8_t index) const {
    return index < count_ ? steppers_[index] : nullptr;
  }

  /// Halt every member. Drivers stay energised and hold position.
  void stopAll();

  /// Returns the first error encountered; every member is still attempted.
  Error setAllSpeed(float rpm);
  Error setAllDirection(Direction direction);

  void setAllEnabled(bool enabled);
  bool anyRunning() const;

  // Range-for support: for (hwsg::Stepper *m : bank) ...
  Stepper *const *begin() const { return steppers_; }
  Stepper *const *end() const { return steppers_ + count_; }

 private:
  Stepper *steppers_[kCapacity] = {};
  uint8_t count_ = 0;
};

}  // namespace hwsg
