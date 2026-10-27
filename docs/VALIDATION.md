# Walidacja

## Jedno polecenie bez sprzętu

```bash
bash tools/check_all
```

Skrypt wykonuje self-test Pythona, testy native, pełny build ESP32, cppcheck,
pobranie przypiętych źródeł (przy pierwszym uruchomieniu) oraz dekodowanie CU8
przez prawdziwy `rtl_433`. Logi: `artifacts/validation/`. Każdy etap ma `pipefail`,
a błąd kończy skrypt kodem niezerowym.

Alternatywa reprodukowalna:

```bash
docker build -t tpms-rc .
docker run --rm -v "$PWD/artifacts:/work/artifacts" tpms-rc
```

## Wyniki z 2026-10-27

Środowisko: Linux, Python 3.14.4 przy teście hosta; PlatformIO 6.1.18;
espressif32 8.0.0; Arduino ESP32 3.3.12; SmartRC CC1101 commit
`de6757ef633b3e626dae5c7835cdb66ab53f9a1d`; rtl_433 commit `406f9a3…`;
rtl_433_tests commit `1aa8c29…`. Pełne SHA znajdują się w `PROTOCOL.md`.

| Komenda | Wynik |
|---|---|
| `pio test -e native` | **20/20 PASS** — ramka, cztery niezależne wektory, negatywne, NVS, fake clock/radio, Clone IDs, LEARN, SINGLE, ACC, restart |
| `pio run -e esp32c3supermini` | **PASS**, własne ostrzeżenia traktowane jako błędy (`-Werror`) |
| `pio check -e native ... --fail-on-defect=low` | **PASS**, 0 defektów po poprawce inicjalizacji struktur |
| `python3 tools/tpms_frame.py --self-test` | **PASS**, nagranie + 500 ramek |
| `tools/verify_rtl433.sh` | **PASS**, 64/64 rekordy referencyjne; 3/3 syntetyczne; 3/3 negatywne odrzucone |
| `git diff --check` | **PASS** |
| `bash tools/package_release.sh` | **PASS**, obraz ESP32-C3 z bootloaderem od `0x0` i manifestem SHA-256 |

Dane referencyjne: każdy z `gfile001`…`gfile004` dał dokładnie 16 zgodnych
rekordów, z oczekiwanym ID/flags/ciśnieniem/temperaturą. Syntetyczne ramki
2FC871/260 kPa/73 F, 3495AC/220 kPa/23 F i ABCDEE/262,5 kPa/75 F zostały
zdekodowane przez niezależny `rtl_433`. Uszkodzony Manchester, zła suma i ucięcie
nie dały rekordu EG53MA4.

## Macierz pokrycia

| Warstwa | Automatyczne | RTL-SDR fizyczny | CC1101 fizyczny | Samochód |
|---|---|---|---|---|
| pola, checksum, kwantyzacja | wykonane | niepotrzebne do pola | niepotrzebne | potwierdzenie interpretacji ECU |
| Manchester/preambuła/długość | wykonane | **niewykonane** | **niewykonane** | niewykonane |
| prawdziwe nagrania → rtl_433 | wykonane, 64/64 | nie dotyczy | nie dotyczy | nie dotyczy |
| syntetyczne CU8 → rtl_433 | wykonane | nie dotyczy | omija hardware | nie dotyczy |
| rejestry CC1101/ogon/moc/częstotliwość | audyt kodu | wymagane | **wymagane** | wymagane |
| NVS, ID, interwały, tryby | wykonane; NVS emulowane blobem | wymagany test restartu | wymagany | wymagany |
| LEARN FL→FR→RR→RL | state-machine PASS | pomocnicze | wymagane | **niewykonane** |
| pobór/brownout/ACC | state-machine ACC; brak pomiarów | nie dotyczy | wymagane | wymagane |

**Brak podstaw do oznaczenia „validated”.** Gitea Actions ma workflow, ale stan
runnera i wynik zdalnego CI należy sprawdzić; lokalny PASS nie jest udawanym CI.

## Checklista HIL — wymaga użytkownika i sprzętu

### CC1101 + analizator/RTL-SDR

- [ ] Odczytać rejestry po inicjalizacji: OOK, 433,92 MHz, fixed 30 B,
  `SYNC_MODE=0`, brak CRC/Manchester/whitening/FEC/append.
- [ ] Na GDO0/ANT potwierdzić dokładnie 30 B, chip ~120 µs, pełny ostatni chip
  i brak sprzętowej preambuły/sync/CRC/dodatkowych bajtów.
- [ ] Zanotować TX-ERR/underflow; przy prawidłowym torze ma ich nie być.
- [ ] Zmierzyć częstotliwość i względną moc dla −10 dBm; nie zakładać kalibracji
  taniego modułu. Nie zwiększać PA bez potrzeby.
- [ ] `send 1`…`send 4`: RTL-SDR dekoduje właściwe ID, 260 kPa i flags.
- [ ] Powtórzyć po resecie i 10 cyklach deep sleep.

### NVS i wejścia

- [ ] Ustawić własne ID/ciśnienia/RF, odłączyć zasilanie na >1 min, ponownie
  sprawdzić `info` i transmisję.
- [ ] Sprawdzić odrzucenie duplikatu ID, złej mocy i złego interwału.
- [ ] `factory YES` przywraca 260 kPa i domyślne RF.
- [ ] Brownout/crank na zabezpieczonym zasilaczu: brak zawieszenia i uszkodzenia
  konfiguracji; zapis wykonywać tylko po stabilizacji zasilania.
- [ ] Jeśli ACC jest montowane: zmierzyć napięcie na GPIO dla OFF/ON/przepięcia,
  sprawdzić interwały i samoczynne wznowienie po ACC ON. Bez ACC pominąć.

### Konkretny Opel Astra J

- [ ] Potwierdzić profil przez VIN/oznaczenia/diagnostykę przed nadawaniem.
- [ ] Clone IDs: odbiór czterech znanych ID bez zakładania, że nauka zniknie.
- [ ] New IDs: pełna nauka FL→FR→RR→RL z jednym aktywnym kołem.
- [ ] Odczyt DTC i danych TPMS przed/po; zapisać wersję ECU i procedurę.
- [ ] Test położenia: konsola środkowa/za schowkiem, ewentualnie bagażnik;
  na postoju, bez zgadywania lokalizacji odbiornika.
- [ ] Kontrola manometrem wszystkich opon przed i po. Nie wykonywać testu jako
  jazdy drogowej z symulatorem zamiast sprawnego TPMS.

## Artefakty i wgrywanie

```bash
bash tools/package_release.sh
```

Katalog `artifacts/release/`:

| Plik | Adres |
|---|---:|
| `bootloader.bin` | `0x0` — adres dla ESP32-C3, nie `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |
| `firmware-full.bin` | `0x0` — pełny obraz 4 MB |
| `firmware.elf` | debug/symbole, nie wgrywać jako flash |
| `SHA256SUMS`, `BUILD-INFO.txt` | kontrola integralności i wersji |

Pełny obraz kasuje poprzedni flash przy zapisie całego obszaru; NVS w partycji
może zostać utracone — zapisz ID przed aktualizacją pełnym obrazem. Aktualizacja
samej aplikacji `firmware.bin` pod `0x10000` zwykle zachowuje NVS, ale przed
release użytkownik powinien wykonać test na własnym module.

```bash
PORT=/dev/ttyACM0 bash tools/flash_release.sh
(cd artifacts/release && sha256sum -c SHA256SUMS)
```

Nie wgrywaj obrazu z innego toolchaina na ślepo. RC nie zawiera fizycznego testu
update ani samochodu.
