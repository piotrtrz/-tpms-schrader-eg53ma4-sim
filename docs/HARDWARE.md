# Sprzęt i montaż

## Zakres

Podstawowy release opisuje **prototyp laboratoryjny**. DevKit, przewody
prototypowe i tania przetwornica nie są gotowym zestawem do trwałego montażu
w samochodzie. Wariant automotive poniżej jest listą wymagań projektowych,
nie zwalidowanym schematem.

## BOM prototypu laboratoryjnego

| Element | Wymaganie | Uwagi |
|---|---|---|
| ESP32 DevKit z ESP32-WROOM-32 | 3,3 V, USB do flash/konsoli | tylko biurko; mostek USB i LDO zwiększają pobór |
| CC1101 | moduł **433 MHz**, SPI, 3,3 V | nie kupuj wersji 868/915 MHz |
| Antena 433 MHz | dopasowana do modułu; na biurko monopol λ/4 ≈ 17 cm | uziemienie/przeciwwaga zgodnie z konstrukcją modułu |
| Przycisk NO | monostabilny | GPIO26 do GND |
| LED + rezystor | 330 Ω | GPIO27, LED do GND |
| Kondensatory | 100 nF przy każdym module + 47–100 µF przy CC1101 | możliwie blisko pinów zasilania |
| Przewody | krótkie, wspólna masa | SPI najlepiej <10 cm |
| Zasilanie laboratoryjne | USB 5 V do DevKit albo regulowane 3,3 V z limitem prądu | nie podawaj jednocześnie rywalizujących źródeł |

Do diagnozy RF przydaje się RTL-SDR, ale nie jest częścią emulatora. Do wersji
energooszczędnej użyj gołego ESP32-WROOM-32E na własnej płytce, z poprawnym
zasilaniem, resetem, bootstrapem i złączem programowania. Sam moduł nie ma USB.

## Pinout i połączenia

| ESP32 | CC1101 / element | Zasada |
|---|---|---|
| 3V3 | CC1101 VCC | **tylko 3,3 V** |
| GND | CC1101 GND, przycisk, LED | wspólna masa |
| GPIO18 | SCK/SCLK | SPI |
| GPIO19 | MISO/SO | SPI |
| GPIO23 | MOSI/SI | SPI |
| GPIO5 | CS/CSn/SS | SPI |
| GPIO2 | GDO0 | diagnostyka biblioteki; GDO2 niepodłączony |
| GPIO26 | przycisk NO do GND | wewnętrzny pull-up; opcjonalny zewnętrzny 10 kΩ |
| GPIO27 | rezystor 330 Ω → LED → GND | aktywna wysoko |
| GPIO34 | opcjonalny ADC zasilania przez dzielnik | nigdy 12 V wprost |
| GPIO32 | opcjonalne ACC po zmianie `PIN_ACC` i zabezpieczeniu | nigdy 12 V wprost |
| GPIO25 | opcjonalny detektor LF po zmianie `PIN_LF_TRIGGER` | domyślnie wyłączone; brak gotowego front-endu w RC |

Przy zasilaniu 3,3 V z zasilacza: ustaw limit np. 0,5 A, sprawdź polaryzację,
połącz GND, wyłącz USB jako drugie źródło. Prąd szczytowy zależy od modułu i PA.

## Antena i test położenia

Nie zgadujemy położenia odbiornika TPMS w Astrze J. Test zasięgu wykonaj na
postoju, z niską mocą, porównując odczyt RTL-SDR/diagnostykę dla kilku pozycji:
konsola środkowa, okolica za schowkiem, ewentualnie bagażnik. Wynik dla jednego
egzemplarza nie jest dowodem lokalizacji odbiornika.

Urządzenie mocuj mechanicznie, z dala od poduszek powietrznych i ich wiązek,
strefy pedałów, ruchomych części, wilgoci, źródeł ciepła i metalowych ekranów.
Nie owijaj anteny wiązką, nie prowadź jej przy żarówce/rozdzielaczu LPG ani przy
przewodach zapłonowych. Obudowa prototypu powinna być nieprzewodząca; otwór i
konieczność ekranowania trzeba ocenić pomiarowo.

## Wariant automotive — wymagania, niezwalidowany projekt

Instalacja 12 V wymaga osobnego schematu, obliczeń, PCB i testów. Minimalny łańcuch:

```text
akumulator → bezpiecznik przy źródle → ochrona reverse polarity
          → ograniczenie energii + właściwie dobrany TVS
          → regulator automotive → odsprzężenie → ESP32 + CC1101
ACC → ochrona przepięciowa + dzielnik/komparator → wejście logiczne 3,3 V
```

Wymagania:

- bezpiecznik dobrany do przewodów i poboru, blisko źródła; nie zastępuje TVS;
- ochrona przed odwrotną polaryzacją i ujemnymi przepięciami;
- TVS dobrany do rzeczywistego profilu przepięć, rezystancji źródła i energii;
  napięcie robocze nie może powodować upływu przy 14,4–14,8 V, a napięcie
  ograniczenia musi pozostawić margines względem absolutnego maksimum regulatora;
- regulator z odpowiednim zakresem wejściowym, prądem szczytowym, niskim Iq,
  stabilnością z wybranymi kondensatorami i kwalifikacją automotive;
- odsprzężenie zgodne z kartą regulatora, ESP32 i CC1101, krótkie pętle prądowe;
- ACC/ADC z ograniczeniem prądu, clampami i progiem odpornym na zakłócenia;
- obliczenia termiczne dla najwyższej temperatury, test brownout/crank/load dump,
  EMC, ESD oraz pomiar snu całego urządzenia.

**MCP1703 ma Vin maks. 16 V i nie jest pełną ochroną automotive nawet z TVS 24 V.**
TVS o progu wyższym niż dopuszczalne Vin nie chroni regulatora. Nie używamy tej
rekomendacji.

Przykład do dalszego projektu, nie gotowa rekomendacja montażowa: **LM5164-Q1**,
buck 6–100 V, 1 A, typowy Iq rzędu 10 µA (wartości zależne od warunków). Sprawdzono
z kartą TI [LM5164-Q1](https://www.ti.com/lit/ds/symlink/lm5164-q1.pdf), dostęp
2026-10-27. Projekt musi uwzględnić m.in. schemat aplikacyjny, cewkę, diodę/układ
wejściowy, kondensatory, stabilność, szczyty prądu ESP32, temperaturę i ograniczenie
przepięcia poniżej dopuszczalnego Vin z marginesem. Sama nazwa „100 V” nie dowodzi
odporności całego urządzenia.

## Zasilanie bateryjne

Ogniwo 18650 w kabinie **nie jest rekomendowane**: temperatura w samochodzie może
przekraczać bezpieczne warunki ogniwa, a zainstalowane ogniwo wymaga ochrony,
właściwego ładowania i obudowy. ESP32 z radiem i okresowym czuwaniem zwykle daje
krótki czas pracy; deklaracje wieloletniej pracy byłyby nieuzasadnione. Wieloletni
wariant bateryjny wymaga innej architektury low-power i nie należy do tego release.

## Opcjonalny LF

RC działa bez EL-50448. Nie ma w nim zwalidowanego odbiornika LF. Przyszły moduł
wymaga osobnego schematu rezonansu/detekcji 125 kHz, progu komparatora, filtracji,
testu odporności na zakłócenia i udowodnionego budzenia. Sam aktywator nie wpisuje
ID do emulatora.
