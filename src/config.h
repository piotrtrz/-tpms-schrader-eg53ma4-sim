/* Hardware configuration. Sensor and RF data live in NVS, no recompile needed. */
#ifndef TPMS_CONFIG_H
#define TPMS_CONFIG_H

#define FW_VERSION "2.0.0-rc.1"

/* ESP32-S2-MINI (ESP32-S2FN4R2) + CC1101 433 MHz (3.3 V).
 * GPIO19/20 are native USB, GPIO26-32 serve in-package flash/PSRAM,
 * GPIO45/46 are strapping pins - do not use them for the CC1101. */
#define PIN_CC1101_SCK 4
#define PIN_CC1101_MISO 5
#define PIN_CC1101_MOSI 6
#define PIN_CC1101_CS 7
#define PIN_CC1101_GDO0 10

/* Control button and wake from deep sleep: the board's BOOT button
 * (GPIO0 to GND), confirmed by a bench test. GPIO0 is a strapping pin:
 * held during power-up/reset it starts the bootloader instead of the firmware.
 * An external momentary button can be wired to GPIO3 instead, selecting
 * -DPIN_BUTTON=3 in build_flags. */
#ifndef PIN_BUTTON
#define PIN_BUTTON 0
#endif
#define BTN_ACTIVE_LEVEL 0
/* The S2 Mini's built-in blue LED on GPIO15 is active-high. */
#define PIN_LED 15
#define LED_ACTIVE_LEVEL 1

/* Options disabled until the hardware is assembled and tested. */
#ifndef PIN_LF_TRIGGER
#define PIN_LF_TRIGGER -1
#endif
/* LF shares the button's GPIO wakeup LOW; on ESP32-S2 wakeup requires an
 * RTC pin (GPIO0-21). GPIO14 is suggested because GPIO1 holds the supply ADC. */
#define LF_ACTIVE_LEVEL 0
#ifndef PIN_ACC
#define PIN_ACC -1
#endif
#ifndef PIN_VBAT_ADC
#define PIN_VBAT_ADC 1
#endif
/* Divider measures the 12 V supply (LOWBATT threshold 11.5 V). R2 = 180 kOhm
 * gives the ADC a full-scale range of about 16.4 V at 2.5 V full scale (11 dB):
 * a 14.8 V charge voltage yields 2.25 V at the pin, so the reading saturates
 * no earlier. The previous 270 kOhm divider saturated the ADC at ~11.8 V. */
#define VBAT_DIVIDER_R1 1000000.0f
#define VBAT_DIVIDER_R2 180000.0f
#define VBAT_CAL 1.000f
#define VBAT_SAMPLES 16
#define LOWBATT_THRESHOLD_MV 11500

/* The only production profile: OOK, Manchester 0->01, 1->10. */
#define TPMS_CHIP_US 120u

/* Schedule and initial power: settings_defaults() in settings.cpp.
 * These are experimental parameters, not a confirmed OE schedule. */
#define BOOT_STAY_AWAKE_MS 10000u

/* Micro-naps: instead of one long sleep the LED blinks every NAP_PERIOD_MS,
 * also between transmissions. Sleep is the remainder of the period after
 * boot time + window, and TPMS frame TX remains on the interval (counter
 * in RTC memory). */
#define NAP_PERIOD_MS 2000u
#define NAP_AWAKE_MS 400u     /* visible LED blink after the timer wake */
#define NAP_MIN_SLEEP_MS 200u
#define SERIAL_BAUD 115200

#define BTN_DEBOUNCE_MS 30u
#define BTN_LONG_MS 1500u
#define BTN_VERYLONG_MS 4000u

#endif
