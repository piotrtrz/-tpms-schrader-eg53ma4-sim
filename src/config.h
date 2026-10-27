/* Konfiguracja sprzetu. Dane czujnikow i RF sa w NVS, nie wymagaja rekompilacji. */
#ifndef TPMS_CONFIG_H
#define TPMS_CONFIG_H

#define FW_VERSION "2.0.0-rc.1"

/* ESP32 DevKit + CC1101 433 MHz (3,3 V). */
#define PIN_CC1101_SCK 18
#define PIN_CC1101_MISO 19
#define PIN_CC1101_MOSI 23
#define PIN_CC1101_CS 5
#define PIN_CC1101_GDO0 2

#define PIN_BUTTON 26
#define BTN_ACTIVE_LEVEL 0
#define PIN_LED 27

/* Opcje wylaczone do czasu zmontowania i przetestowania sprzetu. */
#ifndef PIN_LF_TRIGGER
#define PIN_LF_TRIGGER -1
#endif
#ifndef PIN_ACC
#define PIN_ACC -1
#endif
#ifndef PIN_VBAT_ADC
#define PIN_VBAT_ADC 34
#endif
#define VBAT_DIVIDER_R1 1000000.0f
#define VBAT_DIVIDER_R2 270000.0f
#define VBAT_CAL 1.000f
#define VBAT_SAMPLES 16
#define LOWBATT_THRESHOLD_MV 11500

/* Jedyny profil produkcyjny: OOK, Manchester 0->01, 1->10. */
#define TPMS_CHIP_US 120u

/* Harmonogram i moc poczatkowa: settings_defaults() w settings.cpp.
 * Sa to parametry eksperymentalne, a nie potwierdzony harmonogram OE. */
#define BOOT_STAY_AWAKE_MS 10000u
#define SERIAL_BAUD 115200

#define BTN_DEBOUNCE_MS 30u
#define BTN_LONG_MS 1500u
#define BTN_VERYLONG_MS 4000u

#endif
