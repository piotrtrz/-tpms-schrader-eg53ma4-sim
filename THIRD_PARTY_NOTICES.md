# Licencje i źródła

Projekt jest rozpowszechniany na GPL-2.0-or-later (`LICENSE`). Wybór jest
zgodny z wykorzystaniem w historii modelu algorytmu slicera z rtl_433. Nie
przypisujemy sobie praw do kodu autorów zewnętrznych.

- **rtl_433** — GPL-2.0-or-later; dekoder `schraeder.c`, Copyright Benjamin
  Larsson, Christian W. Zuckschwerdt i kolejni autorzy. Używany jako niezależny
  program do testów. Źródło i pełna historia: <https://github.com/merbanan/rtl_433>.
  Referencja testów: commit `406f9a33550a2b6b3d0d8a8a486417ce5b3f13ab`.
- **rtl_433_tests** — nagrania i metadane należą do ich autorów; nie są kopiowane
  do artefaktu firmware ani release. Skrypt pobiera przypięty commit
  `1aa8c29d6b9383739d1aeed495dbc5069b704392`.
- **SmartRC-CC1101-Driver-Lib** — zewnętrzna biblioteka pobierana przez PlatformIO;
  zachowane są jej oryginalne nagłówki i licencja. Użyty commit
  `de6757ef633b3e626dae5c7835cdb66ab53f9a1d`.
- **Arduino ESP32 / ESP-IDF / PlatformIO / Unity** — zależności build/test na ich
  własnych licencjach; nie są objęte deklaracją autorstwa tego projektu.
- Issue rtl_433 #2978 służył wyłącznie jako wzorzec integracji CC1101. Kod
  protokołu Renault nie jest kodem EG53MA4 i nie został użyty jako protokół.

Pełne teksty licencji zależności znajdują się w ich źródłach pobieranych podczas
buildu. Ten plik nie zawęża ich praw ani licencji.
