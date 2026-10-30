/* Console argument parsing; portable module (native tests). */
#ifndef TPMS_PARSE_H
#define TPMS_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool parse_u32(const char *text, uint32_t lo, uint32_t hi, uint32_t *out,
               int base = 10);
bool parse_i32(const char *text, int32_t lo, int32_t hi, int32_t *out);
/* Rejects NaN and infinity; closed range [lo, hi]. */
bool parse_float(const char *text, float lo, float hi, float *out);

#ifdef __cplusplus
}
#endif

#endif
