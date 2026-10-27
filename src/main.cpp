/*
 * main.cpp - symulator 4 czujnikow TPMS Schrader EG53MA4 (ESP32 + CC1101)
 * dla Opla Astry J Sedan 2016 (EU, 433 MHz, TPMS bezposredni).
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  ZASTRZEZENIE: projekt wylacznie diagnostyczny/testowy.
 *  Symulator dostarcza SZTUCZNE dane cisnienia i temperatury i NIE
 *  zastepuje prawdziwych czujnikow TPMS. Nie uzywac podczas jazdy.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Tryby pracy:
 *  NORMAL - rotacja 4 kol, burst 3-5 ramek co TX_INTERVAL_S, deep sleep
 *           miedzy transmisjami (budzik RTC + budzenie przyciskiem/LF)
 *  LEARN  - nauka czujnikow w aucie: jedno kolo naraz, kolejnosc
 *           FL -> FR -> RR -> RL, przycisk = kolejne kolo, okno czasowe
 *  SINGLE - wymuszone nadawanie jednego ID (testy/procedury serwisowe)
 *
 * Protokol: rtl_433 src/devices/schraeder.c (schrader_EG53MA4),
 * Manchester G.E.T. MC, chip 120 us, ASK/OOK 433.92 MHz (opcjonalnie 2-FSK).
 */
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

#include "config.h"
#include "tpms_frame.h"
#include "radio.h"
#include "power.h"
#include "ui.h"
#include "console.h"
#include "app.h"

/* ------------------------------------------------------------------ */
/*  Dane czujnikow (mapowanie ID -> kolo)                              */
/* ------------------------------------------------------------------ */
#define KPA10(x) ((uint16_t)((x) * 10.0f + 0.5f))

static const tpms_sensor_t SENSOR[WHEEL_COUNT] = {
    { SENSOR_FL_ID, KPA10(SENSOR_FL_PRESS_KPA), SENSOR_TEMP_C, "FL" },
    { SENSOR_FR_ID, KPA10(SENSOR_FR_PRESS_KPA), SENSOR_TEMP_C, "FR" },
    { SENSOR_RR_ID, KPA10(SENSOR_RR_PRESS_KPA), SENSOR_TEMP_C, "RR" },
    { SENSOR_RL_ID, KPA10(SENSOR_RL_PRESS_KPA), SENSOR_TEMP_C, "RL" },
};

/* Kolejnosc nauki (kolejnosc w tablicy SENSOR = FL, FR, RR, RL). */
static const uint8_t LEARN_SEQ[WHEEL_COUNT] = {
    LEARN_ORDER_0, LEARN_ORDER_1, LEARN_ORDER_2, LEARN_ORDER_3,
};

/* ------------------------------------------------------------------ */
/*  Stan w RTC (przetrwuje deep sleep)                                 */
/* ------------------------------------------------------------------ */
/* Magic zwiazany z wersja konfiguracji: zmiana CONFIG_VERSION (np. po
 * zmianie formatu stanu w RTC) zeruje caly stan po wybudzeniu. */
#define RTC_MAGIC (0x54504D53u ^ (uint32_t)CONFIG_VERSION)  // 'TPMS' ^ version

RTC_DATA_ATTR static uint32_t rtc_magic;
RTC_DATA_ATTR static uint8_t  rtc_mode = APP_MODE_NORMAL;
RTC_DATA_ATTR static uint8_t  rtc_seq_idx = 0;       // pozycja w LEARN_SEQ (LEARN)
RTC_DATA_ATTR static uint8_t  rtc_single_wheel = 0;  // kolo w SINGLE (indeks SENSOR)
RTC_DATA_ATTR static uint8_t  rtc_rot_idx = 0;       // rotacja w NORMAL
RTC_DATA_ATTR static uint32_t rtc_wake_count = 0;
RTC_DATA_ATTR static uint32_t rtc_interval_s = 0;    // 0 = auto wg ACC
RTC_DATA_ATTR static uint32_t rtc_flags = TPMS_FLAGS;
RTC_DATA_ATTR static uint64_t rtc_last_acc_ms = 0;

/* Stan sesji (wazny tylko w biezaczym wybudzeniu) */
static uint32_t stay_awake_until = 0;
static uint32_t next_tx_ms = 0;
static uint32_t learn_window_start = 0;
static uint32_t learn_session_start = 0;
static uint32_t single_start = 0;
static bool     request_sleep = false;
static bool     low_batt = false;
static uint32_t supply_mv = 0;
#if PIN_LF_TRIGGER >= 0
static bool     lf_prev = false;       // poprzedni stan wejscia LF (zbocze)
#endif

