/* Schrader EG53MA4: 40 bitow sync + 80 bitow danych, OOK Manchester. */
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

#ifdef __cplusplus
static_assert(TPMS_TX_CHIPS == 1 + 2 * (TPMS_ROW_BITS - 1) + 1,
              "Nieprawidlowa dlugosc bufora Manchester");
#endif

/* Zwraca false dla ID 0, 0xFFFFFF i wartosci spoza 24 bitow. */
bool tpms_build_payload(uint8_t out[TPMS_PAYLOAD_LEN], uint32_t id,
                        uint16_t press_kpa10, int8_t temp_c, uint32_t flags);
bool tpms_build_frame(uint8_t out[TPMS_TX_BYTES], uint32_t id,
                      uint16_t press_kpa10, int8_t temp_c, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
