/* Serial console: configuration stored in NVS, no recompilation needed. */
#include <Arduino.h>
#include <esp_system.h>
#include <string.h>

#include "app.h"
#include "console.h"
#include "parse.h"
#include "power.h"
#include "settings_store.h"

static char line[96];
static uint8_t line_len;
static bool line_overflow;

static bool parse_wheel(const char *text, uint8_t *wheel, bool allow_all = false)
{
    if (allow_all && text != NULL && strcmp(text, "all") == 0) {
        *wheel = 0xFFu;
        return true;
    }
    uint32_t value;
    if (!parse_u32(text, 1u, WHEEL_COUNT, &value)) {
        return false;
    }
    *wheel = (uint8_t)(value - 1u);
    return true;
}

static void cmd_help(void)
{
    Serial.println(F("HELP (configuration changes are saved automatically):"));
    Serial.println(F(" info | hex [1-4|all] | send [1-4|all] | batt"));
    Serial.println(F(" mode normal | mode learn [1-4] | mode single 1-4 | next"));
    Serial.println(F(" id 1-4 XXXXXX       - set 24-bit ID (hex)"));
    Serial.println(F(" ids new              - generate 4 unique IDs"));
    Serial.println(F(" pressure 1-4|all KPA - e.g. pressure all 260"));
    Serial.println(F(" temp 1-4|all C       - temperature -40..85 C"));
    Serial.println(F(" interval normal|ignition|parking 5-86400"));
    Serial.println(F(" rf freq 433.05-434.79 | rf power -30..5 | rf burst 1-8"));
    Serial.println(F(" rf gap 20-500        - gap between frames [ms]"));
    Serial.println(F(" flags HEX            - 32-bit flags (meaning not fully known)"));
    Serial.println(F(" factory YES          - restore factory | save | stay [s] | sleep"));
}

static void cmd_send(const char *arg)
{
    uint8_t wheel;
    bool ok;
    if (arg == NULL) {
        ok = g_app->mode() == AppMode::NORMAL ? g_app->send_all() : g_app->send_current();
    } else if (parse_wheel(arg, &wheel, true)) {
        ok = wheel == 0xFFu ? g_app->send_all() : g_app->send_wheel(wheel);
    } else {
        Serial.println(F("ERROR: send [1-4|all]"));
        return;
    }
    Serial.println(ok ? F("OK") : F("TX ERROR"));
}

static void cmd_mode(const char *arg1, const char *arg2)
{
    if (arg1 == NULL) {
        Serial.println(F("ERROR: mode normal|learn|single [1-4]"));
        return;
    }
    uint8_t wheel = WHEEL_FL;
    if (arg2 != NULL && !parse_wheel(arg2, &wheel)) {
        Serial.println(F("ERROR: wheel 1-4"));
        return;
    }
    if (strcmp(arg1, "normal") == 0) {
        g_app->set_mode(AppMode::NORMAL, WHEEL_FL, millis(), false);
    } else if (strcmp(arg1, "learn") == 0) {
        g_app->set_mode(AppMode::LEARN, wheel, millis());
    } else if (strcmp(arg1, "single") == 0 && arg2 != NULL) {
        g_app->set_mode(AppMode::SINGLE, wheel, millis());
    } else {
        Serial.println(F("ERROR: mode normal|learn [1-4]|single 1-4"));
        return;
    }
    Serial.println(F("OK"));
}

static void cmd_id(const char *wheel_text, const char *id_text)
{
    uint8_t wheel;
    uint32_t id;
    if (!parse_wheel(wheel_text, &wheel) || !parse_u32(id_text, 1u, 0xFFFFFEu, &id, 16)
            || !settings_set_id(&g_settings, wheel, id)) {
        Serial.println(F("ERROR: id 1-4 000001..FFFFFE; IDs must be unique"));
        return;
    }
    app_save_and_apply();
}

static void cmd_pressure_temp(const char *which, const char *wheel_text,
                              const char *value_text)
{
    uint8_t wheel;
    if (!parse_wheel(wheel_text, &wheel, true)) {
        Serial.println(F("ERROR: wheel 1-4 or all"));
        return;
    }
    uint8_t first = wheel == 0xFFu ? 0u : wheel;
    uint8_t last = wheel == 0xFFu ? WHEEL_COUNT : (uint8_t)(wheel + 1u);
    if (strcmp(which, "pressure") == 0) {
        float kpa;
        if (!parse_float(value_text, 0.0f, 637.5f, &kpa)) {
            Serial.println(F("ERROR: pressure 0..637.5 kPa"));
            return;
        }
        uint16_t value = (uint16_t)(kpa * 10.0f + 0.5f);
        for (uint8_t i = first; i < last; ++i) {
            g_settings.wheels[i].pressure_kpa10 = value;
        }
    } else {
        int32_t value;
        if (!parse_i32(value_text, -40, 85, &value)) {
            Serial.println(F("ERROR: temperature -40..85 C"));
            return;
        }
        for (uint8_t i = first; i < last; ++i) {
            g_settings.wheels[i].temp_c = (int16_t)value;
        }
    }
    app_save_and_apply();
}

