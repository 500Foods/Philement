/*
 * Unity Test File: Web Server Compression - Brotli File Exists Test
 * This file contains unit tests for brotli_file_exists() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_compression.h>

// Standard library includes for file operations
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {
    // Create test files for testing
    FILE *fp = fopen("test_file.txt.br", "w");
    if (fp) {
        fprintf(fp, "test compressed content");
        fclose(fp);
    }

    fp = fopen("test_file.br", "w");
    if (fp) {
        fprintf(fp, "test compressed content");
        fclose(fp);
    }
}

void tearDown(void) {
    // Remove any test files created during testing
    unlink("test_file.txt");
    unlink("test_file.txt.br");
    unlink("test_file.br");
    unlink("nonexistent.br");
}

// Test functions
static void test_brotli_file_exists_null_file_path(void) {
    // Test null file path parameter
    char buffer[256];
    TEST_ASSERT_FALSE(brotli_file_exists(NULL, buffer, sizeof(buffer)));
}

static void test_brotli_file_exists_null_buffer(void) {
    // Test null buffer parameter (should still check file existence)
    TEST_ASSERT_TRUE(brotli_file_exists("test_file.txt", NULL, 0));
}

static void test_brotli_file_exists_zero_buffer_size(void) {
    // Test with zero buffer size
    char buffer[256];
    TEST_ASSERT_FALSE(brotli_file_exists("test_file.txt", buffer, 0));
}

static void test_brotli_file_exists_file_without_br_extension(void) {
    // Test file without .br extension - should look for .br file
    char buffer[256];
    TEST_ASSERT_TRUE(brotli_file_exists("test_file.txt", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("test_file.txt.br", buffer);
}

static void test_brotli_file_exists_file_with_br_extension(void) {
    // Test file that already has .br extension - should check if it exists
    char buffer[256];
    TEST_ASSERT_TRUE(brotli_file_exists("test_file.br", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("test_file.br", buffer);
}

static void test_brotli_file_exists_no_br_file(void) {
    // Test when .br file doesn't exist
    char buffer[256];
    TEST_ASSERT_FALSE(brotli_file_exists("nonexistent.txt", buffer, sizeof(buffer)));
}

static void test_brotli_file_exists_empty_file_path(void) {
    // Test with empty file path
    char buffer[256];
    TEST_ASSERT_FALSE(brotli_file_exists("", buffer, sizeof(buffer)));
}

static void test_brotli_file_exists_small_buffer(void) {
    // Test with buffer too small for the path
    char small_buffer[5]; // Too small for "test_file.txt.br"
    TEST_ASSERT_FALSE(brotli_file_exists("test_file.txt", small_buffer, sizeof(small_buffer)));
}

static void test_brotli_file_exists_null_buffer_with_existing_file(void) {
    // Test null buffer with existing file (should still return true)
    TEST_ASSERT_TRUE(brotli_file_exists("test_file.txt", NULL, 0));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_brotli_file_exists_null_file_path);
    RUN_TEST(test_brotli_file_exists_null_buffer);
    RUN_TEST(test_brotli_file_exists_zero_buffer_size);
    RUN_TEST(test_brotli_file_exists_file_without_br_extension);
    RUN_TEST(test_brotli_file_exists_file_with_br_extension);
    RUN_TEST(test_brotli_file_exists_no_br_file);
    RUN_TEST(test_brotli_file_exists_empty_file_path);
    RUN_TEST(test_brotli_file_exists_small_buffer);
    RUN_TEST(test_brotli_file_exists_null_buffer_with_existing_file);

    return UNITY_END();
}
