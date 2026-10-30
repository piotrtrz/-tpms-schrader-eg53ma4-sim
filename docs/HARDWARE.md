# Sprzęt i montaż

## Zakres

Podstawowy release opisuje **prototyp laboratoryjny na ESP32-S2-MINI**
(ESP32-S2FN4R2). Przewody prototypowe i LM317T nie są gotowym zestawem do
trwałego montażu w samochodzie. Wariant z gniazdem zapalniczki poniżej jest
listą połączeń i wymagań projektowych, nie zwalidowanym schematem automotive.

## BOM prototypu laboratoryjnego

| Element | Wymaganie | Uwagi |
|---|---|---|
| ESP32-S2-MINI | ESP32-S2FN4R2 (4 MB flash, 2 MB PSRAM), natywne USB do flash/konsoli; płytka typu Lolin/Wemos S2 Mini, np. ARD-ESP32-S2-MINI | sprzedawana z MicroPythonem — wgrywamy własny firmware; klon trzeba sprawdzić przed zasileniem |
| CC1101 | moduł **433 MHz**, SPI, 3,3 V, ze złączem SMA i anteną, np. ARD-CC1101-ANT | nie kupuj wersji 868/915 MHz |
| Antena 433 MHz | dołączona do modułu SMA; na biurko monopol λ/4 ≈ 17 cm | uziemienie/przeciwwaga zgodnie z konstrukcją modułu |
| Przycisk NO (opcjonalny) | monostabilny | GPIO3 do GND; bez dodatkowego przycisku służy przycisk BOOT płytki (GPIO0) |
| LED | wbudowana na GPIO15 | aktywna stanem wysokim; zewnętrzna LED nie jest potrzebna |
| Ładowarka samochodowa USB | gniazdo zapalniczki → USB 5 V | wariant A zasilania; sprawdzona, z kablem USB-C z danymi |
| LM317T | TO-220 (metal), regulowany 1,25–37 V, 1,5 A | tylko dla wariantu B; nie jest potrzebny przy zasilaniu z USB |
| Rezystory LM317T | R1 = 240 Ω (VOUT–ADJ), R2 = 390 Ω (ADJ–GND) | dają ok. 3,28 V; 1% metalizowane, blisko układu |
| Kondensatory LM317T | 100 nF na wejściu i min. 1 µF na wyjściu | praktycznie 10 µF; blisko układu |
| Radiator | dla LM317T przy zasilaniu z 12–14,4 V | patrz obliczenia termiczne niżej |
| Odsprzężenie modułów | 100 nF przy każdym module + 47–100 µF przy CC1101 | możliwie blisko pinów zasilania |
| Przewody | krótkie, wspólna masa | SPI najlepiej <10 cm |
| Zasilanie laboratoryjne | USB 5 V albo LM317T z bezpiecznego VIN | nie podawaj jednocześnie rywalizujących źródeł 3,3 V |

Do diagnozy RF przydaje się RTL-SDR, ale nie jest częścią emulatora. Standardowy
S2 Mini ma niebieską LED na GPIO15 i przycisk BOOT na GPIO0; przed montażem
sprawdź konkretny klon, bo nazwy handlowe bywają używane dla różnych płytek.

## Pinout i połączenia

| ESP32-S2-MINI | CC1101 / element | Zasada |
|---|---|---|
| 3V3 | CC1101 VCC; wyjście LM317T przy zasilaniu zewnętrznym | **tylko 3,3 V**, nigdy wyjście 5 V ani LM317T ustawionego wyżej |
| GND | CC1101 GND, przycisk, dzielnik LM317T | wspólna masa |
| GPIO4 | SCK/SCLK | SPI |
| GPIO5 | MISO/SO | SPI |
| GPIO6 | MOSI/SI | SPI |
| GPIO7 | CS/CSn/SS | SPI |
| GPIO10 | GDO0 | diagnostyka biblioteki; GDO2 niepodłączony |
| GPIO3 | alternatywny przycisk NO do GND (`-DPIN_BUTTON=3`) | pull-up RTC włączany przez firmware przed deep sleep (pull-up cyfrowy nie działa w deep sleep); RTC GPIO wakeup, zalecany zewnętrzny 10 kΩ |
| GPIO15 | wbudowana LED | aktywna HIGH, firmware steruje właściwą polaryzacją |
| GPIO1 | opcjonalny ADC zasilania przez dzielnik | `PIN_VBAT_ADC`, ADC1_CH0; dzielnik 1 MΩ/180 kΩ (`config.h`), zakres do ok. 16 V; nigdy 12 V wprost |
| GPIO2 | opcjonalne ACC po zmianie `PIN_ACC` i zabezpieczeniu | nigdy 12 V wprost |
| GPIO14 | opcjonalny detektor LF po zmianie `PIN_LF_TRIGGER` | aktywny LOW; RTC GPIO wakeup, domyślnie wyłączone |
| GPIO0 | przycisk BOOT płytki = przycisk sterujący i budzenie RTC (domyślne `PIN_BUTTON`) | pin strapping: trzymany przy zasilaniu/reset uruchamia bootloader zamiast firmware (potwierdzone działanie budzenia na sprzęcie); nie używać do CC1101 |

