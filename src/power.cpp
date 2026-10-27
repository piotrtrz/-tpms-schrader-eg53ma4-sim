/* Zasilanie i deep sleep. Stan zasilania NIE zmienia pol protokolu TPMS. */
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <esp32-hal-bt.h>

#include "config.h"
#include "power.h"
#include "radio.h"

void power_init(void)
{
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_MOSI);
    btStop();
#if PIN_VBAT_ADC >= 0
    analogSetPinAttenuation(PIN_VBAT_ADC, ADC_11db);
#endif
}

uint32_t power_read_mv(void)
{
#if PIN_VBAT_ADC < 0
    return 0u;
#else
    uint64_t sum = 0;
    for (uint8_t i = 0; i < VBAT_SAMPLES; ++i) {
        sum += analogReadMilliVolts(PIN_VBAT_ADC);
    }
    uint32_t pin_mv = (uint32_t)(sum / VBAT_SAMPLES);
    float value = (float)pin_mv * ((VBAT_DIVIDER_R1 + VBAT_DIVIDER_R2) / VBAT_DIVIDER_R2)
            * VBAT_CAL;
    return (uint32_t)(value + 0.5f);
#endif
}

bool power_low_battery(uint32_t mv)
{
    return mv > 500u && mv < (uint32_t)LOWBATT_THRESHOLD_MV;
}

void power_deep_sleep(uint32_t seconds)
{
    radio_sleep();
    gpio_hold_en((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_en((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_en((gpio_num_t)PIN_CC1101_MOSI);
    gpio_deep_sleep_hold_en();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    /* PARKING_SHUTDOWN nie istnieje: kazdy sen ma budzik i nie mozna
     * zatrzymac urzadzenia do czasu recznego wybudzenia. */
    if (seconds < SETTINGS_INTERVAL_MIN_S) {
        seconds = SETTINGS_INTERVAL_MIN_S;
    }
    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);

#if PIN_BUTTON >= 0
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, BTN_ACTIVE_LEVEL);
#endif

#if PIN_LF_TRIGGER >= 0
    uint64_t ext1_mask = 1ULL << PIN_LF_TRIGGER;
    esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
#endif
    esp_deep_sleep_start();
}
