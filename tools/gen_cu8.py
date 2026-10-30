#!/usr/bin/env python3
"""CU8 synthesis of OOK/Manchester for an independent rtl_433 test (no RF)."""
import argparse
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tpms_frame import build_row, build_tx_bits, CHIP_US

SAMPLE_RATE = 250000


def synth(bits: str, amp: float, noise: float, rng: random.Random):
    samples_per_chip = SAMPLE_RATE * CHIP_US / 1e6
    phase = 0.0
    for n in range(int(round(len(bits) * samples_per_chip))):
        chip = bits[int(n / samples_per_chip) % len(bits)]
        amplitude = amp if chip == "1" else 0.0
        phase += 0.1  # any carrier inside the IQ band; the demodulator uses amplitude
        yield (amplitude * math.cos(phase) + rng.gauss(0, noise),
               amplitude * math.sin(phase) + rng.gauss(0, noise))


def write_cu8(path, samples):
    with open(path, "wb") as output:
        for i, q in samples:
            output.write(bytes((max(0, min(255, int(round(i + 127.5)))),
                                max(0, min(255, int(round(q + 127.5)))))))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--id", default="2FC871")
    parser.add_argument("--pressure", type=float, default=260.0)
    parser.add_argument("--temp", type=float, default=23.0)
    parser.add_argument("--flags", default="4C900070")
    parser.add_argument("--frames", type=int, default=3)
    parser.add_argument("--gap-ms", type=float, default=50.0)
    parser.add_argument("--amp", type=float, default=60.0)
    parser.add_argument("--noise", type=float, default=2.0)
    parser.add_argument("--corrupt", choices=("checksum", "manchester", "truncate"))
    parser.add_argument("-o", "--out", required=True)
    args = parser.parse_args()
    if not 1 <= args.frames <= 32 or not 20 <= args.gap_ms <= 500:
        parser.error("frames=1..32, gap-ms=20..500")

    row = build_row(int(args.id, 16), args.pressure, args.temp, int(args.flags, 16))
    if args.corrupt == "checksum":
        row = row[:-1] + ("0" if row[-1] == "1" else "1")
    bits = build_tx_bits(row)
    if args.corrupt == "manchester":
        bits = bits[:20] + "00" + bits[22:]
    if args.corrupt == "truncate":
        bits = bits[:-24]

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    rng = random.Random(1)
    gap = [(0.0, 0.0)] * int(SAMPLE_RATE * args.gap_ms / 1000)
    output = []
    for _ in range(args.frames):
        output.extend(synth(bits, args.amp, args.noise, rng))
        output.extend(gap)
    write_cu8(args.out, output)
    print(f"{args.out}: {len(output) / SAMPLE_RATE:.3f} s, {args.frames} frames")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
