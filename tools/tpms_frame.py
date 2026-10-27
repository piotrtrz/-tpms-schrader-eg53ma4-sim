#!/usr/bin/env python3
"""Reference frame builder for the Schrader EG53MA4 TPMS simulator.

This tool mirrors bit-for-bit what the ESP32 firmware (src/tpms_frame.c)
puts on air, and contains a copy of rtl_433's OOK_MC_ZEROBIT slicer
(pulse_slicer_manchester_zerobit() from src/pulse_slicer.c) so the frame
can be self-checked without any radio hardware.

Protocol reference: rtl_433 src/devices/schraeder.c (schrader_EG53MA4),
"Schrader TPMS EG53MA4, Saab, Opel, Vauxhall, Chevrolet", and the reference
capture rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01/gfile001.cu8.

On-air frame (G.E.T. Manchester, chip = one half bit-cell):

    lead chip '1' + 119 two-chip cells + zero padding to a byte boundary

    cell "01" -> data bit 0, cell "10" -> data bit 1   (G.E.T. MC, no inversion)

The rtl_433 MC_ZEROBIT slicer synthesises the very first bit of the decoded
row (a hardcoded 0), so the transmitter sends the row *without* its first bit.
Decoded row (120 bits) = 40 bit sync (zeros) + 80 bit payload:

    SSSS SSSS SSSS SSSS SSSS SSSS SSSS SSSS | FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF
    IIII IIII IIII IIII IIII IIII | PPPP PPPP | TTTT TTTT | CCCC CCCC

- S: sync (40 bits, not checked by the decoder, kept as in real captures)
- F: flags (32 bit) - status / battery nibble, unanalysed upstream
- I: sensor id (24 bit)
- P: pressure, 25 mbar per bit (kPa = byte * 2.5)
- T: temperature, degrees Fahrenheit
- C: checksum = sum(bytes[0..8]) & 0xff

DISCLAIMER: this is a diagnostic/test tool. It generates *artificial* tyre
pressure data and is no replacement for real TPMS sensors.
"""

import argparse
import math
import sys

# --- protocol constants (keep in sync with src/config.h / src/tpms_frame.c) ---
CHIP_US = 120.0            # one Manchester half bit-cell, 8.333 kBaud chip rate
FRAME_BITS = 120           # decoded row length expected by schrader_EG53MA4
SYNC_BITS = 40
TX_BYTES = 30              # 240 chips = 1 lead + 238 data + 1 pad
DEFAULT_FLAGS = 0x4C900070 # captured from real EG53MA4 sensors


def cell(bit: int) -> str:
    """G.E.T. Manchester cell for one data bit."""
    return "01" if bit == 0 else "10"


def build_row(sensor_id: int, pressure_kpa: float, temp_c: float,
              flags: int = DEFAULT_FLAGS, low_batt: bool = False,
              flags_lowbat_mask: int = 0x00000008) -> str:
    """Return the 120 decoded row bits as a string of '0'/'1'.

    Uses the same half-up integer rounding as src/tpms_frame.cpp.
    """
    kpa10 = int(math.floor(pressure_kpa * 10.0 + 0.5))
    press_byte = min(max((kpa10 + 12) // 25, 0), 255)      # 2.5 kPa per bit
    tc = int(math.floor(temp_c + 0.5))
    num = 9 * tc + 160                                      # temp_F = (9C+160)/5
    temp_f = (num + 2) // 5 if num >= 0 else -((-num + 2) // 5)
    temp_f = min(max(temp_f, 0), 255)
    if low_batt:
        flags |= flags_lowbat_mask
    payload = [
        (flags >> 24) & 0xFF,
        (flags >> 16) & 0xFF,
        (flags >> 8) & 0xFF,
        flags & 0xFF,
        (sensor_id >> 16) & 0xFF,
        (sensor_id >> 8) & 0xFF,
        sensor_id & 0xFF,
        press_byte,
        temp_f,
    ]
    payload.append(sum(payload) & 0xFF)
    row = "0" * SYNC_BITS
    for b in payload:
        row += f"{b:08b}"
    assert len(row) == FRAME_BITS
    return row


def build_tx_bits(row: str) -> str:
    """Return the chip bit string actually shifted into the CC1101 FIFO.

    240 chips: 1 lead chip + 119 two-chip cells + 1 pad chip = 30 bytes.
    """
    assert len(row) == FRAME_BITS
    bits = "1"                                  # lead chip (carrier on)
    for b in row[1:]:                           # row[0] is synthesised by the receiver
        bits += cell(int(b))
    while len(bits) % 8:                        # pad with 'off' chips (silence for OOK)
        bits += "0"
    return bits


def tx_bytes(bits: str) -> bytes:
    assert len(bits) % 8 == 0
    return bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))


