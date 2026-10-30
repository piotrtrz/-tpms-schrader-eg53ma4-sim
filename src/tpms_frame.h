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

/* Kalibracja pola P: tyle setnych kPa przypada na jeden bit bajtu P.
 * Skala rtl_433 (250 = 2,5 kPa/bit) nie jest potwierdzona nagraniami
 * (wszystkie maja P=0), a pomiar z auta: 260 kPa -> 284 kPa na wyswietlaczu
 * przy P=104, wiec korekta skalujaca 2,73 kPa/bit daje P=95 dla 260 kPa.
 * Potwierdzone z autem: 260 kPa -> 260 kPa na wyswietlaczu (docs/AUDIT.md) -
 * zmienic tez PRESSURE_KPA100_PER_RAW w tools/tpms_frame.py. */
#define TPMS_PRESSURE_KPA100_PER_RAW 273

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
