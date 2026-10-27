# Bezpieczeństwo i ograniczenia

## Sztuczne dane mogą ukryć usterkę

To urządzenie generuje dane, których nie mierzy w oponach. Domyślne 260 kPa jest
wartością testową, nie potwierdzeniem ciśnienia. Symulator może spowodować, że
kontrolka nie ostrzeże o realnym przebiciu, utracie ciśnienia lub uszkodzeniu opony.

- Projekt wyłącznie diagnostyczno-warsztatowy, na postoju i pod kontrolą osoby
  wykonującej test.
- Nie jest homologowanym czujnikiem ani zamiennikiem sprawnego TPMS.
- Nie używaj w jeździe drogowej i nie przedstawiaj jako bezpiecznego do tego celu.
- Przed i po teście sprawdź wszystkie opony niezależnym manometrem oraz ich stan.
- Po diagnozie przywróć sprawne, właściwie dobrane czujniki i wykonaj wymaganą
  procedurę pojazdu.

## Radio i zgodność pojazdu

Profil EG53MA4/OOK 433,92 MHz jest hipotezą roboczą dla wskazanego pojazdu do
potwierdzenia. OE 13506028, numer powiązany lub oferta zamiennika nie wystarcza.
Użyj VIN, oznaczeń fabrycznych czujników, diagnostyki albo kontrolowanego testu.
Nie nadawaj z maksymalną mocą „dla pewności”. Zacznij od −10 dBm, ogranicz czas
transmisji i przestrzegaj lokalnych przepisów dotyczących 433 MHz oraz ingerencji
w elektronikę pojazdu. Urządzenie może być odbierane przez inne odbiorniki w pobliżu.

## Montaż

- Tylko prototyp laboratoryjny jest opisany jako gotowy do złożenia. Wersja 12 V
  wymaga osobnego projektu i testów automotive.
- Nie podawaj 12 V na GPIO/ADC ani 5 V na CC1101.
- Nie umieszczaj urządzenia w strefie poduszek, pedałów, ruchomych elementów,
  wilgoci, wysokiej temperatury ani przy instalacji LPG.
- Zabezpiecz obudowę i przewody przed poluzowaniem. Unikaj ekranowania anteny.
- Nie używaj 18650 jako rekomendowanego zasilania w kabinie.

## Diagnostyka bez fałszywej pewności

Poprawne dekodowanie przez rtl_433 nie dowodzi odbioru przez samochód. Zielone CI
nie zastępuje pomiaru CC1101, testu HIL ani kontroli opon. Zgaśnięcie kontrolki
również nie dowodzi, że opony są sprawne. Zapisuj pomiary i nie zmieniaj wielu
parametrów naraz. Brak potwierdzonego bitu niskiej baterii czujnika; niskie
zasilanie emulatora nie jest kodowane jako usterka symulowanego czujnika.
