/* Symulator diagnostyczny TPMS. Sztuczne cisnienie moze ukryc usterke opony. */
#include <Arduino.h>
#include <esp_sleep.h>

#include "app.h"
#include "config.h"
#include "console.h"
#include "power.h"
#include "radio.h"
#include "settings_store.h"
#include "tpms_frame.h"
#include "ui.h"

AppState *g_app = nullptr;

static const char *const WHEEL_NAMES[WHEEL_COUNT] = {"FL", "FR", "RR", "RL"};
static uint32_t stay_awake_until;
static bool request_sleep;
static uint32_t supply_mv;
#if PIN_LF_TRIGGER >= 0
static bool lf_previous;
#endif

static bool ignition_on(void)
{
#if PIN_ACC >= 0
    return digitalRead(PIN_ACC) == HIGH;
#else
    return false;
#endif
}

class HardwareRadio final : public RadioSink {
public:
    bool send(uint8_t wheel) override
    {
        if (wheel >= WHEEL_COUNT) {
            return false;
        }
        const sensor_settings_t &sensor = g_settings.wheels[wheel];
        uint8_t frame[TPMS_TX_BYTES];
        if (!tpms_build_frame(frame, sensor.id, sensor.pressure_kpa10,
                              (int8_t)sensor.temp_c, g_settings.flags)) {
            return false;
        }
        bool ok = radio_send_burst(frame, TPMS_TX_BYTES, g_settings.burst_frames,
                                   g_settings.frame_gap_ms);
        Serial.printf("TX %s id=%06X %u.%u kPa %d C flags=%08X%s\n",
                      WHEEL_NAMES[wheel], (unsigned)sensor.id,
                      sensor.pressure_kpa10 / 10u, sensor.pressure_kpa10 % 10u,
                      (int)sensor.temp_c, (unsigned)g_settings.flags,
                      ok ? "" : " TX-ERR");
        return ok;
    }
};

static HardwareRadio hardware_radio;
static AppState app_state(&g_settings, &hardware_radio);

void app_stay_awake(uint32_t ms)
{
    uint32_t until = millis() + ms;
    if ((int32_t)(until - stay_awake_until) > 0) {
        stay_awake_until = until;
    }
}

void app_request_sleep(void)
{
    request_sleep = true;
}

bool app_save_and_apply(void)
{
    if (!settings_store_save()) {
        Serial.println(F("BLAD: nie zapisano NVS (walidacja lub zapis)"));
        return false;
    }
    if (!radio_init(&g_settings)) {
        Serial.println(F("OSTRZEZENIE: zapisano, ale CC1101 nie odpowiada"));
    }
    Serial.println(F("OK: zapisano w NVS"));
    return true;
}

void app_print_frame(uint8_t wheel)
{
    if (wheel >= WHEEL_COUNT) {
        return;
    }
    const sensor_settings_t &s = g_settings.wheels[wheel];
    uint8_t frame[TPMS_TX_BYTES];
    uint8_t payload[TPMS_PAYLOAD_LEN];
    if (!tpms_build_frame(frame, s.id, s.pressure_kpa10, (int8_t)s.temp_c,
                          g_settings.flags)
            || !tpms_build_payload(payload, s.id, s.pressure_kpa10,
                                   (int8_t)s.temp_c, g_settings.flags)) {
        Serial.println(F("BLAD: nieprawidlowa konfiguracja ramki"));
        return;
    }
    Serial.printf("%s payload:", WHEEL_NAMES[wheel]);
    for (uint8_t i = 0; i < TPMS_PAYLOAD_LEN; ++i) {
        Serial.printf(" %02X", payload[i]);
    }
    Serial.printf("\n%s frame:", WHEEL_NAMES[wheel]);
    for (uint8_t i = 0; i < TPMS_TX_BYTES; ++i) {
        Serial.printf(" %02X", frame[i]);
    }
    Serial.println();
}

void app_print_info(void)
{
    Serial.printf("FW %s NVS v%u mode=%u wheel=%s errors=%lu\n", FW_VERSION,
                  (unsigned)SETTINGS_VERSION, (unsigned)g_app->mode(),
                  WHEEL_NAMES[g_app->active_wheel()],
                  (unsigned long)g_app->last_error_count());
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        const sensor_settings_t &s = g_settings.wheels[i];
        Serial.printf("  %s id=%06X pressure=%u.%u kPa temp=%d C\n",
                      WHEEL_NAMES[i], (unsigned)s.id, s.pressure_kpa10 / 10u,
                      s.pressure_kpa10 % 10u, (int)s.temp_c);
    }
    Serial.printf("flags=%08X OOK chip=%u us burst=%u gap=%u ms\n",
                  (unsigned)g_settings.flags, TPMS_CHIP_US, g_settings.burst_frames,
                  g_settings.frame_gap_ms);
    Serial.printf("RF=%.2f MHz power=%d dBm intervals=%lu/%lu/%lu s\n",
                  g_settings.frequency_mhz, g_settings.tx_power_dbm,
                  (unsigned long)g_settings.normal_interval_s,
                  (unsigned long)g_settings.ignition_interval_s,
                  (unsigned long)g_settings.parking_interval_s);
    Serial.printf("Vcc=%u mV (nie zmienia flag TPMS)\n", (unsigned)supply_mv);
}

