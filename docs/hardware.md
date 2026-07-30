# Hardware notes

## Wiring

| Driver pin | ESP32 | Notes |
| --- | --- | --- |
| STEP | any output-capable GPIO | receives the LEDC pulse train |
| DIR | any output-capable GPIO | |
| ENABLE | any output-capable GPIO | optional; tie off in hardware and set `enablePin = kNoPin` |
| GND | GND | common ground is required |

Set the driver's current limit before powering the motor, and never
disconnect the motor while the driver is energised.

Most 5 V drivers accept the ESP32's 3.3 V logic on STEP/DIR, but some TB6600
clones need a level shifter or the series-resistor configuration from their
manual.

### Pin selection

`begin()` rejects pins that cannot be outputs on the running chip (e.g. the
ESP32's input-only GPIO34-39). It cannot know about board-level conflicts, so
also avoid:

- strapping pins (GPIO0, 2, 5, 12, 15 on the ESP32) — a driver pulling one at
  reset changes the boot mode
- flash/PSRAM pins (GPIO6-11 on the ESP32; GPIO33-37 on S3 modules with octal
  PSRAM)
- the USB-Serial/JTAG pins on C3/C6/H2/S3 if you use the built-in USB console

The examples use GPIO 25/26/27, which are free on the boards covered here.

## LEDC constraints

### One timer per motor

The Arduino core binds LEDC channels to timers in pairs (`timer =
(channel / 2) % 4`, `group = channel / 8`), and channels sharing a timer
share a frequency. The library therefore allocates one channel per timer:

| SoC | LEDC channels | Max motors |
| --- | --- | --- |
| ESP32 | 16 (2 groups x 8) | 8 |
| ESP32-S2, S3, P4 | 8 | 4 |
| ESP32-C3, C6, H2 | 6 | 3 |

These numbers are derived from `soc_caps.h` at compile time, not from this
table. `hwsg::Stepper::availableChannels()` is the authoritative answer at
runtime; `begin()` returns `Error::NoFreeChannel` when the chip is out.

### Frequency range

`ledcWriteTone()` reconfigures the timer to 10-bit duty resolution on every
call, in both core 2.x and 3.x, which fixes the achievable range at
`source_clock / 2^10` down to `source_clock / (1024 * 2^10)`. The core picks
the source clock: XTAL where supported, APB (80 MHz) on the classic ESP32.

| SoC | Source clock | Step rate |
| --- | --- | --- |
| ESP32 | 80 MHz (APB) | 76.3 Hz - 78.1 kHz |
| S2, S3, C3, C6, P4 | 40 MHz (XTAL) | 38.1 Hz - 39.1 kHz |
| H2 | 32 MHz (XTAL) | 30.5 Hz - 31.3 kHz |

In rpm, via `stepsPerRev * microsteps`:

| Gearing | ESP32 | S2/S3/C3/C6/P4 | H2 |
| --- | --- | --- | --- |
| 200 x 1 | 22.9 - 23438 | 11.4 - 11719 | 9.2 - 9375 |
| 200 x 8 | 2.9 - 2930 | 1.4 - 1465 | 1.1 - 1172 |
| 200 x 16 | 1.4 - 1465 | 0.7 - 732 | 0.6 - 586 |
| 200 x 32 | 0.7 - 732 | 0.4 - 366 | 0.3 - 293 |

The upper end is a peripheral limit; no motor follows it. The lower end is
the practical constraint: for very slow motion, raise the microstepping.
Going from 1x to 16x lowers the minimum speed 16-fold.

`minRpm()` / `maxRpm()` report the range for the configured gearing, and
`setSpeed()` returns `Error::FrequencyOutOfRange` instead of silently running
at a clamped speed.

### Quantisation

The timer divider has 10 integer and 8 fractional bits, so not every
frequency is exactly reachable. `actualStepFrequencyHz()` and
`actualSpeedRpm()` report what the hardware settled on. Deviations above
`HWSG_FREQUENCY_TOLERANCE` (5 % default) are treated as a failed request.

### Sharing LEDC with other code

`analogWrite()`, servo libraries and LED dimmers draw from the same channel
pool. The library claims channels in `begin()` and releases them in `end()`,
but on core 2.x the Arduino core keeps its own reservations in a file-static
table with no query API, so neither side can see the other. A third party
that takes over a stepper's timer changes the motor's speed silently.

Three things reduce the risk, in order of effort:

- Channels are allocated from the highest index down, while `analogWrite()`
  and the common servo libraries allocate from 0 up. Collisions therefore
  need the two pools to meet in the middle.
- `Stepper::checkTimer()` reads the frequency back from the peripheral and
  reports `Error::TimerConflict` if it no longer matches what was commanded.
  Call it from a housekeeping task to turn a silent takeover into an error.
- `StepperConfig::ledcChannel` pins a specific channel. Keep other code off
  that channel *and* its timer partner (`channel ^ 1`).

Core 3.x registers channels with the core's peripheral manager, so conflicts
there are usually caught by `begin()` and reported as `Error::BackendFailure`.

## Build-time overrides

| Macro | Default | Purpose |
| --- | --- | --- |
| `HWSG_LEDC_CHANNEL_COUNT` | from `soc_caps.h` | override channel detection |
| `HWSG_MAX_STEPPERS` | channels / 2 | `StepperBank` capacity |
| `HWSG_LEDC_SOURCE_CLOCK_HZ` | per variant | corrects the advisory frequency limits |
| `HWSG_FREQUENCY_TOLERANCE` | `0.05f` | accepted request-vs-actual deviation |
| `HWSG_TONE_RESOLUTION_BITS` | `10` | must match the core's `ledcWriteTone()` |
| `HWSG_BACKEND_HEADER` / `HWSG_BACKEND_TYPE` | LEDC | substitute a custom backend |

```ini
build_flags = -DHWSG_FREQUENCY_TOLERANCE=0.02f
```