# ---------------------------------------------------------------------------
# Reference slicer: verbatim model of rtl_433 pulse_slicer_manchester_zerobit()
# (src/pulse_slicer.c), used for self-testing the encoder.
# ---------------------------------------------------------------------------
def mc_zerobit_slicer(chips, short_us=123.0, chip_us=CHIP_US, reset_us=300.0):
    runs = []  # (level, length_us)
    cur, cnt = chips[0], 1
    for c in chips[1:]:
        if c == cur:
            cnt += 1
        else:
            runs.append((cur, cnt * chip_us))
            cur, cnt = c, 1
    runs.append((cur, cnt * chip_us))

    pairs = []  # (pulse_us, gap_us)
    i = 0
    while i < len(runs):
        lvl, ln = runs[i]
        if lvl:
            gap = runs[i + 1][1] if i + 1 < len(runs) and not runs[i + 1][0] else 0.0
            pairs.append((ln, gap))
            i += 2
        else:
            pairs.append((0.0, ln))
            i += 1

    row = ["0"]  # hardcoded leading zero bit
    t = 0.0
    for n, (pulse, gap) in enumerate(pairs):
        last = n == len(pairs) - 1
        if pulse + t > short_us * 1.5:
            row.append("1")
            t = 0.0
        else:
            t += pulse
        if last:
            break
        if gap > reset_us:
            break  # end of message
        if gap + t > short_us * 1.5:
            row.append("0")
            t = 0.0
        else:
            t += gap
    return "".join(row)


def decode_row(row: str):
    if len(row) != FRAME_BITS:
        return None
    by = [int(row[SYNC_BITS + 8 * k:SYNC_BITS + 8 * (k + 1)], 2) for k in range(10)]
    return {
        "flags": by[0] << 24 | by[1] << 16 | by[2] << 8 | by[3],
        "id": by[4] << 16 | by[5] << 8 | by[6],
        "pressure_kPa": by[7] * 2.5,
        "temperature_F": by[8],
        "checksum_ok": (sum(by[:9]) & 0xFF) == by[9],
        "bytes": by,
    }


def self_test() -> int:
    """Cross-check against the rtl_433 reference capture (gfile001.cu8)."""
    fails = 0
    ref = build_row(0x2FC871, 0.0, (90 - 32) * 5 / 9, flags=0x4C900070)
    # the capture carries temperature as the raw byte 90 (0x5A) = 90 F
    ref_row = ("0" * 40
               + "01001100100100000000000001110000001011111100100001110001000000000101101000001110")
    if ref_row[40:] != ref[40:]:
        print("FAIL: reference capture row mismatch")
        print("  want:", ref_row)
        print("  got :", ref)
        fails += 1

    # random round trip: encode -> slicer -> data must survive intact
    import random
    random.seed(42)
    for _ in range(500):
        sid = random.randrange(0, 0x1000000)
        kpa = round(random.uniform(0, 637.5), 1)
        tc = random.uniform(-40, 85)
        row = build_row(sid, kpa, tc)
        bits = build_tx_bits(row)
        chips = [int(b) for b in bits]
        out = mc_zerobit_slicer(chips)
        if len(out) != FRAME_BITS or out[SYNC_BITS:] != row[SYNC_BITS:]:
            print("FAIL: round trip", hex(sid), kpa, tc)
            print("  row:", row)
            print("  out:", out)
            fails += 1
            break
        d = decode_row(out)
        if not d["checksum_ok"] or d["id"] != sid:
            print("FAIL: decode", hex(sid), d)
            fails += 1
            break
    print("self-test:", "OK" if fails == 0 else "FAILED")
    return 1 if fails else 0


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--id", default="2FC871", help="sensor id (hex, 24 bit)")
    p.add_argument("--pressure", type=float, default=260.0, help="kPa")
    p.add_argument("--temp", type=float, default=23.0, help="deg C")
    p.add_argument("--flags", default="4C900070", help="flags word (hex)")
    p.add_argument("--low-batt", action="store_true")
    p.add_argument("--self-test", action="store_true")
    args = p.parse_args()

    if args.self_test:
        return self_test()

    sid = int(args.id, 16)
    flags = int(args.flags, 16)
    row = build_row(sid, args.pressure, args.temp, flags, args.low_batt)
    bits = build_tx_bits(row)
    raw = tx_bytes(bits)
    d = decode_row(row)

    print(f"row  (120 bit): {row}")
    print(f"chips({len(bits):3d} bit): {bits}")
    print(f"tx bytes ({len(raw)}): {' '.join(f'{b:02X}' for b in raw)}")
    print(f"payload : {' '.join(f'{b:02X}' for b in d['bytes'])}")
    print(f"checksum: {'OK' if d['checksum_ok'] else 'BAD'}")
    print(f"id={d['id']:06X}  pressure={d['pressure_kPa']:.1f} kPa  "
          f"temperature={d['temperature_F']} F ({args.temp:.0f} C)  flags={d['flags']:08X}")
    out = mc_zerobit_slicer([int(b) for b in bits])
    print(f"slicer check: {'OK' if out[SYNC_BITS:] == row[SYNC_BITS:] and len(out) == FRAME_BITS else 'FAILED'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
