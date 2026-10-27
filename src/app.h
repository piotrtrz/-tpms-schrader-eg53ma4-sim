/*
 * app.h - API aplikacji (tryby pracy) uzywane przez konsole i maszyne stanow
 */
#ifndef TPMS_APP_H
#define TPMS_APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    APP_MODE_NORMAL = 0,   // rotacja wszystkich 4 kol + deep sleep
    APP_MODE_LEARN  = 1,   // nauka czujnikow w aucie, jedno kolo naraz
    APP_MODE_SINGLE = 2,   // wymuszone nadawanie wskazanego ID
};

uint8_t app_mode(void);
uint8_t app_wheel(void);            // aktywne kolo (w LEARN/SINGLE)

void app_set_mode(uint8_t mode, uint8_t wheel);
void app_next_wheel(void);
void app_send_burst_wheel(uint8_t wheel);
void app_send_burst_all(void);

void app_print_info(void);
void app_print_frame(uint8_t wheel);

uint32_t app_supply_mv(void);
void app_set_interval(uint32_t seconds);   // 0 = automatyczny (wg ACC)
void app_set_flags(uint32_t flags);
void app_request_sleep(void);              // zaspij teraz (tryb NORMAL)
void app_stay_awake(uint32_t ms);          // przedluz okno konsoli

#ifdef __cplusplus
}
#endif

#endif /* TPMS_APP_H */
