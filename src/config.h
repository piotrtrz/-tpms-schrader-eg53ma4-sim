/*
 * config.h - konfiguracja symulatora czujnikow TPMS Schrader EG53MA4
 * dla Opla Astry J Sedan 2016 (EU, TPMS bezposredni, czujniki Schrader).
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  ZASTRZEZENIE / DISCLAIMER:
 *  Projekt wylacznie do celow diagnostycznych i testowych.
 *  Symulator dostarcza SZTUCZNE dane cisnienia i temperatury i NIE
 *  zastepuje prawdziwych czujnikow TPMS. Nie uzywaj tego urzadzenia do
 *  jazdy po drogach publicznych - prawdziwy TPMS jest elementem
 *  bezpieczenstwa czynnego pojazdu.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *  Protokol wg rtl_433 src/devices/schraeder.c (schrader_EG53MA4):
 *  "Schrader TPMS EG53MA4, Saab, Opel, Vauxhall, Chevrolet"
 *  (Opel OEM 13348393), weryfikacja: tools/tpms_frame.py, tools/gen_cu8.py.
 */
#ifndef TPMS_CONFIG_H
#define TPMS_CONFIG_H

#define FW_VERSION       "1.0.0"
#define CONFIG_VERSION   4      // podnos przy zmianie formatu danych w RTC

/* ======================================================================
 *  1. ZASILANIE
 *  POWER_12V     - stale zasilanie z akumulatora 12 V przez buck/ LDO
 *                  o niskim pradzie spoczynkowym (np. TPS62740, Iq < 1 uA)
 *  POWER_BATTERY - ogniwo 3.3-4.2 V (Li-SOCl2 / LiPo) bez przetwornicy
 * ====================================================================== */
#define POWER_SUPPLY_MODE   POWER_12V
#define POWER_12V           1
#define POWER_BATTERY       2

/* ======================================================================
 *  2. PINY  (ESP32 DevKit + CC1101 po SPI)
 *  Piny CC1101 zgodne z wymaganiami projektu:
 *  SCK=18  MISO=19  MOSI=23  CS=5  GDO0=2
 * ====================================================================== */
#define PIN_CC1101_SCK      18
#define PIN_CC1101_MISO     19
#define PIN_CC1101_MOSI     23
#define PIN_CC1101_CS       5
#define PIN_CC1101_GDO0     2

/* Przycisk sterowania trybami (wejscie budzace EXT0).
 * Dolaczenie do GND + rezystor podciagajacy 10k do 3V3 (aktywny niski). */
#define PIN_BUTTON          26
#define BTN_ACTIVE_LEVEL    0          // 0 = aktywny niski (przycisk -> GND)

/* LED statusowy (rezystor szeregowy 330R, aktywny wysoki). */
#define PIN_LED             27

/* Opcjonalne wejscie aktywacji LF 125 kHz (cewka + prostownik + tranzystor).
 * Impuls wysoki = "programator przy kole" -> natychmiastowa transmisja.
 * -1 wylacza funkcje (role pelni przycisk "aktywuj kolo"). */
#define PIN_LF_TRIGGER      25

/* Opcjonalny sygnal zaplonu / ACC (dzielnik 12 V -> 3.3 V). -1 = brak. */
#define PIN_ACC             32

/* Pomiar napiecia zasilania: dzielnik R1(gora)/R2(dol) na pin ADC1.
 * Uwaga: GPIO34/35/32/33 = ADC1, mozliwe rowniez jako wejscia EXT. */
#define PIN_VBAT_ADC        34
#define VBAT_DIVIDER_R1     100000.0f  // ohm, gora (do zasilania)
#define VBAT_DIVIDER_R2     33000.0f   // ohm, dol (do masy)
#define VBAT_CAL            1.000f     // mnoznik kalibracji (por. z multimetrem)
#define VBAT_SAMPLES        16

/* ======================================================================
 *  3. CZUJNIKI - 4 identyfikatory (hex, po jednym na kolo)
 *  Kolejnosc nauki w Astrze: przod lewy -> przod prawy -> tyl prawy -> tyl lewy
 *  ID sa przykladowe (24-bit). Najlepiej przepisz ID ze starych czujnikow
 *  (odczytaj je rtl_433: rtl_433 -f 433.92M) - auto wtedy "widzi" znajome ID.
 * ====================================================================== */
#define WHEEL_FL            0
#define WHEEL_FR            1
#define WHEEL_RR            2
#define WHEEL_RL            3
#define WHEEL_COUNT         4

/* Kolejnosc nauki (kolejnosc pozycji w tablicy SENSOR[] ponizej = kolejnosc
 * learn; domyslnie FL -> FR -> RR -> RL). Kolejnosc mozesz zmienic po prostu
 * przestawiajac wpisy w tablicy SENSOR[] w main.cpp (lub zmieniajac ponizsze
 * makra kolejnosci w trybie learn). */
#define LEARN_ORDER_0       WHEEL_FL
#define LEARN_ORDER_1       WHEEL_FR
#define LEARN_ORDER_2       WHEEL_RR
#define LEARN_ORDER_3       WHEEL_RL

