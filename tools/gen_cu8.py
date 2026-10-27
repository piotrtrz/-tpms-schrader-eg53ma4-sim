#!/usr/bin/env python3
"""Synthesise a .cu8 IQ capture of the TPMS simulator's RF output.

The generated file is what an RTL-SDR would record while the ESP32+CC1101
simulator is transmitting. Feed it to rtl_433 to verify the frame format
without any hardware:

    python3 tools/gen_cu8.py --id 2FC871 --pressure 260 --temp 23 -o /tmp/sim.cu8
    rtl_433 -r /tmp/sim.cu8

    expected:
    model     : Schrader-EG53MA4  id : 2FC871  pressure_kPa: 260.0  temperature_F: 73

Modulations:
    ook  ASK/OOK, Manchester data  (default; matches the rtl_433 reference
         capture and the built-in Schrader-EG53MA4 decoder)
    fsk  2-FSK, +/-47.6 kHz deviation, same data (verify with
         rtl_433 -X 'n=schrader,m=FSK_MC_ZEROBIT,s=120,l=120,r=300')

DISCLAIMER: diagnostic/test tool only, generates artificial pressure data.
"""

import argparse
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tpms_frame import build_row, build_tx_bits, CHIP_US  # noqa: E402

SAMPLE_RATE = 250000


def synth(bits: str, mode: str, deviation_hz: float, amp: float, noise: float,
          rng: random.Random):
    """Yield (i, q) int8 pairs for one chip string."""
    sps = SAMPLE_RATE * CHIP_US / 1e6  # samples per chip (~30)
    total = int(round(len(bits) * sps))
    phase = 0.0
    for n in range(total):
        k = int(n / sps) % len(bits)
        bit = bits[k]
        if mode == "fsk":
            freq = deviation_hz if bit == "1" else -deviation_hz
            phase += 2 * math.pi * freq / SAMPLE_RATE
            a = amp
        else:
            a = amp if bit == "1" else 0.0
        i = a * math.cos(phase) + rng.gauss(0, noise)
        q = a * math.sin(phase) + rng.gauss(0, noise)
        yield i, q


def write_cu8(path, samples):
    with open(path, "wb") as f:
        for i, q in samples:
            f.write(bytes((max(0, min(255, int(round(i + 127.5)))),
                           max(0, min(255, int(round(q + 127.5)))))))


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--id", default="2FC871")
    p.add_argument("--pressure", type=float, default=260.0)
    p.add_argument("--temp", type=float, default=23.0)
    p.add_argument("--flags", default="4C900070")
    p.add_argument("--mode", choices=("ook", "fsk"), default="ook")
    p.add_argument("--deviation", type=float, default=47.6, help="FSK deviation kHz")
    p.add_argument("--frames", type=int, default=3)
    p.add_argument("--gap-ms", type=float, default=50.0, help="gap between burst frames")
    p.add_argument("--amp", type=float, default=60.0)
    p.add_argument("--noise", type=float, default=2.0)
    p.add_argument("-o", "--out", default="/tmp/opencode/tpms_ref/sim.cu8")
    args = p.parse_args()

    row = build_row(int(args.id, 16), args.pressure, args.temp, int(args.flags, 16))
    bits = build_tx_bits(row)
    print(f"frame: {len(bits)} chips, tx bytes:"
          f" {' '.join(f'{int(bits[i:i+8],2):02X}' for i in range(0, len(bits), 8))}")

    rng = random.Random(1)
    gap_sps = int(SAMPLE_RATE * args.gap_ms / 1000)
    out = []
    for f in range(args.frames):
        out.extend(synth(bits, args.mode, args.deviation * 1000, args.amp,
                         args.noise, rng))
        out.extend((0.0, 0.0) for _ in range(gap_sps))
    write_cu8(args.out, out)
    print(f"wrote {args.out} ({len(out) / SAMPLE_RATE:.3f} s, {args.frames} frames)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
