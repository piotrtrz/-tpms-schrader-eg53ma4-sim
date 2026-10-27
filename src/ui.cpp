/*
 * ui.cpp - przycisk (gesty) i LED statusowy (nieblokujaco, bez delay())
 */
#include <Arduino.h>
#include "config.h"
#include "ui.h"

/* ---------------------------- przycisk ---------------------------- */
/* Debounce: nowy stan surowy musi sie utrzymac BTN_DEBOUNCE_MS, zanim
 * zostanie uznany za stan stabilny (kandydat + czas jego zmiany). */

static bool     btn_raw_last = false;    // ostatni surowy odczyt
static uint32_t btn_raw_change = 0;      // czas ostatniej zmiany surowej
static bool     btn_stable = false;      // stan po debounce
static uint32_t btn_press_start = 0;
static bool     btn_long_fired = false;  // bardzo dlugie zdarzenie juz wyslane

bool ui_button_down(void)
{
    return btn_stable;
}

void ui_init(void)
{
    pinMode(PIN_BUTTON, BTN_ACTIVE_LEVEL ? INPUT_PULLDOWN : INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    uint32_t now = millis();
    bool raw = (digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL);
    btn_raw_last = raw;
    btn_raw_change = now;
    btn_stable = raw;
    btn_press_start = now;      // start z wcisnietym przyciskiem = poczatek gestu
    btn_long_fired = false;
}

ui_event_t ui_poll(void)
{
    ui_event_t ev = UI_EV_NONE;
    bool raw = (digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL);
    uint32_t now = millis();

    if (raw != btn_raw_last) {
        /* kandydat - czekaj az stan sie utrzyma */
        btn_raw_last = raw;
        btn_raw_change = now;
    }
    else if (raw != btn_stable && (now - btn_raw_change) >= BTN_DEBOUNCE_MS) {
        btn_stable = raw;               // zaakceptowany po debounce
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

    /* bardzo dlugi przycisk sygnalizujemy od razu (nie czekajac na puszczenie) */
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
static bool     led_series = false;   // seria migniec w trakcie
static uint32_t led_next = 0;

void ui_set_pattern(ui_pat_t pat)
{
    led_pat = pat;
    led_blinks_left = 0;
    led_on = false;
    led_series = false;
    digitalWrite(PIN_LED, LOW);
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
            /* koniec migniecia */
            digitalWrite(PIN_LED, LOW);
            led_on = false;
            if (led_blinks_left > 0) {
                led_blinks_left--;
            }
            led_next = now + 90;
        }
        else if (led_blinks_left > 0) {
            /* kolejne migniecie */
            digitalWrite(PIN_LED, HIGH);
            led_on = true;
            led_next = now + 90;
        }
        else {
            /* koniec serii - przerwa zalezna od wzorca */
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

    /* rozpoczynamy serie wg wzorca */
    if (led_pat == UI_PAT_OFF) {
        digitalWrite(PIN_LED, LOW);
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
