/*
 * ui.h - button (gestures) and status LED
 */
#ifndef TPMS_UI_H
#define TPMS_UI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_EV_NONE = 0,
    UI_EV_SHORT,        // < BTN_LONG_MS
    UI_EV_LONG,         // >= BTN_LONG_MS  (enter/exit LEARN)
    UI_EV_VERYLONG,     // >= BTN_VERYLONG_MS (enter/exit SINGLE)
} ui_event_t;

typedef enum {
    UI_PAT_OFF = 0,
    UI_PAT_NORMAL,      // 1 blink every 2 s
    UI_PAT_LEARN,       // 3 quick blinks every 0.5 s
    UI_PAT_SINGLE,      // 2 quick blinks every 0.5 s
    UI_PAT_LOWBAT,      // 10 quick blinks every 3 s
} ui_pat_t;

void ui_init(void);

/* Non-blocking button and LED handling - call in every loop. */
ui_event_t ui_poll(void);
void ui_update(void);

bool ui_button_down(void);
void ui_set_pattern(ui_pat_t pat);

/* Series of n quick blinks (e.g. wheel number: 1-4). */
void ui_blink_count(uint8_t n);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_UI_H */
