# TPMS EG53MA4 Simulator 2.0.0-rc.1

**Release candidate — 2026-10-27. This is not a "validated" release.**

A diagnostic simulator of four Schrader EG53MA4 sensors for the ESP32-S2-MINI
(ESP32-S2FN4R2) + CC1101 433 MHz with an SMA antenna. The target profile
includes tests with an Opel Astra J Sedan 2016, but compatibility of the
specific car must be confirmed. Synthetic data can hide a tyre fault; do not
use while driving.

## Features

- four persistent, independent 24-bit IDs; 260 kPa by default for every wheel;
- OOK/Manchester 433.92 MHz, low configurable power from −10 dBm;
- Clone IDs and New IDs + LEARN FL→FR→RR→RL, SINGLE/diagnostics;
- NVS configuration over UART without recompilation, version/CRC/
  validation/factory reset;
- button, LED, timeouts, deep sleep with RTC and optional hardware ACC;
- automated native/RF/build tests, Docker, and SHA-256 artifacts.

## Required hardware

ESP32-S2-MINI (ESP32-S2FN4R2), a CC1101 **433 MHz** 3.3 V module with an SMA
connector (e.g. ARD-CC1101-ANT), an antenna, a button, and decoupling. The LED
is built in. Target power from the cigarette-lighter socket: a car USB charger
or an LM317T with a 240/390 Ω divider and proper capacitors; the 12 V variant
requires input protection and tests. Full BOM/pinout: HARDWARE. Neither the
S2 Mini nor the LM317T is a validated automotive solution on its own.

## Installation and update

New installation: `bash tools/check_all`, then
`pio run -e esp32s2mini -t upload`.
Release images: `bash tools/package_release.sh`; addresses and flashing are
described in VALIDATION. Before a full flash, write down your IDs — the NVS
partition may be erased. Verify the configuration after a restart with `info`.
There is no migration from version 1.0 without NVS; enter the IDs in the
console once.

## Test results without hardware (2026-10-28)

- native: 24/24 PASS (after the hardware–firmware integration audit);
- full ESP32 build with `-Werror`: PASS;
- cppcheck: PASS, 0 defects;
- rtl_433: 64/64 reference records, 3/3 synthetic, 3/3 negative: PASS.

Exact commands and pinned commits: VALIDATION/PROTOCOL.

## Known limitations

- The real CC1101 emission, tail, power, and frequency were not measured.
- RTL-SDR with an antenna, consumption measurement, brownout, and the ACC
  test were not performed.
- Clone IDs/LEARN with a car were not performed; OE 13506028 was not confirmed
  for a specific VIN.
- The meaning of all flags remains unknown; there is no guessed low-battery
  bit.
- The schedule is experimental, not confirmed as OE.
- The LM317T and power from the cigarette-lighter socket are not a validated
  automotive design; with power only from an ignition-switched socket there
  is no transmission after the ignition goes off — the car ECU does not
  listen then anyway.
- Gitea Actions requires a working runner; local results do not mean CI PASS.

Until the HIL items in VALIDATION are checked off, the result remains a
release candidate.
