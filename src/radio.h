#ifndef TPMS_RADIO_H
#define TPMS_RADIO_H

#include <stdbool.h>
#include <stdint.h>
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

bool radio_init(const settings_t *cfg);
bool radio_send_frame(const uint8_t *buf, uint8_t len);
bool radio_send_burst(const uint8_t *buf, uint8_t len, uint8_t frames,
                      uint16_t gap_ms);
void radio_sleep(void);

#ifdef __cplusplus
}
#endif

#endif
