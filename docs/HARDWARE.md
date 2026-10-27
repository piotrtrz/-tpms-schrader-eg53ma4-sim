# Sprzęt i montaż

## Zakres

Podstawowy release opisuje **prototyp laboratoryjny na ESP32-C3 SuperMini**.
Przewody prototypowe i LF33CV nie są gotowym zestawem do trwałego montażu
w samochodzie. Wariant „po stacyjce” poniżej jest listą połączeń i wymagań
projektowych, nie zwalidowanym schematem automotive.

## BOM prototypu laboratoryjnego

| Element | Wymaganie | Uwagi |
|---|---|---|
| ESP32-C3 SuperMini | ESP32-C3, natywne USB-C do flash/konsoli | nie ESP32-WROOM DevKit; klon trzeba sprawdzić przed zasileniem |
| CC1101 | moduł **433 MHz**, SPI, 3,3 V | nie kupuj wersji 868/915 MHz |
| Antena 433 MHz | dopasowana do modułu; na biurko monopol λ/4 ≈ 17 cm | uziemienie/przeciwwaga zgodnie z konstrukcją modułu |
| Przycisk NO | monostabilny | GPIO3 do GND |
| LED | wbudowana na GPIO8 | aktywna stanem niskim; zewnętrzna LED nie jest potrzebna |
| LF33CV | TO-220, stałe 3,3 V | tylko przy zasilaniu zewnętrznym; nie jest potrzebny przy USB |
| Kondensatory LF33CV | 100 nF na wejściu i min. 2,2 µF na wyjściu | praktycznie 10 µF elektrolityczny o ESR zgodnym z kartą; blisko układu |
| Odsprzężenie modułów | 100 nF przy każdym module + 47–100 µF przy CC1101 | możliwie blisko pinów zasilania |
| Przewody | krótkie, wspólna masa | SPI najlepiej <10 cm |
| Zasilanie laboratoryjne | USB-C 5 V albo LF33CV z bezpiecznego VIN | nie podawaj jednocześnie rywalizujących źródeł 3,3 V |

Do diagnozy RF przydaje się RTL-SDR, ale nie jest częścią emulatora. Standardowy
SuperMini ma niebieską LED GPIO8 i przycisk BOOT GPIO9; przed montażem sprawdź
konkretny klon, bo nazwy handlowe bywają używane dla różnych płytek.

## Pinout i połączenia

| ESP32-C3 SuperMini | CC1101 / element | Zasada |
|---|---|---|
| 3V3 | CC1101 VCC; wyjście LF33CV przy zasilaniu zewnętrznym | **tylko 3,3 V**, nigdy wyjście 5 V |
| GND | CC1101 GND, przycisk, LF33CV GND | wspólna masa |
| GPIO4 | SCK/SCLK | SPI |
| GPIO5 | MISO/SO | SPI |
| GPIO6 | MOSI/SI | SPI |
| GPIO7 | CS/CSn/SS | SPI |
| GPIO10 | GDO0 | diagnostyka biblioteki; GDO2 niepodłączony |
| GPIO3 | przycisk NO do GND | wewnętrzny pull-up; RTC GPIO wakeup, opcjonalny zewnętrzny 10 kΩ |
| GPIO8 | wbudowana LED | aktywna LOW, firmware steruje właściwą polaryzacją |
| GPIO0 | opcjonalny ADC zasilania przez dzielnik | `PIN_VBAT_ADC`; nigdy 12 V wprost |
| GPIO2 | opcjonalne ACC po zmianie `PIN_ACC` i zabezpieczeniu | pin strapping; nigdy 12 V wprost |
| GPIO1 | opcjonalny detektor LF po zmianie `PIN_LF_TRIGGER` | aktywny LOW; RTC GPIO wakeup, domyślnie wyłączone |
| GPIO9 | przycisk BOOT płytki | nie używać do CC1101 ani przycisku sterującego |

GPIO2, GPIO8 i GPIO9 są pinami strapping. Nie dokładaj do nich zewnętrznych
rezystorów ani układów wymuszających stan podczas resetu. GPIO20/21 pozostają
wolne; konsola korzysta z natywnego USB-C, więc zewnętrzny mostek USB-UART nie
jest potrzebny.

Przy laboratoryjnym zasilaniu 3,3 V przez LF33CV: zacznij od VIN = 5 V, ustaw
limit np. 0,5 A, sprawdź polaryzację i napięcie wyjściowe bez podłączonego
SuperMini. Połącz GND, a na czas zasilania zewnętrznego odłącz USB. Prąd
szczytowy zależy od płytki i ustawionego PA.

## LF33CV — wyprowadzenia i kondensatory

Dla **LF33CV w TO-220**, patrząc na oznaczenia od przodu z nóżkami w dół:

| Nóżka | Funkcja | Połączenie |
|---:|---|---|
| 1 | VOUT | 3V3 SuperMini + VCC CC1101 |
| 2 | GND | wspólna masa |
| 3 | VIN | zabezpieczone napięcie wejściowe |

