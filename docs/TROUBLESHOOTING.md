# Rozwiązywanie problemów

Pracuj po jednej zmianie i zapisuj pomiar. Nie przełączaj losowo OOK/FSK, flags,
ID ani interwałów. Podstawowy firmware nie ma trybu FSK.

| Objaw | Możliwa przyczyna | Pomiar / sprawdzenie | Naprawa |
|---|---|---|---|
| `BLAD: brak CC1101` | złe zasilanie, CS/SPI, moduł nie 433 MHz | 3,3 V na VCC-GND; ciągłość SCK/MISO/MOSI/CS = 4/5/6/7; oznaczenie modułu | popraw okablowanie, skróć SPI, użyj właściwego modułu |
| Reset przy TX | spadek 3,3 V, słabe przewody | napięcie przy ESP32/CC1101 podczas `send`, log brownout | popraw zasilacz i 47–100 µF przy module; nie zwiększaj PA |
| `TX-ERR` | underflow, niezakończony TX, zła konfiguracja CC1101 | log dla każdego koła; MARCSTATE/rejestry w HIL | sprawdź SPI i zasilanie; zapisz log do analizy, nie maskuj błędu |
| USB/UART nie odpowiada | za krótkie okno konsoli, zły baud/port, kabel tylko do ładowania | 115200 8N1, reset, właściwy `/dev/ttyACM*` lub `/dev/ttyUSB*` | wpisz `stay 300` od razu po starcie; użyj kabla z danymi |
| Zmiana ID znika | błąd zapisu NVS, pełny flash | komunikat `OK: zapisano`, restart i `info` | nie kontynuuj przy błędzie NVS; sprawdź flash/zasilanie |
| ID odrzucone | duplikat, 0, FFFFFF, zła długość | sześć cyfr hex i `info` | nadaj cztery unikalne ID z 000001–FFFFFE |
| rtl_433 nic nie dekoduje | antena, częstotliwość, zły tor RF, zbyt mała moc | `send 1`; odbiornik 433,92 MHz blisko; pomiar ANT/GDO0 | popraw antenę/zasilanie; stopniowo testuj niskie moce; nie zmieniaj na FSK |
| rtl_433 widzi złe dane | stara konfiguracja, sąsiedni czujnik | `info`, `hex 1`, filtrowanie ID w odbiorniku | porównaj ID i wymuś `send 1` w izolowanym miejscu |
| Auto nie widzi znanych ID | niepotwierdzony profil, ECU wymaga nauki, zasięg | VIN/oznaczenia/DTC; odbiór RTL-SDR przy aucie | potwierdź profil; wykonaj LEARN; zmień położenie po pomiarze |
| LEARN stoi na jednym kole | użyto `send` zamiast `next` | `info` pokazuje aktywne koło | `next` po potwierdzeniu kroku; `send` tylko wymusza TX |
| LEARN wróciło do NORMAL | 60 s na kole / 15 min sesji | czas od wejścia, komunikat i LED | uruchom `mode learn 1` i powtórz świadomie od FL |
| Po RL nie ma następnego koła | sekwencja została zakończona | NORMAL w `info` | zakończ procedurę auta i sprawdź DTC |
| Kontrolka nie gaśnie mimo RTL-SDR OK | ECU nie zna ID, inny profil/flags/procedura | odczyt ID i DTC z diagnostyki, nie tylko SDR | Clone IDs albo LEARN; potwierdź profil auta; nie zgaduj flags |
| Pobór snu duży | SuperMini, LF33CV, LED, dzielniki | pomiar całej płytki i regulatora wg `POWER.md` | popraw zasilanie; nie zmieniaj protokołu |
| Po postoju brak TX | zasilanie tylko z ACC, brownout, błąd NVS/CC1101 | czy po ACC OFF zostaje 3,3 V; log po restarcie, `batt`, `info` | przy zasilaniu tylko z ACC brak TX jest oczekiwany; w innym układzie popraw zasilanie |
| ACC nie zmienia interwału | funkcja domyślnie wyłączona lub zły próg | `PIN_ACC`, napięcie na GPIO OFF/ON | zmontuj zabezpieczone wejście; nigdy 12 V na GPIO |

## Gdy brak odbioru w samochodzie

Kolejność diagnostyczna:

1. **Udowodnij lokalny TX:** `send 1` + RTL-SDR/analizator.
2. **Udowodnij właściwy profil:** VIN, oznaczenia czujników, odczyt auta.
3. **Sprawdź ID i kolejność:** `info`, Clone IDs albo jeden aktywny w LEARN.
4. **Sprawdź zasięg:** niska moc najpierw, konsola/za schowkiem, ewentualnie
   bagażnik; bez demontażu poduszek i zgadywania lokalizacji ECU.
5. **Dopiero analizuj flags:** porównaj z nagraniem oryginalnego czujnika tego
   samego trybu. Brak potwierdzonego bitu low battery — nie ustawiaj go.
6. Jeśli nie ma pomiaru lub danych, zatrzymaj test. Losowe zmiany tworzą nowe
   zmienne i nie stanowią diagnozy.
