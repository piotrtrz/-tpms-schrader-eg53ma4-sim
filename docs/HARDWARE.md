# Hardware and assembly

## Scope

The base release describes a **laboratory prototype on the ESP32-S2-MINI**
(ESP32-S2FN4R2). Prototype wires and the LM317T are not a ready kit for
permanent installation in a car. The cigarette-lighter socket variant below
is a list of connections and design requirements, not a validated automotive
schematic.

## Laboratory prototype BOM

| Item | Requirement | Notes |
|---|---|---|
| ESP32-S2-MINI | ESP32-S2FN4R2 (4 MB flash, 2 MB PSRAM), native USB for flash/console; a Lolin/Wemos S2 Mini type board, e.g. ARD-ESP32-S2-MINI | sold with MicroPython — we flash our own firmware; verify a clone before powering it |
| CC1101 | **433 MHz** module, SPI, 3.3 V, with an SMA connector and antenna, e.g. ARD-CC1101-ANT | do not buy the 868/915 MHz version |
| 433 MHz antenna | supplied with the SMA module; for a bench, a λ/4 monopole ≈ 17 cm | ground plane/counterweight per the module's design |
| NO button (optional) | momentary | GPIO3 to GND; without an extra button the board's BOOT button (GPIO0) is used |
| LED | built-in on GPIO15 | active high; an external LED is not needed |
| Car USB charger | cigarette-lighter socket → USB 5 V | power variant A; tested, with a USB-C cable that carries data |
| LM317T | TO-220 (metal), adjustable 1.25–37 V, 1.5 A | only for variant B; not needed when powering from USB |
| LM317T resistors | R1 = 240 Ω (VOUT–ADJ), R2 = 390 Ω (ADJ–GND) | give about 3.28 V; 1% metal-film, close to the device |
| LM317T capacitors | 100 nF on the input and at least 1 µF on the output | practically 10 µF; close to the device |
| Heatsink | for the LM317T when fed from 12–14.4 V | see the thermal calculations below |
| Module decoupling | 100 nF at each module + 47–100 µF at the CC1101 | as close to the supply pins as possible |
| Wires | short, common ground | SPI best under 10 cm |
| Laboratory supply | USB 5 V or the LM317T from a safe VIN | do not feed competing 3.3 V sources at the same time |

RTL-SDR is useful for RF diagnosis but is not part of the emulator. The
standard S2 Mini has a blue LED on GPIO15 and a BOOT button on GPIO0; before
assembly verify the specific clone, because trade names are sometimes used
for different boards.

## Pinout and connections

| ESP32-S2-MINI | CC1101 / item | Rule |
|---|---|---|
| 3V3 | CC1101 VCC; LM317T output when externally powered | **3.3 V only**, never the 5 V pin nor an LM317T set higher |
| GND | CC1101 GND, button, LM317T divider | common ground |
| GPIO4 | SCK/SCLK | SPI |
| GPIO5 | MISO/SO | SPI |
| GPIO6 | MOSI/SI | SPI |
| GPIO7 | CS/CSn/SS | SPI |
| GPIO10 | GDO0 | library diagnostics; GDO2 unconnected |
| GPIO3 | alternative NO button to GND (`-DPIN_BUTTON=3`) | RTC pull-up enabled by the firmware before deep sleep (a digital pull-up does not work in deep sleep); RTC GPIO wakeup, an external 10 kΩ recommended |
| GPIO15 | built-in LED | active HIGH; the firmware drives the correct polarity |
| GPIO1 | optional supply ADC through a divider | `PIN_VBAT_ADC`, ADC1_CH0; 1 MΩ/180 kΩ divider (`config.h`), range up to about 16 V; never 12 V directly |
| GPIO2 | optional ACC after changing `PIN_ACC` and protecting it | never 12 V directly |
| GPIO14 | optional LF detector after changing `PIN_LF_TRIGGER` | active LOW; RTC GPIO wakeup, disabled by default |
| GPIO0 | the board's BOOT button = the control button and RTC wake (default `PIN_BUTTON`) | strapping pin: held at power-up/reset it starts the bootloader instead of the firmware (wake behaviour confirmed on hardware); do not use for the CC1101 |

GPIO0, GPIO45 and GPIO46 are strapping pins. Do not add external resistors or
state-forcing circuits to them during reset. GPIO19/20 carry native USB, and
GPIO26–32 in the ESP32-S2FN4R2 serve the in-package flash and PSRAM — do not
use them. GPIO33–40 remain free; the console uses native USB, so an external
USB-UART bridge is not needed.

With a laboratory 3.3 V supply from the LM317T: start with VIN = 5–9 V, set a
limit of e.g. 0.5 A, check the polarity and output voltage without the S2
Mini connected. Join the grounds, and disconnect USB while externally powered.
Peak current depends on the board and the configured PA.

## LM317T — pinout, divider, and capacitors

For the **LM317T in TO-220**, looking at the markings from the front with the
legs pointing down:

| Leg | Function | Connection |
|---:|---|---|
| 1 | ADJ | divider: R1 to VOUT, R2 to GND |
| 2 | VOUT | 3V3 of the S2 Mini + CC1101 VCC |
| 3 | VIN | protected input voltage |

