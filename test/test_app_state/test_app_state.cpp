#include <unity.h>
#include "app_state.h"

class FakeRadio final : public RadioSink {
public:
    uint8_t wheels[128];
    uint8_t count = 0;
    bool fail = false;

    bool send(uint8_t wheel) override
    {
        if (count < sizeof(wheels)) wheels[count] = wheel;
        ++count;
        return !fail;
    }
};

void setUp(void) {}
void tearDown(void) {}

static void test_normal_clone_ids_sends_four_wheels_and_repeats(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    cfg.wheels[0].id = 0x111111u;
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.begin(0u);
    TEST_ASSERT_EQUAL_UINT8(4u, radio.count);
    static const uint8_t expected[] = {0, 1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, radio.wheels, 4);
    app.tick(29999u, false);
    TEST_ASSERT_EQUAL_UINT8(4u, radio.count);
    app.tick(30000u, false);
    TEST_ASSERT_EQUAL_UINT8(8u, radio.count);
}

static void test_new_ids_learn_order_force_and_finish(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    settings_generate_ids(&cfg, 42u);
    TEST_ASSERT_TRUE(settings_validate(&cfg));
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.begin(0u, false);
    app.set_mode(AppMode::LEARN, WHEEL_FL, 0u);
    TEST_ASSERT_EQUAL(AppMode::LEARN, app.mode());
    TEST_ASSERT_EQUAL_UINT8(1u, radio.count);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_FL, radio.wheels[0]);
    TEST_ASSERT_TRUE(app.send_current());
    TEST_ASSERT_EQUAL_UINT8(2u, radio.count);
    app.short_press(100u);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_FR, app.active_wheel());
    app.short_press(200u);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_RR, app.active_wheel());
    app.short_press(300u);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_RL, app.active_wheel());
    app.short_press(400u);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
    static const uint8_t expected[] = {0, 0, 1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, radio.wheels, 5);
}

static void test_learn_period_window_and_total_timeout(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.set_mode(AppMode::LEARN, WHEEL_FL, 0u);
    app.tick(1999u, false);
    TEST_ASSERT_EQUAL_UINT8(1u, radio.count);
    app.tick(2000u, false);
    TEST_ASSERT_EQUAL_UINT8(2u, radio.count);
    app.tick(59999u, false);
    TEST_ASSERT_EQUAL(AppMode::LEARN, app.mode());
    app.tick(60000u, false);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
    TEST_ASSERT_EQUAL_UINT8(3u, radio.count);

    app.set_mode(AppMode::LEARN, WHEEL_FL, 100000u);
    app.tick(1000000u, false);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
}

static void test_single_isolated_wheel_and_timeout(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.set_mode(AppMode::SINGLE, WHEEL_FR, 0u);
    TEST_ASSERT_EQUAL_UINT8(1u, radio.count);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_FR, radio.wheels[0]);
    app.tick(2000u, false);
    TEST_ASSERT_EQUAL_UINT8(2u, radio.count);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_FR, radio.wheels[1]);
    app.next_wheel(3000u);
    TEST_ASSERT_EQUAL_UINT8(WHEEL_RR, app.active_wheel());
    app.tick(300000u, false);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
}

static void test_button_gestures(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.begin(0u, false);
    app.long_press(0u);
    TEST_ASSERT_EQUAL(AppMode::LEARN, app.mode());
    app.long_press(10u);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
    app.very_long_press(20u);
    TEST_ASSERT_EQUAL(AppMode::SINGLE, app.mode());
    app.very_long_press(30u);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, app.mode());
    app.short_press(40u);
    TEST_ASSERT_EQUAL_UINT8(6u, radio.count);
}

static void test_acc_intervals_without_permanent_shutdown(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    cfg.ignition_interval_s = 15u;
    cfg.parking_interval_s = 120u;
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.begin(0u);
    app.tick(30000u, true);
    app.tick(45000u, true);
    TEST_ASSERT_EQUAL_UINT8(12u, radio.count);
    app.tick(60000u, false);
    TEST_ASSERT_EQUAL_UINT8(16u, radio.count);
    app.tick(179999u, false);
    TEST_ASSERT_EQUAL_UINT8(16u, radio.count);
    app.tick(180000u, false);
    TEST_ASSERT_EQUAL_UINT8(20u, radio.count);
    TEST_ASSERT_EQUAL_UINT32(15u, app.interval_s(true));
    TEST_ASSERT_EQUAL_UINT32(120u, app.interval_s(false));
}

static void test_power_restart_and_nvs_reloaded_configuration(void)
{
    settings_t persisted;
    settings_defaults(&persisted);
    persisted.wheels[3].id = 0x654321u;
    settings_blob_t blob;
    settings_make_blob(&persisted, &blob);
    settings_t reloaded;
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_OK, settings_decode_blob(&blob, &reloaded));
    FakeRadio radio;
    AppState after_restart(&reloaded, &radio);
    after_restart.begin(1000u);
    TEST_ASSERT_EQUAL(AppMode::NORMAL, after_restart.mode());
    TEST_ASSERT_EQUAL_UINT8(4u, radio.count);
    TEST_ASSERT_EQUAL_HEX32(0x654321u, reloaded.wheels[3].id);
}

static void test_radio_failure_is_counted_and_other_wheels_continue(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    FakeRadio radio;
    radio.fail = true;
    AppState app(&cfg, &radio);
    app.begin(0u);
    TEST_ASSERT_EQUAL_UINT8(4u, radio.count);
    TEST_ASSERT_EQUAL_UINT32(4u, app.last_error_count());
}

static void test_millis_wrap_does_not_stall_learn(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    FakeRadio radio;
    AppState app(&cfg, &radio);
    app.set_mode(AppMode::LEARN, WHEEL_FL, UINT32_MAX - 1000u);
    app.tick(1000u, false);
    TEST_ASSERT_EQUAL_UINT8(2u, radio.count);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_normal_clone_ids_sends_four_wheels_and_repeats);
    RUN_TEST(test_new_ids_learn_order_force_and_finish);
    RUN_TEST(test_learn_period_window_and_total_timeout);
    RUN_TEST(test_single_isolated_wheel_and_timeout);
    RUN_TEST(test_button_gestures);
    RUN_TEST(test_acc_intervals_without_permanent_shutdown);
    RUN_TEST(test_power_restart_and_nvs_reloaded_configuration);
    RUN_TEST(test_radio_failure_is_counted_and_other_wheels_continue);
    RUN_TEST(test_millis_wrap_does_not_stall_learn);
    return UNITY_END();
}
