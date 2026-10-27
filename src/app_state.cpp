#include "app_state.h"
#include "config.h"

AppState::AppState(const settings_t *settings, RadioSink *radio)
        : settings_(settings), radio_(radio)
{
}

void AppState::begin(uint32_t now_ms, bool transmit)
{
    set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, transmit);
}

uint32_t AppState::interval_s(bool ignition) const
{
#if PIN_ACC >= 0
    return ignition ? settings_->ignition_interval_s : settings_->parking_interval_s;
#else
    (void)ignition;
    return settings_->normal_interval_s;
#endif
}

bool AppState::transmit(uint8_t wheel)
{
    bool ok = radio_ != NULL && radio_->send(wheel);
    if (!ok) {
        ++error_count_;
    }
    return ok;
}

bool AppState::send_current()
{
    return transmit(active_wheel_);
}

bool AppState::send_wheel(uint8_t wheel)
{
    return wheel < WHEEL_COUNT && transmit(wheel);
}

bool AppState::send_all()
{
    bool ok = true;
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        ok = transmit(i) && ok;
    }
    return ok;
}

void AppState::schedule(uint32_t now_ms, bool immediate)
{
    next_tx_ms_ = immediate ? now_ms : now_ms + LEARN_PERIOD_MS;
}

bool AppState::set_mode(AppMode mode, uint8_t wheel, uint32_t now_ms,
                        bool transmit_now)
{
    if (wheel >= WHEEL_COUNT) {
        return false;
    }
    mode_ = mode;
    active_wheel_ = wheel;
    mode_start_ms_ = now_ms;
    wheel_start_ms_ = now_ms;
    schedule(now_ms, transmit_now);
    if (mode == AppMode::NORMAL) {
        next_tx_ms_ = transmit_now ? now_ms : UINT32_MAX;
    }
    if (transmit_now && mode != AppMode::NORMAL) {
        send_current();
        schedule(now_ms, false);
    } else if (transmit_now) {
        send_all();
        next_tx_ms_ = now_ms + settings_->normal_interval_s * 1000u;
    }
    return true;
}

bool AppState::next_wheel(uint32_t now_ms)
{
    if (mode_ == AppMode::NORMAL) {
        return false;
    }
    if (mode_ == AppMode::LEARN && active_wheel_ == WHEEL_RL) {
        return set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, false);
    }
    active_wheel_ = (uint8_t)((active_wheel_ + 1u) % WHEEL_COUNT);
    wheel_start_ms_ = now_ms;
    send_current();
    schedule(now_ms, false);
    return true;
}

void AppState::short_press(uint32_t now_ms)
{
    if (mode_ == AppMode::NORMAL) {
        send_all();
        next_tx_ms_ = now_ms + settings_->normal_interval_s * 1000u;
    } else {
        next_wheel(now_ms);
    }
}

void AppState::long_press(uint32_t now_ms)
{
    if (mode_ == AppMode::LEARN) {
        set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, false);
    } else {
        set_mode(AppMode::LEARN, active_wheel_, now_ms);
    }
}

void AppState::very_long_press(uint32_t now_ms)
{
    if (mode_ == AppMode::SINGLE) {
        set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, false);
    } else {
        set_mode(AppMode::SINGLE, active_wheel_, now_ms);
    }
}

void AppState::tick(uint32_t now_ms, bool ignition)
{
    if (mode_ == AppMode::LEARN) {
        if ((uint32_t)(now_ms - mode_start_ms_) >= LEARN_TOTAL_MS
                || (uint32_t)(now_ms - wheel_start_ms_) >= LEARN_WINDOW_MS) {
            set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, false);
            return;
        }
    } else if (mode_ == AppMode::SINGLE
            && (uint32_t)(now_ms - mode_start_ms_) >= SINGLE_TIMEOUT_MS) {
        set_mode(AppMode::NORMAL, WHEEL_FL, now_ms, false);
        return;
    }

    if (next_tx_ms_ != UINT32_MAX && (int32_t)(now_ms - next_tx_ms_) >= 0) {
        if (mode_ == AppMode::NORMAL) {
            send_all();
            next_tx_ms_ = now_ms + interval_s(ignition) * 1000u;
        } else {
            send_current();
            schedule(now_ms, false);
        }
    }
}
