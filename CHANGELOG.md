# Changelog

## Zmiany robocze po 2.0.0-rc.1

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
