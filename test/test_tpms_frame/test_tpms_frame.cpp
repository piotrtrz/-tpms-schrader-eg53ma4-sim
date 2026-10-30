#include <string.h>
#include <unity.h>
#include "tpms_frame.h"

void setUp(void) {}
void tearDown(void) {}

struct capture_vector_t {
    uint32_t id;
    int16_t temp_c;
    uint8_t temp_f;
    uint8_t checksum;
};

static void test_four_independent_capture_payloads(void)
{
    /* rtl_433_tests commit 1aa8c29, cztery nagrania, po 16 dekodowan.
     * Wszystkie: flags=4c900070, pressure=0. Wartosci nie pochodza z generatora. */
    static const capture_vector_t vectors[] = {
        {0x2FC871u, 32, 90u, 0x0Eu},
        {0x2EAAF9u, 32, 90u, 0x77u},
        {0x3495ACu, 32, 90u, 0x1Bu},
        {0x2FC780u, 33, 91u, 0x1Du},
    };
    for (uint8_t i = 0; i < 4u; ++i) {
        uint8_t payload[TPMS_PAYLOAD_LEN];
        TEST_ASSERT_TRUE(tpms_build_payload(payload, vectors[i].id, 0u,
                                            (int8_t)vectors[i].temp_c, 0x4C900070u));
        TEST_ASSERT_EQUAL_HEX32(vectors[i].id,
                ((uint32_t)payload[4] << 16) | ((uint32_t)payload[5] << 8) | payload[6]);
        TEST_ASSERT_EQUAL_HEX8(0x00, payload[7]);
        TEST_ASSERT_EQUAL_HEX8(vectors[i].temp_f, payload[8]);
        TEST_ASSERT_EQUAL_HEX8(vectors[i].checksum, payload[9]);
    }
}

static void test_known_frame_and_pressure_temperature(void)
{
    /* Wektor zregenerowany po kalibracji pola P wg auta (260 kPa -> P=95);
     * rownolegle do tools/tpms_frame.py. Sam kod Manchester bez zmian. */
    static const uint8_t expected[TPMS_TX_BYTES] = {
        0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,
        0xCB,0x4B,0x2C,0xAA,0xAA,0xAA,0xD4,0xAA,0xB3,0x55,
        0x4B,0x2A,0xD4,0xAC,0xCD,0x54,0xCB,0x2C,0xCD,0x4A,
    };
    uint8_t frame[TPMS_TX_BYTES];
    uint8_t payload[TPMS_PAYLOAD_LEN];
    TEST_ASSERT_TRUE(tpms_build_frame(frame, 0x2FC871u, 2600u, 23, 0x4C900070u));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, frame, TPMS_TX_BYTES);
    TEST_ASSERT_TRUE(tpms_build_payload(payload, 0x123456u, 2600u, 23, 0x4C900070u));
    TEST_ASSERT_EQUAL_HEX8(0x5F, payload[7]); /* 260 kPa -> 95 (2,73 kPa/bit) */
    TEST_ASSERT_EQUAL_UINT8(73u, payload[8]);
    TEST_ASSERT_EQUAL_HEX8(0x90, payload[9]);
}

static bool decode_strict(const uint8_t *frame, size_t length,
                          uint8_t payload[TPMS_PAYLOAD_LEN])
{
    if (frame == NULL || length != TPMS_TX_BYTES || (frame[0] & 0x80u) == 0u) {
        return false;
    }
    uint8_t row[15] = {0};
    unsigned chip = 1u;
    for (unsigned bit = 1u; bit < TPMS_ROW_BITS; ++bit) {
        unsigned first = (frame[chip >> 3] >> (7u - (chip & 7u))) & 1u;
        unsigned second = (frame[(chip + 1u) >> 3] >> (7u - ((chip + 1u) & 7u))) & 1u;
        if (first == second) {
            return false;
        }
        unsigned value = first != 0u ? 1u : 0u;
        row[bit >> 3] |= (uint8_t)(value << (7u - (bit & 7u)));
        chip += 2u;
    }
    if ((frame[29] & 1u) != 0u) {
        return false;
    }
    memcpy(payload, &row[5], TPMS_PAYLOAD_LEN);
    uint8_t sum = 0;
    for (uint8_t i = 0; i < 9u; ++i) sum = (uint8_t)(sum + payload[i]);
    return sum == payload[9];
}

