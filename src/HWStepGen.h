// SPDX-License-Identifier: MIT
#pragma once

#if !defined(ESP32) && !defined(ARDUINO_ARCH_ESP32)
#error "HWStepGen targets the ESP32 family. Select an ESP32 board."
#endif

#define HWSTEPGEN_VERSION_MAJOR 1
#define HWSTEPGEN_VERSION_MINOR 0
#define HWSTEPGEN_VERSION_PATCH 0
#define HWSTEPGEN_VERSION "1.0.0"

#include "hwsg/Ramp.h"
#include "hwsg/Stepper.h"
#include "hwsg/StepperBank.h"
#include "hwsg/Types.h"
