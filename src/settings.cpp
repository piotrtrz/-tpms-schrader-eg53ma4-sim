#include "settings.h"

#include <string.h>

void settings_defaults(settings_t *out)
{
    static const uint32_t ids[WHEEL_COUNT] = {
        0x2FC871u, 0x2EAAF9u, 0x3495ACu, 0x2FC780u,
    };
    memset(out, 0, sizeof(*out));
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        out->wheels[i].id = ids[i];
        out->wheels[i].pressure_kpa10 = 2600u;
        out->wheels[i].temp_c = 23;
    }
    out->flags = 0x4C900070u;
    out->normal_interval_s = 30u;
    out->ignition_interval_s = 30u;
    out->parking_interval_s = 120u;
    out->frequency_mhz = 433.92f;
    out->tx_power_dbm = -10;
    out->burst_frames = 3u;
    out->frame_gap_ms = 50u;
}

bool settings_valid_id(uint32_t id)
{
    return id > 0u && id < 0xFFFFFFu;
}

bool settings_valid_tx_power(int power_dbm)
{
    static const int8_t allowed[] = {-30, -20, -15, -10, -6, 0, 5};
    for (uint8_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); ++i) {
        if (power_dbm == allowed[i]) {
            return true;
        }
    }
    return false;
}

static bool valid_interval(uint32_t value)
{
    return value >= SETTINGS_INTERVAL_MIN_S && value <= SETTINGS_INTERVAL_MAX_S;
}

bool settings_validate(const settings_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        if (!settings_valid_id(cfg->wheels[i].id)
                || cfg->wheels[i].pressure_kpa10 > SETTINGS_PRESSURE_MAX_KPA10
                || cfg->wheels[i].temp_c < -40 || cfg->wheels[i].temp_c > 85) {
            return false;
        }
        for (uint8_t j = (uint8_t)(i + 1u); j < WHEEL_COUNT; ++j) {
            if (cfg->wheels[i].id == cfg->wheels[j].id) {
                return false;
            }
        }
    }
    return valid_interval(cfg->normal_interval_s)
            && valid_interval(cfg->ignition_interval_s)
            && valid_interval(cfg->parking_interval_s)
            && cfg->frequency_mhz >= 433.05f && cfg->frequency_mhz <= 434.79f
            && settings_valid_tx_power(cfg->tx_power_dbm)
            && cfg->burst_frames >= 1u && cfg->burst_frames <= 8u
            && cfg->frame_gap_ms >= 20u && cfg->frame_gap_ms <= 500u;
}

bool settings_set_id(settings_t *cfg, uint8_t wheel, uint32_t id)
{
    if (cfg == NULL || wheel >= WHEEL_COUNT || !settings_valid_id(id)) {
        return false;
    }
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        if (i != wheel && cfg->wheels[i].id == id) {
            return false;
        }
    }
    cfg->wheels[wheel].id = id;
    return true;
}

void settings_generate_ids(settings_t *cfg, uint32_t seed)
{
    if (seed == 0u) {
        seed = 0x6D2B79F5u;
    }
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        uint32_t candidate;
        bool duplicate;
        do {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            candidate = (seed & 0xFFFFFFu);
            if (!settings_valid_id(candidate)) {
                candidate = (candidate % 0xFFFFFEu) + 1u;
            }
            duplicate = false;
            for (uint8_t j = 0; j < i; ++j) {
                duplicate |= cfg->wheels[j].id == candidate;
            }
        } while (duplicate);
        cfg->wheels[i].id = candidate;
    }
}

uint32_t settings_crc32(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
        }
    }
    return ~crc;
}

void settings_make_blob(const settings_t *cfg, settings_blob_t *blob)
{
    memset(blob, 0, sizeof(*blob));
    blob->magic = SETTINGS_MAGIC;
    blob->version = SETTINGS_VERSION;
    blob->size = sizeof(blob->values);
    blob->values = *cfg;
    blob->crc32 = settings_crc32(blob, offsetof(settings_blob_t, crc32));
}

settings_load_result_t settings_decode_blob(const settings_blob_t *blob,
                                             settings_t *out)
{
    if (blob == NULL || out == NULL) {
        return SETTINGS_LOAD_IO_ERROR;
    }
    if (blob->magic != SETTINGS_MAGIC) {
        return SETTINGS_LOAD_BAD_MAGIC;
    }
    if (blob->version != SETTINGS_VERSION) {
        return SETTINGS_LOAD_BAD_VERSION;
    }
    if (blob->size != sizeof(blob->values)) {
        return SETTINGS_LOAD_BAD_SIZE;
    }
    uint32_t expected = settings_crc32(blob, offsetof(settings_blob_t, crc32));
    if (expected != blob->crc32) {
        return SETTINGS_LOAD_BAD_CRC;
    }
    if (!settings_validate(&blob->values)) {
        return SETTINGS_LOAD_INVALID;
    }
    *out = blob->values;
    return SETTINGS_LOAD_OK;
}

const char *settings_load_result_text(settings_load_result_t result)
{
    switch (result) {
    case SETTINGS_LOAD_OK: return "OK";
    case SETTINGS_LOAD_EMPTY: return "pusta konfiguracja";
    case SETTINGS_LOAD_BAD_MAGIC: return "nieprawidlowy magic";
    case SETTINGS_LOAD_BAD_VERSION: return "nieobslugiwana wersja";
    case SETTINGS_LOAD_BAD_SIZE: return "nieprawidlowy rozmiar";
    case SETTINGS_LOAD_BAD_CRC: return "blad CRC";
    case SETTINGS_LOAD_INVALID: return "nieprawidlowe wartosci";
    case SETTINGS_LOAD_IO_ERROR: return "blad NVS";
    default: return "nieznany blad";
    }
}