/* ------------------------------------------------------------------ */
/*  Pomocnicze                                                         */
/* ------------------------------------------------------------------ */
static uint32_t compute_interval(void);

/* Aktywne kolo: SINGLE trzyma bezposredni indeks SENSOR, LEARN - pozycje
 * w sekwencji nauki LEARN_SEQ (moga sie roznic po zmianie LEARN_ORDER_*). */
static uint8_t active_wheel(void)
{
    return (rtc_mode == APP_MODE_SINGLE)
            ? (uint8_t)(rtc_single_wheel % WHEEL_COUNT)
            : LEARN_SEQ[rtc_seq_idx % WHEEL_COUNT];
}

/* Pozycja kola w sekwencji nauki (do trybu LEARN). */
static uint8_t seq_pos_of(uint8_t wheel)
{
    for (uint8_t i = 0; i < WHEEL_COUNT; i++) {
        if (LEARN_SEQ[i] == wheel) {
            return i;
        }
    }
    return 0;
}

static void refresh_power(void)
{
    supply_mv = power_read_mv();
    low_batt = power_low_battery(supply_mv);
}

static void build_frame_for(uint8_t wheel, uint8_t out[TPMS_TX_BYTES])
{
    const tpms_sensor_t *s = &SENSOR[wheel % WHEEL_COUNT];
    tpms_build_frame(out, s->id, s->press_kpa10, s->temp_c, rtc_flags, low_batt);
}

void app_send_burst_wheel(uint8_t wheel)
{
    uint8_t frame[TPMS_TX_BYTES];
    build_frame_for(wheel, frame);
    bool ok = radio_send_burst(frame, TPMS_TX_BYTES, TX_BURST_FRAMES);
    Serial.printf("TX %s id=%06X %.1f kPa %d C flags=%08X%s%s\n",
                  SENSOR[wheel % WHEEL_COUNT].name,
                  (unsigned)SENSOR[wheel % WHEEL_COUNT].id,
                  SENSOR[wheel % WHEEL_COUNT].press_kpa10 / 10.0f,
                  (int)SENSOR[wheel % WHEEL_COUNT].temp_c,
                  (unsigned)rtc_flags, low_batt ? " LOWBATT" : "",
                  ok ? "" : " TX-ERR");
}

void app_send_burst_all(void)
{
    for (uint8_t i = 0; i < WHEEL_COUNT; i++) {
        app_send_burst_wheel((uint8_t)((rtc_rot_idx + i) % WHEEL_COUNT));
    }
    rtc_rot_idx = (uint8_t)((rtc_rot_idx + 1) % WHEEL_COUNT);
}

void app_print_frame(uint8_t wheel)
{
    uint8_t frame[TPMS_TX_BYTES];
    uint8_t payload[TPMS_PAYLOAD_LEN];
    const tpms_sensor_t *s = &SENSOR[wheel % WHEEL_COUNT];
    build_frame_for(wheel, frame);
    tpms_build_payload(payload, s->id, s->press_kpa10, s->temp_c, rtc_flags, low_batt);

    Serial.printf("%s frame:", s->name);
    for (uint8_t i = 0; i < TPMS_TX_BYTES; i++) {
        Serial.printf(" %02X", frame[i]);
    }
    Serial.printf("\n%s payload:", s->name);
    for (uint8_t i = 0; i < TPMS_PAYLOAD_LEN; i++) {
        Serial.printf(" %02X", payload[i]);
    }
    Serial.printf("  crc=%02X id=%06X press=%.1f kPa temp=%d F\n",
                  payload[9], (unsigned)s->id,
                  payload[7] * 2.5f, (int)payload[8]);
}

void app_print_info(void)
{
    Serial.printf("FW %s  config v%d  mode=%s  wheel=%s\n",
                  FW_VERSION, CONFIG_VERSION,
                  rtc_mode == APP_MODE_NORMAL ? "NORMAL" :
                  rtc_mode == APP_MODE_LEARN ? "LEARN" : "SINGLE",
                  SENSOR[active_wheel()].name);
    for (uint8_t i = 0; i < WHEEL_COUNT; i++) {
        Serial.printf("  %s id=%06X press=%.1f kPa temp=%d C\n",
                      SENSOR[i].name, (unsigned)SENSOR[i].id,
                      SENSOR[i].press_kpa10 / 10.0f, (int)SENSOR[i].temp_c);
    }
    Serial.printf("flags=%08X mod=%s chip=%dus burst=%d interval=%us wake=%lu\n",
                  (unsigned)rtc_flags,
                  TPMS_MODULATION == MODE_FSK ? "FSK" : "OOK",
                  TPMS_CHIP_US, TX_BURST_FRAMES,
                  (unsigned)compute_interval(),
                  (unsigned long)rtc_wake_count);
    Serial.printf("Vcc=%u mV%s\n", (unsigned)supply_mv, low_batt ? " LOWBATT" : "");
}

