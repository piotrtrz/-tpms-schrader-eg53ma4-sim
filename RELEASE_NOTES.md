# TPMS EG53MA4 Simulator 2.0.0-rc.1

**Release candidate — 2026-10-27. Nie jest to release „validated”.**

Diagnostyczny symulator czterech czujników Schrader EG53MA4 dla ESP32-C3
SuperMini + CC1101 433 MHz. Profil docelowy obejmuje testy z Oplem Astra J Sedan 2016,
ale zgodność konkretnego auta trzeba potwierdzić. Sztuczne dane mogą ukryć usterkę
opony; nie używać w jeździe drogowej.

## Funkcje

- cztery trwałe, niezależne 24-bit ID; domyślnie 260 kPa dla każdego koła;
- OOK/Manchester 433,92 MHz, niska konfigurowalna moc od −10 dBm;
- Clone IDs oraz New IDs + LEARN FL→FR→RR→RL, SINGLE/diagnostics;
- konfiguracja NVS przez UART bez rekompilacji, wersja/CRC/walidacja/factory reset;
- przycisk, LED, timeouty, deep sleep z RTC i opcjonalne sprzętowe ACC;
- automatyczne testy native/RF/build, Docker i artefakty SHA-256.

## Wymagany sprzęt

ESP32-C3 SuperMini, CC1101 **433 MHz** 3,3 V, antena, przycisk i odsprzężenie.
LED jest wbudowana. Do zasilania zewnętrznego przewidziano LF33CV z właściwymi
kondensatorami; wariant 12 V wymaga ochrony wejścia i testów. Pełny BOM/pinout:
HARDWARE. SuperMini ani LF33CV nie są samodzielnym, zwalidowanym rozwiązaniem
automotive.

## Instalacja i aktualizacja

Nowa instalacja: `bash tools/check_all`, potem
`pio run -e esp32c3supermini -t upload`.
Gotowe obrazy: `bash tools/package_release.sh`; adresy i flash opisano w VALIDATION.
Przed pełnym flashem zapisz ID — partycja NVS może zostać skasowana. Konfigurację
sprawdź po restarcie przez `info`. Migracji z wersji 1.0 bez NVS nie ma; należy
wpisać ID w konsoli jeden raz.

## Wyniki testów bez sprzętu (2026-10-27)

- native: 20/20 PASS;
- pełny ESP32 build z `-Werror`: PASS;
- cppcheck: PASS, 0 defektów;
- rtl_433: 64/64 rekordy referencyjne, 3/3 syntetyczne, 3/3 negatywne: PASS.

Dokładne komendy i przypięte commity: VALIDATION/PROTOCOL.

## Znane ograniczenia

- Nie zmierzono rzeczywistej emisji CC1101, ogona, mocy ani częstotliwości.
- Nie wykonano RTL-SDR z anteną, pomiaru poboru, brownout ani testu ACC.
- Nie wykonano Clone IDs/LEARN z samochodem; nie potwierdzono OE 13506028 dla
  konkretnego VIN.
- Znaczenie wszystkich flags pozostaje nieznane; brak zgadywanego low battery.
- Harmonogram jest eksperymentalny, nie potwierdzony OE.
- LF33CV i zasilanie z ACC nie są zwalidowanym projektem automotive; przy
  zasilaniu wyłącznie z ACC nie ma transmisji po wyłączeniu zapłonu.
- Gitea Actions wymaga działającego runnera; lokalne wyniki nie oznaczają CI PASS.

Do czasu odhaczenia HIL w VALIDATION wynik pozostaje release candidate.
