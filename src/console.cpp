/*
 * console.cpp - debugowa konsola szeregowa (bez String i bez malloc)
 *
 * Komendy:
 *   help                  - lista komend
 *   info                  - wersja, konfiguracja, stan, napiecie
 *   hex [n|all]           - hexdump ramki TPMS dla kola n (1-4) lub wszystkich
 *   send [n|all]          - wyslij burst ramki teraz
 *   batt                  - pomiar napiecia zasilania
 *   mode normal|learn|single [n] - zmiana trybu
 *   next                  - kolejne kolo (learn/single)
 *   interval <s>          - reczy ustalony interwal (0 = auto wg ACC)
 *   flags <hex>           - slowo flags wyslane w ramce
 *   sleep                 - deep sleep teraz
 *   stay [s]              - zostan w stanie czuwania jeszcze s sekund
 */
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "app.h"
#include "console.h"

static char line[64];
static uint8_t line_len = 0;

static void cmd_help(void)
{
    Serial.println(F("Komendy:"));
    Serial.println(F("  help                     - ta pomoc"));
    Serial.println(F("  info                     - wersja/config/stan/napiecie"));
    Serial.println(F("  hex [n|all]              - hexdump ramki kola n (1-4)"));
    Serial.println(F("  send [n|all]             - nadaj burst teraz"));
    Serial.println(F("  batt                     - napiecie zasilania [mV]"));
    Serial.println(F("  mode normal|learn|single [n] - tryb pracy"));
    Serial.println(F("  next                     - kolejne kolo (learn/single)"));
    Serial.println(F("  interval <s>             - interwal nadawania (0=auto)"));
    Serial.println(F("  flags <hex>              - slowo flags w ramce"));
    Serial.println(F("  sleep                    - deep sleep teraz"));
    Serial.println(F("  stay [s]                - czuwaj jeszcze s sekund"));
}

static int parse_wheel(const char *arg, uint8_t *wheel)
{
    if (arg == NULL) {
        return -1;
    }
    if (strcmp(arg, "all") == 0) {
        *wheel = 0xFF;
        return 0;
    }
    long n = strtol(arg, NULL, 10);
    if (n < 1 || n > WHEEL_COUNT) {
        return -1;
    }
    *wheel = (uint8_t)(n - 1);
    return 0;
}

static void handle_line(char *s)
{
    char *cmd = strtok(s, " \t");
    if (cmd == NULL) {
        return;
    }
    char *arg1 = strtok(NULL, " \t");
    char *arg2 = strtok(NULL, " \t");

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    }
    else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "config") == 0) {
        app_print_info();
    }
    else if (strcmp(cmd, "hex") == 0) {
        uint8_t w;
        if (parse_wheel(arg1, &w) != 0) {
            w = 0xFF;
        }
        if (w == 0xFF) {
            for (uint8_t i = 0; i < WHEEL_COUNT; i++) {
                app_print_frame(i);
            }
        }
        else {
            app_print_frame(w);
        }
    }
    else if (strcmp(cmd, "send") == 0) {
        uint8_t w;
        if (parse_wheel(arg1, &w) != 0) {
            w = 0xFF;
        }
        if (w == 0xFF) {
            app_send_burst_all();
        }
        else {
            app_send_burst_wheel(w);
        }
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "batt") == 0) {
        Serial.printf("Vcc = %u mV\n", (unsigned)app_supply_mv());
    }
    else if (strcmp(cmd, "next") == 0) {
        app_next_wheel();
    }
    else if (strcmp(cmd, "mode") == 0) {
        uint8_t w = 0;
        if (arg1 == NULL) {
            Serial.println(F("mode normal|learn|single [n]"));
            return;
        }
        if (strcmp(arg1, "normal") == 0) {
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        else if (strcmp(arg1, "learn") == 0) {
            if (arg2 != NULL) {
                if (parse_wheel(arg2, &w) != 0) {
                    w = 0;
                }
            }
            app_set_mode(APP_MODE_LEARN, w);
        }
        else if (strcmp(arg1, "single") == 0) {
            if (arg2 == NULL || parse_wheel(arg2, &w) != 0 || w == 0xFF) {
                Serial.println(F("single wymaga numeru kola 1-4"));
                return;
            }
            app_set_mode(APP_MODE_SINGLE, w);
        }
        else {
            Serial.println(F("mode normal|learn|single [n]"));
            return;
        }
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "interval") == 0) {
        if (arg1 == NULL) {
            Serial.println(F("interval <s>"));
            return;
        }
        app_set_interval((uint32_t)strtoul(arg1, NULL, 10));
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "flags") == 0) {
        if (arg1 == NULL) {
            Serial.println(F("flags <hex>"));
            return;
        }
        app_set_flags((uint32_t)strtoul(arg1, NULL, 16));
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "sleep") == 0) {
        app_request_sleep();
    }
    else if (strcmp(cmd, "stay") == 0) {
        app_stay_awake(arg1 ? (uint32_t)strtoul(arg1, NULL, 10) * 1000u : 60000u);
        Serial.println(F("OK"));
    }
    else {
        Serial.println(F("Nieznana komenda - 'help'"));
    }
}

void console_init(void)
{
    line_len = 0;
}

void console_poll(void)
{
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (line_len > 0) {
                line[line_len] = '\0';
                handle_line(line);
                line_len = 0;
            }
        }
        else if (line_len < sizeof(line) - 1u) {
            line[line_len++] = c;
        }
        else {
            line_len = 0;   // przepełnienie - porzuc linie
        }
    }
}
