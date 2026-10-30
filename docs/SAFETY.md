# Safety and limitations

## Synthetic data can hide a fault

This device generates data it does not measure in the tyres. The default
260 kPa is a test value, not confirmation of pressure. The simulator may cause
the warning light not to alert about a real puncture, pressure loss or tyre
damage.

- A diagnostic and workshop project only, on a parked car and under the
  control of the person running the test.
- It is not a homologated sensor nor a replacement for a working TPMS.
- Do not use while driving and do not present it as safe for that purpose.
- Before and after the test, check all tyres with an independent gauge and
  inspect their condition.
- After the diagnosis, restore working, correctly matched sensors and perform
  the vehicle's required procedure.

## Radio and vehicle compatibility

The EG53MA4/OOK 433.92 MHz profile is a working hypothesis for the vehicle
indicated, to be confirmed. OE 13506028, a related part number, or a
replacement offer is not enough. Use the VIN, factory sensor markings,
diagnostics, or a controlled test. Do not transmit at maximum power "to be
sure". Start at −10 dBm, limit transmission time, and follow local 433 MHz
regulations and rules about interfering with vehicle electronics. The device
may be received by other receivers nearby.

## Installation

- Only the laboratory prototype is described as ready to assemble. The 12 V
  version requires a separate design and automotive tests.
- Do not apply 12 V to GPIO/ADC or 5 V to the CC1101.
- Do not place the device near airbags, pedals, moving parts, moisture, high
  temperature, or the LPG installation.
- Secure the enclosure and cables against loosening. Avoid shielding the
  antenna.
- Do not use an 18650 cell as the recommended power source in the cabin.

## Diagnostics without false confidence

Correct decoding by rtl_433 does not prove reception by the car. Green CI does
not replace a CC1101 measurement, an HIL test, or a tyre check. The warning
light going out also does not prove the tyres are sound. Record measurements
and do not change several parameters at once. There is no confirmed low
battery bit of the sensor; low emulator supply is not encoded as a fault of
the simulated sensor.
