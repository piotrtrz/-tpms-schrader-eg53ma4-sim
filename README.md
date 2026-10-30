# Schrader EG53MA4 TPMS Simulator — ESP32-S2-MINI + CC1101

**Version 2.0.0-rc.1 — release candidate without hardware validation.**

A workshop device that transmits four independent TPMS identifiers with the
OOK/Manchester Schrader EG53MA4 profile on 433.92 MHz. By default each wheel
reports **260 kPa (2.6 bar; protocol byte `0x68`)**. The target scenario is
diagnostics of an Opel Astra J Sedan 1.4 LPG (2016, EU), but **compatibility
must be confirmed for the specific car**: via VIN/service listings, markings of
factory sensors, a diagnostic readout, or a reception test. OE number
13506028, related part numbers, or a listing offer alone do not confirm the
radio profile.

> **This is not a homologated sensor nor a replacement for a working TPMS.
> Do not use while driving.** Synthetic pressure can hide a punctured or
> underinflated tyre and delay the warning. Before and after the test, check
> all tyres with an independent gauge. See [SAFETY](docs/SAFETY.md).

## What is done, and what is not

- Done: ESP32 build, frame/NVS/state-machine tests, synthetic CU8 decoded by
  an independent `rtl_433`, persistent configuration from the console, Clone
  IDs and LEARN.
- **Not done:** measurement of a real CC1101, RTL-SDR with an antenna, a test
  with a car, current measurement, and automotive tests. That is why the
  result is RC, not "validated".
- The only base profile is **OOK**. Do not switch to FSK "just to try it".
- The ESP32 does not generate the 433 MHz carrier — a separate CC1101 433 MHz
  is required.

## Concept sketch

```text
  Bench prototype:
  USB-C ──> [ESP32-S2-MINI] ── SPI ──> [CC1101 433 MHz] ──> SMA antenna
                  │      │                     │
                  │      └── GPIO0: button (BOOT)  └── common GND, 3.3 V only
                  └── GPIO15: built-in LED (active HIGH)

  Target supply from the car cigarette-lighter socket:
  variant A: socket ──> [USB car charger] ──> board USB-C
  variant B: socket ──> fuse/TVS ──> [LM317T 3.3 V] ──> 3V3 + CC1101 VCC

  Configuration: USB CDC 115200 → NVS → 4 × {ID, pressure, temperature}
```

Set the LM317T output with the divider to **3.3 V and connect it to 3V3, not
to 5V**. Do not connect 12 V to GPIO, 3V3, or 5V. When powering from an
external 3.3 V source, disconnect USB unless source separation has been
verified for the specific board revision. In most cars the cigarette-lighter
socket is powered only after the ignition is switched on — **check this in
your own car**. Then the emulator draws no current while parked; the car
computer does not read TPMS state with the ignition off anyway, so lacking
parking transmission does not matter. Full pinout, the LM317T divider, and
capacitors: [HARDWARE](docs/HARDWARE.md).

## Quick Start

### 1. Assemble the prototype

