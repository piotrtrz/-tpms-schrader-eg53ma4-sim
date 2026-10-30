# Release-v2 audit register

| Priority | Original problem | Repair criteria | Status / evidence |
|---|---|---|---|
| P0 | Guessed low-battery bit dependent on the emulator supply | no mask and no coupling; flags test | fixed; code grep and 20/20 native |
| P0 | MCP1703 + 24 V TVS described as automotive protection | remove the recommendation; requirements and a datasheet example | fixed in HARDWARE |
| P0 | Lack of physical compatibility evidence with the car presented too confidently | RC, explicit HIL, VIN/marking verification | documentation fixed; HIL still open |
| P1 | IDs/pressures/RF only via recompilation | NVS with a console, validation, version/CRC | fixed; test_settings and build |
| P1 | Monolith prevented testing timeouts/ACC | portable state machine, fake clock/radio | fixed; test_app_state |
| P1 | FSK in the base flow without evidence | OOK only in firmware/generator | fixed |
| P1 | PARKING_SHUTDOWN without a guaranteed ACC wake | remove it or guarantee waking | removed; every sleep has RTC |
| P1 | TX ended on time and TXBYTES, tail risk | MARCSTATE/underflow/limit | improved; requires HIL confirmation |
| P1 | Vectors mostly self-authored | four real recordings + independent rtl_433 | 64/64 PASS |
| P2 | Default LF without a validated front end | disabled by default, LEARN without LF | fixed |
| P2 | Intervals presented as OE | explicitly experimental | PROTOCOL/SETUP/POWER |
| P2 | No full single check/build artifacts | check_all, CI, Docker, package_release | added; results in VALIDATION |
| P2 | A console window after every RTC wake raises consumption | sleep right after the RTC cycle | improved; HIL measurement open |
| P2 | README inconsistent with behaviour | new Quick Start, one UI table | rewritten |
| P1 | Digital pull-up of button/LF does not work in deep sleep (EXT1, RTC_PERIPH off) | RTC pulls + HOLD before sleep | fixed in power.cpp; HIL wake still open |
| P1 | `nan` passed console validation (pressure/RF) and reached NVS | rejecting NaN/inf + native tests | fixed; `parse` + test_parse |
| P2 | ADC divider saturating above ~11.8 V — no 12–14.4 V reading | range up to ~16 V | fixed in config.h (R2 180 kΩ); requires measurement |
| P2 | Full release image with `40m` vs build header `80m` | flash parameter consistency | fixed in package_release.sh; header verification |
| P2 | Python model without temperature saturation (discrepancy with C++) | clamp 0–255 + self-test | fixed in tpms_frame.py |
| P3 | Negative checksum test exercised Manchester instead of the sum | invert the whole cell | fixed in test_tpms_frame |
| P3 | Wrong bit offsets in the PROTOCOL.md diagram | correct 72/96/104/112/120 | fixed |
| P1 | Scale of the pressure field `P` (2.5 kPa/bit) unverified — all captures have `P=0`, the car shows 284 kPa for 260 kPa | calibrate against the car display, fix the conversion and tests | fixed; scaling correction 2.73 kPa/bit (`P=95` for 260 kPa), HIL: 260 kPa → 260 kPa on the display; a second measurement point (e.g. 300 kPa) would confirm linearity in range |

## Open, requiring hardware

These cannot be closed by declaration: CC1101/tail/power/frequency
measurement, an RTL-SDR test with an antenna, consumption and brownout,
protected ACC, a Clone IDs/LEARN test with the specific Astra, and the design
and testing of automotive power. They are the checklist in `VALIDATION.md`
and they block the "validated" status.
