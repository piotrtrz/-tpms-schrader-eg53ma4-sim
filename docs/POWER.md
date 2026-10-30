# Energy and parking behaviour

## Measurement status

**No current measurements of the physical prototype have been performed.**
The numbers below are budget estimates, not validated results. The S2 Mini
is not suitable for declaring µA consumption without measurement: the built-in
regulator, LED, and auxiliary circuits add consumption. The LM317T itself
loads the supply with a divider current of about 5 mA (R1 = 240 Ω) plus its
own quiescent current, so a permanent supply needs a separate energy budget.
The target supply from an ignition-switched cigarette-lighter socket removes
this problem: with the ignition off the emulator draws no current at all.

## Firmware schedule

- NORMAL after the RTC wake: initialisation, a burst of four wheels, exit of
  TX, and deep sleep. There is no 10-second console window after a plain wake.
- Micro-naps: sleep is split into pieces so that the LED blinks about every
  2 s, also between transmissions (`NAP_PERIOD_MS` in `config.h`). After a
  timer wake the device shows one visible blink and sleeps the rest of the
  period after boot time + window (`NAP_AWAKE_MS`). The button wakes
  immediately (EXT1) regardless of the micro-naps.
- TX of frames remains on the interval (default 30 s): the sleep counter in
  RTC memory counts the slept pieces since the last TX cycle; `interval`
  takes effect on the next cycle. After a longer awake period (e.g. `stay`),
  the first piece is a full one.
- **Micro-naps raise consumption** (a boot about every 2 s instead of one
  sleep per interval) — the numbers below describe the old model and require
  HIL measurement.
- After power-on, a button, or LF: a 10 s console window; `stay N` extends it.
- LEARN: TX of one wheel every 2 s, without deep sleep, max 60 s per wheel/
  15 min per session.
- SINGLE: TX of one wheel every 2 s, without deep sleep, max 5 min.
- The CC1101 enters SPWD before sleep; CS is high, SCK/MOSI low, LED off.
- Every sleep has an RTC alarm. `PARKING_SHUTDOWN` was removed — the device
  cannot remain asleep until a purely manual wake.
- ACC (optional in hardware) selects the ignition/parking interval. It is not
  needed as the only wake source, because the RTC always runs. If the whole
  assembly is powered only from an ignition-switched lighter socket or from
  ACC, then with ACC off there is no power, no RTC, and no transmission —
  and that is the target scenario.

## Charge estimate for the default settings

One frame is 240 × 120 µs = 28.8 ms on air. For 4 wheels × 3 frames:
345.6 ms of transmission plus 8 × 50 ms gaps inside the bursts. On top of
that come boot, configuration, SPI, and logging. For energy calculations you
must measure the whole cycle, not just the OOK time.

An example model (to be replaced by measurement): if ESP32 activity is 1.0 s
at 80 mA every 30 s, the ESP32 alone averages about 2.7 mA at 3.3 V. The
CC1101, boot, and sleep current add further terms. With a 120 s interval the
activity share falls about fourfold, but the constant Iq of the regulator and
the dividers remains. These are not measured 2.7 mA nor a promise of
consumption from a 12 V battery.

## Measurement method required before "validated"

1. **The whole S2 Mini in sleep:** a µA meter in series at the input, not
   through USB. Wait until TX ends and record the minimum/steady state and
   the board model.
2. **S2 Mini + LM317T:** repeat the measurement on the protected VIN input
   and on the 3.3 V rail. Include the regulator divider current, TVS, the
   other dividers, and pull-ups.
3. **NORMAL cycle:** a shunt + oscilloscope or a logger with sufficient
   bandwidth. Record boot, 12 frames, gaps, and entering sleep over at least
   10 cycles.
4. **Average:** integrate the charge for at least 15 min at 30 s and 120 s.
   State the conditions: voltage, temperature, PA, board and firmware
   version.
5. **LEARN/SINGLE:** measure the continuous-awake current separately.
6. **ACC/brownout/crank:** check restart, the return to transmission, and NVS
   integrity. Do not test on GPIO without confirmed input protection.

## Conclusions allowed before measurement

- The interval, awake time, PA, and the whole board's Iq have the largest
  impact.
- A power-saving version requires measuring the whole S2 Mini and LM317T;
  with power only from an ignition-switched lighter socket the assembly draws
  no current after the ignition goes off, but then it also transmits no
  parking mode — which matches the assumption, because the car ECU does not
  listen then.
- We do not recommend an 18650 in the cabin, nor multi-year battery operation
  with an ESP32.
- The emulator's constant consumption must be accounted for during a long
  park; we do not give a "safe for the battery" time without measuring the
  whole car.
