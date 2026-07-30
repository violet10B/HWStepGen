// SPDX-License-Identifier: MIT

#include "hwsg/Types.h"

namespace hwsg {

const char *toString(Error error) {
  switch (error) {
    case Error::Ok:
      return "Ok";
    case Error::InvalidStepPin:
      return "InvalidStepPin";
    case Error::InvalidDirPin:
      return "InvalidDirPin";
    case Error::InvalidEnablePin:
      return "InvalidEnablePin";
    case Error::PinConflict:
      return "PinConflict";
    case Error::InvalidStepsPerRev:
      return "InvalidStepsPerRev";
    case Error::InvalidMicrosteps:
      return "InvalidMicrosteps";
    case Error::NoFreeChannel:
      return "NoFreeChannel";
    case Error::AlreadyStarted:
      return "AlreadyStarted";
    case Error::NotStarted:
      return "NotStarted";
    case Error::FrequencyOutOfRange:
      return "FrequencyOutOfRange";
    case Error::BackendFailure:
      return "BackendFailure";
    case Error::BankFull:
      return "BankFull";
    case Error::AlreadyInBank:
      return "AlreadyInBank";
  }
  return "Unknown";
}

}  // namespace hwsg
