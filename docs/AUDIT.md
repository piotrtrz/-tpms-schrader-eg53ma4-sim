# Rejestr audytu release-v2

| Priorytet | Problem początkowy | Kryterium naprawy | Stan / dowód |
|---|---|---|---|
| P0 | Zgadywany bit low battery zależny od zasilania emulatora | brak maski i sprzężenia; test flags | naprawione; grep kodu i 20/20 native |
| P0 | MCP1703 + TVS 24 V opisane jako ochrona automotive | usunąć rekomendację; wymagania i przykład z datasheet | naprawione w HARDWARE |
| P0 | Brak fizycznego dowodu zgodności z autem przedstawiany zbyt pewnie | RC, jawne HIL, weryfikacja VIN/oznaczeń | naprawiona dokumentacja; HIL nadal otwarte |
| P1 | ID/ciśnienia/RF tylko przez rekompilację | NVS z konsolą, walidacja, wersja/CRC | naprawione; test_settings i build |
| P1 | Monolit uniemożliwiał test timeoutów/ACC | przenośny state-machine, fake clock/radio | naprawione; test_app_state |
| P1 | FSK w podstawowym flow bez dowodu | tylko OOK w firmware/generatorze | naprawione |
| P1 | PARKING_SHUTDOWN bez gwarantowanego ACC wake | usunąć albo zapewnić budzenie | usunięty; każdy sen ma RTC |
| P1 | TX kończone czasem i TXBYTES, ryzyko ogona | MARCSTATE/underflow/limit | poprawione; wymaga potwierdzenia HIL |
| P1 | Wektory głównie własne | cztery prawdziwe nagrania + niezależny rtl_433 | 64/64 PASS |
| P2 | Domyślne LF bez zwalidowanego front-endu | wyłączyć domyślnie, LEARN bez LF | naprawione |
| P2 | Interwały przedstawiane jak OE | jawnie eksperymentalne | PROTOCOL/SETUP/POWER |
| P2 | Brak pełnego jednego check/build artefaktów | check_all, CI, Docker, package_release | dodane; wyniki VALIDATION |
| P2 | Okno konsoli po każdym RTC zwiększa pobór | sen od razu po cyklu RTC | poprawione; pomiar HIL otwarty |
| P2 | README niezgodne z zachowaniem | nowy Quick Start, jedna tabela UI | przepisane |

## Otwarte, wymagające sprzętu

Nie można ich zamknąć deklaracją: pomiar CC1101/ogon/moc/częstotliwość, test
RTL-SDR z anteną, pobór i brownout, zabezpieczone ACC, test Clone IDs/LEARN z
konkretną Astrą oraz projekt i test zasilania automotive. Są checklistą w
`VALIDATION.md` i blokują status „validated”.
