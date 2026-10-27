/*
 * power.cpp - zasilanie, pomiar napiecia, deep sleep
 *
 * Zastrzezenie: urzadzenie diagnostyczne/testowe - symulator TPMS
 * dostarcza sztuczne dane cisnienia i nie zastepuje prawdziwych czujnikow.
 *
 * Budzet pradu (patrz README):
 *  - ESP32 deep sleep: ~10 uA (WiFi/BT nie sa wcale inicjalizowane)
 *  - CC1101 w SLEEP (SPWD): < 1 uA
 *  - gpio_hold trzyma linie CC1101 w stanach niskich/wysokiej CS
 */
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <esp32-hal-bt.h>

#include "config.h"
#include "power.h"
#include "radio.h"

RTC_DATA_ATTR static uint64_t rtc_elapsed_ms = 0;

void power_init(void)
{
    /* Zwolnij hold z poprzedniego snu - po deep sleep piny sa zatrzymane. */
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_MOSI);

    /* WiFi nie jest wcale uruchamiany (brak init), BT wylaczamy defensywnie. */
    btStop();

    analogSetPinAttenuation(PIN_VBAT_ADC, ADC_11db);
}

uint32_t power_read_mv(void)
{
    uint64_t sum = 0;
    for (int i = 0; i < VBAT_SAMPLES; i++) {
        sum += analogReadMilliVolts(PIN_VBAT_ADC);
    }
    uint32_t pin_mv = (uint32_t)(sum / VBAT_SAMPLES);
    float v = (float)pin_mv * ((VBAT_DIVIDER_R1 + VBAT_DIVIDER_R2) / VBAT_DIVIDER_R2)
              * VBAT_CAL;
    return (uint32_t)(v + 0.5f);
}

bool power_low_battery(uint32_t mv)
{
    return mv > 500u && mv < (uint32_t)LOWBATT_THRESHOLD_MV;
}

void power_deep_sleep(uint32_t seconds)
{
    /* CC1101 w sleep + bezpieczne stany linii (CS=1 utrzymuje SPWD). */
    radio_sleep();

    /* Zatrzymaj piny w tych stanach na czas snu. */
    gpio_hold_en((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_en((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_en((gpio_num_t)PIN_CC1101_MOSI);
    gpio_deep_sleep_hold_en();

    /* Budzik RTC. */
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (seconds > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    }

    /* Przycisk jako zrodlo EXT0 (poziom konfigurowalny). */
#if PIN_BUTTON >= 0
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, BTN_ACTIVE_LEVEL);
#endif

    /* Opcjonalne wejscie LF 125 kHz (impuls wysoki) jako EXT1.
     * Uwaga: poziom musi byc niski w spoczynku, inaczej ESP32 obudzi sie
     * natychmiast po wejsciu w sen. */
    uint64_t ext1_mask = 0;
#if PIN_LF_TRIGGER >= 0
    ext1_mask |= (1ULL << PIN_LF_TRIGGER);
#endif
    if (ext1_mask != 0) {
        esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
    }

    esp_deep_sleep_start();
}

uint64_t power_elapsed_ms(void)
{
    return rtc_elapsed_ms;
}

void power_elapsed_add_ms(uint64_t delta_ms)
{
    rtc_elapsed_ms += delta_ms;
}
