#ifndef TPMS_APP_H
#define TPMS_APP_H

#include <stdint.h>
#include "app_state.h"

extern AppState *g_app;

void app_print_info(void);
void app_print_frame(uint8_t wheel);
bool app_save_and_apply(void);
void app_request_sleep(void);
void app_stay_awake(uint32_t ms);

#endif
