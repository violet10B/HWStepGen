// SPDX-License-Identifier: MIT
#pragma once

// Selects the step-pulse backend Stepper is built on. The backend is a
// compile-time policy stored by value, not a virtual interface. A backend
// provides:
//
//   Error   attach(uint8_t stepPin, int8_t requestedChannel);
//   void    detach();
//   Error   setFrequency(float hz, float &actualHz);
//   bool    isAttached() const;
//   int8_t  channel() const;
//   static uint8_t availableChannels();
//
// It must be default-constructible, release its hardware in the destructor,
// and leave the STEP pin driven low while stopped. Substitute your own with
// -DHWSG_BACKEND_HEADER='"my/Backend.h"' -DHWSG_BACKEND_TYPE=my::Backend.

#ifdef HWSG_BACKEND_HEADER
#include HWSG_BACKEND_HEADER
#else
#include "hwsg/detail/LedcBackend.h"
#endif

namespace hwsg {
namespace detail {

#ifdef HWSG_BACKEND_TYPE
using StepBackend = HWSG_BACKEND_TYPE;
#else
using StepBackend = LedcBackend;
#endif

}  // namespace detail
}  // namespace hwsg
