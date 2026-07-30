# Contributing

Bug reports, documentation fixes and new backends are welcome.

## Setup

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/).

```bash
git clone https://github.com/violet10B/HWStepGen.git
cd HWSG
pio run -e esp32dev          # builds examples/BasicSingleMotor
```

`platformio.ini` builds one example at a time; change `src_dir` to pick
another, or build any example on any board directly:

```bash
pio ci examples/SpeedRamp \
  --board=esp32dev \
  --project-option="platform=espressif32@6.10.0" \
  --project-option="framework=arduino" \
  --project-option="lib_deps=symlink://$PWD"
```

`pio run` with no arguments builds every environment, matching CI. The core
3.x environments pull the
[pioarduino](https://github.com/pioarduino/platform-espressif32) platform,
which is a large first download; the core 2.x environments (`esp32dev`,
`esp32-s2`, `esp32-s3`, `esp32-c3`) are enough for day-to-day work.

## Pull requests

- Build at least one core 2.x and one core 3.x target. The LEDC API differs
  between them and it is easy to break one while fixing the other.
- Run clang-format (`.clang-format` is in the repo, CI enforces it).
- Keep the build warning-free under `-Wall -Wextra`.
- Update `CHANGELOG.md`, and the docs your change invalidates.

## Constraints

These are design decisions, not suggestions:

- No heap allocation. If a change needs `new`, `malloc` or `std::vector`, it
  needs a different design.
- No exceptions, no `abort()`, no printing. Fallible operations return
  `hwsg::Error` and the caller decides what a failure means.
- Nothing blocks beyond the driver's DIR setup time.
- Chip capabilities come from `soc_caps.h`. Per-variant `#if`s are a last
  resort and need a comment explaining why.

Public API changes need doc comments in the header; the headers are the
reference documentation.

## Backends

Alternative step generators (RMT, MCPWM+PCNT) plug in behind the interface
described in [docs/architecture.md](docs/architecture.md#the-backend-interface)
without touching `Stepper`. Open an issue first — for positioning support in
particular, the API shape needs agreement before code.

## Bug reports

Include the chip variant, Arduino core version, driver, and a minimal
sketch. The issue template asks for these to keep the round trips short.

## Releasing

1. Bump the version in `library.json`, `library.properties` and the
   `HWSTEPGEN_VERSION*` macros in `src/HWStepGen.h`.
2. Update `CHANGELOG.md`.
3. Tag `vX.Y.Z` and push the tag.

The project follows [Semantic Versioning](https://semver.org/). Anything
under `src/hwsg/detail/` is private and may change in a minor release.
