/*
 * tpms_frame.h - generator ramek TPMS Schrader EG53MA4
 *
 * Zastrzezenie: dane sa sztuczne (diagnostyka/testy), nie zastepuja
 * prawdziwych czujnikow TPMS.
 *
 * Ramka na zrzuconej warstwie bitowej (120 bitow, po dekodowaniu
 * Manchester przez rtl_433 OOK_MC_ZEROBIT):
 *
 *   40 bit sync (zera) | flags(4B) | id(3B) | pressure(1B) | temp(1B) | crc(1B)
 *
 * - flags: 32 bity, wg dekodera "moze zawierac preamble/status/battery"
 * - pressure: 25 mbar/bit  (cisnienie w kPa = bajt * 2.5)
 * - temperature: stopnie Fahrenheita
 * - crc: suma 9 poprzednich bajtow mod 256 (add_bytes w rtl_433)
 *
 * Kodowanie na zrzuconej warstwie RF (G.E.T. Manchester):
 *   bit 0 -> chipy "01", bit 1 -> chipy "10", chip = 120 us
 * Odbiornik rtl_433 (MC_ZEROBIT) dopisuje pierwszy bit ramki samodzielnie,
 * wiec nadajnik wysyla ramke BEZ pierwszego bitu, poprzedzona jednym
 * chipem rozbiegowym '1'. Bufor wyjsciowy jest dopełniany zerami do
 * granicy bajtu (dla OOK sa to chipy "wylaczone" = cisza).
 */
#ifndef TPMS_FRAME_H
#define TPMS_FRAME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TPMS_ROW_BITS     120   // dlugosc dekodowanej ramki (wymagana przez rtl_433)
#define TPMS_SYNC_BITS    40
#define TPMS_PAYLOAD_LEN  10    // flags(4)+id(3)+press+temp+crc
#define TPMS_TX_BYTES     30    // 240 chipow = 1 lead + 238 danych + 1 padding
#define TPMS_TX_CHIPS     (TPMS_TX_BYTES * 8)

#ifdef __cplusplus
/* Zaleznosc bufora nadawczego od dlugosci ramki: 1 chip rozbiegowy +
 * 2 chipy na kazdy z (TPMS_ROW_BITS-1) bitow (pierwszy bit dopisuje
 * odbiornik) + padding do granicy bajtu (< 8 chipow). */
static_assert(TPMS_TX_CHIPS >= 1 + 2 * (TPMS_ROW_BITS - 1)
              && TPMS_TX_CHIPS < 1 + 2 * (TPMS_ROW_BITS - 1) + 8,
              "TPMS_TX_BYTES musi miescic 1 + 2*(TPMS_ROW_BITS-1) chipow");
#endif

typedef struct {
    uint32_t id;             // 24-bit identyfikator czujnika
    uint16_t press_kpa10;    // cisnienie w kPa * 10 (np. 2600 = 260.0 kPa)
    int8_t   temp_c;         // temperatura w st. C
    const char *name;        // "FL", "FR", "RR", "RL"
} tpms_sensor_t;

/* Zbuduj 10-bajtowy payload (bez sync). Zwraca crc (tez wpisane do out[9]). */
uint8_t tpms_build_payload(uint8_t out[TPMS_PAYLOAD_LEN], uint32_t id,
                           uint16_t press_kpa10, int8_t temp_c,
                           uint32_t flags, bool low_batt);

/* Zbuduj kompletny bufor nadawczy (TPMS_TX_BYTES) gotowy do CC1101 FIFO. */
void tpms_build_frame(uint8_t out[TPMS_TX_BYTES], uint32_t id,
                      uint16_t press_kpa10, int8_t temp_c,
                      uint32_t flags, bool low_batt);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_FRAME_H */
