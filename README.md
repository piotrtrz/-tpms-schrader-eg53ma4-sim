# Symulator TPMS Schrader EG53MA4 — ESP32 + CC1101

**Wersja 2.0.0-rc.1 — release candidate bez walidacji sprzętowej.**

Urządzenie warsztatowe nadaje cztery niezależne identyfikatory TPMS profilem
OOK/Manchester Schrader EG53MA4 na 433,92 MHz. Domyślnie każde koło raportuje
**260 kPa (2,6 bar; bajt protokołu `0x68`)**. Docelowy scenariusz to diagnostyka
Opla Astry J Sedan 1.4 LPG (2016, EU), ale **zgodność trzeba potwierdzić dla
konkretnego auta**: przez VIN/wykaz serwisowy, oznaczenia fabrycznych czujników,
odczyt diagnostyczny albo test odbioru. Numer OE 13506028, numery powiązane ani
ogłoszenie sprzedażowe same w sobie nie potwierdzają profilu radiowego.

> **Nie jest to homologowany czujnik ani zamiennik sprawnego TPMS. Nie używać
> w jeździe drogowej.** Sztuczne ciśnienie może ukryć przebitą lub niedopompowaną
> oponę i opóźnić ostrzeżenie. Przed i po teście sprawdź wszystkie opony
> niezależnym manometrem. Zobacz [SAFETY](docs/SAFETY.md).

## Co jest gotowe, a czego nie

- Gotowe: build ESP32, testy ramek/NVS/maszyny stanów, syntetyczny CU8 dekodowany
  przez niezależny `rtl_433`, trwała konfiguracja z konsoli, Clone IDs i LEARN.
- **Niewykonane:** pomiar rzeczywistego CC1101, RTL-SDR z anteną, test z samochodem,
  pomiar prądu i testy automotive. Dlatego wynik jest RC, nie „validated”.
- Jedyny profil podstawowy to **OOK**. Nie przełączaj na FSK „na próbę”.
- ESP32 nie generuje nośnej 433 MHz — wymagany jest osobny CC1101 433 MHz.

## Schemat ideowy

```text
 USB 5 V (TYLKO prototyp)       3,3 V                    OOK 433,92 MHz
        │                         │                             │
 [ESP32 DevKit] ── SPI ───────> [CC1101 433 MHz] ── antena ────┘
        │                         │
        ├── GPIO26: przycisk      └── wspólna GND
        └── GPIO27: LED + 330 Ω

 Konfiguracja: USB/UART 115200 → NVS → 4 × {ID, ciśnienie, temperatura}
```

Nie podłączaj 12 V do GPIO, 3V3 ani VIN DevKit. Połączenia i wariant automotive:
[HARDWARE](docs/HARDWARE.md).

## Quick Start

### 1. Złóż prototyp

