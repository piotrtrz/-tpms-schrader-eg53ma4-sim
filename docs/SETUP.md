# Configuration and startup

## Before you start

1. Verify compatibility of the specific car (VIN, sensor markings,
   diagnostics, or reception). Do not rely solely on a number from a listing.
2. Assemble the prototype per `HARDWARE.md`, flash the firmware, and open a
   115200 UART.
3. Type `stay 300`. `info` shows the active configuration, not just the saved
   one.
4. Work on a parked car. The synthetic 260 kPa does not describe the real
   tyres.

Wheels: `1=FL` front left, `2=FR` front right, `3=RR` rear right, `4=RL` rear
left. Configuration commands save to NVS automatically and verify the read
back. An NVS error must be resolved before further testing, not ignored.

## Path A — Clone IDs (preferred)

Use it when the original IDs were read earlier and the car still knows them.

1. Write down four IDs as six-digit hex.
2. Enter in turn (replace the example values with your own):

   ```text
   id 1 2FC871
   id 2 2EAAF9
   id 3 3495AC
   id 4 2FC780
   pressure all 260
   info
   ```

3. Check in `info` that all IDs and wheel assignments are correct. A
   duplicate, `000000`, `FFFFFF`, or a value beyond 24 bits will be rejected.
4. Optionally, `send` forces a cycle of all wheels. In NORMAL the device does
   it automatically as well; after a restart the configuration remains.
5. Confirm reception externally. Known IDs **may** eliminate learning, but we
   do not promise that for every ECU state.

Do not use the example IDs in a vehicle if they may coincide with another
device nearby.

## Path B — New IDs + LEARN

### Preparation

1. `ids new` generates four unique IDs. You can use four `id` commands
   instead.
2. `info` — write the IDs down on paper.
3. Read the learning procedure for the specific model/equipment version. This
   document does not guess menus, pedal sequences, or the receiver location.
4. Start the learning procedure in the car according to its service
   instructions.

### Learning from the console

1. `mode learn` — starts at FL; only FL transmits every 2 s.
2. `send` — immediate FL burst, without changing the wheel.
3. After completing the step for FL: `next` — FR; the LED shows 2 blinks.
4. Repeat `send`, then `next` — RR, then `next` — RL.
5. After the last `next` the device returns to NORMAL. The order is always
   **FL → FR → RR → RL**.
6. Finish the car procedure according to its instructions and check the
   diagnostic readout.

### Learning with one button

- enter: hold ≥1.5 s;
- short press: next wheel and an immediate burst;
- force without changing the wheel: console `send`;
- exit before the end: hold ≥1.5 s again or `mode normal`.

A 60 s timeout per wheel aborts LEARN to NORMAL; the whole session limit is
15 min. This is not an automatic learning success — you have to start again
from the right wheel.

## SINGLE / diagnostics

`mode single 2` transmits only FR every 2 s. `send` forces FR, `next` moves
on. `mode normal` ends it; the automatic limit is 5 min. The mode does not
save the wheel selection in NVS and starts safely in NORMAL after a power
loss.

## Other settings

| Command | Range / effect |
|---|---|
| `pressure all 260` | 0–637.5 kPa, P byte step ≈ 2.73 kPa (calibrated per the car, `PROTOCOL.md`) |
| `temp all 23` | −40…85 °C; °F in the frame |
| `interval normal 30` | interval without ACC, 5–86400 s |
| `interval ignition 30` | with hardware ACC active |
| `interval parking 120` | without active ACC |
| `rf freq 433.92` | 433.05–434.79 MHz; changes require justification |
| `rf power -10` | −30, −20, −15, −10, −6, 0, 5 dBm |
| `rf burst 3` | 1–8 frames per wheel |
| `rf gap 50` | 20–500 ms between frames |
| `flags 4C900070` | experiment; bit meanings are unknown |
| `factory YES` | erases your configuration and restores defaults |

Schedule parameters are experimental. Power starts at −10 dBm; increase it
only after a measurement and with the regulations in mind. Low emulator
supply sets no sensor flag.

## After a restart or brownout

The firmware reads NVS again. A correct version/CRC and ranges are required.
A damaged or incompatible entry produces a message and a return to defaults,
rather than transmitting a partly corrupted configuration. Every deep sleep
has an alarm; there is no `PARKING_SHUTDOWN` mode that could leave the car
without transmission until a manual wake.
