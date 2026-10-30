/* Power and deep sleep. Supply state does NOT change TPMS protocol fields. */
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
    /* ESP32-S2 has no Bluetooth radio; btStop only exists on chips with BT. */
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

void power_deep_sleep(uint32_t ms)
{
    radio_sleep();
    gpio_hold_en((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_en((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_en((gpio_num_t)PIN_CC1101_MOSI);
    gpio_deep_sleep_hold_en();

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    /* PARKING_SHUTDOWN does not exist: every sleep has an RTC alarm and the
     * device cannot be stopped until a manual wake. The lower bound also
     * guarantees an alarm for micro-naps. */
    if (ms < NAP_MIN_SLEEP_MS) {
        ms = NAP_MIN_SLEEP_MS;
    }
    esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000ULL);

#if PIN_BUTTON >= 0 || PIN_LF_TRIGGER >= 0
    /* Waking from RTC pins (on ESP32-S2: GPIO0-21). Both inputs are active
     * low, so they can share one mode. ESP32-S2 does not support GPIO wakeup
     * in deep sleep - it uses EXT1 ANY_LOW. */
    uint64_t wake_mask = 0;
#if PIN_BUTTON >= 0
    wake_mask |= 1ULL << PIN_BUTTON;
#endif
#if PIN_LF_TRIGGER >= 0
    wake_mask |= 1ULL << PIN_LF_TRIGGER;
#endif
#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
    /* Digital pull-up/pull-down (pinMode) does not work in deep sleep while
     * the RTC_PERIPH domain is off. Without RTC pulls the wake pins float and
     * EXT1 can wake randomly; we configure them here and keep them with the
     * HOLD mechanism (per ESP-IDF sleep_modes for EXT1). */
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
