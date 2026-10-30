#!/usr/bin/env python3
"""Validation of real and synthetic CU8 with the pinned rtl_433."""
import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from tpms_frame import press_raw

EXPECTED = {
    "gfile001.cu8": ("2FC871", 90.0),
    "gfile002.cu8": ("2EAAF9", 90.0),
    "gfile003.cu8": ("3495AC", 90.0),
    "gfile004.cu8": ("2FC780", 91.0),
}


def decode(rtl433: str, capture: pathlib.Path):
    result = subprocess.run([rtl433, "-q", "-F", "json", "-r", str(capture)],
                            text=True, capture_output=True, check=False)
    records = []
    for line in result.stdout.splitlines():
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue
        if record.get("model") == "Schrader-EG53MA4":
            records.append(record)
    return result.returncode, records, result.stdout + result.stderr


def require(condition, message, failures):
    if not condition:
        failures.append(message)
    return condition


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rtl433", required=True)
    parser.add_argument("--reference-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    failures = []
    print("rtl_433 commit: 406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab")
    print("rtl_433_tests commit: 1aa8c29d6b9383739d1aeed495dbc5069b704392")

    for filename, (sensor_id, temp_f) in EXPECTED.items():
        capture = args.reference_dir / filename
        if not require(capture.is_file(), f"missing {capture}", failures):
            continue
        status, records, raw = decode(args.rtl433, capture)
        valid = status == 0 and len(records) == 16 and all(
            record.get("id") == sensor_id and record.get("flags") == "4c900070"
            and record.get("pressure_kPa") == 0.0
            and record.get("temperature_F") == temp_f
            and record.get("mic") == "CHECKSUM" for record in records)
        require(valid, f"{filename}: mismatched records\n{raw}", failures)
        print(f"{'OK' if valid else 'FAIL'} reference {filename}: "
              f"{len(records)}/16, id={sensor_id}, flags=4c900070")

    # rtl_433 computes the P field on a 2.5 kPa/bit scale (unconfirmed); the
    # firmware corrects it with the PRESSURE_KPA100_PER_RAW constant, so the
    # expected rtl_433 value is the raw P byte * 2.5, not the requested kPa.
    synthetic = [("2FC871", 260, 23, 73.0), ("3495AC", 220, -5, 23.0),
                 ("ABCDEE", 262.5, 24, 75.0)]
    with tempfile.TemporaryDirectory(prefix="tpms-rtl433-") as temp:
        for index, (sensor_id, pressure, temp_c, temp_f) in enumerate(synthetic):
            capture = pathlib.Path(temp) / f"valid{index}.cu8"
            subprocess.run([sys.executable, str(root / "tools/gen_cu8.py"),
                            "--id", sensor_id, "--pressure", str(pressure),
                            "--temp", str(temp_c), "-o", str(capture)],
                           check=True, capture_output=True)
            status, records, raw = decode(args.rtl433, capture)
            expect_kpa = press_raw(pressure) * 2.5
            valid = status == 0 and len(records) >= 1 and all(
                record.get("id") == sensor_id
                and record.get("pressure_kPa") == expect_kpa
                and record.get("temperature_F") == temp_f
                and record.get("flags") == "4c900070"
                and record.get("mic") == "CHECKSUM" for record in records)
            require(valid, f"synthetic {sensor_id}: {raw}", failures)
            print(f"{'OK' if valid else 'FAIL'} synthetic id={sensor_id}: "
                  f"{len(records)} dekodowan, P={press_raw(pressure)} "
                  f"(rtl_433: {expect_kpa:.1f} kPa z {pressure:.1f} kPa), "
                  f"{temp_f:.1f} F")

        for corruption in ("checksum", "manchester", "truncate"):
            capture = pathlib.Path(temp) / f"bad-{corruption}.cu8"
            subprocess.run([sys.executable, str(root / "tools/gen_cu8.py"),
                            "--corrupt", corruption, "-o", str(capture)],
                           check=True, capture_output=True)
            status, records, raw = decode(args.rtl433, capture)
            valid = status == 0 and not records
            require(valid, f"negative {corruption}: decoded incorrectly\n{raw}", failures)
            print(f"{'OK' if valid else 'FAIL'} negative {corruption}: "
                  f"{len(records)} dekodowan (oczekiwano 0)")

    print(f"RESULT: {'PASS' if not failures else 'FAIL'}; errors={len(failures)}")
    for failure in failures:
        print(" - " + failure)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