static void sync_ui(void)
{
    static AppMode previous_mode = AppMode::NORMAL;
    static uint8_t previous_wheel = UINT8_MAX;
    AppMode mode = g_app->mode();
    uint8_t wheel = g_app->active_wheel();
    if (mode != previous_mode || wheel != previous_wheel) {
        ui_set_pattern(mode == AppMode::LEARN ? UI_PAT_LEARN
                : mode == AppMode::SINGLE ? UI_PAT_SINGLE : UI_PAT_NORMAL);
        if (mode != AppMode::NORMAL) {
            ui_blink_count((uint8_t)(wheel + 1u));
        }
        previous_mode = mode;
        previous_wheel = wheel;
    }
}

static bool wake_sources_active(void)
{
    if (ui_button_down() || digitalRead(PIN_BUTTON) == BTN_ACTIVE_LEVEL) {
        return true;
    }
#if PIN_LF_TRIGGER >= 0
    if (digitalRead(PIN_LF_TRIGGER) == LF_ACTIVE_LEVEL) {
        return true;
    }
#endif
    return false;
}

static void go_sleep(void)
{
    if (g_app->mode() != AppMode::NORMAL) {
        return;
    }
    if (wake_sources_active()) {
        app_stay_awake(500u);
        return;
    }
    uint32_t seconds = g_app->interval_s(ignition_on());
    ui_set_pattern(UI_PAT_OFF);
    Serial.printf("sleep %lu s\n", (unsigned long)seconds);
    Serial.flush();
    power_deep_sleep(seconds);
}

void setup(void)
{
    Serial.begin(SERIAL_BAUD);
    ui_init();
    power_init();
    settings_store_begin();
    console_init();
#if PIN_LF_TRIGGER >= 0
    pinMode(PIN_LF_TRIGGER, INPUT_PULLUP);
    lf_previous = digitalRead(PIN_LF_TRIGGER) == LF_ACTIVE_LEVEL;
#endif
#if PIN_ACC >= 0
    pinMode(PIN_ACC, INPUT);
#endif

    g_app = &app_state;
    bool radio_ready = radio_init(&g_settings);
    supply_mv = power_read_mv();
    Serial.printf("\nTPMS DIAG FW %s - DANE SZTUCZNE, NIE DO JAZDY\n", FW_VERSION);
    Serial.printf("NVS: %s; dostep=%s\n",
                  settings_load_result_text(settings_store_last_load()),
                  settings_store_available() ? "tak" : "nie");
    if (!radio_ready) {
        Serial.println(F("BLAD: brak CC1101; sprawdz 3V3/SPI"));
    }
    if (power_low_battery(supply_mv)) {
        Serial.println(F("OSTRZEZENIE: niskie zasilanie emulatora"));
    }

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool transmit = cause != ESP_SLEEP_WAKEUP_GPIO;
    g_app->begin(millis(), transmit);
#if PIN_LF_TRIGGER >= 0
    uint64_t wake_mask = cause == ESP_SLEEP_WAKEUP_GPIO
            ? esp_sleep_get_gpio_wakeup_status() : 0;
    if ((wake_mask & (1ULL << PIN_LF_TRIGGER)) != 0) {
        g_app->send_current();
    }
#endif
    stay_awake_until = cause == ESP_SLEEP_WAKEUP_TIMER
            ? millis() : millis() + BOOT_STAY_AWAKE_MS;
    sync_ui();
    app_print_info();
    Serial.println(F("gotowy; help = komendy; stay 60 = konsola 60 s"));
}

void loop(void)
{
    uint32_t now = millis();
    ui_update();
    switch (ui_poll()) {
    case UI_EV_SHORT: g_app->short_press(now); break;
    case UI_EV_LONG: g_app->long_press(now); break;
    case UI_EV_VERYLONG: g_app->very_long_press(now); break;
    default: break;
    }
    console_poll();
#if PIN_LF_TRIGGER >= 0
    bool lf = digitalRead(PIN_LF_TRIGGER) == LF_ACTIVE_LEVEL;
    if (lf && !lf_previous) {
        g_app->send_current();
        app_stay_awake(BOOT_STAY_AWAKE_MS);
    }
    lf_previous = lf;
#endif
    g_app->tick(millis(), ignition_on());
    sync_ui();

    if (request_sleep) {
        request_sleep = false;
        go_sleep();
    } else if (g_app->mode() == AppMode::NORMAL
            && (int32_t)(millis() - stay_awake_until) >= 0) {
        go_sleep();
    }
}