GPIO0, GPIO45 i GPIO46 są pinami strapping. Nie dokładaj do nich zewnętrznych
rezystorów ani układów wymuszających stan podczas resetu. GPIO19/20 obsługują
natywne USB, a GPIO26–32 w ESP32-S2FN4R2 obsługują flash i PSRAM w obudowie —
nie używaj ich. GPIO33–40 pozostają wolne; konsola korzysta z natywnego USB,
więc zewnętrzny mostek USB-UART nie jest potrzebny.

Przy laboratoryjnym zasilaniu 3,3 V przez LM317T: zacznij od VIN = 5–9 V, ustaw
limit np. 0,5 A, sprawdź polaryzację i napięcie wyjściowe bez podłączonego
S2 Mini. Połącz GND, a na czas zasilania zewnętrznego odłącz USB. Prąd
szczytowy zależy od płytki i ustawionego PA.

## LM317T — wyprowadzenia, dzielnik i kondensatory

Dla **LM317T w TO-220**, patrząc na oznaczenia od przodu z nóżkami w dół:

| Nóżka | Funkcja | Połączenie |
|---:|---|---|
| 1 | ADJ | dzielnik: R1 do VOUT, R2 do GND |
| 2 | VOUT | 3V3 S2 Mini + VCC CC1101 |
| 3 | VIN | zabezpieczone napięcie wejściowe |

Metalowy TAB jest połączony z **VOUT** — nie z GND jak w LF33CV czy 7805.
Radiator i obudowa TAB nie mogą dotykać masy ani innych potencjałów.
**Nie stosuj schematu nóżek 7805** — tutaj kolejność to ADJ–VOUT–IN.

Napięcie wyjściowe ustawia dzielnik: `VOUT = 1,25 V × (1 + R2/R1)`. R1 = 240 Ω
między VOUT a ADJ oraz R2 = 390 Ω między ADJ a GND dają ok. **3,28 V** i
jednocześnie obciążenie ok. 5 mA, zbliżone do minimalnego prądu obciążenia
LM317 — przy większych wartościach rezystorów zmierz VOUT bez obciążenia.
Między VIN a GND daj 100 nF. Między VOUT a GND daj min. 1 µF; praktyczny
punkt startowy to 10 µF. LM317 nie wymaga szczególnego ESR jak LDO, ale
kondensatory mają być blisko układu. Dodatkowe 100 nF przy modułach
i 47–100 µF przy CC1101 pozostają potrzebne.

Nie podłączaj VOUT LM317T do pinu **5V** S2 Mini. Pin 5V zasila wbudowany
regulator płytki, natomiast zewnętrzne, stabilizowane 3,3 V należy doprowadzić
do **3V3**. Nie zasilaj jednocześnie z USB i LM317T bez sprawdzonej separacji
źródeł, ponieważ konstrukcje klonów różnią się połączeniami zasilania.

## Antena i test położenia

Nie zgadujemy położenia odbiornika TPMS w Astrze J. Test zasięgu wykonaj na
postoju, z niską mocą, porównując odczyt RTL-SDR/diagnostykę dla kilku pozycji:
konsola środkowa, okolica za schowkiem, ewentualnie bagażnik. Wynik dla jednego
egzemplarza nie jest dowodem lokalizacji odbiornika.

