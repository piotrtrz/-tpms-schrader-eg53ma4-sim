/* EG53MA4 frame generator. Synthetic data for diagnostics only. */
#include <string.h>

#include "settings.h"
#include "tpms_frame.h"

static void put_bit(uint8_t *buf, unsigned *nbits, unsigned value)
{
    if (value != 0u) {
        buf[*nbits >> 3] |= (uint8_t)(0x80u >> (*nbits & 7u));
    }
    ++(*nbits);
}

/* Polarity confirmed by captures and rtl_433 OOK_MC_ZEROBIT: 0=01, 1=10. */
static void put_cell(uint8_t *buf, unsigned *nbits, unsigned bit)
{
    put_bit(buf, nbits, bit != 0u ? 1u : 0u);
    put_bit(buf, nbits, bit != 0u ? 0u : 1u);
}

bool tpms_build_payload(uint8_t out[TPMS_PAYLOAD_LEN], uint32_t id,
                        uint16_t press_kpa10, int8_t temp_c, uint32_t flags)
{
    if (out == NULL || !settings_valid_id(id)) {
        return false;
    }

    int32_t pressure = ((int32_t)press_kpa10 * 10 + TPMS_PRESSURE_KPA100_PER_RAW / 2)
            / TPMS_PRESSURE_KPA100_PER_RAW;
    if (pressure > 255) {
        pressure = 255;
    }

    int32_t numerator = 9 * (int32_t)temp_c + 160;
    int32_t temperature_f = numerator >= 0
            ? (numerator + 2) / 5 : -((-numerator + 2) / 5);
    if (temperature_f < 0) {
        temperature_f = 0;
    } else if (temperature_f > 255) {
        temperature_f = 255;
    }

    out[0] = (uint8_t)(flags >> 24);
    out[1] = (uint8_t)(flags >> 16);
    out[2] = (uint8_t)(flags >> 8);
    out[3] = (uint8_t)flags;
    out[4] = (uint8_t)(id >> 16);
    out[5] = (uint8_t)(id >> 8);
    out[6] = (uint8_t)id;
    out[7] = (uint8_t)pressure;
    out[8] = (uint8_t)temperature_f;
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 9u; ++i) {
        checksum = (uint8_t)(checksum + out[i]);
    }
    out[9] = checksum;
    return true;
}

bool tpms_build_frame(uint8_t out[TPMS_TX_BYTES], uint32_t id,
                      uint16_t press_kpa10, int8_t temp_c, uint32_t flags)
{
    uint8_t payload[TPMS_PAYLOAD_LEN];
    if (out == NULL || !tpms_build_payload(payload, id, press_kpa10, temp_c, flags)) {
        return false;
    }

    memset(out, 0, TPMS_TX_BYTES);
    unsigned nbits = 0;
    put_bit(out, &nbits, 1u); /* starting chip */
    for (unsigned i = 0; i < TPMS_SYNC_BITS - 1u; ++i) {
        put_cell(out, &nbits, 0u);
    }
    for (unsigned bit = 0; bit < 8u * TPMS_PAYLOAD_LEN; ++bit) {
        unsigned value = (payload[bit >> 3] >> (7u - (bit & 7u))) & 1u;
        put_cell(out, &nbits, value);
    }
    while ((nbits & 7u) != 0u) {
        put_bit(out, &nbits, 0u);
    }
    return nbits == TPMS_TX_CHIPS;
}
