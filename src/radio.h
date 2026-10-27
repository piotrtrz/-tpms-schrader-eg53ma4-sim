/*
 * radio.h - sterowanie CC1101 (nadawanie ramek TPMS, tryb uśpienia)
 */
#ifndef TPMS_RADIO_H
#define TPMS_RADIO_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicjalizacja SPI + konfiguracja CC1101. Zwraca false przy braku modulu. */
bool radio_init(void);

/* Wyslij jeden bufor nadawczy (TPMS_TX_BYTES) - blokujace do zakonczenia
 * transmisji na eterze (~29 ms). */
void radio_send_frame(const uint8_t *buf, uint8_t len);

/* Burts: n ramek tego samego bufora z odstepem TX_FRAME_GAP_MS. */
void radio_send_burst(const uint8_t *buf, uint8_t len, uint8_t frames);

/* CC1101 do trybu SLEEP (<1 uA) + przygotowanie linii pod gpio_hold. */
void radio_sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_RADIO_H */
