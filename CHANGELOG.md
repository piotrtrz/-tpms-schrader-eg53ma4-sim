# Changelog

## Work in progress after 2.0.0-rc.1

- Micro-naps instead of one long sleep (`config.h`: `NAP_PERIOD_MS`):
  the LED blinks approximately every 2 s also between transmissions, and after
  every timer wake a blink is visible (`NAP_AWAKE_MS`). Sleep is the remainder
  of the period after boot time + window; TX of frames remains on the interval
  (sleep counter in RTC memory, `interval` takes effect on the next cycle).
  `power_deep_sleep` takes milliseconds. Previously, waking from the timer
  turned the LED off in the same `loop()` — the blink lasted a few ms and the
  wake-up was invisible.
- Button: the board's BOOT button by default (`PIN_BUTTON = 0`, GPIO0) —
  EXT1 behaviour confirmed on hardware. An external button on GPIO3 is
  selected with `-DPIN_BUTTON=3`. Note: GPIO0 is a strapping pin (held at
  power-up/reset it starts the bootloader). Every button gesture opens a 10 s
  console window (per `POWER.md`) — the device does not fall asleep
  immediately after, e.g., leaving LEARN/SINGLE. LEARN/SINGLE still do not
  sleep (unchanged).
- Calibration of the pressure field `P` against the car display: the rtl_433
  scale (2.5 kPa/bit) is not confirmed (the captures have `P=0`), and the car
  showed 284 kPa for 260 kPa. The scaling correction
  `TPMS_PRESSURE_KPA100_PER_RAW` (273 = 2.73 kPa/bit; `P=95` for 260 kPa) was
  applied in the firmware, the Python model, and the `verify_rtl433.py`
  expectations. Confirmed on hardware: 260 kPa → 260 kPa on the car display.
- Hardware–firmware integration audit (2026-10-28), consistency of
  ESP32-S2-MINI + CC1101 SMA + LM317T:
  - EXT1 wake: the RTC pulls for the button and LF are configured before deep
    sleep. A digital pull-up (`pinMode`) does not work with the RTC_PERIPH
    domain disabled (ESP-IDF documentation), so the wake pins floated and
    EXT1 could wake randomly. An external 10 kΩ is also recommended.
  - Console: number parsing rejects NaN and infinity (previously
    `pressure nan` / `rf freq nan` passed validation; NaN ended up in NVS as
    pressure, and `rf freq nan` blocked later writes). The parsing logic was
    extracted into the portable `parse` module with native tests.
  - Power: ADC divider R2 270 kΩ → 180 kΩ. The previous divider saturated the
    ADC (2.5 V full scale) already above about 11.8 V and did not measure the
    12 V installation under a 12.6–14.4 V operating range.
  - `tools/package_release.sh`: the full `merge_bin` image uses `--flash-freq
    80m` consistent with the `lolin_s2_mini` manifest; the previous `40m`
    overwrote the bootloader header (80 MHz) with a parameter that differed
    from the build.
  - `tools/tpms_frame.py`: temperature saturation to 0–255 consistent with
    the firmware (temperatures ≤ −19 °C corrupted the frame in the Python
    model).
  - Tests: the negative checksum case now inverts the whole Manchester cell
    (previously it exercised a Manchester violation instead of the sum); a new
    `test_parse` suite (ranges, NaN/inf); corrected the bit diagram in
    `PROTOCOL.md`.
- Target hardware changed to ESP32-S2-MINI (ESP32-S2FN4R2, e.g.
  ARD-ESP32-S2-MINI, board `lolin_s2_mini`); updated the PlatformIO target,
  native USB CDC, SPI/button/ADC pinout, and built-in LED polarity
  (GPIO15, active HIGH). Supply ADC on GPIO1, optional LF on GPIO14.
  `btStop()` is compiled only on chips with Bluetooth — the ESP32-S2 has none.
