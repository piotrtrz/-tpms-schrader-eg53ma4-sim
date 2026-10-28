/* Zasilanie i deep sleep. Stan zasilania NIE zmienia pol protokolu TPMS. */
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <soc/soc_caps.h>
#if SOC_BT_SUPPORTED
#include <esp32-hal-bt.h>
#endif

#include "config.h"
#include "power.h"
#include "radio.h"

void power_init(void)
{
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_MOSI);
#if SOC_BT_SUPPORTED
    /* ESP32-S2 nie ma radia Bluetooth; btStop istnieje tylko na chipach z BT. */
    btStop();
#endif
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

#if PIN_BUTTON >= 0 || PIN_LF_TRIGGER >= 0
    /* Budzenie z pinow RTC (na ESP32-S2: GPIO0-21). Oba wejscia sa aktywne
     * nisko, wiec moga wspoldzielic jeden tryb. ESP32-S2 nie obsluguje
     * GPIO wakeup w deep sleep - uzywa EXT1 ANY_LOW. */
    uint64_t wake_mask = 0;
#if PIN_BUTTON >= 0
    wake_mask |= 1ULL << PIN_BUTTON;
#endif
#if PIN_LF_TRIGGER >= 0
    wake_mask |= 1ULL << PIN_LF_TRIGGER;
#endif
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
    /* Cyfrowe pull-up/pull-down (pinMode) nie dzialaja w deep sleep, gdy
     * domena RTC_PERIPH jest wylaczona. Bez pociagniec RTC piny budzenia
     * plywaja i EXT1 moze budzic losowo; konfigurujemy je tu i utrzymujemy
     * mechanizmem HOLD (wg ESP-IDF sleep_modes dla EXT1). */
#if PIN_BUTTON >= 0
    rtc_gpio_init((gpio_num_t)PIN_BUTTON);
#if BTN_ACTIVE_LEVEL == 0
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_BUTTON);
    rtc_gpio_pullup_en((gpio_num_t)PIN_BUTTON);
#else
    rtc_gpio_pullup_dis((gpio_num_t)PIN_BUTTON);
    rtc_gpio_pulldown_en((gpio_num_t)PIN_BUTTON);
#endif
#endif
#if PIN_LF_TRIGGER >= 0
    rtc_gpio_init((gpio_num_t)PIN_LF_TRIGGER);
#if LF_ACTIVE_LEVEL == 0
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_LF_TRIGGER);
    rtc_gpio_pullup_en((gpio_num_t)PIN_LF_TRIGGER);
#else
    rtc_gpio_pullup_dis((gpio_num_t)PIN_LF_TRIGGER);
    rtc_gpio_pulldown_en((gpio_num_t)PIN_LF_TRIGGER);
#endif
#endif
#endif /* SOC_RTCIO_INPUT_OUTPUT_SUPPORTED */
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
    esp_deep_sleep_enable_gpio_wakeup(wake_mask, ESP_GPIO_WAKEUP_GPIO_LOW);
#else
    esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
#endif
    esp_deep_sleep_start();
}
