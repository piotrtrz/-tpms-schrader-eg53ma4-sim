#include <errno.h>
#include <stdlib.h>

#include "parse.h"

bool parse_u32(const char *text, uint32_t lo, uint32_t hi, uint32_t *out,
               int base)
{
    if (text == NULL || *text == '\0' || *text == '-') {
        return false;
    }
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, base);
    if (errno != 0 || end == text || *end != '\0'
            || value < lo || value > hi) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

bool parse_i32(const char *text, int32_t lo, int32_t hi, int32_t *out)
{
    if (text == NULL || *text == '\0') {
        return false;
    }
    errno = 0;
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < lo || value > hi) {
        return false;
    }
    *out = value;
    return true;
}

bool parse_float(const char *text, float lo, float hi, float *out)
{
    if (text == NULL || *text == '\0') {
        return false;
    }
    errno = 0;
    char *end = NULL;
    float value = strtof(text, &end);
    /* Inverted comparison also rejects NaN (both sides false). */
    if (errno != 0 || end == text || *end != '\0'
            || !(value >= lo && value <= hi)) {
        return false;
    }
    *out = value;
    return true;
}
