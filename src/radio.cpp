/* CC1101: surowe 30 bajtow Manchester w fixed-length FIFO, bez wstawek HW. */
#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <driver/gpio.h>

#include "config.h"
#include "radio.h"
#include "tpms_frame.h"

static bool radio_ok = false;

bool radio_init(const settings_t *cfg)
{
    radio_ok = false;
    if (cfg == NULL) {
        return false;
    }
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_MOSI);

    ELECHOUSE_cc1101.setSpiPin(PIN_CC1101_SCK, PIN_CC1101_MISO,
                               PIN_CC1101_MOSI, PIN_CC1101_CS);
    ELECHOUSE_cc1101.setGDO0(PIN_CC1101_GDO0);
    ELECHOUSE_cc1101.Init();
    if (!ELECHOUSE_cc1101.getCC1101()) {
        return false;
    }

    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setModulation(2); /* ASK/OOK; brak profilu FSK. */
    ELECHOUSE_cc1101.setMHZ(cfg->frequency_mhz);
    ELECHOUSE_cc1101.setDRate(1000.0f / (float)TPMS_CHIP_US);
    ELECHOUSE_cc1101.setPA(cfg->tx_power_dbm);
    ELECHOUSE_cc1101.setChannel(0);

    /* SYNC_MODE=0: packet engine nie dodaje preambuly ani slowa sync. */
    ELECHOUSE_cc1101.setSyncMode(0);
    ELECHOUSE_cc1101.setPRE(0);
    ELECHOUSE_cc1101.setAdrChk(0);
    ELECHOUSE_cc1101.setAddr(0);
    ELECHOUSE_cc1101.setWhiteData(0);
    ELECHOUSE_cc1101.setManchester(0);
    ELECHOUSE_cc1101.setFEC(0);
    ELECHOUSE_cc1101.setCrc(0);
    ELECHOUSE_cc1101.setCRC_AF(0);
    ELECHOUSE_cc1101.setAppendStatus(0);
    ELECHOUSE_cc1101.setDcFilterOff(0);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.setLengthConfig(0);
    ELECHOUSE_cc1101.setPacketLength(TPMS_TX_BYTES);
    radio_ok = true;
    return true;
}

bool radio_send_frame(const uint8_t *buf, uint8_t len)
{
    if (!radio_ok || buf == NULL || len != TPMS_TX_BYTES) {
        return false;
    }

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);
    ELECHOUSE_cc1101.SpiWriteBurstReg(CC1101_TXFIFO, (uint8_t *)buf, len);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);

    /* Czekamy na MARCSTATE, nie na samo TXBYTES=0: ostatni bajt moze byc
     * jeszcze w rejestrze przesuwnym. Limit obejmuje start i pelny ogon. */
    const uint32_t timeout_us = (uint32_t)len * 8u * TPMS_CHIP_US + 10000u;
    const uint32_t start = micros();
    bool finished = false;
    bool underflow = false;
    while ((uint32_t)(micros() - start) < timeout_us) {
        uint8_t state = ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1Fu;
        if (state == 0x16u) { /* TXFIFO_UNDERFLOW */
            underflow = true;
            break;
        }
        if (state == 0x01u) { /* IDLE po automatycznym zakonczeniu pakietu */
            finished = true;
            break;
        }
        delayMicroseconds(50);
    }

    uint8_t txbytes = ELECHOUSE_cc1101.SpiReadStatus(CC1101_TXBYTES);
    underflow |= (txbytes & 0x80u) != 0u;
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);
    return finished && !underflow && (txbytes & 0x7Fu) == 0u;
}

bool radio_send_burst(const uint8_t *buf, uint8_t len, uint8_t frames,
                      uint16_t gap_ms)
{
    bool ok = true;
    for (uint8_t i = 0; i < frames; ++i) {
        ok = radio_send_frame(buf, len) && ok;
        if ((uint8_t)(i + 1u) < frames) {
            delay(gap_ms);
        }
    }
    return ok;
}

void radio_sleep(void)
{
    if (radio_ok) {
        ELECHOUSE_cc1101.goSleep();
        radio_ok = false;
    }
    SPI.end();
    pinMode(PIN_CC1101_CS, OUTPUT);
    digitalWrite(PIN_CC1101_CS, HIGH);
    pinMode(PIN_CC1101_SCK, OUTPUT);
    digitalWrite(PIN_CC1101_SCK, LOW);
    pinMode(PIN_CC1101_MOSI, OUTPUT);
    digitalWrite(PIN_CC1101_MOSI, LOW);
    pinMode(PIN_CC1101_GDO0, INPUT_PULLDOWN);
}
