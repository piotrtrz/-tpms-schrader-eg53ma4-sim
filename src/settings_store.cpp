#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

#include "settings_store.h"

static Preferences preferences;
static settings_load_result_t last_load = SETTINGS_LOAD_EMPTY;
static bool store_available = false;
settings_t g_settings;

void settings_store_begin()
{
    settings_defaults(&g_settings);
    store_available = preferences.begin("tpms", false);
    if (!store_available) {
        last_load = SETTINGS_LOAD_IO_ERROR;
        return;
    }

    settings_blob_t blob;
    size_t length = preferences.getBytesLength("config");
    if (length == 0u) {
        last_load = SETTINGS_LOAD_EMPTY;
        settings_store_save();
        return;
    }
    if (length != sizeof(blob)
            || preferences.getBytes("config", &blob, sizeof(blob)) != sizeof(blob)) {
        last_load = SETTINGS_LOAD_BAD_SIZE;
    } else {
        last_load = settings_decode_blob(&blob, &g_settings);
    }
    if (last_load != SETTINGS_LOAD_OK) {
        settings_defaults(&g_settings);
        /* Odzyskiwanie po uszkodzeniu: zapisz spojne domyslne. */
        settings_store_save();
    }
}

bool settings_store_save()
{
    if (!store_available || !settings_validate(&g_settings)) {
        return false;
    }
    settings_blob_t blob;
    settings_make_blob(&g_settings, &blob);
    if (preferences.putBytes("config", &blob, sizeof(blob)) != sizeof(blob)) {
        return false;
    }
    settings_blob_t verify;
    if (preferences.getBytes("config", &verify, sizeof(verify)) != sizeof(verify)
            || memcmp(&blob, &verify, sizeof(blob)) != 0) {
        return false;
    }
    return true;
}

bool settings_store_factory_reset()
{
    settings_defaults(&g_settings);
    return settings_store_save();
}

settings_load_result_t settings_store_last_load()
{
    return last_load;
}

bool settings_store_available()
{
    return store_available;
}
