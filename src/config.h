/* Konfiguracja sprzetu. Dane czujnikow i RF sa w NVS, nie wymagaja rekompilacji. */
#ifndef TPMS_CONFIG_H
#define TPMS_CONFIG_H

#define FW_VERSION "2.0.0-rc.1"

/* ESP32-S2-MINI (ESP32-S2FN4R2) + CC1101 433 MHz (3,3 V).
 * GPIO19/20 to natywne USB, GPIO26-32 obsluguja flash/PSRAM w obudowie,
 * GPIO0/45/46 to piny strapping - nie uzywac ich do CC1101 ani przycisku. */
#define PIN_CC1101_SCK 4
#define PIN_CC1101_MISO 5
#define PIN_CC1101_MOSI 6
#define PIN_CC1101_CS 7
#define PIN_CC1101_GDO0 10

#define PIN_BUTTON 3
#define BTN_ACTIVE_LEVEL 0
/* Wbudowana niebieska LED S2 Mini na GPIO15 jest aktywna stanem wysokim. */
#define PIN_LED 15
#define LED_ACTIVE_LEVEL 1

/* Opcje wylaczone do czasu zmontowania i przetestowania sprzetu. */
#ifndef PIN_LF_TRIGGER
#define PIN_LF_TRIGGER -1
#endif
/* LF dzieli z przyciskiem GPIO wakeup LOW; na ESP32-S2 wakeup wymaga pinu
 * RTC (GPIO0-21). Proponowane GPIO14, bo GPIO1 zajmuje ADC zasilania. */
#define LF_ACTIVE_LEVEL 0
#ifndef PIN_ACC
#define PIN_ACC -1
#endif
#ifndef PIN_VBAT_ADC
#define PIN_VBAT_ADC 1
#endif
/* Dzielnik mierzy zasilanie 12 V (prog LOWBATT 11,5 V). R2 = 180 kOhm
 * daje pelny zakres ADC ok. 16,4 V przy pelnej skali 2,5 V (11 dB):
 * 14,8 V ladowania daje 2,25 V na pinie, wiec odczyt nie nasyci sie.
 * Wczesniejsze 270 kOhm nasycale ADC juz przy ok. 11,8 V. */
#define VBAT_DIVIDER_R1 1000000.0f
#define VBAT_DIVIDER_R2 180000.0f
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
