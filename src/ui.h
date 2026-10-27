/*
 * ui.h - przycisk (gesty) i LED statusowy
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
    UI_EV_LONG,         // >= BTN_LONG_MS  (wejscie/wyjscie LEARN)
    UI_EV_VERYLONG,     // >= BTN_VERYLONG_MS (wejscie/wyjscie SINGLE)
} ui_event_t;

typedef enum {
    UI_PAT_OFF = 0,
    UI_PAT_NORMAL,      // 1 migniecie co 2 s
    UI_PAT_LEARN,       // 3 szybkie migniecia co 0.5 s
    UI_PAT_SINGLE,      // 2 szybkie migniecia co 0.5 s
    UI_PAT_LOWBAT,      // 10 szybkich migniec co 3 s
} ui_pat_t;

void ui_init(void);

/* Nieblokujaca obsluga przycisku i LED - wywolywac w kazdej petli. */
ui_event_t ui_poll(void);
void ui_update(void);

bool ui_button_down(void);
void ui_set_pattern(ui_pat_t pat);

/* Seria n szybkich migniec (np. numer kola: 1-4). */
void ui_blink_count(uint8_t n);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_UI_H */
