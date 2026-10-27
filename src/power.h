/*
 * power.h - zasilanie, pomiar napiecia, deep sleep
 */
#ifndef TPMS_POWER_H
#define TPMS_POWER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Wywolac raz po starcie (zwalnia gpio_hold z poprzedniego snu). */
void power_init(void);

/* Pomiar napiecia zasilania [mV] (dzielnik ADC + kalibracja). */
uint32_t power_read_mv(void);

/* Decyzja o fladze "niska bateria" (prog LOWBATT_THRESHOLD_MV). */
bool power_low_battery(uint32_t mV);

/* Przygotuj i wejdz w deep sleep na `seconds` (0 = bez budzika, tylko
 * przycisk/LF). Funkcja nie wraca. */
void power_deep_sleep(uint32_t seconds);

/* Czas rzeczywisty [ms] od startu systemu - licznik RTC przetrwuje deep
 * sleep (ESP32 przywraca czas z RTC po wybudzeniu), wiec rosnie o rzeczywisty
 * czas spedzony w snie, nie o planowany interwal. Uzywane do logiki
 * "5 min po zaniku ACC". */
uint64_t power_elapsed_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_POWER_H */
