/* Konfiguracja sprzetu. Dane czujnikow i RF sa w NVS, nie wymagaja rekompilacji. */
#ifndef TPMS_CONFIG_H
#define TPMS_CONFIG_H

#define FW_VERSION "2.0.0-rc.1"

/* ESP32-C3 SuperMini + CC1101 433 MHz (3,3 V). */
#define PIN_CC1101_SCK 4
#define PIN_CC1101_MISO 5
#define PIN_CC1101_MOSI 6
#define PIN_CC1101_CS 7
#define PIN_CC1101_GDO0 10

#define PIN_BUTTON 3
#define BTN_ACTIVE_LEVEL 0
/* Wbudowana niebieska LED na GPIO8 jest aktywna stanem niskim. */
#define PIN_LED 8
#define LED_ACTIVE_LEVEL 0

/* Opcje wylaczone do czasu zmontowania i przetestowania sprzetu. */
#ifndef PIN_LF_TRIGGER
#define PIN_LF_TRIGGER -1
#endif
/* LF dzieli z przyciskiem GPIO wakeup LOW na ESP32-C3. */
#define LF_ACTIVE_LEVEL 0
#ifndef PIN_ACC
#define PIN_ACC -1
#endif
#ifndef PIN_VBAT_ADC
#define PIN_VBAT_ADC 0
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