/* ID poszczegolnych kol (24 bity) */
#define SENSOR_FL_ID        0x2FC871u
#define SENSOR_FR_ID        0x2EAAF9u
#define SENSOR_RR_ID        0x3495ACu
#define SENSOR_RL_ID        0x2FC780u

/* Cisnienie startowe: DOMYSLNIE 2,6 bar = 260 kPa dla wszystkich kol.
 * Rozdzielczosc protokolu: 2,5 kPa/bit (0-637,5 kPa). */
#define SENSOR_FL_PRESS_KPA 260.0f
#define SENSOR_FR_PRESS_KPA 260.0f
#define SENSOR_RR_PRESS_KPA 260.0f
#define SENSOR_RL_PRESS_KPA 260.0f

/* Temperatura otoczenia (st. C) - konwertowana na st. F wg protokolu. */
#define SENSOR_TEMP_C       23

/* ======================================================================
 *  4. PARAMETRY RF (Schrader EG53MA4)
 *  - dane: Manchester G.E.T. MC, chip = 120 us (8.33 kBaud)
 *  - nośna 433.92 MHz
 *  - modulacja: ASK/OOK (domyślna - tak pracuje referencyjny EG53MA4 z
 *    nagran rtl_433_tests i tak dekoderuje wbudowany Schrader-EG53MA4)
 *    albo 2-FSK z dewiacja 47.6 kHz (wariant konfigurowalny - jesli Twoj
 *    nadajnik/odbiornik auta pracuje FSK, przełacz MODE_FSK).
 * ====================================================================== */
#define MODE_OOK            0
#define MODE_FSK            1

#define TPMS_FREQUENCY_MHZ  433.92f
#define TPMS_MODULATION     MODE_OOK
#define TPMS_DEVIATION_KHZ  47.6f      // dla MODE_FSK
#define TPMS_TX_POWER_DBM   10         // -30,-20,-15,-10,-6,0,5,7,10,11,12
#define TPMS_CHIP_US        120        // czas trwania jednego chipa Manchester
                                       // (8.33 kBaud - zmierzone na nagraniach)

/* Ramka (dane z dekodera rtl_433 schrader_EG53MA4):
 * 40 bit sync + 4 bajty flags + 3 bajty ID + cisnienie + temperatura + CRC.
 * Suma kontrolna: suma 9 bajtow modulo 256 (add_bytes). */
#define TPMS_FLAGS          0x4C900070u // flags ze zrzutow prawdziwych czujnikow
#define TPMS_FLAGS_LOWBAT_MASK 0x00000008u  // bit "rozladowana bateria"
                                            // (pozycja niepewna - dostosuj
                                            // jesli Twoj sterownik oczekuje
                                            // innego bitu)

/* ======================================================================
 *  5. HARMONOGRAM NADAWANIA / SEN
 * ====================================================================== */
#define TX_BURST_FRAMES     3          // 3-6 ramek w burście (jak prawdziwy czujnik)
#define TX_FRAME_GAP_MS     50         // odstep miedzy ramkami w burscie
#define TX_INTERVAL_S       30         // interwal: kazde kolo co 30 s
#define TX_INTERVAL_IGNITION_S  15     // krotki interwal przy zaplonie (ACC)
#define TX_INTERVAL_PARKING_S   120    // interwal postojowy po zaniku ACC
#define ACC_GRACE_MS        (5UL*60UL*1000UL)  // 5 min po zaniku ACC
#define PARKING_SHUTDOWN    0          // 1 = po ACC_GRACE calkowite uspienie
                                       //    (sen bez budzika, obudza przycisk/LF)
#define BOOT_STAY_AWAKE_MS  3000       // okno na konsoli szeregowej po starcie

/* ======================================================================
 *  6. TRYB LEARN / SINGLE
 * ====================================================================== */
#define LEARN_TX_PERIOD_MS  2000       // co ile ms wysylac bursta w learn
#define LEARN_WINDOW_MS     60000      // okno czasowe na 1 czujnik (60 s)
#define LEARN_WINDOW_EXPIRE_TO_NORMAL 1  // 1 = po uplywie okna powrot do
                                         //     trybu normalnego (wg specyfikacji),
                                         // 0 = automatycznie kolejne kolo,
                                         //     po ostatnim -> NORMAL
#define LEARN_TOTAL_TIMEOUT_MS 900000  // bezpiecznik: 15 min i koniec learn
#define SINGLE_TIMEOUT_MS   300000     // 0 = bez limitu

/* ======================================================================
 *  7. PROGI / DIAGNOSTYKA
 * ====================================================================== */
#define LOWBATT_THRESHOLD_MV 11500     // dla POWER_12V (prog alarmu TPMS)
                                       // dla POWER_BATTERY ustaw np. 3200
#define SERIAL_BAUD         115200

/* ======================================================================
 *  8. GESTY PRZYCISKU
 * ====================================================================== */
#define BTN_DEBOUNCE_MS     30
#define BTN_LONG_MS         1500       // >= 1.5 s  = wejscie/wyjscie LEARN
#define BTN_VERYLONG_MS     4000       // >= 4 s    = wejscie/wyjscie SINGLE

#endif /* TPMS_CONFIG_H */