Urządzenie mocuj mechanicznie, z dala od poduszek powietrznych i ich wiązek,
strefy pedałów, ruchomych części, wilgoci, źródeł ciepła i metalowych ekranów.
Nie owijaj anteny wiązką, nie prowadź jej przy żarówce/rozdzielaczu LPG ani przy
przewodach zapłonowych. Obudowa prototypu powinna być nieprzewodząca; otwór na
złącze SMA i konieczność ekranowania trzeba ocenić pomiarowo.

## Wariant „z gniazda zapalniczki” — wymagania, niezwalidowany projekt

Docelowe zasilanie pochodzi z gniazda zapalniczki samochodowej, na dwa sposoby:

```text
Wariant A (prostszy):
gniazdo zapalniczki → ładowarka samochodowa USB (5 V) → kabel USB-C → USB S2 Mini

Wariant B (stabilizator):
gniazdo zapalniczki 12 V → bezpiecznik przy źródle → ochrona przed odwrotną
polaryzacją → ograniczenie energii + właściwie dobrany TVS
→ VIN LM317T → VOUT 3,3 V → 3V3 S2 Mini + VCC CC1101
```

W większości aut gniazdo zapalniczki jest zasilane tylko po przekręceniu
stacyjki — **zweryfikuj to w swoim pojeździe**, bo część aut ma gniazdo
zasilane na stałe. Przy gnieździe „po stacyjce” emulator nie pobiera prądu,
gdy samochód jest wyłączony. Komputer samochodowy i tak zwykle nie odczytuje
wtedy stanu TPMS, więc brak transmisji parkingowej nie ma znaczenia.

Samo wyjście gniazda nadal ma przepięcia i zakłócenia instalacji pojazdu.
**Nie podłączaj go wprost do VIN LM317T jako gotowej instalacji samochodowej.**
LM317T znosi różnicę VIN−VOUT do 37–40 V, więc jest odporniejszy na przepięcia
niż LF33CV (16 V), ale ochrona nadal musi ograniczać napięcie z marginesem.
TVS o progu powyżej dopuszczalnego VIN nie stanowi ochrony LM317T.

Wymagania przed montażem (wariant B):

- bezpiecznik dobrany do przewodów i poboru, blisko źródła; nie zastępuje TVS;
- ochrona przed odwrotną polaryzacją i ujemnymi przepięciami;
- TVS i ograniczenie energii dobrane do rzeczywistego profilu przepięć, bez
  przewodzenia przy normalnym 14,4–14,8 V i bez przekraczania VIN LM317T;
- kondensatory zgodne z kartą LM317, krótkie pętle prądowe;
- pomiar szczytów prądu S2 Mini + CC1101 i sprawdzenie stabilności 3,3 V;
- obliczenia termiczne dla najwyższej temperatury oraz testy brownout/crank/load
  dump, EMC i ESD na docelowej płytce.

LM317T jest regulatorem liniowym: `P = (VIN − 3,3 V) × I`. Już przy 14,4 V
i 300 mA trzeba odprowadzić ok. **3,3 W**. Bez obliczenia i radiatora TO-220
może się przegrzewać; samo „do 1,5 A” z karty nie gwarantuje zasilania szczytów
ESP32-S2 w każdych warunkach. Zmierz prąd i temperaturę przy nadawaniu.

**Konsekwencja zasilania wyłącznie z gniazda zapalniczki:** po wyłączeniu
zapłonu cały emulator traci zasilanie. Nie wykonuje wtedy deep sleep ani
transmisji parkingowych, RTC nie odlicza interwału, a ustawienie
`interval parking` nie ma znaczenia. Wejście `PIN_ACC` nie jest potrzebne.
Po przekręceniu stacyjki urządzenie po prostu startuje i nadaje. Jeżeli
urządzenie ma nadawać również na postoju, potrzebne jest stale zasilane,
zabezpieczone wejście oraz osobny, zabezpieczony sygnał ACC na GPIO2. Wtedy
trzeba też zmierzyć pobór całego układu: dzielnik LM317T pobiera stale ok.
5 mA, do tego S2 Mini, CC1101 i dzielniki — może być istotny przy długim
postoju.

Parametry LM317T i wymagane kondensatory: karta ST
[LM317](https://www.st.com/resource/en/datasheet/lm317.pdf), schemat
typowej aplikacji i rysunek połączeń TO-220. Dostęp: 2026-10-28.

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
GPIO14 ma być aktywne stanem niskim i w spoczynku wysokim. Sam aktywator nie
wpisuje ID do emulatora.
