/*
 * console.h - debugowa konsola szeregowa (bez String, stalobuforowa)
 */
#ifndef TPMS_CONSOLE_H
#define TPMS_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void console_init(void);
void console_poll(void);     // nieblokujaca obsluga linii - wywolywac w petli

#ifdef __cplusplus
}
#endif

#endif /* TPMS_CONSOLE_H */
