#ifndef TPMS_POWER_H
#define TPMS_POWER_H

#include <stdbool.h>
#include <stdint.h>

void power_init(void);
uint32_t power_read_mv(void);
bool power_low_battery(uint32_t mv); /* tylko ostrzezenie o zasilaniu emulatora */
void power_deep_sleep(uint32_t seconds); /* zawsze ma budzik RTC; nie wraca */

#endif
