# Energia i zachowanie na postoju

## Status pomiarów

**Nie wykonano pomiarów prądu fizycznego prototypu.** Liczby poniżej są
szacunkami budżetu, nie wynikiem walidacji. SuperMini nie nadaje się do deklaracji
poboru µA bez pomiaru: wbudowany regulator, LED i obwody pomocnicze zwiększają
pobór. Sam LF33CV ma w stanie ON typowo ok. 0,5 mA prądu własnego (maks. 1 mA
przy braku obciążenia), więc zasilanie stałe wymaga osobnego bilansu energetycznego.

## Harmonogram firmware

- NORMAL po wybudzeniu RTC: inicjalizacja, burst czterech kół, wyjście z TX i deep
  sleep. Nie ma 10-sekundowego okna konsoli po zwykłym budziku.
- Po power-on, przycisku lub LF: okno konsoli 10 s; `stay N` wydłuża je.
- LEARN: TX jednego koła co 2 s, bez deep sleep, maks. 60 s na koło/15 min sesji.
- SINGLE: TX jednego koła co 2 s, bez deep sleep, maks. 5 min.
- CC1101 przechodzi w SPWD przed snem; CS jest wysoki, SCK/MOSI niskie, LED off.
- Każdy sen ma budzik RTC. `PARKING_SHUTDOWN` usunięto — urządzenie nie może
  pozostać uśpione do czasu wyłącznie ręcznego wybudzenia.
- ACC (opcjonalny sprzętowo) wybiera interwał ignition/parking. Nie jest potrzebne
  jako jedyne źródło wybudzenia, ponieważ RTC zawsze działa. Jeżeli cały układ
  zasilany jest tylko z ACC, po ACC OFF nie ma zasilania, RTC ani transmisji.

## Szacunek ładunku dla ustawień domyślnych

Jedna ramka to 240 × 120 µs = 28,8 ms na antenie. Dla 4 kół × 3 ramki:
345,6 ms samej transmisji plus 8 × 50 ms przerw wewnątrz burstów. Do tego dochodzi
boot, konfiguracja, SPI i logi. Do obliczeń energii trzeba zmierzyć cały cykl,
a nie tylko czas OOK.

Przykładowy model (do zastąpienia pomiarem): jeżeli aktywność ESP32 wyniesie 1,0 s
przy 80 mA co 30 s, sam ESP32 uśrednia ok. 2,7 mA na 3,3 V. CC1101, boot i prąd snu
dodają kolejne składniki. Przy interwale 120 s udział aktywności spada ok. czterokrotnie,
ale stały Iq zasilacza i dzielników pozostaje. Nie są to zmierzone 2,7 mA ani
obietnica poboru z akumulatora 12 V.

## Metoda pomiaru wymagana przed „validated”

1. **Cały SuperMini w sleep:** miernik µA szeregowo na wejściu, nie przez USB.
   Odczekać do końca TX i zanotować minimum/stan stabilny oraz model płytki.
2. **SuperMini + LF33CV:** powtórzyć pomiar na zabezpieczonym wejściu VIN i na
   szynie 3,3 V. Uwzględnić Iq regulatora, TVS, dzielniki i pull-upy.
3. **Cykl NORMAL:** bocznik + oscyloskop albo logger z odpowiednim pasmem.
   Zarejestrować boot, 12 ramek, przerwy, wejście w sen przez co najmniej 10 cykli.
4. **Średnia:** całkować ładunek co najmniej 15 min przy 30 s i 120 s. Podawać
   warunki: napięcie, temperatura, PA, wersja płytki i firmware.
5. **LEARN/SINGLE:** osobno zmierzyć prąd ciągłego czuwania.
6. **ACC/brownout/crank:** sprawdzić restart, powrót do nadawania i integralność
   NVS. Nie wykonywać na GPIO bez potwierdzonej ochrony wejścia.

## Wnioski dopuszczalne przed pomiarem

- Największy wpływ mają interwał, czas czuwania, PA i Iq całej płytki.
- Do wersji energooszczędnej trzeba zmierzyć cały SuperMini i LF33CV; przy
  zasilaniu wyłącznie z ACC układ nie pobiera prądu po wyłączeniu zapłonu, ale
  wtedy również nie nadaje w trybie parkingowym.
- Nie rekomendujemy 18650 w kabinie ani wieloletniej pracy bateryjnej z ESP32.
- Stały pobór emulatora trzeba uwzględnić przy długim postoju pojazdu; nie podajemy
  czasu „bezpiecznego dla akumulatora” bez pomiaru całego samochodu.
