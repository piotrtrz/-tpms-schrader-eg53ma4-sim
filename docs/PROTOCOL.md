# Protokół Schrader EG53MA4 — fakty i niewiadome

## Przypięte źródła

| Źródło | Wersja użyta do walidacji | Zakres |
|---|---|---|
| `rtl_433/src/devices/schraeder.c` | commit `406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab` | dekoder i parametry slicera |
| `rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01` | commit `1aa8c29d6b9383739d1aeed495dbc5069b704392` | cztery nagrania CU8 + JSON |
| PlatformIO | 6.1.18 | build/test |
| `espressif32` | 8.0.0 | toolchain/platforma ESP32 |
| SmartRC-CC1101-Driver-Lib | `de6757ef633b3e626dae5c7835cdb66ab53f9a1d` (3.0.2) | konfiguracja CC1101 |
| Praktyczna analiza odbioru | <https://milek7.pl/tpmsreceiver/> | kontekst; nie źródło pól ramki |
| Wzorzec integracji ESP32+CC1101 | <https://github.com/merbanan/rtl_433/issues/2978> | integracja, nie protokół EG53MA4 |

Nie używamy nieoznaczonego `master` jako referencji reprodukowalnej. Komercyjny
emulator <https://spooftpms.com/us-eu-gm-1-2-family-tpms-simulator/> potwierdza
jedynie istnienie takiej klasy urządzenia, nie zgodność tego projektu.

## Potwierdzony format po dekodowaniu Manchester

Dekoder wymaga dokładnie 120 bitów: 40 bitów synchronizacji i 80 bitów danych.

```text
0                   40          72       96      104     112       120
| 40 × sync (0)     | flags 4 B | ID 3 B | P 1 B | T 1 B | sum 1 B |
                   4C 90 00 70  2F C8 71    68      49      65
```

- `flags`: 32 bity; znaczenie pól nie jest w pełni znane.
- `ID`: 24 bity; firmware odrzuca 0, `FFFFFF` i duplikaty między kołami.
- `P`: 2,5 kPa/bit **wg dekodera rtl_433, ale skala nie jest potwierdzona** —
  wszystkie cztery nagrania mają `P = 0`, więc żadne prawdziwe ciśnienie nie
  zweryfikowało przelicznika (w źródle dekodera to „probable"). Pomiar z auta:
  `P = 104` (260 kPa wg skali rtl_433) pokazuje się jako **284 kPa**, więc
  firmware koryguje skalę stałą `TPMS_PRESSURE_KPA100_PER_RAW`
  (`tpms_frame.h`, domyślnie 273 = 2,73 kPa/bit; `P = 95` dla 260 kPa).
  Kwantyzacja: `(kPa×100 + 136) / 273`. Kalibracja oparta na jednym punkcie
  pomiarowym — do potwierdzenia i ewentualnego dostrajania (`AUDIT.md`).
- `T`: bajt w °F zgodnie z dekoderem, nie °C. 23 °C → 73 °F.
- `sum`: suma dziewięciu poprzednich bajtów modulo 256; to nie CRC-8.
- Wartość `4c900070` występuje we wszystkich 64 rekordach czterech nagrań.
  Nie dowodzi to, że każdy czujnik/tryb auta używa zawsze tej wartości.

## Polaryzacja, preambuła i ogon

Nagrania i slicer `OOK_PULSE_MANCHESTER_ZEROBIT` potwierdzają G.E.T. Manchester:
`0 → 01`, `1 → 10`. Slicer dopisuje pierwszy zerowy bit. Bufor CC1101 zawiera:

```text
1 chip startowy + 39 komórek sync + 80 komórek danych + 1 chip OFF dopełnienia
= 240 chipów = 30 bajtów, MSB first.
```

Ostatni chip dopełnienia jest zerem OOK (cisza). Test sprawdza dokładnie tę
długość, polaryzację i odrzuca ucięty/uszkodony bufor.

## Timing i powtórzenia

- Dekoder ma `short_width=123 µs`, `reset_limit=300 µs`.
- Analiza `gfile001` pokazuje typowy okres pulse+gap ok. 236 µs, czyli chip
  ok. **118 µs**; szerokości impulsów różnią się od szerokości przerw. Firmware
  używa eksperymentalnie **120 µs**, co przechodzi dekodowanie niezależnym
  `rtl_433`. Dokładny zegar fizycznego CC1101 wymaga pomiaru HIL.
- Każde z czterech nagrań zawiera **16 poprawnych powtórzeń**. W `gfile001`
  odstępy początków dekodowań wahają się ok. 115–172 ms; nie są stałe.
- Domyślne **3 ramki, przerwa 50 ms, cykl 30 s** są eksperymentalnymi
  ustawieniami emulatora, a nie potwierdzonym harmonogramem OE. Interwały
  ACC/parking również nie mają statusu zgodności z OE.

## Nośna i modulacja

Podstawowy profil: **433,92 MHz, ASK/OOK**. Nagrania są zdekodowane przy tej
częstotliwości. README zestawu zawiera też dane produktu 315 MHz i wzmiankę
ASK+FSK; to nie jest dowód FSK w analizowanych plikach ani zgodności z konkretną
Astrą. FSK nie jest dostępny w podstawowym firmware/generatorze.

## CC1101 packet engine

Firmware ustawia fixed-length 30 B, normal FIFO, MSB first oraz wyłącza sprzętowe:
preambułę/sync (`SYNC_MODE=0`), Manchester, whitening, FEC, CRC, address check,
append status. Cała preambuła i suma protokołu są w naszych 30 B. Koniec TX jest
oczekiwany przez `MARCSTATE=IDLE` z limitem; `TXFIFO_UNDERFLOW` i pozostałe bajty
są błędem. To zapobiega obcięciu ogona przez samo `TXBYTES=0`.

Konfiguracja została audytowana, ale syntetyczny CU8 omija CC1101. Brak dodatkowych
bajtów i dokładny ogon trzeba jeszcze potwierdzić na logic analyzer/RTL-SDR w HIL.

## Niewiadome — nie wolno ich „dostrajać” losowo

1. Znaczenie poszczególnych bitów `flags`, w tym jakiegokolwiek bitu niskiej
   baterii czujnika. **`TPMS_FLAGS_LOWBAT_MASK` nie istnieje w firmware.**
2. Zależność flags od nauki, ruchu, LF i stanu baterii oryginalnego czujnika.
3. Wymagany przez konkretne ECU interwał i minimalna liczba powtórzeń.
4. Zgodność OE 13506028/numera powiązanego z konkretnym egzemplarzem bez VIN,
   oznaczeń, odczytu albo testu.
5. Fizyczna dokładność częstotliwości, moc i ogon generowanego CC1101 przed HIL.

Niskie zasilanie emulatora jest wyłącznie ostrzeżeniem w konsoli i nie zmienia
żadnego pola symulowanego czujnika.
