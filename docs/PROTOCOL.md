# Schrader EG53MA4 protocol — facts and unknowns

## Pinned sources

| Source | Version used for validation | Scope |
|---|---|---|
| `rtl_433/src/devices/schraeder.c` | commit `406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab` | decoder and slicer parameters |
| `rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01` | commit `1aa8c29d6b9383739d1aeed495dbc5069b704392` | four CU8 recordings + JSON |
| PlatformIO | 6.1.18 | build/test |
| `espressif32` | 8.0.0 | toolchain/ESP32 platform |
| SmartRC-CC1101-Driver-Lib | `de6757ef633b3e626dae5c7835cdb66ab53f9a1d` (3.0.2) | CC1101 configuration |
| Practical receiver analysis | <https://milek7.pl/tpmsreceiver/> | context; not a source of frame fields |
| ESP32+CC1101 integration pattern | <https://github.com/merbanan/rtl_433/issues/2978> | integration, not the EG53MA4 protocol |

We do not use the unlabelled `master` as a reproducible reference. The
commercial emulator <https://spooftpms.com/us-eu-gm-1-2-family-tpms-simulator/>
only confirms that such a class of device exists, not this project's
compatibility.

## Confirmed format after Manchester decoding

The decoder requires exactly 120 bits: 40 bits of synchronisation and 80 data
bits.

```text
0                   40          72       96      104     112       120
| 40 × sync (0)     | flags 4 B | ID 3 B | P 1 B | T 1 B | sum 1 B |
                    4C 90 00 70  2F C8 71    68      49      65
```

- `flags`: 32 bits; the field meanings are not fully known.
- `ID`: 24 bits; the firmware rejects 0, `FFFFFF`, and duplicates between
  wheels.
- `P`: 2.5 kPa/bit **according to the rtl_433 decoder, but the scale is not
  confirmed** — all four captures have `P = 0`, so no real pressure verified
  the conversion (in the decoder source it is "probable"). A car measurement:
  `P = 104` (260 kPa on the rtl_433 scale) shows up as **284 kPa**, so the
  firmware corrects the scale with the constant
  `TPMS_PRESSURE_KPA100_PER_RAW` (`tpms_frame.h`, default 273 = 2.73 kPa/bit;
  `P = 95` for 260 kPa). Quantisation: `(kPa×100 + 136) / 273`. Calibration
  confirmed on hardware: 260 kPa → 260 kPa on the car display (`AUDIT.md`).
- `T`: byte in °F per the decoder, not °C. 23 °C → 73 °F.
- `sum`: the sum of the nine preceding bytes modulo 256; it is not CRC-8.
- The value `4c900070` occurs in all 64 records of the four captures. That
  does not prove that every sensor/car mode always uses this value.

## Polarity, preamble, and tail

The recordings and the `OOK_PULSE_MANCHESTER_ZEROBIT` slicer confirm G.E.T.
Manchester: `0 → 01`, `1 → 10`. The slicer prepends the first zero bit. The
CC1101 buffer contains:

```text
1 starting chip + 39 sync cells + 80 data cells + 1 OFF padding chip
= 240 chips = 30 bytes, MSB first.
```

The last padding chip is an OOK zero (silence). The test checks exactly this
length and polarity and rejects a truncated/corrupted buffer.

## Timing and repetitions

- The decoder has `short_width=123 µs`, `reset_limit=300 µs`.
- Analysis of `gfile001` shows a typical pulse+gap period of about 236 µs,
  i.e. a chip of about **118 µs**; pulse widths differ from gap widths. The
  firmware uses **120 µs** experimentally, which decodes on an independent
  `rtl_433`. The exact clock of a physical CC1101 requires an HIL measurement.
- Each of the four recordings contains **16 valid repetitions**. In
  `gfile001` the gaps between decode starts vary roughly 115–172 ms; they are
  not constant.
- The defaults of **3 frames, a 50 ms gap, a 30 s cycle** are experimental
  emulator settings, not a confirmed OE schedule. The ACC/parking intervals
  also have no OE-compatibility status.

## Carrier and modulation

The base profile: **433.92 MHz, ASK/OOK**. The recordings are decoded at this
frequency. The kit's README also contains 315 MHz product data and a mention
of ASK+FSK; that is no evidence of FSK in the analysed files nor of
compatibility with the specific Astra. FSK is not available in the base
firmware/generator.

## CC1101 packet engine

The firmware sets fixed-length 30 B, a normal FIFO, MSB first, and disables
the hardware preamble/sync (`SYNC_MODE=0`), Manchester, whitening, FEC, CRC,
address check, and append status. The whole preamble and the protocol's sum
are inside our 30 bytes. End of TX is expected via `MARCSTATE=IDLE` with a
limit; `TXFIFO_UNDERFLOW` and leftover bytes are errors. This prevents the
tail from being truncated by `TXBYTES=0` alone.

The configuration has been audited, but the synthetic CU8 bypasses the
CC1101. The absence of extra bytes and the exact tail still have to be
confirmed on a logic analyser/RTL-SDR in HIL.

## Unknowns — they must not be "tuned" randomly

1. The meaning of individual `flags` bits, including any sensor low-battery
   bit. **`TPMS_FLAGS_LOWBAT_MASK` does not exist in the firmware.**
2. The dependence of flags on learning, motion, LF, and the original sensor's
   battery state.
3. The interval and minimal number of repetitions required by a specific ECU.
4. The compatibility of OE 13506028/a related part number with a specific
   unit without a VIN, markings, a readout, or a test.
5. The physical accuracy of frequency, power, and the tail of the generated
   CC1101 transmission before HIL.

Low emulator supply is only a console warning and does not change any field
of the simulated sensor.
