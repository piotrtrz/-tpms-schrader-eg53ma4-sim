/*
 * tpms_frame.cpp - generator ramek TPMS Schrader EG53MA4 (bez zaleznosci
 * od Arduino - kompilowalne takze natywnie w testach).
 *
 * Zastrzezenie: symulator generuje sztuczne dane cisnienia/temperatury
 * i nie zastepuje prawdziwych czujnikow TPMS.
 */
#include <string.h>
#include "config.h"
#include "tpms_frame.h"

static void put_bit(uint8_t *buf, unsigned *nbits, unsigned v)
{
    if (v) {
        buf[*nbits >> 3] |= (uint8_t)(0x80u >> (*nbits & 7u));
    }
    (*nbits)++;
}

/* G.E.T. Manchester: 0 -> "01", 1 -> "10" */
static void put_cell(uint8_t *buf, unsigned *nbits, unsigned bit)
{
    if (bit) {
        put_bit(buf, nbits, 1);
        put_bit(buf, nbits, 0);
    }
    else {
        put_bit(buf, nbits, 0);
        put_bit(buf, nbits, 1);
    }
}

uint8_t tpms_build_payload(uint8_t out[TPMS_PAYLOAD_LEN], uint32_t id,
                           uint16_t press_kpa10, int8_t temp_c,
                           uint32_t flags, bool low_batt)
{
    if (low_batt) {
        flags |= TPMS_FLAGS_LOWBAT_MASK;
    }

    /* cisnienie: 25 mbar/bit = 2.5 kPa/bit, zaokraglenie w gore do polowy bitu */
    int32_t press = ((int32_t)press_kpa10 + 12) / 25;
    if (press < 0)   press = 0;
    if (press > 255) press = 255;

    /* temperatura: protokol podaje stopnie Fahrenheita */
    int32_t num = 9 * (int32_t)temp_c + 160;
    int32_t tf  = (num >= 0) ? (num + 2) / 5 : -((-num + 2) / 5);
    if (tf < 0)   tf = 0;
    if (tf > 255) tf = 255;

    out[0] = (uint8_t)(flags >> 24);
    out[1] = (uint8_t)(flags >> 16);
    out[2] = (uint8_t)(flags >> 8);
    out[3] = (uint8_t)(flags);
    out[4] = (uint8_t)(id >> 16);
    out[5] = (uint8_t)(id >> 8);
    out[6] = (uint8_t)(id);
    out[7] = (uint8_t)press;
    out[8] = (uint8_t)tf;

    uint8_t crc = 0;
    for (unsigned i = 0; i < 9; i++) {
        crc = (uint8_t)(crc + out[i]);   // suma mod 256 (add_bytes w rtl_433)
    }
    out[9] = crc;
    return crc;
}

void tpms_build_frame(uint8_t out[TPMS_TX_BYTES], uint32_t id,
                      uint16_t press_kpa10, int8_t temp_c,
                      uint32_t flags, bool low_batt)
{
    uint8_t payload[TPMS_PAYLOAD_LEN];
    tpms_build_payload(payload, id, press_kpa10, temp_c, flags, low_batt);

    memset(out, 0, TPMS_TX_BYTES);
    unsigned nbits = 0;

    put_bit(out, &nbits, 1);                 // chip rozbiegowy (carrier on)

    /* 39 bitow sync - pierwszy (zerowy) bit dopisuje odbiornik (MC_ZEROBIT) */
    for (unsigned i = 0; i < TPMS_SYNC_BITS - 1; i++) {
        put_cell(out, &nbits, 0);
    }

    /* 80 bitow payloadu */
    for (unsigned b = 0; b < 8u * TPMS_PAYLOAD_LEN; b++) {
        unsigned bit = (payload[b >> 3] >> (7 - (b & 7u))) & 1u;
        put_cell(out, &nbits, bit);
    }

    /* padding do granicy bajtu - dla OOK sa to chipy "off" = cisza w eterze */
    while (nbits % 8u) {
        put_bit(out, &nbits, 0);
    }
}
