# Validation

## One command without hardware

```bash
bash tools/check_all
```

The script runs the Python self-test, native tests, the full ESP32 build,
cppcheck, fetching the pinned sources (on first run), and CU8 decoding by the
real `rtl_433`. Logs: `artifacts/validation/`. Every stage has `pipefail`,
and an error ends the script with a non-zero code.

A reproducible alternative:

```bash
docker build -t tpms-rc .
docker run --rm -v "$PWD/artifacts:/work/artifacts" tpms-rc
```

## Results from 2026-10-28

Environment: Linux, Python 3.14.4 on the host test; PlatformIO 6.1.18;
espressif32 8.0.0; Arduino ESP32 3.3.12; SmartRC CC1101 commit
`de6757ef633b3e626dae5c7835cdb66ab53f9a1d`; rtl_433 commit `406f9a3…`;
rtl_433_tests commit `1aa8c29…`. Full SHAs are in `PROTOCOL.md`.

| Command | Result |
|---|---|
| `pio test -e native` | **24/24 PASS** — frame, four independent vectors, negative tests, NVS, console parsing (NaN/ranges), fake clock/radio, Clone IDs, LEARN, SINGLE, ACC, restart |
| `pio run -e esp32s2mini` | **PASS**, own warnings treated as errors (`-Werror`) |
| `pio check -e native ... --fail-on-defect=low` | **PASS**, 0 defects after the struct initialisation fix |
| `python3 tools/tpms_frame.py --self-test` | **PASS**, capture + 500 frames |
| `tools/verify_rtl433.sh` | **PASS**, 64/64 reference records; 3/3 synthetic; 3/3 negative rejected |
| `git diff --check` | **PASS** |
| `bash tools/package_release.sh` | **PASS**, ESP32-S2 image with the bootloader at `0x1000` and a SHA-256 manifest |

Reference data: each of `gfile001`…`gfile004` produced exactly 16 matching
records, with the expected ID/flags/pressure/temperature. The synthetic frames
2FC871/260 kPa/73 F, 3495AC/220 kPa/23 F and ABCDEE/262.5 kPa/75 F were
decoded by an independent `rtl_433`. Damaged Manchester, a wrong sum, and
truncation produced no EG53MA4 record.

## Coverage matrix

| Layer | Automated | Physical RTL-SDR | Physical CC1101 | Car |
|---|---|---|---|---|
| fields, checksum, quantisation | done | not needed for the field | not needed | confirmation of ECU interpretation |
| Manchester/preamble/length | done | **not done** | **not done** | not done |
| real recordings → rtl_433 | done, 64/64 | n/a | n/a | n/a |
| synthetic CU8 → rtl_433 | done | n/a | bypasses hardware | n/a |
| CC1101 registers/tail/power/frequency | code audit | required | **required** | required |
| NVS, IDs, intervals, modes | done; NVS emulated with a blob | a restart test is required | required | required |
| LEARN FL→FR→RR→RL | state machine PASS | auxiliary | required | **not done** |
| consumption/brownout/ACC | ACC in the state machine; no measurements | n/a | required | required |

**There is no basis for a "validated" label.** Gitea Actions has a workflow,
but the runner state and the remote CI result must be checked; a local PASS
is not pretend CI.

## HIL checklist — requires a user and hardware

### CC1101 + analyser/RTL-SDR

- [ ] Read the registers after initialisation: OOK, 433.92 MHz, fixed 30 B,
  `SYNC_MODE=0`, no CRC/Manchester/whitening/FEC/append.
- [ ] On GDO0/ANT confirm exactly 30 B, a ~120 µs chip, a full last chip, and
  no hardware preamble/sync/CRC/extra bytes.
- [ ] Record TX-ERR/underflow; there should be none on a correct RF path.
- [ ] Measure the frequency and relative power at −10 dBm; do not assume the
  calibration of a cheap module. Do not raise the PA unnecessarily.
- [ ] `send 1`…`send 4`: RTL-SDR decodes the correct IDs, 260 kPa and flags.
- [ ] Repeat after a reset and 10 deep sleep cycles.

### NVS and inputs

- [ ] Set your own IDs/pressures/RF, disconnect power for >1 min, and re-check
  `info` and transmission.
- [ ] Verify rejection of a duplicate ID, a bad power, and a bad interval.
- [ ] `factory YES` restores 260 kPa and the default RF.
- [ ] Brownout/crank on a protected supply: no hang or configuration damage;
  write settings only after the supply stabilises.
- [ ] If ACC is installed: measure the GPIO voltage for OFF/ON/transients,
  check the intervals and automatic resumption after ACC ON. Skip without
  ACC.

### The specific Opel Astra J

- [ ] Confirm the profile via VIN/markings/diagnostics before transmitting.
- [ ] Clone IDs: reception of the four known IDs without assuming learning
  will vanish.
- [ ] New IDs: full learning FL→FR→RR→RL with one active wheel.
- [ ] DTC and TPMS data read before/after; record the ECU version and
  procedure.
- [ ] Position test: centre console/behind the glovebox, possibly the boot;
  while parked, without guessing the receiver location.
- [ ] A tyre gauge check of all wheels before and after. Do not perform the
  test as a road drive with the simulator in place of a working TPMS.

## Artifacts and flashing

```bash
bash tools/package_release.sh
```

The `artifacts/release/` directory:

| File | Address |
|---|---:|
| `bootloader.bin` | `0x1000` — the address for ESP32-S2, not `0x0` as in ESP32-C3 |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |
| `firmware-full.bin` | `0x0` — the full 4 MB image |
| `firmware.elf` | debug/symbols, do not flash it |
| `SHA256SUMS`, `BUILD-INFO.txt` | integrity and version check |

The full image erases the previous flash while writing the whole area; NVS in
that partition may be lost — write down your IDs before updating with the
full image. Updating only the application `firmware.bin` at `0x10000`
usually preserves NVS, but before a release the user should test it on their
own module.

```bash
PORT=/dev/ttyACM0 bash tools/flash_release.sh
(cd artifacts/release && sha256sum -c SHA256SUMS)
```

Do not flash an image from another toolchain blindly. The RC contains no
physical update test or car test.
