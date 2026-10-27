# Symulator 4 czujników TPMS Schrader EG53MA4 (ESP32 + CC1101)

Firmware dla ESP32 DevKit + moduł CC1101 (433 MHz), symulujący **4 czujniki
ciśnienia w oponach** (po jednym na koło) dla **Opla Astry J Sedan 2016**
(rynek EU, TPMS bezpośredni, czujniki typu Schrader / Opel OEM 13348393,
protokół **Schrader EG53MA4**).

> ## ⚠️ ZASTRZEŻENIE
> Projekt **wyłącznie do celów diagnostycznych i testowych**.
> Symulator dostarcza **sztuczne dane ciśnienia i temperatury** i **nie
> zastępuje prawdziwych czujników TPMS**. TPMS jest elementem bezpieczeństwa
> czynnego pojazdu — **nie używaj tego urządzenia podczas jazdy po drogach
> publicznych**. Nadawanie w paśmie 433 MHz może podlegać lokalnym
> przepisom (np. w Polsce praca krótkiego zasięgu w 433 MHz jest dozwolona
> z ograniczeniami mocy) — sprawdź przepisy obowiązujące w Twoim kraju.

---

## 1. Spis treści

1. [Spis treści](#1-spis-treści)
2. [Jak to działa](#2-jak-to-działa)
3. [Budowa projektu](#3-budowa-projektu)
4. [Kompilacja](#4-kompilacja)
5. [(a) Okablowanie i lista podzespołów](#5-a-okablowanie)
6. [(b) Konfiguracja ID i ciśnień](#6-b-konfiguracja-id-i-cisnień)
7. [(c) Test nadawania — RTL-SDR + rtl_433](#7-c-test-nadawania--rtl-sdr--rtl_433)
8. [(d) Programowanie czujników w aucie (tryb learn)](#8-d-programowanie-czujników-w-aucie-tryb-learn)
9. [(e) Pomiar poboru prądu](#9-e-pomiar-poboru-prądu)
10. [(f) Weryfikacja, że błąd zniknął](#10-f-weryfikacja-że-błąd-zniknął)
11. [Tryby pracy i sterowanie](#11-tryby-pracy-i-sterowanie)
12. [Konsola szeregowa](#12-konsola-szeregowa)
13. [Budżet prądu](#13-budżet-prądu)
14. [Protokół — szczegóły techniczne](#14-protokół--szczegóły-techniczne)
15. [Rozwiązywanie problemów](#15-rozwiązywanie-problemów)

---

## 2. Jak to działa

Urządzenie zachowuje się jak 4 czujniki TPMS Schrader EG53MA4:

* co `TX_INTERVAL_S` (domyślnie 30 s) budzi się z **deep sleep**,
  konfiguruje CC1101 i wysyła **burst 3 ramek** dla każdego z 4 kół
  (tak jak prawdziwy czujnik, który powtarza pakiet 3–6 razy),
* ramka zawiera: `flags` + 24-bit ID czujnika + ciśnienie (2,5 kPa/bit)
  + temperaturę (°F) + sumę kontrolną — **zgodnie z dekoderem rtl_433
  `schrader_EG53MA4`**,
* dane kodowane są Manchesterem **G.E.T. MC** (chip 120 µs),
  modulacja **ASK/OOK 433.92 MHz** (opcjonalnie **2-FSK, dewiacja 47.6 kHz**),
* między transmisjami ESP32 śpi (budzik RTC + budzenie przyciskiem lub
  wejściem LF), CC1101 jest w trybie SLEEP,
* tryb **LEARN** pozwala wprowadzić ID do pamięci auta po kolei
  (przód lewy → przód prawy → tył prawy → tył lewy),
* tryb **SINGLE** wymusza nadawanie jednego wskazanego ID.

Wszystkie dane (ID, ciśnienia, temperatury, interwały) są **sztuczne**
i konfigurowalne w `src/config.h`.

---

## 3. Budowa projektu

```
tpms/
├── platformio.ini          # konfiguracja budowania (ESP32 Arduino + testy natywne)
├── src/
│   ├── config.h            # <<< WSZYSTKIE USTAWIENIA (ID, piny, interwały) >>>
│   ├── main.cpp            # maszyna stanów: NORMAL / LEARN / SINGLE, deep sleep
│   ├── tpms_frame.h/.cpp   # generator ramki + kodowanie Manchester (bez Arduino)
│   ├── radio.h/.cpp        # konfiguracja CC1101, nadawanie, tryb SLEEP
│   ├── power.h/.cpp        # pomiar napięcia, gpio_hold, deep sleep
│   ├── ui.h/.cpp           # przycisk (gesty) + LED statusowy
│   ├── console.h/.cpp      # konsola szeregowa (debug bez samochodu)
│   └── app.h               # API aplikacji (tryby)
├── test/test_tpms_frame/   # testy jednostkowe generatora ramek (native)
└── tools/
    ├── tpms_frame.py       # referencyjna implementacja ramki + self-test
    ├── gen_cu8.py          # synteza nagrania IQ (OOK/FSK) do testów z rtl_433
    └── verify_rtl433.sh    # automatyczna weryfikacja ramek przez rtl_433
```

Referencje protokołu (źródła, na których oparto kod):

* `rtl_433/src/devices/schraeder.c` (historycznie: `tpms_schrader.c`) —
  dekoder **Schrader TPMS EG53MA4, Saab, Opel, Vauxhall, Chevrolet**
  (Opel OEM 13348393): format ramki, checksum, skale ciśnienia/temperatury:
  <https://github.com/merbanan/rtl_433>
* `rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01/` — referencyjne nagrania
  RF prawdziwych czujników (na ich podstawie odtworzono timing i modulację):
  <https://github.com/merbanan/rtl_433_tests>
* <https://github.com/merbanan/rtl_433/issues/2978> — wzorzec struktury
  symulatora TPMS na ESP32+CC1101 (inicjalizacja CC1101, Manchester, CRC,
  wysyłanie cykliczne)
* <https://github.com/avicarmeli/TPMS-SDR> — okablowanie ESP32↔CC1101,
  struktura projektu

---

## 4. Kompilacja

### PlatformIO (zalecane)

```bash
pio run -e esp32dev            # kompilacja
pio run -e esp32dev -t upload  # wgrywanie
pio device monitor             # monitor szeregowy 115200
pio test -e native             # testy generatora ramek (bez sprzętu)
```

Biblioteka CC1101 (`SmartRC-CC1101-Driver-Lib`, dawniej
`ELECHOUSE_CC1101_SRC_DRV`) jest pobierana automatycznie z `platformio.ini`
(przypięta do commita zweryfikowanego buildu). **Wymagana wersja ≥ 2.5.7**
(wspólne API `setSpiPin`/`setGDO0` dla 2.5.x i 3.x; starsze 1.x używały
`setSCK`/`setMISO`/... i nie są obsługiwane).

### arduino-cli (alternatywa)

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install "SmartRC-CC1101-Driver-Lib"   # albo: lib ZIP z githuba
arduino-cli compile --fqbn esp32:esp32:esp32 --libraries . .
```

(Utwórz folder `src/` jako szkic albo użyj `--build-property` z
`build.extra_flags`; najwygodniej jednak otworzyć katalog w PlatformIO.)

### Weryfikacja bez sprzętu (opcjonalna)

```bash
python3 tools/tpms_frame.py --self-test           # zgodność z dekoderem rtl_433
RTL433=rtl_433 ./tools/verify_rtl433.sh           # pełne testy z prawdziwym rtl_433
```

`tools/verify_rtl433.sh` generuje syntetyczne nagrania IQ takie, jakie
odbiornik RTL-SDR zobaczy w eterze, i sprawdza, czy **prawdziwy rtl_433**
dekoduje je jako poprawny czujnik Schrader z oczekiwanym ID/ciśnieniem/
temperaturą. Ostatni wynik testów projektu:

```
== OOK (domyslna modulacja, wbudowany dekoder Schrader-EG53MA4) ==
OK   c1.cu8  id=2FC871 260.0 kPa 73.0 F
OK   c2.cu8  id=3495AC 220.0 kPa 23.0 F
OK   c3.cu8  id=ABCDEE 262.5 kPa 75.0 F
== FSK (wariant konfigurowalny, dekoder flex) ==
OK   c4.cu8  (FSK) bitstream zgodny: 00000000004c9000702fc871684965
WSZYSTKO OK - ramki dekoduja sie poprawnie przez rtl_433
```

---

## 5. (a) Okablowanie

### Lista podzespołów (BOM) — co kupić

Koszty orientacyjne (zł, z dostawą z AliExpress/Allegro, stan na 2026 r.).
Wystarczy **jedna sztuka** każdego podzespołu.

**Obowiązkowe:**

| Lp. | Podzespół | Parametry / typ | Uwagi | Koszt orient. |
|-----|-----------|-----------------|-------|----------------|
| 1   | ESP32 DevKit (WROOM-32) | moduł ESP32-WROOM-32, USB | Do testów na biurku OK. **Do montażu w aucie kup sam moduł ESP32-WROOM-32E** (bez mostka USB/LDO) + płytkę — patrz sekcja 13 o poborze prądu | 15–30 zł |
| 2   | Moduł CC1101 **433 MHz** | 8-pin, SPI, złącze goldpin | Koniecznie wersja 433 MHz (nie 868/915); dokup/drut antenowy ~17 cm; sprawdzić złącze ANT | 10–25 zł |
| 3   | Przycisk monostabilny | NO, wpuszczany lub panelowy | Tryby pracy (learn/single) | 2–5 zł |
| 4   | LED + rezystor 330 Ω | LED 3 mm/5 mm dowolny kolor | Status trybów | 1–3 zł |
| 5   | Rezystor 10 kΩ | 1 szt. | Podciągnięcie przycisku do 3V3 | < 1 zł |
| 6   | Rezystory 1 MΩ + 270 kΩ | po 2 szt. | Dzielniki pomiaru napięcia i ACC (~9 µA/szt. przy 12 V) | < 1 zł |
| 7   | Kondensatory | 100 µF/16 V elektrolityczny + 2× 100 nF ceramiczny | Odsprzężenie zasilania CC1101 i ESP32 (obowiązkowo przy długich przewodach) | 1–3 zł |
| 8   | Bezpiecznik + uchwyt | 1 A, zwłoczny | Szeregowo z +12 V | 2–5 zł |
| 9   | Przewody, złącza | goldpiny 2,54 mm, kostki, ekran SPI | Przewody SPI możliwie krótkie (< 10 cm) | 5–15 zł |
| 10  | Obudowa | IP65, min. 80×60×30 mm | Montaż w komorze silnika/bagażniku | 10–25 zł |

**Zasilanie — wybierz wariant (konfigurowalne w `config.h`, `POWER_SUPPLY_MODE`):**

| Lp. | Podzespół | Parametry / typ | Uwagi | Koszt orient. |
|-----|-----------|-----------------|-------|----------------|
| 11a | **Wariant 12 V**: LDO 3,3 V o niskim Iq i **wejściu ≥ 16 V** | np. **MCP1703-3302** (Vin 2,7–16 V, Iq ≈ 2 µA) | + zabezpieczenie wejścia: bezpiecznik, **TVS 24 V**, rezystor/dioda szeregowa (przepięcia load-dump!). Sprawdź moc strat: (14,4−3,3) V × ~30 mA ≈ 0,33 W — stosuj obudowę SOT-223/TO-92 i nie przeciążaj | 2–8 zł |
| 11b | **Wariant 12 V** (wydajniejszy): buck o szerokim wejściu | np. **LM5164-Q1** (Vin 6–100 V, Iq ≈ 10 µA, 1 A) lub inny buck automotive | Wymaga cewki i płytki; **TPS62740 (Vin 2,2–5,5 V) i tanie bucki (LM2596, MP1584, Mini360, Iq mA) NIE nadają się do 12 V / snu < 50 µA** | 20–50 zł |
| 12  | **Wariant bateryjny**: ogniwo 3,6 V lub LiPo | Li-SOCl2 3,6 V (AA/half-AA, np. ER14505) + uchwyt **bezposrednio**; LiPo 3,7 V **przez LDO 3,3 V** (np. MCP1700, Vin max 6 V) | **Nie zasilaj LiPo 4,2 V wprost** — max ESP32 to 3,6 V, CC1101 to 3,9 V! | 10–30 zł |

**Opcjonalne (ulepszenia i testy):**

| Lp. | Podzespół | Parametry / typ | Uwagi | Koszt orient. |
|-----|-----------|-----------------|-------|----------------|
| 13  | Front-end LF 125 kHz | cewka rezonansowa (ferrytowa ramka z programatora TPMS), dioda Schottkyego (BAT54/1N5711), 100 pF + 100 kΩ, komparator LM393 | Reakcja na aktywację czujnika programatorem przy kole (`PIN_LF_TRIGGER`) | 10–30 zł |
| 14  | Dzielnik ACC | 1 MΩ + 270 kΩ + 100 nF | Sygnał zapłonu 12 V → GPIO32 (tryb ignition-aware) | < 2 zł |
| 15  | RTL-SDR (dongle) | R820T2/RTL2832U | Testy z `rtl_433` (sekcja 7) — można wypożyczyć | 60–150 zł |
| 16  | Kabel USB + ew. programator | do wgrywania firmware | W DevKit kabel USB-A/Micro-USB wystarczy | 5–15 zł |

**Narzędzia (do pomiarów, sekcja 9):** multimetr z zakresem µA (obowiązkowo do
pomiaru snu), opcjonalnie INA219/INA226 lub oscyloskop do pomiaru prądu w burst.

**Na skróty — zestaw minimalny „na biurko"** (testy RF + nauka): pozycje 1–10
z wariantem 12 V lub zasilaniem z powerbanka 5 V przez DevKit (testy RF nie
wymagają spełnienia budżetu prądu).

Gdzie kupić: AliExpress (CC1101, LM5164, ESP32), Allegro/sklepy elektroniczne
(pasywne elementy, obudowa, bezpieczniki, TVS). Przy zamawianiu CC1101 sprawdź, czy
moduł jest oznaczony **433 MHz** — wersje 868/915 MHz nie zadziałają bez
przestrojenia.

### Podłączenie

| ESP32 DevKit | CC1101      | Uwagi                                  |
|--------------|-------------|----------------------------------------|
| 3V3          | VCC         | **3.3 V!** (CC1101 nie toleruje 5 V)   |
| GND          | GND         | masa wspólna                           |
| GPIO18       | SCK (SCLK)  | `PIN_CC1101_SCK`                       |
| GPIO19       | MISO (SO)   | `PIN_CC1101_MISO`                      |
| GPIO23       | MOSI (SI)   | `PIN_CC1101_MOSI`                      |
| GPIO5        | CS (CSn/SS) | `PIN_CC1101_CS`                        |
| GPIO2        | GDO0        | `PIN_CC1101_GDO0`                      |
| —            | GDO2        | nieużywany (zostaw odłączony)          |

Pozostałe elementy (numery pinów konfigurowalne w `config.h`):

| Element                 | Pin ESP32 | Podłączenie                                    |
|-------------------------|-----------|------------------------------------------------|
| Przycisk trybów         | GPIO26    | GPIO —— przycisk —— GND, rezystor 10 kΩ do 3V3 (aktywny niski, `BTN_ACTIVE_LEVEL 0`) |
| LED statusowy           | GPIO27    | GPIO —— 330 Ω —— LED —— GND                     |
| Wejście LF 125 kHz (opcjonalnie) | GPIO25 | wyjście detektora LF (impuls do 3,3 V), stan spoczynkowy **niski**; bez modułu: ustaw `PIN_LF_TRIGGER -1` |
| ACC / zapłon (opcjonalnie) | GPIO32 | dzielnik 12 V → ~3 V (1 MΩ / 270 kΩ + 100 nF), wejście **INPUT** (bez podciągania); **domyślnie wyłączone** (`PIN_ACC -1`) — ustaw 32 po podłączeniu |
| Pomiar napięcia         | GPIO34    | węzeł środkowy dzielnika `R1=1 MΩ / R2=270 kΩ` (ADC1) + 100 nF |

Zasilanie (wybór w `config.h`, `POWER_SUPPLY_MODE`):

```
 POWER_12V (na stałe z akumulatora) - UWAGA: zakres samochodowy to nie
                                      same "12 V": 14,4 V przy ladowaniu
                                      i przepięcia load-dump > 40 V:
   +12 V ──[bezpiecznik 1 A]──[TVS 24 V do masy]──[rezystor/dioda]── regulator
        ── 3,3 V ── ESP32 + CC1101

   Regulator (wybierz jeden):
   - LDO o Iq ~2 µA i wejściu >= 16 V, np. MCP1703-3302
     (straty: (14,4-3,3) V x ~30 mA = ~0,33 W - zwróć uwagę na obudowę),
   - albo buck automotive o szerokim wejściu, np. LM5164-Q1 (6-100 V,
     Iq ~10 µA) - wydajniejszy, wymaga cewki.
   TPS62740 (Vin 2,2-5,5 V!) i tanie bucki (LM2596/MP1584/Mini360, Iq mA)
   NIE nadają się do tej aplikacji.

 POWER_BATTERY:
   Li-SOCl2 3,6 V ── bezposrednio 3V3 (ESP32 wymaga >= 3,0 V pod obciazeniem)
   LiPo 3,7 V     ── PRZEZ LDO 3,3 V (np. MCP1700). Bezposrednie zasilanie
                     LiPo 4,2 V przekracza max ESP32 (3,6 V) i CC1101 (3,9 V)!
```

```
            +12V ──[bezpiecznik]──[TVS]──> regulator IN
   Pomiar:  +12V ──[R1 1M]──+── GPIO34 ( + 100 nF do masy )
                            └──[R2 270k]── GND
   (12 V -> 2,55 V, 14,4 V -> 3,05 V, prad dzielnika ~9 uA)
```

Ważne:

* **moduły DevKit z mostkiem USB-UART (CH340/CP2102) i LDO AMS1117 pobierają
  w deep sleep 0,5–2 mA** — w aucie użyj gołego modułu ESP32-WROOM na własnej
  płytce z przetwornicą o niskim Iq (patrz sekcja 13),
* CC1101 zasilaj **3,3 V**, antena 433 MHz (kawałek drutu ~17 cm) przy złączu
  CC1101; trzymaj przewody SPI krótkie,
* masa dzielnika pomiarowego i CC1101 musi być wspólna z ESP32.

**Opcjonalny front-end LF 125 kHz** (reakcja na aktywację czujnika
programatorem TPMS przy kole): cewka rezonansowa ~125 kHz (kilka zwojów,
np. ferrytowa ramka z programatora) + detektor obwiedni (dioda Schottkyego
+ kondensator + rezystor rozładowczy + bramka komparatora/logiki) →
impuls 3,3 V na `PIN_LF_TRIGGER`. Stan spoczynkowy musi być **niski**
(wejście ESP32 ma wewnętrzne podciągnięcie do dołu włączone przez firmware).
Bez modułu LF jego rolę pełni przycisk — patrz sekcja 8.

---

## 6. (b) Konfiguracja ID i ciśnień

Wszystko w **`src/config.h`**:

```c
/* ID poszczególnych kół (24 bity) */
#define SENSOR_FL_ID        0x2FC871u     // przód lewy
#define SENSOR_FR_ID        0x2EAAF9u     // przód prawy
#define SENSOR_RR_ID        0x3495ACu     // tył prawy
#define SENSOR_RL_ID        0x2FC780u     // tył lewy

/* Ciśnienie startowe: DOMYŚLNIE 2,6 bar = 260 kPa dla wszystkich kół */
#define SENSOR_FL_PRESS_KPA 260.0f
#define SENSOR_FR_PRESS_KPA 260.0f
#define SENSOR_RR_PRESS_KPA 260.0f
#define SENSOR_RL_PRESS_KPA 260.0f

/* Temperatura otoczenia (st. C) */
#define SENSOR_TEMP_C       23
```

Rozdzielczość protokołu: **2,5 kPa/bit** (zakres 0–637,5 kPa),
temperatura w ramce w °F (przeliczenie automatyczne).

### Jakie ID wpisać? — to jest kluczowa decyzja

| Strategia | Kiedy stosować | Co zrobić |
|---|---|---|
| **A. Oryginalne ID (najlepsze)** | Auto ma już zapamiętane 4 ID starych czujników | **Przed** wymianą odczytaj ID oryginalnych czujników: `rtl_433 -f 433.92M` (pole `id` modelu `Schrader-EG53MA4`), wgraj je do `config.h`. ECU słyszy „znajome" czujniki i **nie trzeba nic programować**. |
| **B. Nowe ID + procedura nauki** | Oryginalne ID nieznane / ECU nie pamięta | Wpisz dowolne unikalne ID (jak wyżej) i wykonaj **procedurę learn** w aucie (sekcja 8), aby ECU zarejestrowało nowe ID. |

Dodatkowo (w razie potrzeby):

```c
#define TPMS_FLAGS          0x4C900070u       // słowo "flags" z prawdziwych czujników
#define TPMS_FLAGS_LOWBAT_MASK 0x00000008u    // bit "rozładowana bateria"
```

Wartość `TPMS_FLAGS` domyślnie pochodzi z nagrań prawdziwych czujników
EG53MA4 (stała `4c900070`). Bitu niskiej baterii rtl_433 nie analizuje —
jeśli Twój sterownik pokazuje ostrzeżenie „service TPMS" mimo dobrych
ciśnień, spróbuj zmienić `TPMS_FLAGS_LOWBAT_MASK` (lub wyczyść bit przez
`flags` w konsoli) i powtórz naukę.

Po zmianach: `pio run -e esp32dev -t upload`.

---

## 7. (c) Test nadawania — RTL-SDR + rtl_433

Zanim włożysz urządzenie do auta, sprawdź transmisję „w eterze":

1. Zasil urządzenie (powerbank / 3,3 V), ustaw `monitor` szeregowy.
2. Na komputerze z RTL-SDR:

   ```bash
   rtl_433 -f 433.92M
   ```

3. Urządzenie co 30 s wysyła po 3 ramki dla każdego koła. Powinieneś widzieć:

   ```
   time      : ...
   model     : Schrader-EG53MA4   type: TPMS   flags: 4c900070   ID        : 2FC871
   Pressure  : 260.0 kPa          Temperature: 73.0 F            Integrity : CHECKSUM
   ```

   dla kolejno: `2FC871` (FL), `2EAAF9` (FR), `3495AC` (RR), `2FC780` (RL),
   każde 260.0 kPa i 73.0 F (23 °C).

4. Bez RTL-SDR możesz wywołać ramki z konsoli: `send`, `hex` (hexdump).

**Wariant FSK** (`TPMS_MODULATION MODE_FSK` w `config.h`) — rtl_433 nie ma
dla tego wariantu wbudowanego dekodera EG53MA4 (deklaracja OOK), weryfikuj
przez dekoder flex:

```bash
rtl_433 -f 433.92M -X 'n=schrader,m=FSK_MC_ZEROBIT,s=120,l=120,r=300'
```

> Uwaga praktyczna: nazwy plików `.cu8` do testów **nie mogą** zawierać
> słów `ook`, `fsk`, `am`, `fm` (parser formatów rtl_433 je rozpoznaje).
> Np. `tpms_sim.cu8` jest OK, `sim_ook.cu8` — nie.

---

## 8. (d) Programowanie czujników w aucie (tryb learn)

### Kolejność kół w Astrze J

**przód lewy (FL) → przód prawy (FR) → tył prawy (RR) → tył lewy (RL)**

(kolejność konfigurowalna przez `LEARN_ORDER_0..3` / kolejność wpisów w tablicy
`SENSOR[]` w `main.cpp`).

### Sterowanie (przycisk `PIN_BUTTON`)

| Gest                     | Tryb NORMAL              | Tryb LEARN               | Tryb SINGLE              |
|--------------------------|--------------------------|--------------------------|--------------------------|
| krótkie naciśnięcie      | natychmiast burst 4 kół  | **następne koło**        | **następne koło**        |
| długie (≥1,5 s)          | → **LEARN**              | → NORMAL                 | → LEARN                  |
| bardzo długie (≥4 s)     | → **SINGLE**             | → SINGLE                 | → NORMAL                 |

(Przycisk nie jest drzemany przy wciśniętym — urządzenie nie wejdzie w sen,
dopóki go nie puścisz; bardzo długi gest zdąży się zawsze zarejestrować.)

LED: seria **N mrugnięć = N-ty numer koła** (1–4) przy zmianie koła,
wzorzec stały: NORMAL — 1 mrugnięcie co 2 s, LEARN — 3 mrugnięcia co 0,5 s,
SINGLE — 2 mrugnięcia co 0,5 s, niskie napięcie — 10 mrugnięć co 3 s.

### Procedura krok po kroku

1. **Odczytaj oryginalne ID** (jeśli to możliwe) — patrz strategia A w sekcji 6.
2. Włącz zapłon, uruchom procedurę nauki TPMS w aucie
   (Astra J: menu kierowcy → ustawienia → ciśnienie w oponach → nauka /
   „Tyre learning"; w wersjach bez menu — procedura z pedałem gazu
   lub przez OP-COM/TECH2 — sprawdź instrukcję obsługi swojej wersji).
3. **Wejdź w tryb LEARN** (długie naciśnięcie) — urządzenie nadaje
   **tylko koło FL** (ID z `SENSOR_FL_ID`), LED mruga 1×.
4. Urządzenie nadaje burst co 2 s. Jeśli procedura w aucie wymaga
   **aktywacji czujnika przy kole** (puls LF 125 kHz / programator TPMS):
   * podejdź do koła FL i użyj programatora LF — jeśli masz moduł LF
     (`PIN_LF_TRIGGER`), urządzenie **natychmiast** wyśle właściwe ID,
   * bez modułu LF: **krótkie naciśnięcie przycisku = „aktywuj koło"**
     (w LEARN służy też do przejścia dalej — użyj komendy `send`
     w konsoli, jeśli potrzebna jest aktywacja bez zmiany koła).
5. **Krótkie naciśnięcie = następne koło** (LED: 2×, 3×, 4×) i powtórz
   aktywację przy kolejnych kołach w kolejności **FL → FR → RR → RL**.
6. **Okno czasowe**: na każde koło jest `LEARN_WINDOW_MS` (60 s).
   Po jego upływie urządzenie **wraca do trybu normalnego**
   (`LEARN_WINDOW_EXPIRE_TO_NORMAL 1`); ustaw `0`, jeśli wolisz
   automatyczne przejście do kolejnego koła. Cała sesja learn kończy się
   też po `LEARN_TOTAL_TIMEOUT_MS` (15 min).
7. Zakończ procedurę w aucie (zwykle: zapłon OFF, jazda testowa).
8. Po zakończeniu nauki urządzenie samo wraca do NORMAL i nadaje
   wszystkie 4 koła co 30 s — także przy postoju (transmisja jest ciągła,
   niezależna od ruchu; auto-learn może wymagać jazdy, ale ramki lecą zawsze).

### Jeśli auto „nie zapamiętuje" czujników

* **Użyj oryginalnych ID** (strategia A) — wtedy nie trzeba w ogóle
  programować; ECU zna już te ID i zaakceptuje je natychmiast.
* Sprawdź, czy procedura nauki w aucie była uruchomiona **przed**
  transmisjami (ECU w trybie nauki nasłuchuje nowych ID).
* W trybie LEARN nadawane jest **tylko jedno ID naraz** — jeśli nadawałeś
  wszystkie 4 jednocześnie, ECU mógł się „pomylić" (stąd tryb learn).
* Upewnij się, że ciśnienie jest w zakresie akceptowanym przez ECU
  (260 kPa = 2,6 bar jest bezpieczne dla Astry J).
* Spróbuj wariantu modulacji: `MODE_OOK` ↔ `MODE_FSK` (patrz sekcja 7) —
  wersje EU/US różnią się; referencyjne nagrania EG53MA4 to **ASK/OOK**.
* Procedura nauki niektórych wersji Astry J wymaga **jazdy > 20 km/h**
  przez kilka minut — urządzenie nadaje przez cały czas, więc wystarczy
  wykonać jazdę próbną po starcie procedury.
* Sprawdź DTC przed i po (OP-COM/TECH2): kody czujników (np. C0750–C0759)
  powinny zniknąć po udanej nauce.
* Zmierz napięcie zasilania (`batt`) — przy `POWER_12V` urządzenie raportuje
  stan akumulatora i flagę niskiego napięcia w ramce (`LOWBATT`).

### Tryb SINGLE

Wymuszone nadawanie jednego ID (testy, procedury uczące jedno koło
wielokrotnymi próbami):

* bardzo długie naciśnięcie (≥4 s) — SINGLE na bieżącym kole,
* konsola: `mode single 2` (koło FR), `next` — zmiana koła,
* wyjście: bardzo długie naciśnięcie lub `mode normal`,
  limit czasu `SINGLE_TIMEOUT_MS` (domyślnie 5 min).

---

## 9. (e) Pomiar poboru prądu

### Co zmierzyć

| Stan                 | Co się dzieje                                     | Oczekiwana wartość |
|----------------------|---------------------------------------------------|--------------------|
| **Deep sleep**       | między transmisjami (większość czasu)             | **< 50 µA** (goły moduł ESP32 + CC1101 + buck o niskim Iq); ~10 µA ESP32 + < 1 µA CC1101 + Iq przetwornicy |
| **Transmisja (burst)** | 4 koła × 3 ramki, ~0,35 s na antenie + 1,2 s aktywności | ~30 mA (CC1101 TX) + ~80 mA (ESP32) przez ok. 1,2 s co interwał |
| **Średnia**          | suma                                          | ~4 mA przy interwale 30 s, ~1 mA przy 120 s (patrz sekcja 13) |

### Jak zmierzyć

1. **Sen** — multimetr w zakresie µA **szeregowo z zasilaniem**,
   po 30 s od ostatniej transmisji (gdy ESP32 jest w deep sleep).
   Mierz na pinach modułu, nie przez port USB!
2. **Burst** — rezystor bocznikowy 1 Ω + oscilloskop albo INA219/INA226
   (szyna I²C), odczyt średniego prądu przez ≥ 60 s.
3. **Średnia doby** — mierz przez 1–2 minuty w zakresie mA i przelicz
   (albo użyj miernika z integracją, np. µCurrent + logger).
4. Porównaj z budżetem z sekcji 13; odchylenie 2× w dół/w górę jest normalne.

### Typowe pułapki

* **DevKit z USB-UART**: mostek CP2102/CH340 + LDO AMS1117 = 0,5–2 mA
  w sleep → cel < 50 µA **niedotrzymany z winy płytki**, nie firmware'u.
  Rozwiązanie: goły moduł ESP32-WROOM + regulator z sekcji 5
  (MCP1703/ LM5164-Q1 dla 12 V, MCP1700 dla LiPo).
* **Dzielniki pomiarowe**: 2 × 1 MΩ/270 kΩ ≈ 19 µA stale — to już połowa
  budżetu < 50 µA; w wariancie bateryjnym możesz je całkowicie odpiąć
  (`PIN_VBAT_ADC -1` nie jest obsługiwany — po prostu nie montuj dzielnika
  i zignoruj odczyt) albo zwiększyć rezystancje kosztem dokładności ADC.
* Rezystor podciągający przycisku (10 kΩ) pobiera 330 µA przy stanie
  aktywnym niskim — w sleep przycisk jest w stanie spoczynkowym (HIGH),
  więc prąd nie płynie; gdybyś użył aktywnego HIGH, zwiększ rezystor do 100 kΩ.
* LED — w sleep zgaszony (firmware go gasi przed snem).

---

## 10. (f) Weryfikacja, że błąd zniknął

1. **Przed testem**: skasuj stare DTC (OP-COM/TECH2 lub odłącz akumulator
   na kilka minut — utrata ustawień radia/godziny jest możliwa).
2. Uruchom procedurę learn w aucie (sekcja 8) albo użyj oryginalnych ID.
3. Włącz zapłon, odczekaj 1–3 minuty (ECU zbiera ramki) lub wykonaj jazdę
   próbną 5–10 min powyżej 20 km/h.
4. Sprawdź:
   * **kontrolka TPMS** (żółta ikona opony) — powinna zgasnąć,
   * **wyświetlacz kierowcy** (DIC): menu ciśnienia w oponach powinno
     pokazywać 4 wartości (2,6 bar),
   * **OP-COM/TECH2**: brak aktywnych kodów czujników TPMS,
   * **rtl_433**: `rtl_433 -f 433.92M` dekoduje 4 ID co ok. 30 s
     (to potwierdza, że transmisja jest ciągła).
5. Test długotrwały: pozostaw auto na noc z włączonym zapłonem raz na
   kilka godzin — po ponownym uruchomieniu błąd nie powinien wracać.
6. Jeśli kontrolka zapala się ponownie po dłuższym postoju — sprawdź
   interwał parkingowy (`TX_INTERVAL_PARKING_S`) i flagę baterii
   (`TPMS_FLAGS_LOWBAT_MASK`).

---

## 11. Tryby pracy i sterowanie

| Tryb     | Zachowanie                                                                 | Wejście                        |
|----------|----------------------------------------------------------------------------|--------------------------------|
| NORMAL   | rotacja 4 kół: burst 3 ramek × 4 koła co `TX_INTERVAL_S` (30 s), potem deep sleep | stan domyślny, `mode normal`   |
| LEARN    | **tylko jedno koło**, burst co 2 s, okno 60 s na koło, przycisk = następne koło | długie naciśnięcie, `mode learn` |
| SINGLE   | wymuszone jedno ID, burst co 2 s                                           | bardzo długie naciśnięcie, `mode single n` |

### Deep sleep i budzenie

* budzik RTC co `TX_INTERVAL_S` / `TX_INTERVAL_IGNITION_S` /
  `TX_INTERVAL_PARKING_S`,
* **EXT0**: przycisk (`PIN_BUTTON`) — natychmiastowe wybudzenie,
* **EXT1**: wejście LF (`PIN_LF_TRIGGER`) — aktywacja czujnika przy kole
  = natychmiastowa transmisja odpowiedniego ID,
* między transmisjami: CC1101 w SLEEP (`SPWD`), linie SPI trzymane przez
  `gpio_hold` (CS wysoki — utrzymuje CC1101 w sleep, SCK/MOSI niskie),
* WiFi **nie jest w ogóle inicjalizowany**, BT wyłączany (`btStop()`),
  brak dynamicznej alokacji pamięci (same bufory statyczne),
  brak opóźnień blokujących — jedyny „blocking" to oczekiwanie na
  zakończenie nadania ramki (~29 ms) i odstępy między ramkami w burście
  (transmisja musi fizycznie trwać tyle, ile trwa).

### Tryb ignition-aware (ACC)

* `PIN_ACC` wysoki (zapłon) → interwał `TX_INTERVAL_IGNITION_S` (15 s),
* po zaniku ACC + `ACC_GRACE_MS` (5 min) → interwał `TX_INTERVAL_PARKING_S`
  (120 s) albo całkowite uśpienie (`PARKING_SHUTDOWN 1`) — budzi wtedy
  tylko przycisk/LF,
* bez podłączonego ACC (`PIN_ACC -1`) urządzenie pracuje stale
  z interwałem `TX_INTERVAL_S`.

---

## 12. Konsola szeregowa

115200 8N1, po starcie okno `BOOT_STAY_AWAKE_MS` (3 s) — wciśnij `stay 60`,
żeby zostać dłużej. Komendy:

| Komenda                      | Działanie                                      |
|------------------------------|------------------------------------------------|
| `help`                       | lista komend                                    |
| `info` / `config`            | wersja, konfiguracja, ID, stan, napięcie        |
| `hex [n\|all]`               | hexdump ramki koła n (1–4) lub wszystkich       |
| `send [n\|all]`              | wyślij burst teraz                              |
| `batt`                       | pomiar napięcia zasilania [mV]                  |
| `mode normal\|learn\|single [n]` | zmiana trybu                                |
| `next`                       | następne koło (learn/single)                    |
| `interval <s>`               | ręczny interwał (0 = automatycznie wg ACC)      |
| `flags <hex>`                | słowo `flags` wysyłane w ramce                  |
| `sleep`                      | deep sleep teraz                                |
| `stay [s]`                   | czuwaj jeszcze s sekund                         |

Przykładowy `hex 1` (koło FL):

```
FL frame: AA AA AA AA AA AA AA AA AA AA CB 4B 2C AA AA AA D4 AA B3 55 4B 2A D4 AC D3 2A CB 2C D2 CC
FL payload: 4C 90 00 70 2F C8 71 68 49 65  crc=65 id=2FC871 press=260.0 kPa temp=73 F
```

---

## 13. Budżet prądu

Założenia: burst = 4 koła × 3 ramki, ramka na antenie 240 chipów × 120 µs
= **28,8 ms**, przerwa 50 ms między ramkami; wybudzenie + konfiguracja CC1101
+ ADC ≈ 1,2 s aktywności na cykl; ESP32 aktywny ~80 mA, CC1101 TX ~30 mA
(+10 dBm), CC1101 idle ~15 mA, deep sleep ≈ 15 µA (ESP32 ~10 µA + CC1101
< 1 µA + Iq regulatora) **+ ~19 µA na dzielnikach pomiarowych**
(2 × 1 MΩ/270 kΩ). Przeliczenia poniżej robione są jednym wzorem:
pobór z 12 V = pobór na szynie 3,3 V × (3,3/12) / sprawność regulatora (0,9).

**Cykl co 30 s** (2880 cykli/dobę):

| Składnik                              | Ilość na cykl      | Ładunek na cykl |
|---------------------------------------|--------------------|-----------------|
| ESP32 aktywny (80 mA × 1,2 s)         | 1,2 s              | 96 mAs ≈ 27 µAh |
| CC1101 TX (30 mA × 0,35 s)            | 0,35 s             | 10 mAs ≈ 3 µAh  |
| CC1101 idle/reszta (15 mA × 0,85 s)   | 0,85 s             | 13 mAs ≈ 4 µAh  |
| deep sleep + dzielniki (~34 µA × 30 s)| 30 s               | ≈ 0,3 µAh       |
| **Razem**                             |                    | **≈ 34 µAh/cykl** |

* **Dobowy pobór na szynie 3,3 V: 34 µAh × 2880 ≈ 98 mAh/dobę**
  (średnio ≈ 4 mA).
* Przy zasilaniu 12 V (sprawność 90%): **≈ 30 mAh/dobę z akumulatora**
  (98 × 3,3/12/0,9 = 30; średnio ≈ 1,25 mA przy 12 V).

**Akumulator 60 Ah (Pb), bez innych odbiorników:**

* prąd potrzebny do rozruchu wymaga napięcia > ~11,8 V, czyli ~50% pojemności
  = 30 Ah użyteczne,
* 30 000 mAh / 30 mAh ≈ **1000 dni ≈ 2,7 roku** — teoretycznie;
* **w praktyce granicą jest samorozładowanie akumulatora kwasowego
  (3–5%/mies.)** — akumulator bez odbiorników rozładowuje się w 6–12 mies.,
  więc symulator **nie jest czynnikiem limitującym**.

**Warianty oszczędne** (pobór z 12 V z uwzględnieniem sprawności 0,9):

| Interwał | Średni prąd (3,3 V) | Dobowy pobór (12 V) | 60 Ah (30 Ah użyteczne) |
|----------|--------------------|--------------------|--------------------------|
| 30 s     | ~4 mA              | ~30 mAh/dobę       | ~2,7 roku*               |
| 120 s (parking) | ~1 mA       | ~8 mAh/dobę        | ~10 lat*                 |
| 300 s    | ~0,4 mA            | ~3,5 mAh/dobę      | ogranicza samorozładowanie |

\* przed samorozładowaniem akumulatora.

**Wariant bateryjny** (Li-SOCl2 3,6 V 2400 mAh, bez strat przetwornicy):
~98 mAh/dobę przy 30 s to ok. **24 dni**; przy 300 s ~10 mAh/dobę to ok.
**240 dni (~8 mies.)**. Do zasilania bateryjnego stosuj dłuższe interwały,
mniejszą liczbę ramek w burście (`TX_BURST_FRAMES 2`) albo
`PARKING_SHUTDOWN 1`.

Wskazówki: zmniejsz `TX_BURST_FRAMES`, wydłuż interwał, użyj mocy TX
`TPMS_TX_POWER_DBM 0` (wystarczy w aucie), ustaw `PARKING_SHUTDOWN 1`
(przy postoju sen do przycisku).

---

## 14. Protokół — szczegóły techniczne

Dekoder referencyjny: `rtl_433/src/devices/schraeder.c` →
`schrader_EG53MA4` („Schrader TPMS EG53MA4, Saab, Opel, Vauxhall,
Chevrolet"), zasilany sygnałem **OOK Manchester** (`OOK_PULSE_MANCHESTER_ZEROBIT`,
`s=123 µs`, `r=300 µs`). Wymagana długość dekodowanego wiersza: **120 bitów**.

Ramka (bity po dekodowaniu Manchester):

```
 0        40       56        80        88        96      104
 +--------+--------+---------+---------+---------+--------+
 |  sync  |   flags (4 B)    |   id (3 B)        |press|T |crc|
 | 40 bit | 4c   90   00   70| 2F   C8   71      | 68  |49| 65|
 +--------+--------+---------+---------+---------+--------+
```

* `flags` — 4 bajty (status/bateria, wg dekodera nie w pełni rozpoznane),
* `id` — 24-bitowy identyfikator czujnika,
* `press` — ciśnienie: **25 mbar/bit = 2,5 kPa/bit** (260 kPa → `0x68`),
* `T` — temperatura w **°F** (23 °C → `73`),
* `crc` — **suma 9 poprzednich bajtów mod 256** (`add_bytes` w rtl_433).

Kodowanie na warstwie RF (G.E.T. Manchester, zgodnie z analizą nagrań
`rtl_433_tests/tests/Schrader_EG53MA4_TPMS/01/`):

* chip = **120 µs** (8,33 kBaud),
* bit `0` → chipy `01`, bit `1` → chipy `10`,
* nadajnik wysyła **1 chip rozbiegowy + 119 komórek** (ramkę bez pierwszego
  bitu — pierwszy bit dopisuje dekoder MC_ZEROBIT), dopełnienie zerowe do
  bajtu jest ciszą w eterze (OOK),
* modulacja: **ASK/OOK 433.92 MHz** (jak w referencyjnych nagraniach;
  FCC MRXEG53MA4: „Type of Modulation: ASK + FSK", „Frames are Manchester
  encoded ASK data"), opcjonalnie **2-FSK 433.92 MHz, dewiacja 47,6 kHz**
  (`TPMS_MODULATION MODE_FSK`).

Weryfikacja implementacji (bez sprzętu):

* `tools/tpms_frame.py` — referencyjna implementacja ramki + **kopia
  dekodera `pulse_slicer_manchester_zerobit()` z rtl_433** (self-test 500
  losowych ramek + wektor z prawdziwego nagrania `gfile001.cu8`),
* `tools/gen_cu8.py` + `tools/verify_rtl433.sh` — synteza nagrań IQ
  i dekodowanie **prawdziwym rtl_433** (wynik w sekcji 4),
* `pio test -e native` — testy kodu C firmware'u (te same wektory).

---

## 15. Rozwiązywanie problemów

| Objaw | Przyczyna / rozwiązanie |
|---|---|
| `BLAD: brak modulu CC1101` | Sprawdź SPI (SCK=18, MISO=19, MOSI=23, CS=5), zasilanie 3,3 V, masę |
| rtl_433 nic nie dekoduje | Sprawdź antenę, odległość (< 1 m na test), `rtl_433 -f 433.92M`; spróbuj `MODE_FSK` + decoder flex; sprawdź `send` w konsoli i `hex` |
| rtl_433 dekoduje inne ID niż w config | Skompiluj i wgraj ponownie po zmianie `config.h`; sprawdź `info` |
| Auto nie zapamiętuje ID | Strategia A (oryginalne ID) lub procedura learn w trybie LEARN (jedno koło naraz), jazda testowa > 20 km/h |
| Kontrolka TPMS świeci mimo OK | Zmień `TPMS_FLAGS` / `TPMS_FLAGS_LOWBAT_MASK`, skasuj DTC, powtórz naukę |
| Pobór w sleep > 50 µA | DevKit z mostkiem USB — użyj gołego modułu; sprawdź rezystory podciągające i dzielnik pomiarowy (R1+R2 = 133 kΩ → 27 µA przy 3,6 V — rozważ 470 kΩ/150 kΩ) |
| Reset przy transmisji | Za słabe zasilanie — dodaj kondensator 100 µF przy CC1101, użyj bucka o wystarczającym prądzie |
| `pio test -e native` nie przechodzi | Upewnij się, że nie zmieniłeś `tpms_frame.cpp` bez aktualizacji wektorów w teście (generuj przez `tools/tpms_frame.py`) |

---

## Wersje

* `CONFIG_VERSION` w `src/config.h` — podnieś przy zmianie formatu danych
  w pamięci RTC (magic RTC zawiera tę wersję, więc stan zostanie wyzerowany).
* Firmware testowany z: PlatformIO `espressif32@8.0.0`, Arduino core ESP32,
  `SmartRC-CC1101-Driver-Lib` commit `de6757e` (3.0.2, wymagane ≥ 2.5.7),
  rtl_433 (master).

**Pamiętaj: symulator generuje sztuczne dane ciśnienia i nie zastępuje
prawdziwych czujników TPMS.**