Buy the parts from the [BOM](docs/HARDWARE.md#laboratory-prototype-bom).
Connect the CC1101 and the button according to the pin table; the LED is
already on the S2 Mini board. On the bench, power the device through a tested
USB-C cable that carries data. Do not install anything in the car yet.

### 2. Install the tools and verify the project

Required: Python 3, Git, CMake, a C/C++ compiler, `pkg-config`, and the
libusb and librtlsdr headers. The easiest way is Docker:

```bash
docker build -t tpms-rc .
docker run --rm -v "$PWD/artifacts:/work/artifacts" tpms-rc
```

Without Docker, the script creates a `.venv` with PlatformIO, fetches the
pinned `rtl_433`, and the reference recordings:

```bash
bash tools/check_all
```

The last message must read `PASS: all hardware-free tests...`.
It is not a test of physical radio.

### 3. Build and flash

```bash
.venv/bin/pio run -e esp32s2mini
.venv/bin/pio run -e esp32s2mini -t upload
.venv/bin/pio device monitor --baud 115200
```

If the port is not detected automatically, add `--upload-port /dev/ttyACM0`
(for `run`) or `--port /dev/ttyACM0` (for `device monitor`). Depending on the
system and USB mode it may also be `/dev/ttyUSB0`. In the PlatformIO monitor,
the exit shortcut is usually `Ctrl+]`. If the upload does not start, hold
**BOOT**, press and release **RESET**, then release BOOT.

After startup, type `stay 300` and press Enter. You have 5 minutes to
configure; the `info` command shows active values. Every configuration change
is saved to NVS immediately and survives a restart — **do not edit IDs in
code**.

### 4A. Preferred: Clone IDs

If you know the original 24-bit IDs (e.g. from an earlier `rtl_433` readout),
enter them in FL, FR, RR, RL order. Example — replace the values with your
own:

```text
stay 300
id 1 2FC871
id 2 2EAAF9
id 3 3495AC
id 4 2FC780
pressure all 260
info
send
```

An ID must have six hex digits, be non-zero, differ from `FFFFFF`, and be
unique. The device returns to NORMAL and transmits the known IDs. **Learning
may be unnecessary, but that is not guaranteed.** Confirm reception
diagnostically; do not start with a road drive.

### 4B. Alternative: New IDs + LEARN

1. Type `ids new`, then `info`, and write down the generated IDs.
2. Start the TPMS learning procedure appropriate for your car model/version
   according to its service instructions. Do not guess menu/pedal sequences.
3. Type `mode learn`. It transmits FL only, every 2 s. The LED shows 1 blink.
4. `send` forces the current wheel without changing it. After confirming the
   step in the car, type `next`: FR → RR → RL. A short button press does the
   same; after RL it returns to NORMAL.
5. If 60 s passes on a wheel, LEARN interrupts to NORMAL. The whole session
   has a 15 min limit. When finished, verify the IDs and diagnostic codes.

EL-50448/OEC-T5 is a 125 kHz LF activator, **not an ESP32 programmer**. This
procedure does not require it. Details: [SETUP](docs/SETUP.md).

## Controls — one table

The LED works only while awake; in deep sleep it is off. After a wheel change,
a series of 1–4 blinks indicates FL/FR/RR/RL.

| Mode | LED pattern | Short button | Long ≥1.5 s | Very long ≥4 s |
|---|---|---|---|---|
| NORMAL | 1 blink / 2 s | burst of all 4 | enter LEARN | enter SINGLE |
| LEARN | 3 blinks / 0.5 s; changes with wheel number | next wheel + TX; after RL done | exit to NORMAL | enter SINGLE |
| SINGLE | 2 blinks / 0.5 s; changes with wheel number | next wheel + TX | enter LEARN | exit to NORMAL |

In the console, `send` in NORMAL transmits all wheels, and in LEARN/SINGLE
only the current one. `send 1`…`send 4` select a wheel explicitly; `send all`
transmits four. SINGLE shuts off after 5 min.

## Most common problems

- **`ERROR: no CC1101`**: check 3.3 V, common ground, and SCK/MISO/MOSI/CS
  on GPIO4/5/6/7 and GDO0 on GPIO10; do not use 5 V on the CC1101.
- **Console disappears before you enter commands**: right after restart type
  `stay 300`; a plain RTC wake deliberately does not open the console window.
- **NVS reports an error**: the device uses safe defaults; type `info`.
  `factory YES` erases your IDs and restores the factory configuration.
- **The car does not receive**: first verify the car profile and RTL-SDR
  reception; do not randomly change modulation, flags, or intervals. See
  [TROUBLESHOOTING](docs/TROUBLESHOOTING.md).

## Validation, power, and release

- Protocol and unknowns: [PROTOCOL](docs/PROTOCOL.md)
- Detailed results and the HIL checklist: [VALIDATION](docs/VALIDATION.md)
- Consumption (estimates so far): [POWER](docs/POWER.md)
- LM317T and power from the cigarette-lighter socket:
  [HARDWARE](docs/HARDWARE.md#cigarette-lighter-socket-variant--requirements-unvalidated-design)
- Changes: [CHANGELOG](CHANGELOG.md)

Release images are produced by `bash tools/package_release.sh`. The files and
addresses are described in
[VALIDATION](docs/VALIDATION.md#artifacts-and-flashing). The release candidate
does not include a CC1101 test result or a car test.
