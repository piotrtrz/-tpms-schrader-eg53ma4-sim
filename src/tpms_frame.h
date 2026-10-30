/* Schrader EG53MA4: 40 sync bits + 80 data bits, OOK Manchester. */
#ifndef TPMS_FRAME_H
#define TPMS_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TPMS_ROW_BITS 120
#define TPMS_SYNC_BITS 40
#define TPMS_PAYLOAD_LEN 10
#define TPMS_TX_BYTES 30
#define TPMS_TX_CHIPS (TPMS_TX_BYTES * 8)

/* Calibration of the P field: how many hundredths of a kPa one bit of the
 * P byte represents. The rtl_433 scale (250 = 2.5 kPa/bit) is not confirmed
 * by the captures (all have P=0), while a car measurement showed
 * 260 kPa -> 284 kPa on the display at P=104, so a scaling correction of
 * 2.73 kPa/bit gives P=95 for 260 kPa. Confirmed with the car:
 * 260 kPa -> 260 kPa on the display (docs/AUDIT.md) -
 * also change PRESSURE_KPA100_PER_RAW in tools/tpms_frame.py. */
#define TPMS_PRESSURE_KPA100_PER_RAW 273

#ifdef __cplusplus
static_assert(TPMS_TX_CHIPS == 1 + 2 * (TPMS_ROW_BITS - 1) + 1,
              "Invalid Manchester buffer length");
#endif

/* Returns false for ID 0, 0xFFFFFF and values outside 24 bits. */
bool tpms_build_payload(uint8_t out[TPMS_PAYLOAD_LEN], uint32_t id,
                        uint16_t press_kpa10, int8_t temp_c, uint32_t flags);
bool tpms_build_frame(uint8_t out[TPMS_TX_BYTES], uint32_t id,
                      uint16_t press_kpa10, int8_t temp_c, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
