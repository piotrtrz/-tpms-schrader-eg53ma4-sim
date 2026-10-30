/* Persistent user configuration; module independent of Arduino. */
#ifndef TPMS_SETTINGS_H
#define TPMS_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_MAGIC 0x54504D53u
#define SETTINGS_VERSION 1u
#define WHEEL_COUNT 4
#define SETTINGS_PRESSURE_MAX_KPA10 6375u
#define SETTINGS_INTERVAL_MIN_S 5u
#define SETTINGS_INTERVAL_MAX_S 86400u

typedef enum {
    WHEEL_FL = 0,
    WHEEL_FR = 1,
    WHEEL_RR = 2,
    WHEEL_RL = 3,
} wheel_t;

typedef struct {
    uint32_t id;
    uint16_t pressure_kpa10;
    int16_t temp_c;
} sensor_settings_t;

typedef struct {
    sensor_settings_t wheels[WHEEL_COUNT];
    uint32_t flags;
    uint32_t normal_interval_s;
    uint32_t ignition_interval_s;
    uint32_t parking_interval_s;
    float frequency_mhz;
    int8_t tx_power_dbm;
    uint8_t burst_frames;
    uint16_t frame_gap_ms;
} settings_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    settings_t values;
    uint32_t crc32;
} settings_blob_t;

typedef enum {
    SETTINGS_LOAD_OK = 0,
    SETTINGS_LOAD_EMPTY,
    SETTINGS_LOAD_BAD_MAGIC,
    SETTINGS_LOAD_BAD_VERSION,
    SETTINGS_LOAD_BAD_SIZE,
    SETTINGS_LOAD_BAD_CRC,
    SETTINGS_LOAD_INVALID,
    SETTINGS_LOAD_IO_ERROR,
} settings_load_result_t;

void settings_defaults(settings_t *out);
bool settings_valid_id(uint32_t id);
bool settings_validate(const settings_t *cfg);
bool settings_set_id(settings_t *cfg, uint8_t wheel, uint32_t id);
void settings_generate_ids(settings_t *cfg, uint32_t seed);
bool settings_valid_tx_power(int power_dbm);

uint32_t settings_crc32(const void *data, size_t len);
void settings_make_blob(const settings_t *cfg, settings_blob_t *blob);
settings_load_result_t settings_decode_blob(const settings_blob_t *blob,
                                             settings_t *out);
const char *settings_load_result_text(settings_load_result_t result);

#ifdef __cplusplus
}
#endif

#endif