Metalowy TAB jest połączony z GND. **Nie stosuj schematu nóżek 7805** — tutaj
kolejność to OUT–GND–IN. Między VIN a GND daj 100 nF. Między VOUT a GND daj
kondensator min. 2,2 µF; karta LF33C podaje zakres ESR 0,1–10 Ω. Praktyczny
punkt startowy to 10 µF elektrolityczny zgodny z tym wymaganiem, a nie przypadkowy
sam kondensator ceramiczny o bardzo niskim ESR. Dodatkowe 100 nF przy modułach
i 47–100 µF przy CC1101 pozostają potrzebne.

Nie podłączaj VOUT LF33CV do pinu **5V** SuperMini. Pin 5V zasila wbudowany
regulator płytki, natomiast zewnętrzne, stabilizowane 3,3 V należy doprowadzić
do **3V3**. Nie zasilaj jednocześnie z USB i LF33CV bez sprawdzonej separacji
źródeł, ponieważ konstrukcje klonów różnią się połączeniami zasilania.

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

## Wariant „prąd po stacyjce” — wymagania, niezwalidowany projekt

Docelowy tor zasilania z ACC:

```text
ACC 12 V → bezpiecznik przy źródle → ochrona przed odwrotną polaryzacją
        → ograniczenie energii + właściwie dobrany TVS
        → VIN LF33CV → VOUT 3,3 V → 3V3 SuperMini + VCC CC1101
```

Samo wyjście „po stacyjce” nadal ma przepięcia i zakłócenia instalacji pojazdu.
**Nie podłączaj go wprost do VIN LF33CV jako gotowej instalacji samochodowej.**
Zakres roboczego VIN LF33CV kończy się przy 16 V; fakt, że spoczynkowe 12–14,8 V
mieści się w tym zakresie, nie oznacza odporności na crank/load dump ani przepięcia.
Ochrona musi ograniczać napięcie z marginesem poniżej dopuszczalnego VIN regulatora.
TVS o progu powyżej tego VIN nie stanowi ochrony LF33CV.

Wymagania przed montażem:

- bezpiecznik dobrany do przewodów i poboru, blisko źródła; nie zastępuje TVS;
- ochrona przed odwrotną polaryzacją i ujemnymi przepięciami;
- TVS i ograniczenie energii dobrane do rzeczywistego profilu przepięć, bez
  przewodzenia przy normalnym 14,4–14,8 V i bez przekraczania VIN LF33CV;
- kondensatory i ESR zgodne z kartą LF33CV, krótkie pętle prądowe;
- pomiar szczytów prądu SuperMini + CC1101 i sprawdzenie stabilności 3,3 V;
- ACC/ADC z ograniczeniem prądu, clampami i progiem odpornym na zakłócenia;
- obliczenia termiczne dla najwyższej temperatury oraz testy brownout/crank/load
  dump, EMC i ESD na docelowej płytce.

LF33CV jest regulatorem liniowym: `P = (VIN − 3,3 V) × I`. Już przy 14,4 V
i 200 mA trzeba odprowadzić ok. **2,22 W**. Bez obliczenia i radiatora TO-220
może się przegrzewać; samo „do 500 mA” z karty nie gwarantuje zasilania szczytów
ESP32-C3 w każdych warunkach. Zmierz prąd i temperaturę przy nadawaniu.

**Konsekwencja zasilania wyłącznie z ACC:** po wyłączeniu zapłonu cały emulator
traci zasilanie. Nie wykonuje wtedy deep sleep ani transmisji parkingowych, RTC
nie odlicza interwału, a ustawienie `interval parking` nie ma znaczenia. Samo
zasilanie z ACC nie jest wejściem `PIN_ACC`, więc firmware używa `interval normal`.
Jeżeli urządzenie ma nadawać również na postoju, potrzebne jest stale zasilane,
zabezpieczone wejście oraz osobny, zabezpieczony sygnał ACC na GPIO2. Wtedy trzeba
też zmierzyć pobór całego układu: typowo 0,5 mA samego LF33CV plus SuperMini,
CC1101 i dzielniki może być istotny przy długim postoju.

Parametry LF33CV i wymagane kondensatory: karta ST
[LFxxAB/LFxxC](https://www.st.com/resource/en/datasheet/lfxx.pdf), tabele LF33C
i rysunek połączeń TO-220. Dostęp: 2026-10-27.

## Zasilanie bateryjne

Ogniwo 18650 w kabinie **nie jest rekomendowane**: temperatura w samochodzie może
przekraczać bezpieczne warunki ogniwa, a zainstalowane ogniwo wymaga ochrony,
właściwego ładowania i obudowy. ESP32 z radiem i okresowym czuwaniem zwykle daje
krótki czas pracy; deklaracje wieloletniej pracy byłyby nieuzasadnione. Wieloletni
wariant bateryjny wymaga innej architektury low-power i nie należy do tego release.

## Opcjonalny LF

RC działa bez EL-50448. Nie ma w nim zwalidowanego odbiornika LF. Przyszły moduł
wymaga osobnego schematu rezonansu/detekcji 125 kHz, progu komparatora, filtracji,
testu odporności na zakłócenia i udowodnionego budzenia. Wyjście front-endu na
GPIO1 ma być aktywne stanem niskim i w spoczynku wysokim. Sam aktywator nie
wpisuje ID do emulatora.
