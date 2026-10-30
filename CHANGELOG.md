# Changelog

## Zmiany robocze po 2.0.0-rc.1

- Mikrodrzemki zamiast jednego długiego snu (`config.h`: `NAP_PERIOD_MS`):
  LED miga co ok. 2 s także między wysyłkami, po każdym budzeniu timera jest
  widoczne mignięcie (`NAP_AWAKE_MS`). Sen to reszta okresu po czasie
  boot + okno; TX ramek pozostaje co interwał (licznik snu w pamięci RTC,
  `interval` wchodzi w życie przy następnym cyklu). `power_deep_sleep`
  przyjmuje milisekundy. Wcześniej budzenie z timera gasiło LED w tym samym
  `loop()` — błysk trwał kilka ms i budzenie było niewidoczne.
- Przycisk: domyślnie przycisk BOOT płytki (`PIN_BUTTON = 0`, GPIO0) —
  potwierdzone działanie EXT1 na sprzęcie. Zewnętrzny przycisk na GPIO3
  przez `-DPIN_BUTTON=3`. Uwaga: GPIO0 to pin strapping (trzymany przy
  starcie/reset uruchamia bootloader). Każdy gest przycisku daje okno
  konsoli 10 s (wg `POWER.md`) — urządzenie nie zasypia natychmiast po
  np. wyjściu z LEARN/SINGLE. LEARN/SINGLE nadal nie śpią (bez zmian).
- Kalibracja pola ciśnienia `P` wg wyświetlacza auta: skala rtl_433
  (2,5 kPa/bit) nie jest potwierdzona (nagrania mają `P=0`), a auto pokazywało
  284 kPa dla 260 kPa. Korekta skalująca `TPMS_PRESSURE_KPA100_PER_RAW`
  (273 = 2,73 kPa/bit; `P=95` dla 260 kPa) w firmware, modelu Pythona
  i oczekiwaniach `verify_rtl433.py`. Na podstawie jednego punktu
  pomiarowego — do dostrajania po kolejnych odczytach z auta.
- Audyt integracji sprzęt–firmware (2026-10-28), spójność ESP32-S2-MINI +
  CC1101 SMA + LM317T:
  - Budzenie EXT1: pociągnięcia RTC przycisku i LF konfigurowane przed deep
    sleep. Pull-up cyfrowy (`pinMode`) nie działa przy wyłączonej domenie
    RTC_PERIPH (dokumentacja ESP-IDF), więc piny budzenia pływały i EXT1
    mogło budzić losowo. Zalecany też zewnętrzny 10 kΩ.
  - Konsola: parsowanie liczb odrzuca NaN i nieskończoność (wcześniej
    `pressure nan` / `rf freq nan` przechodziły walidację; NaN trafiał do NVS
    jako ciśnienie, a `rf freq nan` blokował późniejsze zapisy). Logika
    parsowania wydzielona do przenośnego modułu `parse` z testami native.
  - Zasilanie: dzielnik ADC R2 270 kΩ → 180 kΩ. Poprzedni dzielnik nasycał
    ADC (pełna skala 2,5 V) już powyżej ok. 11,8 V i nie mierzył napięcia
    instalacji 12 V w warunkach pracy 12,6–14,4 V.
  - `tools/package_release.sh`: pełny obraz `merge_bin` z `--flash-freq 80m`
    zgodnym z manifestem `lolin_s2_mini`; poprzednie `40m` nadpisywało
    nagłówek bootloadera (80 MHz) innym parametrem niż build.
  - `tools/tpms_frame.py`: saturacja temperatury do 0–255 zgodna z firmware
    (temperatury ≤ −19 °C psuły ramkę w modelu Pythona).
  - Testy: ujemny przypadek checksum odwraca całą komórkę Manchester
    (poprzednio ćwiczył naruszenie Manchester zamiast sumy); nowy zestaw
    `test_parse` (zakresy, NaN/inf); poprawiony diagram bitów w `PROTOCOL.md`.
- Sprzęt docelowy zmieniono na ESP32-S2-MINI (ESP32-S2FN4R2, np.
  ARD-ESP32-S2-MINI, płytka `lolin_s2_mini`); zaktualizowano target PlatformIO,
  natywne USB CDC, pinout SPI/przycisku/ADC i polaryzację wbudowanej LED
  (GPIO15, aktywna HIGH). ADC zasilania na GPIO1, opcjonalny LF na GPIO14.
  `btStop()` kompilowane tylko na chipach z Bluetooth — ESP32-S2 go nie ma.
- Moduł radiowy CC1101 433 MHz SPI ze złączem SMA i anteną (ARD-CC1101-ANT).
- Zasilanie zewnętrzne przez regulowany LM317T (TO-220 metal, 1,25–37 V,
  1,5 A): właściwy pinout ADJ–VOUT–IN, TAB = VOUT, dzielnik 240/390 Ω
  dający ok. 3,28 V, kondensatory oraz wymagania ochrony i termiki.
