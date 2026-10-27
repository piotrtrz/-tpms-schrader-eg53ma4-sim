/*
 * console.cpp - debugowa konsola szeregowa (bez String i bez malloc)
 *
 * Komendy:
 *   help                  - lista komend
 *   info                  - wersja, konfiguracja, stan, napiecie
 *   hex [n|all]           - hexdump ramki TPMS dla kola n (1-4) lub wszystkich
 *   send [n|all]          - wyslij burst ramki teraz
 *   batt                  - odczyt stanu baterii (napiecie zasilania)
 *   mode normal|learn|single [n] - zmiana trybu
 *   next                  - kolejne kolo (learn/single)
 *   interval <s>          - reczy ustalony interwal (0 = auto wg ACC)
 *   flags <hex>           - slowo flags wyslane w ramce
 *   sleep                 - deep sleep teraz
 *   stay [s]              - zostan w stanie czuwania jeszcze s sekund
 *
 * Parser jest scisly: zly argument = komunikat bledu, bez dzialania
 * ubocznego (np. "send bla" NIE nadaje wszystkich kol).
 */
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "app.h"
#include "console.h"

static char line[64];
static uint8_t line_len = 0;
static bool line_overflow = false;

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
    Serial.println(F("  stay [s]                 - czuwaj jeszcze s sekund"));
}

/* Scisly parser liczby calkowitej bez znaku z zakresem. */
static bool parse_u32(const char *s, uint32_t lo, uint32_t hi, uint32_t *out,
                      int base)
{
    if (s == NULL || *s == '\0') {
        return false;
    }
    errno = 0;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, base);
    if (errno != 0 || end == s || *end != '\0') {
        return false;
    }
    if (v < (unsigned long)lo || v > (unsigned long)hi) {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

/* Numer kola 1-4 -> indeks 0-3; "all" -> 0xFF; blad -> -1. */
static int parse_wheel(const char *arg, uint8_t *wheel)
{
    if (arg == NULL) {
        return -1;
    }
    if (strcmp(arg, "all") == 0) {
        *wheel = 0xFF;
        return 0;
    }
    uint32_t n = 0;
    if (!parse_u32(arg, 1, WHEEL_COUNT, &n, 10)) {
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
    uint32_t u = 0;

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    }
    else if (strcmp(cmd, "info") == 0 || strcmp(cmd, "config") == 0) {
        app_print_info();
    }
    else if (strcmp(cmd, "hex") == 0) {
        uint8_t w;
        if (arg1 == NULL || strcmp(arg1, "all") == 0) {
            for (uint8_t i = 0; i < WHEEL_COUNT; i++) {
                app_print_frame(i);
            }
        }
        else if (parse_wheel(arg1, &w) == 0) {
            app_print_frame(w);
        }
        else {
            Serial.println(F("BLAD: hex [1-4|all]"));
        }
    }
    else if (strcmp(cmd, "send") == 0) {
        uint8_t w;
        if (arg1 == NULL || strcmp(arg1, "all") == 0) {
            app_send_burst_all();
            Serial.println(F("OK"));
        }
        else if (parse_wheel(arg1, &w) == 0) {
            app_send_burst_wheel(w);
            Serial.println(F("OK"));
        }
        else {
            Serial.println(F("BLAD: send [1-4|all]"));
        }
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
            Serial.println(F("BLAD: mode normal|learn|single [n]"));
            return;
        }
        if (strcmp(arg1, "normal") == 0) {
            app_set_mode(APP_MODE_NORMAL, 0);
        }
        else if (strcmp(arg1, "learn") == 0) {
            if (arg2 != NULL && parse_wheel(arg2, &w) != 0) {
                Serial.println(F("BLAD: numer kola 1-4"));
                return;
            }
            app_set_mode(APP_MODE_LEARN, w);
        }
        else if (strcmp(arg1, "single") == 0) {
            if (arg2 == NULL || parse_wheel(arg2, &w) != 0 || w == 0xFF) {
                Serial.println(F("BLAD: single wymaga numeru kola 1-4"));
                return;
            }
            app_set_mode(APP_MODE_SINGLE, w);
        }
        else {
            Serial.println(F("BLAD: mode normal|learn|single [n]"));
            return;
        }
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "interval") == 0) {
        if (!parse_u32(arg1, 0, 86400, &u, 10)) {
            Serial.println(F("BLAD: interval <0-86400>"));
            return;
        }
        app_set_interval(u);
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "flags") == 0) {
        if (!parse_u32(arg1, 0, 0xFFFFFFFFu, &u, 16)) {
            Serial.println(F("BLAD: flags <hex>"));
            return;
        }
        app_set_flags(u);
        Serial.println(F("OK"));
    }
    else if (strcmp(cmd, "sleep") == 0) {
        app_request_sleep();
    }
    else if (strcmp(cmd, "stay") == 0) {
        if (arg1 == NULL) {
            app_stay_awake(60000u);
        }
        else if (parse_u32(arg1, 0, 3600, &u, 10)) {
            app_stay_awake(u * 1000u);
        }
        else {
            Serial.println(F("BLAD: stay <0-3600>"));
            return;
        }
        Serial.println(F("OK"));
    }
    else {
        Serial.println(F("Nieznana komenda - 'help'"));
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
                Serial.println(F("BLAD: linia za dluga"));
            }
            else if (line_len > 0) {
                line[line_len] = '\0';
                handle_line(line);
            }
            line_len = 0;
            line_overflow = false;
        }
        else if (line_overflow) {
            /* po przeplenieniu porzucamy cala linie lacznie z ogonem */
        }
        else if (line_len < sizeof(line) - 1u) {
            line[line_len++] = c;
        }
        else {
            line_overflow = true;
            line_len = 0;
        }
    }
}