uint32_t app_supply_mv(void)
{
    refresh_power();
    return supply_mv;
}

void app_set_interval(uint32_t seconds)
{
    rtc_interval_s = seconds;
}

void app_set_flags(uint32_t flags)
{
    rtc_flags = flags;
}

void app_request_sleep(void)
{
    request_sleep = true;
}

void app_stay_awake(uint32_t ms)
{
    uint32_t until = millis() + ms;
    if ((int32_t)(until - stay_awake_until) > 0) {
        stay_awake_until = until;
    }
}

uint8_t app_mode(void)
{
    return rtc_mode;
}

uint8_t app_wheel(void)
{
    return active_wheel();
}

void app_next_wheel(void)
{
    if (rtc_mode == APP_MODE_SINGLE) {
        rtc_single_wheel = (uint8_t)((rtc_single_wheel + 1) % WHEEL_COUNT);
    }
    else {
        rtc_seq_idx = (uint8_t)((rtc_seq_idx + 1) % WHEEL_COUNT);
    }
    ui_blink_count((uint8_t)(active_wheel() + 1));
    Serial.printf("Kolo: %s\n", SENSOR[active_wheel()].name);
    learn_window_start = millis();
}

void app_set_mode(uint8_t mode, uint8_t wheel)
{
    rtc_mode = mode;
    switch (mode) {
    case APP_MODE_LEARN:
        rtc_seq_idx = seq_pos_of((uint8_t)(wheel % WHEEL_COUNT));
        learn_window_start = millis();
        learn_session_start = millis();
        next_tx_ms = 0;
        ui_set_pattern(UI_PAT_LEARN);
        ui_blink_count((uint8_t)(active_wheel() + 1));
        Serial.printf("LEARN start, kolo %s\n", SENSOR[active_wheel()].name);
        app_stay_awake(LEARN_TOTAL_TIMEOUT_MS);
        break;
    case APP_MODE_SINGLE:
        rtc_single_wheel = (uint8_t)(wheel % WHEEL_COUNT);
        single_start = millis();
        next_tx_ms = 0;
        ui_set_pattern(UI_PAT_SINGLE);
        ui_blink_count((uint8_t)(active_wheel() + 1));
        Serial.printf("SINGLE start, kolo %s\n", SENSOR[active_wheel()].name);
        app_stay_awake(SINGLE_TIMEOUT_MS ? SINGLE_TIMEOUT_MS : 3600000u);
        break;
    default:
        rtc_mode = APP_MODE_NORMAL;
        ui_set_pattern(UI_PAT_NORMAL);
        Serial.println(F("NORMAL"));
        next_tx_ms = 0;
        app_stay_awake(BOOT_STAY_AWAKE_MS);
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Cykl nadawczy w trybie NORMAL                                      */
/* ------------------------------------------------------------------ */
static uint32_t compute_interval(void)
{
    if (rtc_interval_s != 0) {
        return rtc_interval_s;
    }
#if PIN_ACC < 0
    /* Bez wejscia ACC stale pracujemy z interwalem podstawowym - logika
     * "5 min po zaniku ACC" nie ma wtedy zastosowania. */
    return (uint32_t)TX_INTERVAL_S;
#else
    uint64_t now_ms = power_elapsed_ms();
    if (digitalRead(PIN_ACC) == HIGH) {
        rtc_last_acc_ms = now_ms;
        return (uint32_t)TX_INTERVAL_IGNITION_S;
    }
    if (now_ms - rtc_last_acc_ms > (uint64_t)ACC_GRACE_MS) {
        return PARKING_SHUTDOWN ? 0u : (uint32_t)TX_INTERVAL_PARKING_S;
    }
    return (uint32_t)TX_INTERVAL_S;
#endif
}

static void tx_cycle(void)
{
    refresh_power();
    app_send_burst_all();
}

/* Czy ktorekolwiek zrodlo budzenia poziomowego jest aktywne? */
static bool wake_sources_active(void)
{
    if (ui_button_down() || digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL) {
        return true;
    }
#if PIN_LF_TRIGGER >= 0
    if (digitalRead(PIN_LF_TRIGGER) == HIGH) {
        return true;
    }
#endif
    return false;
}

static void go_sleep(uint32_t seconds)
{
    /* Nie spij, dopoki przycisk jest wcisniety lub LF aktywne - EXT0/EXT1
     * dzialaja poziomowo, wiec sen przy aktywnym zrodle = natychmiastowa
     * petla wybudzen. Dajemy krotki margines i wracamy do petli. */
    if (wake_sources_active()) {
        app_stay_awake(500);
        return;
    }

    ui_set_pattern(UI_PAT_OFF);
    digitalWrite(PIN_LED, LOW);

    Serial.printf("sleep %us\n", (unsigned)seconds);
    Serial.flush();
    power_deep_sleep(seconds);   // nie wraca
}

/* ------------------------------------------------------------------ */
void setup(void)
{
    Serial.begin(SERIAL_BAUD);
    ui_init();
    power_init();
    console_init();

#if PIN_LF_TRIGGER >= 0
    pinMode(PIN_LF_TRIGGER, INPUT_PULLDOWN);
#endif
#if PIN_ACC >= 0
    /* INPUT (bez podciagania): dolna galazka dzielnika wymusza stan niski,
     * a wewnetrzny pulldown (~45 k) obciazylby dzielnik i zbil prog VIH. */
    pinMode(PIN_ACC, INPUT);
#endif

    if (rtc_magic != RTC_MAGIC) {
        rtc_magic = RTC_MAGIC;
        rtc_mode = APP_MODE_NORMAL;
        rtc_seq_idx = 0;
        rtc_single_wheel = 0;
        rtc_rot_idx = 0;
        rtc_wake_count = 0;
        rtc_interval_s = 0;
        rtc_flags = TPMS_FLAGS;
        rtc_last_acc_ms = 0;
    }
    rtc_wake_count++;

    bool radio_ok = radio_init();
    Serial.println();
    Serial.printf("TPMS simulator FW %s (config v%d) - SYMULATOR, dane sztuczne!\n",
                  FW_VERSION, CONFIG_VERSION);
    if (!radio_ok) {
        Serial.println(F("BLAD: brak modulu CC1101 (sprawdz SPI)"));
    }

    refresh_power();
    if (low_batt) {
        Serial.printf("OSTRZEZENIE: niskie napiecie zasilania %u mV!\n",
                      (unsigned)supply_mv);
    }

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    uint32_t iv = compute_interval();

    switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
        /* zwykly cykl - nadaj i spij bez czuwania */
        next_tx_ms = 0;
        stay_awake_until = 0;
        break;

    case ESP_SLEEP_WAKEUP_EXT0:
        /* przycisk - obsluga gestu w petli, krotkie okno konsoli */
        Serial.println(F("wake: button"));
        next_tx_ms = UINT32_MAX;      // nie nadawaj automatycznie
        stay_awake_until = millis() + BOOT_STAY_AWAKE_MS;
        break;

    case ESP_SLEEP_WAKEUP_EXT1: {
        uint64_t st = esp_sleep_get_ext1_wakeup_status();
        Serial.printf("wake: ext1 mask=%08X%08X\n",
                      (unsigned)(st >> 32), (unsigned)(st & 0xFFFFFFFFu));
        refresh_power();
        app_send_burst_wheel(active_wheel());   // aktywacja LF = natychmiast
        next_tx_ms = UINT32_MAX;
        stay_awake_until = millis() + BOOT_STAY_AWAKE_MS;
        break;
    }

    default:
        Serial.println(F("wake: power-on"));
        next_tx_ms = 0;
        stay_awake_until = millis() + BOOT_STAY_AWAKE_MS;
        break;
    }

    if (rtc_mode == APP_MODE_LEARN) {
        ui_set_pattern(UI_PAT_LEARN);
        learn_window_start = millis();
        learn_session_start = millis();
        next_tx_ms = 0;
        stay_awake_until = millis() + LEARN_TOTAL_TIMEOUT_MS;
    }
    else if (rtc_mode == APP_MODE_SINGLE) {
        ui_set_pattern(UI_PAT_SINGLE);
        single_start = millis();
        next_tx_ms = 0;
        stay_awake_until = millis() +
                           (SINGLE_TIMEOUT_MS ? SINGLE_TIMEOUT_MS : 3600000u);
    }
    else {
        ui_set_pattern(low_batt ? UI_PAT_LOWBAT : UI_PAT_NORMAL);
    }

    /* Po wybudzeniu przyciskiem/LF obsluzylismy juz akcje - nie powtarzaj
     * cyklu natychmiast, tylko odloz go o pelny okres. */
    if (cause == ESP_SLEEP_WAKEUP_EXT0 || cause == ESP_SLEEP_WAKEUP_EXT1) {
        if (rtc_mode != APP_MODE_NORMAL) {
            next_tx_ms = millis() + LEARN_TX_PERIOD_MS;
        }
    }
#if PIN_LF_TRIGGER >= 0
    /* LF ciagle aktywne = brak nowego zbocza = brak kolejnego bursta */
    lf_prev = (digitalRead(PIN_LF_TRIGGER) == HIGH);
#endif

    Serial.printf("Vcc=%u mV  tryb=%d  interwal=%us\n", (unsigned)supply_mv,
                  (int)rtc_mode, (unsigned)iv);
    app_print_info();
    Serial.println(F("gotowy ('help' - komendy)"));
}

void loop(void)
{
    uint32_t now = millis();

    ui_update();

    /* gesty przycisku */
    ui_event_t ev = ui_poll();
    if (ev == UI_EV_SHORT) {
        if (rtc_mode == APP_MODE_NORMAL) {
            refresh_power();
            app_send_burst_all();     // "aktywuj kolo" - nadaj teraz
        }
        else {
            app_next_wheel();         // learn/single: kolejne kolo
        }
    }
    else if (ev == UI_EV_LONG) {
        if (rtc_mode == APP_MODE_LEARN) {
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        else {
            app_set_mode(APP_MODE_LEARN, 0);
        }
    }
    else if (ev == UI_EV_VERYLONG) {
        if (rtc_mode == APP_MODE_SINGLE) {
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        else {
            app_set_mode(APP_MODE_SINGLE, active_wheel());
        }
    }

    console_poll();

#if PIN_LF_TRIGGER >= 0
    /* aktywacja LF tez dziala "na zywo" (nie tylko przez budzenie) */
    {
        bool lf = (digitalRead(PIN_LF_TRIGGER) == HIGH);
        if (lf && !lf_prev) {
            refresh_power();
            app_send_burst_wheel(active_wheel());
            /* po recznej transmisji odloz cykl - bez tego LF moglby
             * wywolac 2-3 bursty zamiast jednego */
            if (rtc_mode != APP_MODE_NORMAL) {
                next_tx_ms = millis() + LEARN_TX_PERIOD_MS;
            }
        }
        lf_prev = lf;
    }
#endif

    if (request_sleep) {
        request_sleep = false;
        go_sleep(compute_interval());
    }

    switch (rtc_mode) {
    case APP_MODE_LEARN:
        if ((int32_t)(now - next_tx_ms) >= 0) {
            refresh_power();
            app_send_burst_wheel(active_wheel());
            next_tx_ms = now + LEARN_TX_PERIOD_MS;
        }
        if (now - learn_window_start >= LEARN_WINDOW_MS) {
            if (LEARN_WINDOW_EXPIRE_TO_NORMAL) {
                Serial.println(F("okno learn minelo - powrot do NORMAL"));
                app_set_mode(APP_MODE_NORMAL, 0);
            }
            else if (rtc_seq_idx + 1u >= WHEEL_COUNT) {
                Serial.println(F("learn ukonczony - powrot do NORMAL"));
                app_set_mode(APP_MODE_NORMAL, 0);
            }
            else {
                app_next_wheel();
            }
        }
        if (now - learn_session_start >= LEARN_TOTAL_TIMEOUT_MS) {
            Serial.println(F("timeout learn - powrot do NORMAL"));
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        break;

    case APP_MODE_SINGLE:
        if ((int32_t)(now - next_tx_ms) >= 0) {
            refresh_power();
            app_send_burst_wheel(active_wheel());
            next_tx_ms = now + LEARN_TX_PERIOD_MS;
        }
        if (SINGLE_TIMEOUT_MS && now - single_start >= SINGLE_TIMEOUT_MS) {
            Serial.println(F("timeout single - powrot do NORMAL"));
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        break;

    default: /* NORMAL */
        if (next_tx_ms != UINT32_MAX && (int32_t)(now - next_tx_ms) >= 0) {
            uint32_t iv = compute_interval();
            tx_cycle();
            next_tx_ms = now + (iv ? iv : 3600u) * 1000u;
        }
        if ((int32_t)(now - stay_awake_until) >= 0) {
            go_sleep(compute_interval());
        }
        break;
    }
}
