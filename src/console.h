/*
 * console.h - debug serial console (no String, fixed buffer)
 */
#ifndef TPMS_CONSOLE_H
#define TPMS_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void console_init(void);
void console_poll(void);     // non-blocking line handling - call in the loop

#ifdef __cplusplus
}
#endif

#endif /* TPMS_CONSOLE_H */
