#ifndef TPMS_SETTINGS_STORE_H
#define TPMS_SETTINGS_STORE_H

#include "settings.h"

extern settings_t g_settings;

void settings_store_begin();
bool settings_store_save();
bool settings_store_factory_reset();
settings_load_result_t settings_store_last_load();
bool settings_store_available();

#endif
