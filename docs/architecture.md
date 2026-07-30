# Architecture

## Layout

```
src/
  HWStepGen.h              umbrella header, version macros, ESP32 guard
  hwsg/
    Types.h  Types.cpp     Error, Direction, EnablePolarity, StepperConfig
    Stepper.h  .cpp        one axis: pins, direction, speed
    StepperBank.h  .cpp    non-owning group operations
    Ramp.h  .cpp           non-blocking velocity profiles with easing
    detail/
      SocCaps.h            per-variant capability detection
      Ledc.h               Arduino core 2.x / 3.x compatibility shim
      LedcBackend.h  .cpp  LEDC pulse generator + channel allocator
      Backend.h            selects which backend Stepper is built on
```

Everything under `detail/` is private and may change without a major version
bump. It is only in the include path because `Stepper` stores its backend by
value.

## Design rules

- No heap allocation anywhere. `StepperBank` is a fixed array sized from the
  chip's timer count; `Stepper` owns its backend by value.
- No hidden ownership. `Stepper` is neither copyable nor movable, and
  `StepperBank` stores raw pointers to steppers the sketch declared. Handing
  out references into a growable container invites use-after-free; an
  immovable type removes that class of bug.
- Errors are values. Nothing throws, aborts or prints; callers decide how
  loud a failure is.
- Chip capabilities come from `soc_caps.h` and `GPIO_IS_VALID_OUTPUT_GPIO`,
  never from a hardcoded per-variant table, so new ESP32 variants work
  without touching the library.
- `setFrequency()` compares what the hardware actually produced against the
  request and reports `FrequencyOutOfRange` past `HWSG_FREQUENCY_TOLERANCE`.
  The compile-time min/max frequencies are advisory only.

## Core 2.x / 3.x compatibility

Arduino core 3.0 removed `ledcSetup()`, `ledcAttachPin()` and
`ledcDetachPin()` and re-centred the LEDC API on pins instead of channels.
`detail/Ledc.h` is the only file that knows this; its wrappers take both a
pin and a channel and discard the unused one at compile time.

Two facts about the core drive the rest of the design:

- `ledcWriteTone()` forces 10-bit duty resolution on every call, in both core
  versions. A resolution setting in the public API would be silently
  overwritten, so there is none, and the usable frequency range is fixed at
  `source_clock / 1024` down to `source_clock / 1048576`.
- LEDC channels are paired onto timers (`timer = (channel / 2) % 4`,
  `group = channel / 8`). Two channels on one timer cannot run at different
  frequencies, so `ChannelAllocator` hands out one channel per timer. The
  motor limit is half the channel count for this reason.

## The backend interface

`Stepper` holds a `detail::StepBackend`, which `detail/Backend.h` aliases to
`LedcBackend` by default. It is a compile-time policy stored by value — no
vtable, no indirect call on the speed-setting path. A backend provides:

```cpp
Error   attach(uint8_t stepPin, int8_t requestedChannel);
void    detach();
Error   setFrequency(float hz, float &actualHz);
bool    isAttached() const;
int8_t  channel() const;
static uint8_t availableChannels();
```

It must be default-constructible, release its hardware in the destructor, and
leave the STEP pin driven low while stopped. Select an alternative with
`-DHWSG_BACKEND_HEADER='"my/Backend.h"' -DHWSG_BACKEND_TYPE=my::Backend`.

## Precise positioning

The most requested feature, and the one thing the LEDC backend cannot do.

LEDC is a PWM generator: it emits a square wave until told to stop and keeps
no record of how many edges it produced. There is no counter to read, no
per-pulse interrupt, and no "emit N cycles" mode. Integrating the commanded
frequency over time drifts with every timer quantisation, which is fine for a
readout and useless for an axis. Positioning therefore needs a different
peripheral behind the backend interface, not a feature on top of LEDC.

Options, roughly in order of attractiveness:

- **RMT** emits a programmed sequence of pulses with individual durations,
  which gives an exact step count and hardware-timed acceleration in one
  mechanism. Long moves need buffer refills from a threshold interrupt. RMT
  exists on every variant this library targets (channel counts vary: 8 on the
  ESP32, 4 TX on the S3, 2 TX on the C3/H2).
- **MCPWM + PCNT**: PCNT counts the pulses and gates MCPWM through its fault
  input, so the axis stops in hardware with zero interrupt latency. Most
  precise, but MCPWM is missing on the S2 and C3. This is what
  FastAccelStepper uses on the classic ESP32.
- **LEDC + PCNT**: route the LEDC output to a PCNT input through the GPIO
  matrix and stop from a watch-point callback. Cheapest to build on what
  exists, but stopping happens in an ISR, so it overshoots by the interrupt
  latency, and the C3 has no PCNT.
- **Timer ISR** toggling the pin: universal and exact, but costs CPU at high
  step rates and reintroduces the jitter this library exists to avoid.

Anyone needing positioning today should use
[FastAccelStepper](https://github.com/gin66/FastAccelStepper), which has done
hardware-timed positioning across the ESP32 family for years. For adding it
here, the path is an RMT backend behind the existing interface, with the
positioning API only advertised by backends that can support it. Open an
issue before starting so the API shape can be agreed on first.