- CC1101 433 MHz SPI radio module with an SMA connector and antenna
  (ARD-CC1101-ANT).
- External power through an adjustable LM317T (TO-220 metal, 1.25–37 V,
  1.5 A): the correct ADJ–VOUT–IN pinout, TAB = VOUT, a 240/390 Ω divider
  giving about 3.28 V, capacitors, and protection and thermal requirements.
- Target power from the car cigarette-lighter socket: a USB charger (variant
  A) or the LM317T (variant B). With an ignition-switched socket the emulator
  draws no current while parked; the lack of parking transmission is
  acceptable because the car ECU does not read TPMS with the ignition off.
- ESP32-S2 release images use `esptool --chip esp32s2` and a bootloader at
  address `0x1000`; older images for ESP32-C3 and ESP32-WROOM are incompatible.

## 2.0.0-rc.1 — 2026-10-27

Release candidate without a physical CC1101 test and a car test.

### Added

- Persistent NVS configuration with versioning, CRC32, range validation,
  unique IDs, factory reset, and safe defaults.
- Console: Clone IDs, a new-ID generator, pressures/temperature, intervals,
  limited RF, LEARN/SINGLE, and diagnostics without recompilation.
- Portable state machine with fake clock/radio, timeouts, and the
  FL→FR→RR→RL order. Restart, ACC, radio-failure, and `millis()` wrap tests.
- Independent validation of four reference recordings and synthetic CU8 with
  the pinned rtl_433; negative checksum/Manchester/truncation tests.
- `tools/check_all`, Dockerfile, Gitea Actions, image scripts, and SHA-256.
- User, hardware, power, safety, and HIL documentation.

### Changed

- Defaults of 260 kPa and −10 dBm, OOK only. The 30-byte Manchester encoder
  is preserved and passes independent decoding.
- The CC1101 waits for `MARCSTATE=IDLE`, checks underflow and leftover bytes,
  instead of relying only on timing and `TXBYTES=0`.
- A plain RTC wake does not open the console window; transmission and the
  return to sleep limit the estimated consumption. The console window remains
  open after user interaction.
- Full build with `-Wall -Wextra -Werror`; cppcheck with zero defects.

### Removed / fixed

- Removed the guessed `TPMS_FLAGS_LOWBAT_MASK` and the coupling between the
  emulator's low supply and the simulated sensor. The supply is only a local
  warning.
- Removed FSK from the main flow: no evidence for the analysed profile.
- Removed `PARKING_SHUTDOWN`; every deep sleep has an RTC alarm, so an ACC
  restart cannot leave the device permanently asleep.
- The default, unvalidated LF input is disabled. Basic LEARN does not require
  the EL-50448.
- Removed the recommendation of MCP1703 + 24 V TVS as automotive protection
  and the promises of multi-year battery operation. The 12 V supply has
  design requirements, not HIL.

### Audit iterations

1. **Initial state:** 7/7 native and build PASS, but configuration through
   recompilation, a monolithic state machine, lowbat, and power needed
   repair. Created `archive/pre-opencode`.
2. **Data model:** added NVS/CRC/validation and portable modes. The first
   test caught wrong test expectations for a pending tick and immediate TX;
   after correction, 20/20 native and the ESP32 build PASS.
3. **RF validation:** an audit of four recordings confirmed 16 repetitions
   and variable gaps. A single vector was replaced with the full script;
   64/64 references, 3/3 synthetic, and 3/3 negative PASS.
4. **Energy:** a console window after every RTC wake was detected; it was
   limited to interaction. Consumption is still clearly marked as estimated.
5. **Static analysis:** cppcheck flagged memset-initialised float structs;
   replaced with value initialisation and reached 0 defects.

### Not physically verified

CC1101 on an antenna/analyser, RTL-SDR with the prototype, consumption,
brownout, ACC, a flash update, Clone IDs and LEARN with the specific Opel. No
"validated" label.
