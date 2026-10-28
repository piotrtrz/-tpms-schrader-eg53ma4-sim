# TPMS EG53MA4 Simulator 2.0.0-rc.1

**Release candidate — 2026-10-27. Nie jest to release „validated”.**

Diagnostyczny symulator czterech czujników Schrader EG53MA4 dla ESP32-S2-MINI
(ESP32-S2FN4R2) + CC1101 433 MHz z anteną SMA. Profil docelowy obejmuje testy z Oplem Astra J Sedan 2016,
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

ESP32-S2-MINI (ESP32-S2FN4R2), CC1101 **433 MHz** 3,3 V ze złączem SMA
(np. ARD-CC1101-ANT), antena, przycisk i odsprzężenie. LED jest wbudowana.
Zasilanie docelowe z gniazda zapalniczki: ładowarka samochodowa USB albo
LM317T z dzielnikiem 240/390 Ω i właściwymi kondensatorami; wariant 12 V
wymaga ochrony wejścia i testów. Pełny BOM/pinout: HARDWARE. S2 Mini ani
LM317T nie są samodzielnym, zwalidowanym rozwiązaniem automotive.

## Instalacja i aktualizacja

Nowa instalacja: `bash tools/check_all`, potem
`pio run -e esp32s2mini -t upload`.
Gotowe obrazy: `bash tools/package_release.sh`; adresy i flash opisano w VALIDATION.
Przed pełnym flashem zapisz ID — partycja NVS może zostać skasowana. Konfigurację
sprawdź po restarcie przez `info`. Migracji z wersji 1.0 bez NVS nie ma; należy
wpisać ID w konsoli jeden raz.

## Wyniki testów bez sprzętu (2026-10-28)

- native: 24/24 PASS (po audycie integracji sprzęt–firmware);
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
- LM317T i zasilanie z gniazda zapalniczki nie są zwalidowanym projektem
  automotive; przy zasilaniu wyłącznie z gniazda „po stacyjce” nie ma
  transmisji po wyłączeniu zapłonu — ECU auta wtedy i tak nie nasłuchuje.
- Gitea Actions wymaga działającego runnera; lokalne wyniki nie oznaczają CI PASS.

Do czasu odhaczenia HIL w VALIDATION wynik pozostaje release candidate.
