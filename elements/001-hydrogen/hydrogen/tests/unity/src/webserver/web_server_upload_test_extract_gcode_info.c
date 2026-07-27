/*
 * Unity Test File: Web Server Upload - Extract G-code Info Test
 * This file contains unit tests for extract_gcode_info() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_upload.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Forward declarations for test functions
void test_extract_gcode_info_null_filename(void);
void test_extract_gcode_info_nonexistent_file(void);
void test_extract_gcode_info_empty_filename(void);
void test_extract_gcode_info_empty_file(void);
void test_extract_gcode_info_simple_file(void);
void test_extract_gcode_info_with_layers(void);
void test_extract_gcode_info_configuration_fields(void);
void test_extract_gcode_info_print_time(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures
    unlink("test_gcode_info.gcode");
    unlink("test_gcode_info_empty.gcode");
    unlink("test_gcode_info_complex.gcode");
}

// Test: NULL filename parameter
void test_extract_gcode_info_null_filename(void) {
    json_t *result = extract_gcode_info(NULL);
    TEST_ASSERT_NULL(result);
}

// Test: Nonexistent file
void test_extract_gcode_info_nonexistent_file(void) {
    json_t *result = extract_gcode_info("nonexistent_file.gcode");
    TEST_ASSERT_NULL(result);
}

// Test: Empty filename
void test_extract_gcode_info_empty_filename(void) {
    json_t *result = extract_gcode_info("");
    TEST_ASSERT_NULL(result);
}

// Test: Empty G-code file
void test_extract_gcode_info_empty_file(void) {
    FILE *fp = fopen("test_gcode_info_empty.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);
    fclose(fp);

    json_t *result = extract_gcode_info("test_gcode_info_empty.gcode");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Should have analysis_start and analysis_end fields
    json_t *start = json_object_get(result, "analysis_start");
    json_t *end = json_object_get(result, "analysis_end");
    TEST_ASSERT_NOT_NULL(start);
    TEST_ASSERT_NOT_NULL(end);

    json_decref(result);
}

// Test: Simple G-code file with basic commands
void test_extract_gcode_info_simple_file(void) {
    FILE *fp = fopen("test_gcode_info.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "G90 ; absolute positioning\n");
    fprintf(fp, "G28 ; home all axes\n");
    fprintf(fp, "G1 X10 Y20 Z0.2 F3000\n");
    fprintf(fp, "G1 X50 Y60 Z0.2 F3000\n");
    fprintf(fp, "M104 S200 ; set extruder temperature\n");
    fprintf(fp, "M140 S60 ; set bed temperature\n");

    fclose(fp);

    json_t *result = extract_gcode_info("test_gcode_info.gcode");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Verify expected fields exist
    json_t *file_size = json_object_get(result, "file_size");
    json_t *total_lines = json_object_get(result, "total_lines");
    json_t *gcode_lines = json_object_get(result, "gcode_lines");
    json_t *objects = json_object_get(result, "objects");
    json_t *layers = json_object_get(result, "layers");
    json_t *configuration = json_object_get(result, "configuration");

    TEST_ASSERT_NOT_NULL(file_size);
    TEST_ASSERT_NOT_NULL(total_lines);
    TEST_ASSERT_NOT_NULL(gcode_lines);
    TEST_ASSERT_NOT_NULL(objects);
    TEST_ASSERT_NOT_NULL(layers);
    TEST_ASSERT_NOT_NULL(configuration);

    // file_size should be > 0
    TEST_ASSERT_GREATER_THAN(0, json_integer_value(file_size));

    // total_lines should be > 0
    TEST_ASSERT_GREATER_THAN(0, json_integer_value(total_lines));

    // objects should be an array
    TEST_ASSERT_TRUE(json_is_array(objects));

    // layers should be an array
    TEST_ASSERT_TRUE(json_is_array(layers));

    // configuration should be an object
    TEST_ASSERT_TRUE(json_is_object(configuration));

    json_decref(result);
}

// Test: G-code file with layer information
void test_extract_gcode_info_with_layers(void) {
    FILE *fp = fopen("test_gcode_info_complex.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, ";LAYER:0\n");
    fprintf(fp, "G28 ; home all axes\n");
    fprintf(fp, "G1 X0 Y0 Z0.2 F3000\n");
    fprintf(fp, ";LAYER:1\n");
    fprintf(fp, "G1 X10 Y10 Z0.4 F3000\n");
    fprintf(fp, "G1 X20 Y20 Z0.4 F3000\n");
    fprintf(fp, ";LAYER:2\n");
    fprintf(fp, "G1 X30 Y30 Z0.6 F3000\n");
    fprintf(fp, ";END\n");

    fclose(fp);

    json_t *result = extract_gcode_info("test_gcode_info_complex.gcode");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    // Verify layer_count_height or layer_count_slicer is present
    json_t *layer_count_height = json_object_get(result, "layer_count_height");
    json_t *layer_count_slicer = json_object_get(result, "layer_count_slicer");
    TEST_ASSERT_NOT_NULL(layer_count_height);
    TEST_ASSERT_NOT_NULL(layer_count_slicer);

    json_t *layers = json_object_get(result, "layers");
    TEST_ASSERT_NOT_NULL(layers);
    TEST_ASSERT_TRUE(json_is_array(layers));

    json_decref(result);
}

// Test: Verify configuration fields are populated
void test_extract_gcode_info_configuration_fields(void) {
    FILE *fp = fopen("test_gcode_info.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "G90 ; absolute positioning\n");
    fprintf(fp, "G28 ; home all axes\n");

    fclose(fp);

    json_t *result = extract_gcode_info("test_gcode_info.gcode");
    TEST_ASSERT_NOT_NULL(result);

    json_t *configuration = json_object_get(result, "configuration");
    TEST_ASSERT_NOT_NULL(configuration);
    TEST_ASSERT_TRUE(json_is_object(configuration));

    // Verify configuration fields
    json_t *accel = json_object_get(configuration, "acceleration");
    json_t *max_speed_xy = json_object_get(configuration, "max_speed_xy");
    json_t *filament_diameter = json_object_get(configuration, "filament_diameter");

    TEST_ASSERT_NOT_NULL(accel);
    TEST_ASSERT_NOT_NULL(max_speed_xy);
    TEST_ASSERT_NOT_NULL(filament_diameter);

    json_decref(result);
}

// Test: Verify estimated_print_time field
void test_extract_gcode_info_print_time(void) {
    FILE *fp = fopen("test_gcode_info.gcode", "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "G21 ; metric values\n");
    fprintf(fp, "G90 ; absolute positioning\n");
    fprintf(fp, "G28 ; home all axes\n");
    fprintf(fp, "G1 X10 Y20 Z0.2 F3000\n");

    fclose(fp);

    json_t *result = extract_gcode_info("test_gcode_info.gcode");
    TEST_ASSERT_NOT_NULL(result);

    json_t *estimated_time = json_object_get(result, "estimated_print_time");
    TEST_ASSERT_NOT_NULL(estimated_time);
    TEST_ASSERT_TRUE(json_is_string(estimated_time));

    const char *time_str = json_string_value(estimated_time);
    TEST_ASSERT_NOT_NULL(time_str);
    TEST_ASSERT_GREATER_THAN(0, strlen(time_str));

    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_gcode_info_null_filename);
    RUN_TEST(test_extract_gcode_info_nonexistent_file);
    RUN_TEST(test_extract_gcode_info_empty_filename);
    RUN_TEST(test_extract_gcode_info_empty_file);
    RUN_TEST(test_extract_gcode_info_simple_file);
    RUN_TEST(test_extract_gcode_info_with_layers);
    RUN_TEST(test_extract_gcode_info_configuration_fields);
    RUN_TEST(test_extract_gcode_info_print_time);

    return UNITY_END();
}
