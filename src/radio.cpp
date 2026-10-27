/*
 * radio.cpp - konfiguracja CC1101 i nadawanie ramek TPMS
 *
 * Modul: CC1101 po SPI (SCK=18, MISO=19, MOSI=23, CS=5, GDO0=2).
 * Biblioteka: SmartRC-CC1101-Driver-Lib (LSatan), naglowek zgodnosciowy
 * ELECHOUSE_CC1101_SRC_DRV.h - obslugiwane sa obie wersje API (2.x i 3.x).
 *
 * Nadawanie: tryb FIFO (pakiet o stalej dlugosci, bez preambuly/sync/CRC
 * CC1101 - wszystkie bity, lacznie z preambula i wlasna suma kontrolna,
 * leza w naszym buforze). Kolejnosc bitow MSB first.
 */
#include <Arduino.h>
#include <SPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <driver/gpio.h>

#include "config.h"
#include "radio.h"
#include "tpms_frame.h"

static bool radio_ok = false;

/* ------------------------------------------------------------------ */
/*  Inicjalizacja pinow                                                */
/*  Wymagana biblioteka SmartRC-CC1101-Driver-Lib >= 2.5.7             */
/*  (wspolne API setSpiPin/setGDO0 dla wersji 2.5.x i 3.x;             */
/*   starsze 1.x mialy setSCK/setMISO/... - nie sa obslugiwane).       */
/* ------------------------------------------------------------------ */
static void radio_set_pins(void)
{
    ELECHOUSE_cc1101.setSpiPin(PIN_CC1101_SCK, PIN_CC1101_MISO,
                               PIN_CC1101_MOSI, PIN_CC1101_CS);
    ELECHOUSE_cc1101.setGDO0(PIN_CC1101_GDO0);
}

bool radio_init(void)
{
    gpio_deep_sleep_hold_dis();
    gpio_hold_dis((gpio_num_t)PIN_CC1101_CS);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_SCK);
    gpio_hold_dis((gpio_num_t)PIN_CC1101_MOSI);

    radio_set_pins();
    ELECHOUSE_cc1101.Init();

    radio_ok = ELECHOUSE_cc1101.getCC1101();
    if (!radio_ok) {
        return false;
    }

    ELECHOUSE_cc1101.setCCMode(1);            // tryb wewnetrzny, dane z FIFO
    if (TPMS_MODULATION == MODE_FSK) {
        ELECHOUSE_cc1101.setModulation(0);    // 2-FSK
        ELECHOUSE_cc1101.setDeviation(TPMS_DEVIATION_KHZ);
    }
    else {
        ELECHOUSE_cc1101.setModulation(2);    // ASK/OOK
    }
    ELECHOUSE_cc1101.setMHZ(TPMS_FREQUENCY_MHZ);
    ELECHOUSE_cc1101.setDRate(1000.0f / (float)TPMS_CHIP_US);  // kBaud (8.33)
    ELECHOUSE_cc1101.setPA(TPMS_TX_POWER_DBM);
    ELECHOUSE_cc1101.setChannel(0);

    ELECHOUSE_cc1101.setSyncMode(0);          // bez preambuly/sync CC1101
    ELECHOUSE_cc1101.setPRE(0);
    ELECHOUSE_cc1101.setAdrChk(0);
    ELECHOUSE_cc1101.setAddr(0);
    ELECHOUSE_cc1101.setWhiteData(0);         // bez whiteningu
    ELECHOUSE_cc1101.setManchester(0);        // Manchester kodujemy sami
    ELECHOUSE_cc1101.setFEC(0);
    ELECHOUSE_cc1101.setCrc(0);               // wlasna suma kontrolna protokolu
    ELECHOUSE_cc1101.setCRC_AF(0);
    ELECHOUSE_cc1101.setAppendStatus(0);
    ELECHOUSE_cc1101.setDcFilterOff(0);
    ELECHOUSE_cc1101.setPktFormat(0);         // normalny tryb FIFO
    ELECHOUSE_cc1101.setLengthConfig(0);      // stala dlugosc pakietu
    ELECHOUSE_cc1101.setPacketLength(TPMS_TX_BYTES);
    return true;
}

bool radio_send_frame(const uint8_t *buf, uint8_t len)
{
    if (!radio_ok) {
        return false;
    }

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);
    ELECHOUSE_cc1101.SpiWriteBurstReg(CC1101_TXFIFO, (uint8_t *)buf, len);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);

    /* Pelny czas serializacji bufora na antenie: sprzet wysyla dokladnie
     * len*8 chipow z predkoscia DRate. UWAGA: TXBYTES == 0 NIE oznacza
     * konca transmisji - po oproznieniu FIFO ostatni bajt jest dopiero
     * w rejestrze przesuwnym (8 chipow = 8*TPMS_CHIP_US), wiec wczesniejsze
     * SIDLE obcialoby ogon ramki. Czekamy zatem na policzony czas + maly
     * margines i dopiero wtedy wychodzimy z TX. */
    uint32_t air_us = (uint32_t)len * 8u * (uint32_t)TPMS_CHIP_US + 1000u;
    uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < air_us) {
        delayMicroseconds(500);
    }

    /* Diagnostyka po zakonczeniu: dwa zgodne odczyty (errata CC1101),
     * kontrola niedopelnienia FIFO i tego, czy bufor zostal faktycznie
     * wyslany. Zwracamy blad zamiast udawac sukces. */
    bool ok = true;
    uint8_t txb = ELECHOUSE_cc1101.SpiReadStatus(CC1101_TXBYTES);
    uint8_t txb2 = ELECHOUSE_cc1101.SpiReadStatus(CC1101_TXBYTES);
    if (txb != txb2) {
        txb = txb2;
    }
    if (txb & 0x80u) {          // TXFIFO_UNDERFLOW
        ok = false;
    }
    if ((txb & 0x7Fu) != 0) {   // cos zostalo w FIFO - transmisja nie wyszla
        ok = false;
    }

    ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
    ELECHOUSE_cc1101.SpiStrobe(CC1101_SFTX);
    return ok;
}

bool radio_send_burst(const uint8_t *buf, uint8_t len, uint8_t frames)
{
    bool ok = true;
    for (uint8_t i = 0; i < frames; i++) {
        if (!radio_send_frame(buf, len)) {
            ok = false;
        }
        if (i + 1u < frames) {
            delay(TX_FRAME_GAP_MS);
        }
    }
    return ok;
}

void radio_sleep(void)
{
    if (radio_ok) {
        ELECHOUSE_cc1101.goSleep();       // SIDLE + SPWD (sen po zboczu CS)
    }
    /* Zwolnij peryferium SPI i ustaw linie w stanach bezpiecznych:
     * CS=1 (utrzymuje CC1101 w sleep), SCK/MOSI=0, GDO0 wejscie z podciaganiem
     * w dol (CC1101 w sleep trzystanuje wyjscia). */
    SPI.end();
    pinMode(PIN_CC1101_CS, OUTPUT);
    digitalWrite(PIN_CC1101_CS, HIGH);
    pinMode(PIN_CC1101_SCK, OUTPUT);
    digitalWrite(PIN_CC1101_SCK, LOW);
    pinMode(PIN_CC1101_MOSI, OUTPUT);
    digitalWrite(PIN_CC1101_MOSI, LOW);
    pinMode(PIN_CC1101_GDO0, INPUT_PULLDOWN);
}
