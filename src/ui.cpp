/*
 * ui.cpp - button (gestures) and status LED (non-blocking, no delay())
 */
#include <Arduino.h>
#include "config.h"
#include "ui.h"

/* ---------------------------- button ---------------------------- */
/* Debounce: the new raw state must persist for BTN_DEBOUNCE_MS before it is
 * accepted as stable (candidate + the time it changed). */

static bool     btn_raw_last = false;    // last raw reading
static uint32_t btn_raw_change = 0;      // time of the last raw change
static bool     btn_stable = false;      // debounced state
static uint32_t btn_press_start = 0;
static bool     btn_long_fired = false;  // very long event already reported

bool ui_button_down(void)
{
    return btn_stable;
}

void ui_init(void)
{
    pinMode(PIN_BUTTON, BTN_ACTIVE_LEVEL ? INPUT_PULLDOWN : INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LED_ACTIVE_LEVEL ? LOW : HIGH);

    uint32_t now = millis();
    bool raw = (digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL);
    btn_raw_last = raw;
    btn_raw_change = now;
    btn_stable = raw;
    btn_press_start = now;      // starting pressed = beginning of a gesture
    btn_long_fired = false;
}

ui_event_t ui_poll(void)
{
    ui_event_t ev = UI_EV_NONE;
    bool raw = (digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL);
    uint32_t now = millis();

    if (raw != btn_raw_last) {
        /* candidate - wait until the state persists */
        btn_raw_last = raw;
        btn_raw_change = now;
    }
    else if (raw != btn_stable && (now - btn_raw_change) >= BTN_DEBOUNCE_MS) {
        btn_stable = raw;               // accepted after debounce
        if (btn_stable) {
            btn_press_start = now;
            btn_long_fired = false;
        }
        else if (!btn_long_fired) {
            uint32_t held = now - btn_press_start;
            if (held >= BTN_VERYLONG_MS)      ev = UI_EV_VERYLONG;
            else if (held >= BTN_LONG_MS)     ev = UI_EV_LONG;
            else                              ev = UI_EV_SHORT;
        }
    }

    /* very long press is signalled immediately (without waiting for release) */
    if (btn_stable && !btn_long_fired && (now - btn_press_start) >= BTN_VERYLONG_MS) {
        btn_long_fired = true;
        ev = UI_EV_VERYLONG;
    }
    return ev;
}

/* ------------------------------- LED ------------------------------ */

static ui_pat_t led_pat = UI_PAT_OFF;
static uint8_t  led_blinks_left = 0;
static bool     led_on = false;
static bool     led_series = false;   // blink series in progress
static uint32_t led_next = 0;

void ui_set_pattern(ui_pat_t pat)
{
    led_pat = pat;
    led_blinks_left = 0;
    led_on = false;
    led_series = false;
    digitalWrite(PIN_LED, LED_ACTIVE_LEVEL ? LOW : HIGH);
    led_next = millis();
}

void ui_blink_count(uint8_t n)
{
    led_blinks_left = n;
    led_on = false;
    led_series = true;
    led_next = millis();
}

void ui_update(void)
{
    uint32_t now = millis();
    if ((int32_t)(now - led_next) < 0) {
        return;
    }

    if (led_series) {
        if (led_on) {
            /* end of a blink */
            digitalWrite(PIN_LED, LED_ACTIVE_LEVEL ? LOW : HIGH);
            led_on = false;
            if (led_blinks_left > 0) {
                led_blinks_left--;
            }
            led_next = now + 90;
        }
        else if (led_blinks_left > 0) {
            /* next blink */
            digitalWrite(PIN_LED, LED_ACTIVE_LEVEL ? HIGH : LOW);
            led_on = true;
            led_next = now + 90;
        }
        else {
            /* end of the series - gap depends on the pattern */
            led_series = false;
            uint32_t idle = 2000;
            switch (led_pat) {
            case UI_PAT_OFF:     idle = 50;   break;
            case UI_PAT_NORMAL:  idle = 2000; break;
            case UI_PAT_LEARN:   idle = 500;  break;
            case UI_PAT_SINGLE:  idle = 500;  break;
            case UI_PAT_LOWBAT:  idle = 3000; break;
            }
            led_next = now + idle;
        }
        return;
    }

    /* start the series for the pattern */
    if (led_pat == UI_PAT_OFF) {
        digitalWrite(PIN_LED, LED_ACTIVE_LEVEL ? LOW : HIGH);
        led_next = now + 50;
        return;
    }
    uint8_t n = 1;
    switch (led_pat) {
    case UI_PAT_NORMAL:  n = 1;  break;
    case UI_PAT_LEARN:   n = 3;  break;
    case UI_PAT_SINGLE:  n = 2;  break;
    case UI_PAT_LOWBAT:  n = 10; break;
    default: break;
    }
    led_blinks_left = n;
    led_on = false;
    led_series = true;
    led_next = now;
}
