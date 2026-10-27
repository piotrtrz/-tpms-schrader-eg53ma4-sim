#include <string.h>
#include <unity.h>
#include "settings.h"

void setUp(void) {}
void tearDown(void) {}

static void test_defaults_are_safe_and_valid(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    TEST_ASSERT_TRUE(settings_validate(&cfg));
    for (uint8_t i = 0; i < WHEEL_COUNT; ++i) {
        TEST_ASSERT_EQUAL_UINT16(2600u, cfg.wheels[i].pressure_kpa10);
        TEST_ASSERT_TRUE(settings_valid_id(cfg.wheels[i].id));
    }
    TEST_ASSERT_EQUAL_INT8(-10, cfg.tx_power_dbm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 433.92f, cfg.frequency_mhz);
}

static void test_ids_must_be_24_bit_nonzero_and_unique(void)
{
    settings_t cfg;
    settings_defaults(&cfg);
    TEST_ASSERT_FALSE(settings_set_id(&cfg, 0u, 0u));
    TEST_ASSERT_FALSE(settings_set_id(&cfg, 0u, 0xFFFFFFu));
    TEST_ASSERT_FALSE(settings_set_id(&cfg, 0u, 0x1000000u));
    TEST_ASSERT_FALSE(settings_set_id(&cfg, 4u, 0x123456u));
    TEST_ASSERT_FALSE(settings_set_id(&cfg, 0u, cfg.wheels[1].id));
    TEST_ASSERT_TRUE(settings_set_id(&cfg, 0u, 0x123456u));
    TEST_ASSERT_EQUAL_HEX32(0x123456u, cfg.wheels[0].id);
}

static void test_generated_ids_are_unique_and_deterministic_for_seed(void)
{
    settings_t a, b;
    settings_defaults(&a);
    settings_defaults(&b);
    settings_generate_ids(&a, 1234u);
    settings_generate_ids(&b, 1234u);
    TEST_ASSERT_TRUE(settings_validate(&a));
    TEST_ASSERT_EQUAL_MEMORY(&a, &b, sizeof(a));
    settings_generate_ids(&a, 0u);
    TEST_ASSERT_TRUE(settings_validate(&a));
}

static void test_blob_round_trip_and_crc(void)
{
    settings_t cfg, decoded;
    settings_defaults(&cfg);
    TEST_ASSERT_TRUE(settings_set_id(&cfg, WHEEL_FR, 0xABCDEFu));
    cfg.wheels[2].pressure_kpa10 = 2555u;
    settings_blob_t blob;
    settings_make_blob(&cfg, &blob);
    TEST_ASSERT_EQUAL_UINT32(SETTINGS_VERSION, blob.version);
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_OK, settings_decode_blob(&blob, &decoded));
    TEST_ASSERT_EQUAL_MEMORY(&cfg, &decoded, sizeof(cfg));
    blob.values.wheels[0].id ^= 1u;
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_BAD_CRC, settings_decode_blob(&blob, &decoded));
}

static void test_incompatible_version_size_magic_are_rejected(void)
{
    settings_t cfg, decoded;
    settings_defaults(&cfg);
    settings_blob_t blob;
    settings_make_blob(&cfg, &blob);
    ++blob.version;
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_BAD_VERSION, settings_decode_blob(&blob, &decoded));
    settings_make_blob(&cfg, &blob);
    --blob.size;
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_BAD_SIZE, settings_decode_blob(&blob, &decoded));
    settings_make_blob(&cfg, &blob);
    blob.magic = 0u;
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_BAD_MAGIC, settings_decode_blob(&blob, &decoded));
}

static void test_invalid_ranges_are_rejected_even_with_valid_crc(void)
{
    settings_t cfg, decoded;
    settings_defaults(&cfg);
    settings_blob_t blob;
    cfg.normal_interval_s = 4u;
    settings_make_blob(&cfg, &blob);
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_INVALID, settings_decode_blob(&blob, &decoded));
    settings_defaults(&cfg);
    cfg.tx_power_dbm = 10;
    settings_make_blob(&cfg, &blob);
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_INVALID, settings_decode_blob(&blob, &decoded));
    settings_defaults(&cfg);
    cfg.wheels[1].id = cfg.wheels[0].id;
    settings_make_blob(&cfg, &blob);
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_INVALID, settings_decode_blob(&blob, &decoded));
    settings_defaults(&cfg);
    cfg.frequency_mhz = 434.80f;
    settings_make_blob(&cfg, &blob);
    TEST_ASSERT_EQUAL(SETTINGS_LOAD_INVALID, settings_decode_blob(&blob, &decoded));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_safe_and_valid);
    RUN_TEST(test_ids_must_be_24_bit_nonzero_and_unique);
    RUN_TEST(test_generated_ids_are_unique_and_deterministic_for_seed);
    RUN_TEST(test_blob_round_trip_and_crc);
    RUN_TEST(test_incompatible_version_size_magic_are_rejected);
    RUN_TEST(test_invalid_ranges_are_rejected_even_with_valid_crc);
    return UNITY_END();
}