- Docelowe zasilanie z gniazda zapalniczki samochodowej: ładowarka USB
  (wariant A) albo LM317T (wariant B). Przy gnieździe zasilanym „po stacyjce”
  emulator nie pobiera prądu na postoju; brak transmisji parkingowej jest
  akceptowalny, bo ECU auta nie odczytuje TPMS przy wyłączonym zapłonie.
- Obrazy release dla ESP32-S2 używają `esptool --chip esp32s2` i bootloadera od
  adresu `0x1000`; stare obrazy dla ESP32-C3 i ESP32-WROOM nie są kompatybilne.

## 2.0.0-rc.1 — 2026-10-27

Release candidate bez testu fizycznego CC1101 i samochodu.

### Dodane

- Trwała konfiguracja NVS z wersją, CRC32, walidacją zakresów, unikalnymi ID,
  przywracaniem fabrycznym i bezpiecznymi domyślnymi.
- Konsola: Clone IDs, generator nowych ID, ciśnienia/temperatura, interwały,
  ograniczone RF, LEARN/SINGLE i diagnostyka bez rekompilacji.
- Przenośna maszyna stanów z fake clock/radio, timeoutami i kolejnością
  FL→FR→RR→RL. Testy restartu, ACC, błędów radia i zawinięcia `millis()`.
- Niezależna walidacja czterech nagrań referencyjnych oraz syntetycznych CU8
  przypiętym rtl_433; testy negatywne checksum/Manchester/ucięcie.
- `tools/check_all`, Dockerfile, Gitea Actions, skrypty obrazów i SHA-256.
- Polska dokumentacja użytkownika, sprzętu, zasilania, bezpieczeństwa i HIL.

### Zmienione

- Domyślnie 260 kPa i −10 dBm, tylko OOK. Zachowany enkoder 30 B Manchester,
  który przechodzi niezależne dekodowanie.
- CC1101 czeka na `MARCSTATE=IDLE`, kontroluje underflow i pozostałe bajty,
  zamiast polegać wyłącznie na czasie i `TXBYTES=0`.
- Zwykły RTC nie otwiera okna konsoli; transmisja i powrót do snu ograniczają
  szacowany pobór. Okno konsoli pozostaje po interakcji użytkownika.
- Pełny build z `-Wall -Wextra -Werror`; cppcheck bez defektów.

### Usunięte / naprawione

- Usunięto zgadywany `TPMS_FLAGS_LOWBAT_MASK` i sprzężenie niskiego zasilania
  emulatora z symulowanym sensorem. Zasilanie jest tylko ostrzeżeniem lokalnym.
- Usunięto FSK z głównego flow: brak dowodu dla analizowanego profilu.
- Usunięto `PARKING_SHUTDOWN`; każdy deep sleep ma RTC, więc restart ACC nie
  może zostawić urządzenia trwale uśpionego.
- Domyślne, niezwalidowane wejście LF jest wyłączone. Podstawowy LEARN nie
  wymaga EL-50448.
- Usunięto rekomendację MCP1703 + TVS 24 V jako ochrony automotive i obietnice
  wieloletniej pracy baterii. Zasilanie 12 V ma wymagania projektowe, nie HIL.

### Iteracje audytu

1. **Stan początkowy:** 7/7 native i build PASS, ale konfiguracja przez
   rekompilację, monolit stanów, lowbat i zasilanie wymagały naprawy. Utworzono
   `archive/pre-opencode`.
2. **Model danych:** dodano NVS/CRC/walidację i przenośne tryby. Pierwszy test
   wykrył błędne oczekiwania testów dla zaległego tick i natychmiastowego TX;
   po korekcji 20/20 native i ESP32 build PASS.
3. **Walidacja RF:** audyt czterech nagrań potwierdził 16 powtórzeń i zmienne
   odstępy. Zastąpiono pojedynczy wektor pełnym skryptem; 64/64 referencji,
   3/3 syntetycznych i 3/3 negatywnych PASS.
4. **Energia:** wykryto okno konsoli po każdym RTC; ograniczono je do interakcji.
   Pobór nadal wyraźnie oznaczony jako szacowany.
5. **Statycznie:** cppcheck wskazał inicjalizację struktur z float przez memset;
   zamieniono na inicjalizację wartości i osiągnięto 0 defektów.

### Niezweryfikowane fizycznie

CC1101 na antenie/analizatorze, RTL-SDR z prototypem, pobór, brownout, ACC,
aktualizacja flash, Clone IDs i LEARN z konkretnym Oplem. Brak oznaczenia
„validated”.