static void cmd_interval(const char *which, const char *value_text)
{
    uint32_t value;
    if (which == NULL || !parse_u32(value_text, SETTINGS_INTERVAL_MIN_S,
                                    SETTINGS_INTERVAL_MAX_S, &value)) {
        Serial.println(F("ERROR: interval normal|ignition|parking 5-86400"));
        return;
    }
    if (strcmp(which, "normal") == 0) {
        g_settings.normal_interval_s = value;
    } else if (strcmp(which, "ignition") == 0) {
        g_settings.ignition_interval_s = value;
    } else if (strcmp(which, "parking") == 0) {
        g_settings.parking_interval_s = value;
    } else {
        Serial.println(F("ERROR: interval kind"));
        return;
    }
    app_save_and_apply();
}

static void cmd_rf(const char *which, const char *value_text)
{
    uint32_t value;
    int32_t signed_value;
    float frequency;
    if (which == NULL) {
        Serial.println(F("ERROR: rf freq|power|burst|gap"));
        return;
    }
    if (strcmp(which, "freq") == 0 && parse_float(value_text, 433.05f, 434.79f, &frequency)) {
        g_settings.frequency_mhz = frequency;
    } else if (strcmp(which, "power") == 0
            && parse_i32(value_text, -30, 5, &signed_value)
            && settings_valid_tx_power((int)signed_value)) {
        g_settings.tx_power_dbm = (int8_t)signed_value;
    } else if (strcmp(which, "burst") == 0 && parse_u32(value_text, 1u, 8u, &value)) {
        g_settings.burst_frames = (uint8_t)value;
    } else if (strcmp(which, "gap") == 0 && parse_u32(value_text, 20u, 500u, &value)) {
        g_settings.frame_gap_ms = (uint16_t)value;
    } else {
        Serial.println(F("ERROR: RF parameter out of the allowed range"));
        return;
    }
    app_save_and_apply();
}

static void handle_line(char *text)
{
    char *cmd = strtok(text, " \t");
    if (cmd == NULL) {
        return;
    }
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");
    uint32_t value;

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "config") == 0) {
        app_print_info();
    } else if (strcmp(cmd, "hex") == 0) {
        uint8_t wheel;
        if (arg1 == NULL) {
            app_print_frame(g_app->active_wheel());
        } else if (parse_wheel(arg1, &wheel, true)) {
            if (wheel == 0xFFu) {
                for (uint8_t i = 0; i < WHEEL_COUNT; ++i) app_print_frame(i);
            } else {
                app_print_frame(wheel);
            }
        } else {
            Serial.println(F("ERROR: hex [1-4|all]"));
        }
    } else if (strcmp(cmd, "send") == 0) {
        cmd_send(arg1);
    } else if (strcmp(cmd, "batt") == 0) {
        Serial.printf("Vcc=%u mV\n", (unsigned)power_read_mv());
    } else if (strcmp(cmd, "mode") == 0) {
        cmd_mode(arg1, arg2);
    } else if (strcmp(cmd, "next") == 0) {
        Serial.println(g_app->next_wheel(millis()) ? F("OK") : F("ERROR: LEARN/SINGLE only"));
    } else if (strcmp(cmd, "id") == 0) {
        cmd_id(arg1, arg2);
    } else if (strcmp(cmd, "ids") == 0 && arg1 != NULL && strcmp(arg1, "new") == 0) {
        settings_generate_ids(&g_settings, esp_random());
        app_save_and_apply();
    } else if (strcmp(cmd, "pressure") == 0 || strcmp(cmd, "temp") == 0) {
        cmd_pressure_temp(cmd, arg1, arg2);
    } else if (strcmp(cmd, "interval") == 0) {
        cmd_interval(arg1, arg2);
    } else if (strcmp(cmd, "rf") == 0) {
        cmd_rf(arg1, arg2);
    } else if (strcmp(cmd, "flags") == 0) {
        if (!parse_u32(arg1, 0u, 0xFFFFFFFFu, &value, 16)) {
            Serial.println(F("ERROR: flags HEX"));
            return;
        }
        g_settings.flags = value;
        app_save_and_apply();
    } else if (strcmp(cmd, "factory") == 0 && arg1 != NULL && strcmp(arg1, "YES") == 0) {
        settings_defaults(&g_settings);
        g_app->set_mode(AppMode::NORMAL, WHEEL_FL, millis(), false);
        app_save_and_apply();
    } else if (strcmp(cmd, "save") == 0) {
        app_save_and_apply();
    } else if (strcmp(cmd, "sleep") == 0) {
        app_request_sleep();
    } else if (strcmp(cmd, "stay") == 0) {
        value = 60u;
        if (arg1 != NULL && !parse_u32(arg1, 1u, 3600u, &value)) {
            Serial.println(F("ERROR: stay 1-3600"));
            return;
        }
        app_stay_awake(value * 1000u);
        Serial.println(F("OK"));
    } else {
        Serial.println(F("Unknown command; help"));
    }
}

void console_init(void)
{
    line_len = 0;
    line_overflow = false;
}

void console_poll(void)
{
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (line_overflow) {
                Serial.println(F("ERROR: line too long"));
            } else if (line_len > 0u) {
                line[line_len] = '\0';
                handle_line(line);
            }
            line_len = 0;
            line_overflow = false;
        } else if (!line_overflow) {
            if (line_len < sizeof(line) - 1u) {
                line[line_len++] = c;
            } else {
                line_overflow = true;
                line_len = 0;
            }
        }
    }
}
