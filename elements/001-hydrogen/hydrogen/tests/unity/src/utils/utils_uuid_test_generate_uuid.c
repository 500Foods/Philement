/*
 * Unity Test File: Utils UUID - Generate UUID Test
 * This file contains unit tests for generate_uuid() in src/utils/utils_uuid.c
 *
 * NOTE: USE_MOCK_SYSTEM is defined globally by the CMake Unity build, so
 * gettimeofday() is provided by mock_system and controlled via
 * mock_system_set_gettimeofday_time(). Do NOT redefine USE_MOCK_SYSTEM here.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/utils/utils_uuid.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Validate the canonical 8-4-4-4-12 UUID shape and hex digits
static bool is_valid_uuid_format(const char *uuid_str) {
    if (uuid_str == NULL) return false;
    if (strlen(uuid_str) != 36) return false;

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid_str[i] != '-') return false;
        } else if (!isxdigit((unsigned char)uuid_str[i])) {
            return false;
        }
    }
    return true;
}

// Hex char -> integer (0-15), assumes valid hex input
static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    return c - 'a' + 10;
}

void setUp(void) {
    mock_system_reset_all();
    // Fixed, predictable time for deterministic tests
    mock_system_set_gettimeofday_time(1609459200, 500000); // 2021-01-01 00:00:00.5 UTC
    srand(42);
}

void tearDown(void) {
    mock_system_reset_all();
}

// Basic generation: proper length, valid shape, and null termination
static void test_generate_uuid_basic_format_and_null_termination(void) {
    char uuid_str[UUID_STR_LEN];

    generate_uuid(uuid_str);

    TEST_ASSERT_EQUAL_size_t(36, strlen(uuid_str));
    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid_str));
    TEST_ASSERT_EQUAL_CHAR('\0', uuid_str[36]);
}

// Hyphens must appear at the canonical positions
static void test_generate_uuid_hyphen_positions(void) {
    char uuid_str[UUID_STR_LEN];

    generate_uuid(uuid_str);

    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[8]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[13]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[18]);
    TEST_ASSERT_EQUAL_CHAR('-', uuid_str[23]);
}

// Every non-separator character must be a hexadecimal digit
static void test_generate_uuid_all_hex_digits(void) {
    char uuid_str[UUID_STR_LEN];

    generate_uuid(uuid_str);

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) continue;
        TEST_ASSERT_TRUE(isxdigit((unsigned char)uuid_str[i]));
    }
}

// Version nibble: third group is (rand & 0xfff) | 0x4000 -> version digit is '4'
static void test_generate_uuid_version_nibble(void) {
    char uuid_str[UUID_STR_LEN];

    generate_uuid(uuid_str);

    // Index 14 is the first char of the third group (after the 2nd hyphen)
    int version = hex_value(uuid_str[14]);
    TEST_ASSERT_EQUAL_INT(4, version & 0xF);
}

// Variant nibble: fourth group is (rand & 0x3fff) | 0x8000 -> 8..B
static void test_generate_uuid_variant_nibble(void) {
    char uuid_str[UUID_STR_LEN];

    generate_uuid(uuid_str);

    // Index 19 is the first char of the fourth group (after the 3rd hyphen)
    int variant = hex_value(uuid_str[19]);
    TEST_ASSERT_TRUE(variant >= 8 && variant <= 11);
}

// Two calls with distinct seed/time produce different identifiers
static void test_generate_uuid_distinct_calls(void) {
    char uuid1[UUID_STR_LEN];
    char uuid2[UUID_STR_LEN];

    mock_system_set_gettimeofday_time(1609459200, 500000);
    srand(42);
    generate_uuid(uuid1);

    mock_system_set_gettimeofday_time(1609459200, 500001);
    srand(43);
    generate_uuid(uuid2);

    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid1));
    TEST_ASSERT_TRUE(is_valid_uuid_format(uuid2));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(uuid1, uuid2));
}

// A batch of identifiers generated with distinct times are all unique
static void test_generate_uuid_uniqueness_over_time(void) {
    char uuids[8][UUID_STR_LEN];

    for (int i = 0; i < 8; i++) {
        mock_system_set_gettimeofday_time(1609459200 + i, 500000 + i * 1000);
        srand((unsigned int)(42 + i));
        generate_uuid(uuids[i]);
        TEST_ASSERT_TRUE(is_valid_uuid_format(uuids[i]));
    }

    for (int i = 0; i < 8; i++) {
        for (int j = i + 1; j < 8; j++) {
            TEST_ASSERT_NOT_EQUAL(0, strcmp(uuids[i], uuids[j]));
        }
    }
}

// Generation must not write past the allocated UUID_STR_LEN buffer
static void test_generate_uuid_does_not_write_past_buffer(void) {
    char uuid_str[UUID_STR_LEN];

    // Fill the entire buffer with a sentinel so any overrun is detectable
    memset(uuid_str, 'X', UUID_STR_LEN);

    generate_uuid(uuid_str);

    // Output is exactly 36 chars plus null terminator
    TEST_ASSERT_EQUAL_size_t(36, strlen(uuid_str));
    TEST_ASSERT_EQUAL_CHAR('\0', uuid_str[36]);

    // Bytes beyond the terminator must remain untouched
    for (int i = 37; i < UUID_STR_LEN; i++) {
        TEST_ASSERT_EQUAL_CHAR('X', uuid_str[i]);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_uuid_basic_format_and_null_termination);
    RUN_TEST(test_generate_uuid_hyphen_positions);
    RUN_TEST(test_generate_uuid_all_hex_digits);
    RUN_TEST(test_generate_uuid_version_nibble);
    RUN_TEST(test_generate_uuid_variant_nibble);
    RUN_TEST(test_generate_uuid_distinct_calls);
    RUN_TEST(test_generate_uuid_uniqueness_over_time);
    RUN_TEST(test_generate_uuid_does_not_write_past_buffer);

    return UNITY_END();
}
