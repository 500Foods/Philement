/*
 * Unity Test File: Web Server Upload - Extract Preview Image Test
 * This file contains unit tests for extract_preview_image() function
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/webserver/web_server_upload.h"

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
    // Remove any test files created during testing
    unlink("test_gcode.gcode");
    unlink("test_no_thumbnail.gcode");
    unlink("empty_file.gcode");
}

// Test functions
static void test_extract_preview_image_null_filename(void) {
    // Test null filename parameter
    char *result = extract_preview_image(NULL);
    TEST_ASSERT_NULL(result);
}

static void test_extract_preview_image_empty_filename(void) {
    // Test empty filename parameter
    char *result = extract_preview_image("");
    TEST_ASSERT_NULL(result);
}

static void test_extract_preview_image_nonexistent_file(void) {
    // Test with nonexistent file
    char *result = extract_preview_image("nonexistent.gcode");
    TEST_ASSERT_NULL(result);
}

static void test_extract_preview_image_no_thumbnail(void) {
    // Test G-code file without thumbnail comments
    FILE *fp = fopen("test_no_thumbnail.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "G90 ; absolute positioning\n");
    fprintf(fp, "M104 S200 ; set extruder temperature\n");
    fprintf(fp, "G28 ; home all axes\n");

    fclose(fp);

    char *result = extract_preview_image("test_no_thumbnail.gcode");
    TEST_ASSERT_NULL(result);
}

static void test_extract_preview_image_empty_file(void) {
    // Test with empty file
    FILE *fp = fopen("empty_file.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);
    fclose(fp);

    char *result = extract_preview_image("empty_file.gcode");
    TEST_ASSERT_NULL(result);
}

static void test_extract_preview_image_valid_thumbnail(void) {
    // Test with valid thumbnail data
    FILE *fp = fopen("test_gcode.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    // Write some G-code with thumbnail data (base64 encoded PNG)
    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "; thumbnail begin\n");
    fprintf(fp, "; iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ");
    fprintf(fp, "; +BAM2AIAAAAASUVORK5CYII=\n");
    fprintf(fp, "; thumbnail end\n");
    fprintf(fp, "G90 ; absolute positioning\n");

    fclose(fp);

    char *result = extract_preview_image("test_gcode.gcode");

    // Should return a data URL with the base64 data
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "data:image/png;base64,") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "+BAM2AIAAAAASUVORK5CYII=") != NULL);

    free(result);
}

static void test_extract_preview_image_multiple_lines(void) {
    // Test with thumbnail data spanning multiple lines
    FILE *fp = fopen("test_gcode.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "; thumbnail begin\n");
    fprintf(fp, "; iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ\n");
    fprintf(fp, "; +BAM2AIAAAAASUVORK5CYII=\n");
    fprintf(fp, "; thumbnail end\n");
    fprintf(fp, "G90 ; absolute positioning\n");

    fclose(fp);

    char *result = extract_preview_image("test_gcode.gcode");

    // Should return a data URL with the base64 data
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "data:image/png;base64,") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "+BAM2AIAAAAASUVORK5CYII=") != NULL);

    free(result);
}

static void test_extract_preview_image_no_end_marker(void) {
    // Test with thumbnail begin but no end marker - function should extract data until EOF
    FILE *fp = fopen("test_gcode.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "; thumbnail begin\n");
    fprintf(fp, "; iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ\n");
    fprintf(fp, "G90 ; absolute positioning\n"); // This line doesn't start with ';', so should be ignored

    // No thumbnail end marker - function should extract data until end of file
    fclose(fp);

    char *result = extract_preview_image("test_gcode.gcode");

    // The function should extract the base64 data and return it as a data URL
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(strstr(result, "data:image/png;base64,") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60AQ") != NULL);

    free(result);
}

static void test_extract_preview_image_empty_thumbnail(void) {
    // Test with empty thumbnail section
    FILE *fp = fopen("test_gcode.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "; thumbnail begin\n");
    fprintf(fp, "; thumbnail end\n");
    fprintf(fp, "G90 ; absolute positioning\n");

    fclose(fp);

    char *result = extract_preview_image("test_gcode.gcode");
    TEST_ASSERT_NULL(result); // Should return NULL for empty thumbnail
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_preview_image_null_filename);
    RUN_TEST(test_extract_preview_image_empty_filename);
    RUN_TEST(test_extract_preview_image_nonexistent_file);
    RUN_TEST(test_extract_preview_image_no_thumbnail);
    RUN_TEST(test_extract_preview_image_empty_file);
    RUN_TEST(test_extract_preview_image_valid_thumbnail);
    RUN_TEST(test_extract_preview_image_multiple_lines);
    RUN_TEST(test_extract_preview_image_no_end_marker);
    RUN_TEST(test_extract_preview_image_empty_thumbnail);

    return UNITY_END();
}