The metal TAB is connected to **VOUT** — not to GND as in the LF33CV or
7805. The heatsink and TAB enclosure must not touch ground or any other
potential. **Do not use the 7805 leg scheme** — the order here is
ADJ–VOUT–IN.

The output voltage is set by the divider: `VOUT = 1.25 V × (1 + R2/R1)`.
R1 = 240 Ω between VOUT and ADJ, and R2 = 390 Ω between ADJ and GND give
about **3.28 V**, while drawing about 5 mA, close to the LM317's minimum load
current — with larger resistor values measure VOUT unloaded. Place 100 nF
between VIN and GND. Place at least 1 µF between VOUT and GND; a practical
starting point is 10 µF. The LM317 does not require special ESR like an LDO,
but the capacitors should sit close to the device. An extra 100 nF at the
modules and 47–100 µF at the CC1101 remain necessary.

Do not connect the LM317T VOUT to the S2 Mini's **5V** pin. The 5V pin feeds
the board's built-in regulator, whereas external, regulated 3.3 V must be
brought to **3V3**. Do not power from USB and the LM317T at the same time
without verified source separation, because clone designs differ in their
power connections.

## Antenna and position test

We do not guess the TPMS receiver location in the Astra J. Perform the range
test while parked, at low power, comparing RTL-SDR readings/diagnostics for a
few positions: centre console, near the glovebox, possibly the boot. A result
for a single unit is not proof of the receiver location.

Mount the device mechanically, away from airbags and their harnesses, the
pedal area, moving parts, moisture, heat sources, and metal shielding. Do not
wrap the antenna in a harness, do not route it near an LPG bulb/regulator or
ignition wiring. The prototype enclosure should be non-conductive; the hole
for the SMA connector and any need for shielding must be assessed by
measurement.

## Cigarette lighter socket variant — requirements, unvalidated design

The target power comes from the car cigarette-lighter socket, in two ways:

```text
Variant A (simpler):
cigarette-lighter socket → car USB charger (5 V) → USB-C cable → S2 Mini USB

Variant B (regulator):
cigarette-lighter socket 12 V → fuse at the source → reverse-polarity
protection → energy limitation + properly rated TVS
→ LM317T VIN → VOUT 3.3 V → S2 Mini 3V3 + CC1101 VCC
```

In most cars the cigarette-lighter socket is powered only after the ignition
is switched on — **verify this in your vehicle**, because some cars have a
socket powered permanently. With an ignition-switched socket the emulator
draws no current while the car is off. The car computer usually does not read
TPMS state then anyway, so the lack of parking transmission does not matter.

The socket output itself still has transients and noise of the vehicle
installation. **Do not connect it directly to the LM317T VIN as a finished
car installation.** The LM317T tolerates a VIN−VOUT difference up to 37–40 V,
so it is more tolerant of transients than the LF33CV (16 V), but protection
still has to limit the voltage with margin. A TVS with a threshold above the
maximum VIN provides no protection for the LM317T.

Requirements before installation (variant B):

- a fuse matched to the wiring and consumption, close to the source; it does
  not replace a TVS;
- reverse-polarity and negative-transient protection;
- TVS and energy limitation matched to the real transient profile, without
  conducting at a normal 14.4–14.8 V and without exceeding the LM317T VIN;
- capacitors per the LM317 datasheet, short current loops;
- a measurement of the current peaks of the S2 Mini + CC1101 and a check of
  3.3 V stability;
- thermal calculations for the highest temperature and tests of
  brownout/crank/load dump, EMC, and ESD on the target board.

The LM317T is a linear regulator: `P = (VIN − 3.3 V) × I`. Already at 14.4 V
and 300 mA you must dissipate about **3.3 W**. Without a calculation and a
TO-220 heatsink it can overheat; the "up to 1.5 A" from the datasheet alone
does not guarantee ESP32-S2 peak supply in all conditions. Measure current
and temperature while transmitting.

**Consequence of powering only from the cigarette-lighter socket:** when the
ignition goes off, the whole emulator loses power. It then performs no deep
sleep or parking transmission, the RTC does not count the interval, and the
`interval parking` setting has no effect. The `PIN_ACC` input is not needed.
After the ignition is switched on, the device simply starts and transmits. If
the device is to transmit while parked too, a permanently powered, protected
input and a separate, protected ACC signal on GPIO2 are required. Then the
consumption of the whole assembly also has to be measured: the LM317T divider
draws a constant ~5 mA, plus the S2 Mini, the CC1101, and the dividers — it
may matter on a long park.

LM317T parameters and required capacitors: the ST datasheet
[LM317](https://www.st.com/resource/en/datasheet/lm317.pdf), the typical
application schematic and the TO-220 pinout drawing. Accessed 2026-10-28.

## Battery power

An 18650 cell in the cabin is **not recommended**: the temperature in a car
can exceed the cell's safe conditions, and an installed cell requires
protection, proper charging, and an enclosure. An ESP32 with a radio and
periodic duty cycling usually gives short runtime; claims of multi-year
operation would be unfounded. A multi-year battery variant requires a
different low-power architecture and is not part of this release.

## Optional LF

The RC works without the EL-50448. It contains no validated LF receiver. A
future module requires a separate 125 kHz resonance/detection schematic, a
comparator threshold, filtering, an interference-immunity test, and proven
waking. The front-end output on GPIO14 is to be active-low and high at rest.
The activator itself does not enter IDs into the emulator.