Kup elementy z [BOM](docs/HARDWARE.md#bom-prototypu-laboratoryjnego). Podłącz
CC1101, przycisk i LED zgodnie z tabelą pinów. Zasil DevKit przez znany kabel USB
na biurku. Nie montuj jeszcze niczego w samochodzie.

### 2. Zainstaluj narzędzia i sprawdź projekt

Wymagane: Python 3, Git, CMake, kompilator C/C++, `pkg-config` i nagłówki libusb.
Najprościej użyć Dockera:

```bash
docker build -t tpms-rc .
docker run --rm -v "$PWD/artifacts:/work/artifacts" tpms-rc
```

Bez Dockera skrypt utworzy `.venv` z PlatformIO, pobierze przypięty `rtl_433`
i referencyjne nagrania:

```bash
bash tools/check_all
```

Ostatni komunikat musi brzmieć `PASS: wszystkie testy bez sprzetu...`.
To nie jest test fizycznego radia.

### 3. Skompiluj i wgraj

```bash
.venv/bin/pio run -e esp32dev
.venv/bin/pio run -e esp32dev -t upload
.venv/bin/pio device monitor --baud 115200
```

Jeżeli port nie jest wykrywany automatycznie, dodaj `--upload-port /dev/ttyUSB0`
(przy `run`) albo `--port /dev/ttyUSB0` (przy `device monitor`). W monitorze
PlatformIO wyjście to zwykle `Ctrl+]`.

Po uruchomieniu wpisz `stay 300` i Enter. Masz 5 minut na konfigurację; polecenie
`info` pokazuje aktywne wartości. Każda zmiana konfiguracji jest od razu zapisywana
w NVS i zachowuje się po restarcie — **nie edytuj ID w kodzie**.

### 4A. Preferowane: Clone IDs

Jeśli znasz oryginalne 24-bit ID (np. z wcześniejszego odczytu `rtl_433`), wpisz
je w kolejności FL, FR, RR, RL. Przykład — zastąp wartości własnymi:

```text
stay 300
id 1 2FC871
id 2 2EAAF9
id 3 3495AC
id 4 2FC780
pressure all 260
info
send
```

ID musi mieć sześć cyfr hex, być niezerowe, różne od `FFFFFF` i unikalne.
Urządzenie wróci do NORMAL i będzie nadawało znane ID. **Nauka może być
niepotrzebna, ale nie jest to gwarantowane.** Potwierdź odbiór diagnostycznie;
nie zaczynaj od jazdy.

### 4B. Alternatywa: New IDs + LEARN

1. Wpisz `ids new`, następnie `info` i zapisz wygenerowane ID.
2. Uruchom właściwą dla swojej wersji auta procedurę nauki TPMS zgodnie z jej
   instrukcją serwisową. Nie zgaduj sekwencji menu/pedałów.
3. Wpisz `mode learn`. Nadaje wyłącznie FL, co 2 s. LED wskaże 1 mrugnięcie.
4. `send` wymusza bieżące koło bez zmiany. Po potwierdzeniu kroku w aucie wpisz
   `next`: FR → RR → RL. Krótki przycisk robi to samo; po RL wraca do NORMAL.
5. Jeśli minie 60 s na kole, LEARN przerwie się do NORMAL. Cała sesja ma limit
   15 min. Po zakończeniu sprawdź ID i kody diagnostyczne.

EL-50448/OEC-T5 jest aktywatorem LF 125 kHz, **nie programatorem ESP32**. Ta
procedura nie wymaga go. Szczegóły: [SETUP](docs/SETUP.md).

## Sterowanie — jedna tabela

LED działa tylko podczas czuwania; w deep sleep jest zgaszona. Po zmianie koła
seria 1–4 mrugnięć wskazuje FL/FR/RR/RL.

| Tryb | Wzorzec LED | Krótki przycisk | Długi ≥1,5 s | Bardzo długi ≥4 s |
|---|---|---|---|---|
| NORMAL | 1 mrugnięcie / 2 s | burst wszystkich 4 | wejście LEARN | wejście SINGLE |
| LEARN | 3 mrugnięcia / 0,5 s; po zmianie numer koła | następne koło + TX; po RL koniec | wyjście NORMAL | wejście SINGLE |
| SINGLE | 2 mrugnięcia / 0,5 s; po zmianie numer koła | następne koło + TX | wejście LEARN | wyjście NORMAL |

W konsoli `send` w NORMAL nadaje wszystkie, a w LEARN/SINGLE tylko bieżące koło.
`send 1`…`send 4` wybiera koło jawnie; `send all` nadaje cztery. SINGLE wyłącza
się po 5 min.

## Najczęstsze problemy

- **`BLAD: brak CC1101`**: sprawdź 3,3 V, wspólną masę i sześć przewodów SPI;
  nie używaj 5 V na CC1101.
- **Konsola znika przed wpisaniem komend**: po restarcie od razu `stay 300`;
  zwykłe wybudzenie RTC celowo nie otwiera okna konsoli.
- **NVS zgłasza błąd**: urządzenie używa bezpiecznych domyślnych; wpisz `info`.
  `factory YES` kasuje własne ID i przywraca konfigurację fabryczną.
- **Auto nie odbiera**: najpierw zweryfikuj profil auta i odbiór RTL-SDR; nie
  zmieniaj losowo modulacji, flag ani interwałów. Zobacz
  [TROUBLESHOOTING](docs/TROUBLESHOOTING.md).

## Walidacja, zasilanie i release

- Protokół i niewiadome: [PROTOCOL](docs/PROTOCOL.md)
- Dokładne wyniki i checklista HIL: [VALIDATION](docs/VALIDATION.md)
- Pobór (na razie szacunki): [POWER](docs/POWER.md)
- Wymagania zasilania 12 V: [HARDWARE](docs/HARDWARE.md#wariant-automotive--wymagania-niezwalidowany-projekt)
- Zmiany: [CHANGELOG](CHANGELOG.md)

Gotowe obrazy tworzy `bash tools/package_release.sh`. Pliki i adresy opisano w
[VALIDATION](docs/VALIDATION.md#artefakty-i-wgrywanie). Release candidate nie
zawiera wyniku testu CC1101 ani samochodu.