static void test_manchester_polarity_and_negative_frames(void)
{
    uint8_t frame[TPMS_TX_BYTES];
    uint8_t payload[TPMS_PAYLOAD_LEN];
    TEST_ASSERT_TRUE(tpms_build_frame(frame, 0x2FC871u, 2600u, 23, 0x4C900070u));
    TEST_ASSERT_TRUE(decode_strict(frame, sizeof(frame), payload));
    TEST_ASSERT_EQUAL_HEX32(0x2FC871u,
            ((uint32_t)payload[4] << 16) | ((uint32_t)payload[5] << 8) | payload[6]);

    uint8_t damaged[TPMS_TX_BYTES];
    memcpy(damaged, frame, sizeof(frame));
    damaged[10] &= 0x3Fu; /* dwa jednakowe chipy: niedozwolony Manchester */
    TEST_ASSERT_FALSE(decode_strict(damaged, sizeof(damaged), payload));
    TEST_ASSERT_FALSE(decode_strict(frame, sizeof(frame) - 1u, payload));

    memcpy(damaged, frame, sizeof(frame));
    /* Odwroc cala komorke Manchester (oba chipy) w bicie sumy kontrolnej:
     * ramka pozostaje poprawnym Manchesterem, ale suma przestaje sie zgadzac,
     * wiec odrzucenie nastepuje wlasnie na etapie checksum. */
    damaged[28] ^= 0x60u;
    TEST_ASSERT_FALSE(decode_strict(damaged, sizeof(damaged), payload));
}

static void test_rounding_and_saturation(void)
{
    uint8_t p[TPMS_PAYLOAD_LEN];
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 12u, 0, 0u));   /* 1.2 kPa */
    TEST_ASSERT_EQUAL_UINT8(0u, p[7]);
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 13u, 0, 0u));   /* prog 13.7 */
    TEST_ASSERT_EQUAL_UINT8(0u, p[7]);
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 14u, 0, 0u));
    TEST_ASSERT_EQUAL_UINT8(1u, p[7]);
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 2608u, -40, 0u)); /* prog 2607.2 */
    TEST_ASSERT_EQUAL_UINT8(96u, p[7]);
    TEST_ASSERT_EQUAL_UINT8(0u, p[8]);
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 2607u, 85, 0u));
    TEST_ASSERT_EQUAL_UINT8(95u, p[7]);
    TEST_ASSERT_EQUAL_UINT8(185u, p[8]);
    TEST_ASSERT_TRUE(tpms_build_payload(p, 1u, 65535u, 127, 0u));
    TEST_ASSERT_EQUAL_UINT8(255u, p[7]);
    TEST_ASSERT_EQUAL_UINT8(255u, p[8]);
}

static void test_invalid_id_and_null_are_rejected(void)
{
    uint8_t p[TPMS_PAYLOAD_LEN];
    TEST_ASSERT_FALSE(tpms_build_payload(p, 0u, 2600u, 23, 0u));
    TEST_ASSERT_FALSE(tpms_build_payload(p, 0xFFFFFFu, 2600u, 23, 0u));
    TEST_ASSERT_FALSE(tpms_build_payload(p, 0x1000000u, 2600u, 23, 0u));
    TEST_ASSERT_FALSE(tpms_build_payload(NULL, 1u, 2600u, 23, 0u));
    TEST_ASSERT_FALSE(tpms_build_frame(NULL, 1u, 2600u, 23, 0u));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_four_independent_capture_payloads);
    RUN_TEST(test_known_frame_and_pressure_temperature);
    RUN_TEST(test_manchester_polarity_and_negative_frames);
    RUN_TEST(test_rounding_and_saturation);
    RUN_TEST(test_invalid_id_and_null_are_rejected);
    return UNITY_END();
}
