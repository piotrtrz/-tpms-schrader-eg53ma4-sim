/* Portable NORMAL/LEARN/SINGLE state machine (fake clock/radio in tests). */
#ifndef TPMS_APP_STATE_H
#define TPMS_APP_STATE_H

#include <stdint.h>
#include "settings.h"

enum class AppMode : uint8_t {
    NORMAL = 0,
    LEARN = 1,
    SINGLE = 2,
};

class RadioSink {
public:
    virtual ~RadioSink() = default;
    virtual bool send(uint8_t wheel) = 0;
};

class AppState {
public:
    AppState(const settings_t *settings, RadioSink *radio);

    void begin(uint32_t now_ms, bool transmit = true);
    void tick(uint32_t now_ms, bool ignition);
    void short_press(uint32_t now_ms);
    void long_press(uint32_t now_ms);
    void very_long_press(uint32_t now_ms);

    bool set_mode(AppMode mode, uint8_t wheel, uint32_t now_ms,
                  bool transmit = true);
    bool next_wheel(uint32_t now_ms);
    bool send_current();
    bool send_wheel(uint8_t wheel);
    bool send_all();

    AppMode mode() const { return mode_; }
    uint8_t active_wheel() const { return active_wheel_; }
    uint32_t interval_s(bool ignition) const;
    uint32_t last_error_count() const { return error_count_; }

private:
    static const uint32_t LEARN_PERIOD_MS = 2000u;
    static const uint32_t LEARN_WINDOW_MS = 60000u;
    static const uint32_t LEARN_TOTAL_MS = 900000u;
    static const uint32_t SINGLE_TIMEOUT_MS = 300000u;

    const settings_t *settings_;
    RadioSink *radio_;
    AppMode mode_ = AppMode::NORMAL;
    uint8_t active_wheel_ = WHEEL_FL;
    uint32_t next_tx_ms_ = 0;
    uint32_t mode_start_ms_ = 0;
    uint32_t wheel_start_ms_ = 0;
    uint32_t error_count_ = 0;

    bool transmit(uint8_t wheel);
    void schedule(uint32_t now_ms, bool immediate);
};

#endif
