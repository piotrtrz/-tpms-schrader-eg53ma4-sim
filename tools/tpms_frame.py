#!/usr/bin/env python3
"""Niezalezny model ramki EG53MA4 i testy pomocnicze (OOK/Manchester).

Referencje: rtl_433 commit 406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab,
rtl_433_tests commit 1aa8c29d6b9383739d1aeed495dbc5069b704392.
Narzedzie generuje sztuczne dane wylacznie do diagnostyki.
"""
import argparse
import math

CHIP_US = 120.0
FRAME_BITS = 120
SYNC_BITS = 40
TX_BYTES = 30
DEFAULT_FLAGS = 0x4C900070

# Kalibracja pola P (patrz docs/AUDIT.md): tyle setnych kPa przypada na jeden
# bit bajtu P. Skala rtl_433 (250 = 2,5 kPa/bit) nie jest potwierdzona
# nagraniami (wszystkie maja P=0), a pomiar z auta: 260 kPa -> 284 kPa na
# wyswietlaczu przy P=104 => korekta skalujaca 2,73 kPa/bit (P=95 dla 260).
# Potwierdzone z autem: 260 kPa -> 260 kPa - zmienic tez
# TPMS_PRESSURE_KPA100_PER_RAW w src/tpms_frame.h.
PRESSURE_KPA100_PER_RAW = 273


def press_raw(pressure_kpa: float) -> int:
    kpa10 = int(math.floor(pressure_kpa * 10.0 + 0.5))
    if not 0 <= kpa10 <= 6375:
        raise ValueError("cisnienie musi byc w zakresie 0..637.5 kPa")
    return (kpa10 * 10 + PRESSURE_KPA100_PER_RAW // 2) // PRESSURE_KPA100_PER_RAW


def valid_id(sensor_id: int) -> bool:
    return 0 < sensor_id < 0xFFFFFF


def cell(bit: int) -> str:
    return "01" if bit == 0 else "10"


def build_row(sensor_id: int, pressure_kpa: float, temp_c: float,
              flags: int = DEFAULT_FLAGS) -> str:
    if not valid_id(sensor_id):
        raise ValueError("ID musi byc w zakresie 000001..FFFFFE")
    if not 0 <= flags <= 0xFFFFFFFF:
        raise ValueError("flags musza miec 32 bity")
    kpa10 = int(math.floor(pressure_kpa * 10.0 + 0.5))
    if not 0 <= kpa10 <= 6375:
        raise ValueError("cisnienie musi byc w zakresie 0..637.5 kPa")
    press_byte = press_raw(pressure_kpa)
    tc = int(math.floor(temp_c + 0.5))
    if not -40 <= tc <= 85:
        raise ValueError("temperatura musi byc w zakresie -40..85 C")
    num = 9 * tc + 160
    temp_f = (num + 2) // 5 if num >= 0 else -((-num + 2) // 5)
    # Saturacja zgodna z tpms_frame.cpp: bez niej temperatura <= -19 C daje
    # ujemny bajt i psuje ramke (roznica miedzy modelem Pythona a firmware).
    temp_f = max(0, min(255, temp_f))
    payload = [flags >> 24, (flags >> 16) & 255, (flags >> 8) & 255,
               flags & 255, sensor_id >> 16, (sensor_id >> 8) & 255,
               sensor_id & 255, press_byte, temp_f]
    payload.append(sum(payload) & 255)
    return "0" * SYNC_BITS + "".join(f"{byte:08b}" for byte in payload)


def build_tx_bits(row: str) -> str:
    if len(row) != FRAME_BITS:
        raise ValueError("ramka musi miec 120 bitow")
    bits = "1" + "".join(cell(int(bit)) for bit in row[1:])
    return bits.ljust(TX_BYTES * 8, "0")


def tx_bytes(bits: str) -> bytes:
    if len(bits) != TX_BYTES * 8:
        raise ValueError("bufor nadawczy musi miec 30 bajtow")
    return bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))


def decode_row(row: str):
    if len(row) != FRAME_BITS:
        return None
    data = [int(row[SYNC_BITS + 8 * i:SYNC_BITS + 8 * (i + 1)], 2)
            for i in range(10)]
    return {"flags": int.from_bytes(bytes(data[:4]), "big"),
            "id": int.from_bytes(bytes(data[4:7]), "big"),
            # 2,5 kPa/bit to skala dekodera rtl_433 (niepotwierdzona); firmware
            # koryguje ja stala PRESSURE_KPA100_PER_RAW, wiec wartosc ponizej
            # != cisnieniu zadanemu w konsoli.
            "pressure_kPa": data[7] * 2.5, "temperature_F": data[8],
            "checksum_ok": sum(data[:9]) & 255 == data[9], "bytes": data}


def self_test() -> int:
    expected = [0x4C, 0x90, 0, 0x70, 0x2F, 0xC8, 0x71, 0, 0x5A, 0x0E]
    row = build_row(0x2FC871, 0, 32)
    got = decode_row(row)["bytes"]
    if got != expected:
        print(f"FAIL: nagranie gfile001: {got} != {expected}")
        return 1
    # Regresja: saturacja temperatury zgodna z firmware (zakres -40..85 C).
    for tc, want in ((-40, 0), (-19, 0), (-18, 0), (23, 73), (85, 185)):
        byte = decode_row(build_row(0x2FC871, 0, tc))["bytes"][8]
        if byte != want:
            print(f"FAIL: temperatura {tc} C: {byte} != {want}")
            return 1
    # Regresja kalibracji pola P (punkt pomiarowy z auta: 260 kPa -> P=95).
    for kpa, want in ((0, 0), (1.2, 0), (1.4, 1), (260.0, 95), (637.5, 234)):
        byte = press_raw(kpa)
        if byte != want:
            print(f"FAIL: cisnienie {kpa} kPa: {byte} != {want}")
            return 1
    import random
    rng = random.Random(42)
    for _ in range(500):
        sensor_id = rng.randrange(1, 0xFFFFFF)
        pressure = round(rng.uniform(0, 637.5), 1)
        temp = rng.randrange(-40, 86)
        decoded = decode_row(build_row(sensor_id, pressure, temp))
        if not decoded["checksum_ok"] or decoded["id"] != sensor_id:
            print("FAIL: round trip")
            return 1
    print("self-test: OK (nagranie + 500 ramek)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--id", default="2FC871")
    parser.add_argument("--pressure", type=float, default=260.0)
    parser.add_argument("--temp", type=float, default=23.0)
    parser.add_argument("--flags", default="4C900070")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    row = build_row(int(args.id, 16), args.pressure, args.temp, int(args.flags, 16))
    raw = tx_bytes(build_tx_bits(row))
    decoded = decode_row(row)
    print(f"row: {row}\ntx: {raw.hex()}")
    print("payload: " + " ".join(f"{byte:02X}" for byte in decoded["bytes"]))
    print(f"id={decoded['id']:06X} pressure={decoded['pressure_kPa']:.1f} kPa "
          f"temp={decoded['temperature_F']} F flags={decoded['flags']:08X}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
