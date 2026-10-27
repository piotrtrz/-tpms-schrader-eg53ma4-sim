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

/* Suma czasu pracy [ms] od pierwszego uruchomienia - licznik w RTC
 * przetrwuje deep sleep; uzywane do logiki "5 min po zaniku ACC". */
uint64_t power_elapsed_ms(void);
void power_elapsed_add_ms(uint64_t delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_POWER_H */
