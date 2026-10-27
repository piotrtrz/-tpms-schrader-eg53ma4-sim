/*
 * test_tpms_frame.cpp - testy generatora ramek TPMS (natywne, bez sprzetu)
 *
 * Wektory odniesienia wygenerowane narzedziem tools/tpms_frame.py,
 * ktore jest zgodne bit-w-bit z dekoderem rtl_433 (schrader_EG53MA4)
 * oraz z referencyjnym nagraniem Schrader_EG53MA4_TPMS/01/gfile001.cu8.
 */
#include <string.h>
#include <unity.h>
#include "config.h"
#include "tpms_frame.h"

static uint8_t hexval(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0;
}

static void unhex(const char *hex, uint8_t *out)
{
    for (unsigned i = 0; hex[2 * i]; i++) {
        out[i] = (uint8_t)((hexval(hex[2 * i]) << 4) | hexval(hex[2 * i + 1]));
    }
}

struct vec_t {
    uint32_t id;
    uint16_t kpa10;
    int8_t   temp_c;
    const char *frame_hex;   // 30 bajtow bufora nadawczego
};

void setUp(void) {}
void tearDown(void) {}

static void test_frame_vectors(void)
{
    static const vec_t v[] = {
        { 0x2FC871u, 2600,  23, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4aab3554b2ad4acd32acb2cd2cc" },
        { 0x2FC871u,    0,  32, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4aab3554b2ad4acaaaacd32ab52" },
        { 0x3495ACu, 2200,  -5, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4aab4cb2ccd334acd2aacd4b4aa" },
        { 0x000001u, 6375,  85, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4aaaaaaaaaaaaad5555352caacc" },
        { 0xABCDEEu, 2625,  24, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4ab33354b4d5352d32ccb34d2d2" },
        { 0x2FC780u, 2580, -40, "aaaaaaaaaaaaaaaaaaaacb4b2caaaaaad4aab3554ad52aaad2d4aaaab32c" },
    };

    for (unsigned i = 0; i < sizeof(v) / sizeof(v[0]); i++) {
        uint8_t got[TPMS_TX_BYTES];
        uint8_t want[TPMS_TX_BYTES];
        tpms_build_frame(got, v[i].id, v[i].kpa10, v[i].temp_c, 0x4C900070u, false);
        unhex(v[i].frame_hex, want);
        TEST_ASSERT_EQUAL_HEX8_ARRAY(want, got, TPMS_TX_BYTES);
    }
}

static void test_reference_capture(void)
{
    /* gfile001.cu8: id=2FC871, flags=4c900070, 0 kPa, 90 F, crc=0x0E */
    uint8_t p[TPMS_PAYLOAD_LEN];
    tpms_build_payload(p, 0x2FC871u, 0, 32, 0x4C900070u, false);
    const uint8_t want[TPMS_PAYLOAD_LEN] = {
        0x4C, 0x90, 0x00, 0x70, 0x2F, 0xC8, 0x71, 0x00, 0x5A, 0x0E
    };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(want, p, TPMS_PAYLOAD_LEN);
}

static void test_pressure_scale(void)
{
    uint8_t p[TPMS_PAYLOAD_LEN];
    /* 260 kPa = 2600 kPa*10 -> 2600/2.5 = 104 = 0x68 */
    tpms_build_payload(p, 0x123456u, 2600, 23, 0x4C900070u, false);
    TEST_ASSERT_EQUAL_HEX8(0x68, p[7]);
    /* 23 C = 73 F */
    TEST_ASSERT_EQUAL_HEX8(73, p[8]);
    /* ograniczenie zakresu */
    tpms_build_payload(p, 0x123456u, 65535, 127, 0x4C900070u, false);
    TEST_ASSERT_EQUAL_HEX8(255, p[7]);
    TEST_ASSERT_EQUAL_HEX8(255, p[8]);
    tpms_build_payload(p, 0x123456u, 0, -99, 0x4C900070u, false);
    TEST_ASSERT_EQUAL_HEX8(0, p[7]);
    TEST_ASSERT_EQUAL_HEX8(0, p[8]);
}

static void test_checksum(void)
{
    uint8_t p[TPMS_PAYLOAD_LEN];
    tpms_build_payload(p, 0x2FC871u, 2600, 23, 0x4C900070u, false);
    unsigned sum = 0;
    for (int i = 0; i < 9; i++) {
        sum += p[i];
    }
    TEST_ASSERT_EQUAL_HEX8(sum & 0xFF, p[9]);
}

static void test_low_batt_flag(void)
{
    uint8_t a[TPMS_PAYLOAD_LEN], b[TPMS_PAYLOAD_LEN];
    tpms_build_payload(a, 0x2FC871u, 2600, 23, 0x4C900070u, false);
    tpms_build_payload(b, 0x2FC871u, 2600, 23, 0x4C900070u, true);
    TEST_ASSERT_EQUAL_HEX32(0x4C900070u,
            ((uint32_t)a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3]);
    TEST_ASSERT_TRUE((((uint32_t)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3])
                     & TPMS_FLAGS_LOWBAT_MASK);
}

static void test_frame_layout(void)
{
    uint8_t f[TPMS_TX_BYTES];
    tpms_build_frame(f, 0x2FC871u, 2600, 23, 0x4C900070u, false);
    /* pierwszy bit = chip rozbiegowy '1' -> najstarszy bit pierwszego bajtu */
    TEST_ASSERT_EQUAL_HEX8(0xAA, f[0]);
    /* dlugosc stale: 240 chipow = 30 bajtow */
    TEST_ASSERT_EQUAL_UINT32(30, TPMS_TX_BYTES);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_frame_vectors);
    RUN_TEST(test_reference_capture);
    RUN_TEST(test_pressure_scale);
    RUN_TEST(test_checksum);
    RUN_TEST(test_low_batt_flag);
    RUN_TEST(test_frame_layout);
    return UNITY_END();
}
