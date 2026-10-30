# Troubleshooting

Work one change at a time and record the measurement. Do not switch OOK/FSK,
flags, IDs, or intervals randomly. The base firmware has no FSK mode.

| Symptom | Possible cause | Measurement / check | Repair |
|---|---|---|---|
| `ERROR: no CC1101` | poor supply, CS/SPI, module is not 433 MHz | 3.3 V across VCC-GND; continuity SCK/MISO/MOSI/CS = 4/5/6/7; module marking | fix wiring, shorten SPI, use the correct module |
| Reset during TX | 3.3 V droop, weak wires | voltage at the ESP32/CC1101 during `send`, brownout log | fix the supply and 47–100 µF at the module; do not raise the PA |
| `TX-ERR` | underflow, unfinished TX, wrong CC1101 configuration | log for each wheel; MARCSTATE/registers in HIL | check SPI and supply; save the log for analysis, do not mask the error |
| USB/UART does not respond | console window too short, wrong baud/port, charge-only cable | 115200 8N1, reset, correct `/dev/ttyACM*` or `/dev/ttyUSB*` | type `stay 300` right after startup; use a data cable |
| ID change disappears | NVS write error, full flash | `OK: saved` message, restart and `info` | do not continue on an NVS error; check flash/supply |
| ID rejected | duplicate, 0, FFFFFF, wrong length | six hex digits and `info` | transmit four unique IDs from 000001–FFFFFE |
| rtl_433 decodes nothing | antenna, frequency, wrong RF path, too little power | `send 1`; 433.92 MHz receiver close by; ANT/GDO0 measurement | fix antenna/supply; step through low powers gradually; do not switch to FSK |
| rtl_433 sees wrong data | stale configuration, neighbouring sensor | `info`, `hex 1`, ID filtering in the receiver | compare IDs and force `send 1` in an isolated spot |
| Car does not see known IDs | unconfirmed profile, ECU requires learning, range | VIN/markings/DTC; RTL-SDR reception at the car | confirm the profile; perform LEARN; change position after measuring |
| LEARN stuck on one wheel | used `send` instead of `next` | `info` shows the active wheel | `next` after confirming the step; `send` only forces TX |
| LEARN returned to NORMAL | 60 s per wheel / 15 min session | elapsed time, message and LED | run `mode learn 1` and consciously repeat from FL |
| After RL there is no next wheel | the sequence completed | NORMAL in `info` | finish the car procedure and check DTC |
| Warning light stays on although RTL-SDR is OK | ECU does not know the IDs, other profile/flags/procedure | read IDs and DTC from diagnostics, not only SDR | Clone IDs or LEARN; confirm the car profile; do not guess flags |
| High sleep consumption | S2 Mini, LM317T, LED, dividers | measure the whole board and regulator per `POWER.md` | fix the supply; do not change the protocol |
| No TX after parking | supply only from the lighter socket/ACC, brownout, NVS/CC1101 error | does 3.3 V remain after the ignition goes off; log after restart, `batt`, `info` | with an ignition-switched socket supply the lack of TX is expected; otherwise fix the supply |
| ACC does not change the interval | feature disabled by default or wrong threshold | `PIN_ACC`, voltage on GPIO OFF/ON | build a protected input; never 12 V on GPIO |

## When the car does not receive

Diagnostic sequence:

1. **Prove local TX:** `send 1` + RTL-SDR/analyser.
2. **Prove the correct profile:** VIN, sensor markings, car readout.
3. **Check IDs and order:** `info`, Clone IDs, or one active wheel in LEARN.
4. **Check range:** low power first, console/behind the glovebox, possibly the
   boot; without disassembling airbags or guessing the ECU location.
5. **Only then analyse flags:** compare with a recording of the original
   sensor in the same mode. There is no confirmed low-battery bit — do not
   set it.
6. If there is no measurement or data, stop the test. Random changes add new
   variables and are not a diagnosis.
