#!/usr/bin/env bash
# verify_rtl433.sh - automatyczna weryfikacja ramek symulatora TPMS
# Schrader EG53MA4 za pomoca prawdziwego dekodera rtl_433 (bez sprzetu RF).
#
# Wymaga: python3, rtl_433 (binarka w PATH albo w zmiennej RTL433)
#
# Uzycie:
#   RTL433=/sciezka/do/rtl_433 ./tools/verify_rtl433.sh
#
# Skrypt:
#   1) generuje syntezyczne nagrania IQ (.cu8) takie, jakie odbierze RTL-SDR
#      podczas nadawania symulatora,
#   2) dekoduje je rtl_433 i porownuje wynik z zadanymi wartosciami:
#      - OOK: wbudowany dekoder Schrader-EG53MA4 (model/id/pressure/temp)
#      - FSK: dekoder flex, porownanie calego bitstreamu z ramka wzorcowa
#
# Uwaga: nazwy plikow nie moga zawierac tokenow "ook"/"fsk"/"am"/"fm" -
# parser formatow rtl_433 traktuje je jako opis formatu pliku.
set -u

RTL433="${RTL433:-rtl_433}"
DIR="$(cd "$(dirname "$0")/.." && pwd)"
TMP="${TMPDIR:-/tmp}/tpms_verify_$$"
mkdir -p "$TMP"

if ! command -v "$RTL433" >/dev/null 2>&1; then
    echo "BLAD: nie znaleziono rtl_433 (ustaw zmienna RTL433=...)"; exit 1
fi

fail=0

check_ook() {  # $1=plik $2=id $3=kPa $4=temp_F
    local file="$1" id="$2" kpa="$3" tf="$4"
    local out
    out="$("$RTL433" -r "$file" 2>/dev/null)"
    if echo "$out" | grep -qi "Schrader-EG53MA4" \
       && echo "$out" | grep -q "ID        : $id" \
       && echo "$out" | grep -q "Pressure  : $kpa kPa" \
       && echo "$out" | grep -q "Temperature: $tf"; then
        echo "OK   $(basename "$file")  id=$id $kpa kPa $tf F"
    else
        echo "FAIL $(basename "$file")  (oczekiwano id=$id $kpa kPa $tf F)"
        echo "$out" | sed 's/^/     /'
        fail=1
    fi
}

check_fsk() {  # $1=plik $2=id $3=kPa $4=temp_C
    local file="$1" id="$2" kpa="$3" tc="$4"
    local out codes want
    out="$("$RTL433" -r "$file" \
           -X 'n=schrader,m=FSK_MC_ZEROBIT,s=120,l=120,r=300' 2>/dev/null)"
    codes="$(echo "$out" | grep -o 'codes     : {120}[0-9a-f]*' | head -1 | sed 's/.*{120}//')"
    want="0000000000$(python3 "$DIR/tools/tpms_frame.py" --id "$id" \
          --pressure "$kpa" --temp "$tc" | grep payload | sed 's/.*payload : //' \
          | tr 'A-Z' 'a-z' | tr -d ' ')"
    if [ -n "$codes" ] && [ "$codes" = "$want" ]; then
        echo "OK   $(basename "$file")  (FSK) bitstream zgodny: $codes"
    else
        echo "FAIL $(basename "$file")  (FSK)"
        echo "     otrzymano: ${codes:-<brak>}"
        echo "     oczekiwano: $want"
        fail=1
    fi
}

echo "== OOK (domyslna modulacja, wbudowany dekoder Schrader-EG53MA4) =="
python3 "$DIR/tools/gen_cu8.py" --id 2FC871 --pressure 260 --temp 23 \
        -o "$TMP/c1.cu8" >/dev/null
check_ook "$TMP/c1.cu8" 2FC871 260.0 73.0

python3 "$DIR/tools/gen_cu8.py" --id 3495AC --pressure 220 --temp -5 \
        -o "$TMP/c2.cu8" >/dev/null
check_ook "$TMP/c2.cu8" 3495AC 220.0 23.0

python3 "$DIR/tools/gen_cu8.py" --id ABCDEE --pressure 262.5 --temp 24 \
        -o "$TMP/c3.cu8" >/dev/null
check_ook "$TMP/c3.cu8" ABCDEE 262.5 75.0

echo "== FSK (wariant konfigurowalny, dekoder flex) =="
python3 "$DIR/tools/gen_cu8.py" --id 2FC871 --pressure 260 --temp 23 \
        --mode fsk -o "$TMP/c4.cu8" >/dev/null
check_fsk "$TMP/c4.cu8" 2FC871 260 23

rm -rf "$TMP"
if [ "$fail" -eq 0 ]; then
    echo "WSZYSTKO OK - ramki dekoduja sie poprawnie przez rtl_433"
else
    echo "WERYFIKACJA NIEUDANA"
fi
exit "$fail"
