#include <string.h>
#include <unity.h>
#include "parse.h"

void setUp(void) {}
void tearDown(void) {}

static void test_u32_ranges_and_bases(void)
{
    uint32_t value = 0u;
    TEST_ASSERT_TRUE(parse_u32("260", 0u, 6375u, &value));
    TEST_ASSERT_EQUAL_UINT32(260u, value);
    TEST_ASSERT_TRUE(parse_u32("2FC871", 1u, 0xFFFFFEu, &value, 16));
    TEST_ASSERT_EQUAL_HEX32(0x2FC871u, value);
    TEST_ASSERT_FALSE(parse_u32("0", 1u, 10u, &value));
    TEST_ASSERT_FALSE(parse_u32("11", 1u, 10u, &value));
    TEST_ASSERT_FALSE(parse_u32("-1", 0u, 10u, &value));
    TEST_ASSERT_FALSE(parse_u32("1x", 0u, 10u, &value));
    TEST_ASSERT_FALSE(parse_u32("", 0u, 10u, &value));
    TEST_ASSERT_FALSE(parse_u32(NULL, 0u, 10u, &value));
}

static void test_i32_ranges(void)
{
    int32_t value = 0;
    TEST_ASSERT_TRUE(parse_i32("-40", -40, 85, &value));
    TEST_ASSERT_EQUAL_INT32(-40, value);
    TEST_ASSERT_TRUE(parse_i32("85", -40, 85, &value));
    TEST_ASSERT_EQUAL_INT32(85, value);
    TEST_ASSERT_FALSE(parse_i32("-41", -40, 85, &value));
    TEST_ASSERT_FALSE(parse_i32("86", -40, 85, &value));
    TEST_ASSERT_FALSE(parse_i32("2.5", -40, 85, &value));
    TEST_ASSERT_FALSE(parse_i32(NULL, -40, 85, &value));
}

static void test_float_ranges(void)
{
    float value = 0.0f;
    TEST_ASSERT_TRUE(parse_float("260.5", 0.0f, 637.5f, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 260.5f, value);
    TEST_ASSERT_TRUE(parse_float("0", 0.0f, 637.5f, &value));
    TEST_ASSERT_TRUE(parse_float("637.5", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("637.6", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("-0.1", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("260,5", 0.0f, 637.5f, &value));
}

static void test_float_rejects_nan_and_infinity(void)
{
    /* Regresja: NaN przechodzil stary test zakresu (obie strony falszywe),
     * trafial do NVS jako cisnienie i blokowal zapis czestotliwosci RF. */
    float value = 0.0f;
    TEST_ASSERT_FALSE(parse_float("nan", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("-nan", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("NaN", 0.0f, 637.5f, &value));
    TEST_ASSERT_FALSE(parse_float("inf", 433.05f, 434.79f, &value));
    TEST_ASSERT_FALSE(parse_float("-inf", 433.05f, 434.79f, &value));
    TEST_ASSERT_FALSE(parse_float("nan", -40.0f, 85.0f, &value));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_u32_ranges_and_bases);
    RUN_TEST(test_i32_ranges);
    RUN_TEST(test_float_ranges);
    RUN_TEST(test_float_rejects_nan_and_infinity);
    return UNITY_END();
}
