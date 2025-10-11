/*
 * Unity Test File: Beryllium beryllium_analyze_gcode Function Tests
 * This file contains comprehensive unit tests for the beryllium_analyze_gcode() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test G-code analysis functionality with various inputs
 * - Test object tracking and layer detection
 * - Test time calculations and movement analysis
 * - Test memory management and error handling
 * - Test real-world G-code file processing
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"

// Enable Unity configuration
#include "unity.h"

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

// Standard C headers for file operations
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {
    // No setup needed for G-code analysis tests
}

void tearDown(void) {
    // No teardown needed for G-code analysis tests
}

// Forward declaration for the function being tested
BerylliumStats beryllium_analyze_gcode(FILE *file, const BerylliumConfig *config);

// Function prototypes for test functions
void test_beryllium_analyze_gcode_null_file(void);
void test_beryllium_analyze_gcode_null_config(void);
void test_beryllium_analyze_gcode_empty_file(void);
void test_beryllium_analyze_gcode_simple_gcode(void);
void test_beryllium_analyze_gcode_with_layer_changes(void);
void test_beryllium_analyze_gcode_with_objects(void);
void test_beryllium_analyze_gcode_movement_analysis(void);
void test_beryllium_analyze_gcode_dwell_commands(void);
void test_beryllium_analyze_gcode_extrusion_tracking(void);
void test_beryllium_analyze_gcode_complex_print(void);
void test_beryllium_analyze_gcode_memory_management(void);
void test_beryllium_analyze_gcode_error_recovery(void);

//=============================================================================
// Helper Functions
//=============================================================================

// Helper function to create a temporary file with given content
static FILE* create_temp_file(const char* content) {
    FILE* file = tmpfile();
    if (file && content) {
        fputs(content, file);
        rewind(file);
    }
    return file;
}

// Helper function to create a test config
static BerylliumConfig create_test_config(void) {
    BerylliumConfig config = {
        .acceleration = 1000.0,
        .z_acceleration = 200.0,
        .extruder_acceleration = 300.0,
        .max_speed_xy = 100.0,
        .max_speed_travel = 150.0,
        .max_speed_z = 20.0,
        .default_feedrate = 3000.0,
        .filament_diameter = 1.75,
        .filament_density = 1.24
    };
    return config;
}

//=============================================================================
// Basic Parameter Validation Tests
//=============================================================================

void test_beryllium_analyze_gcode_null_file(void) {
    BerylliumConfig config = create_test_config();
    BerylliumStats stats = beryllium_analyze_gcode(NULL, &config);

    // Should handle NULL file gracefully
    TEST_ASSERT_FALSE(stats.success);
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);
    TEST_ASSERT_EQUAL_INT(0, stats.num_objects);
}

void test_beryllium_analyze_gcode_null_config(void) {
    FILE* file = create_temp_file("G1 X10 Y10");
    BerylliumStats stats = beryllium_analyze_gcode(file, NULL);

    // Should handle NULL config gracefully
    TEST_ASSERT_FALSE(stats.success);
    TEST_ASSERT_NULL(stats.object_times);
    TEST_ASSERT_NULL(stats.object_infos);

    if (file) fclose(file);
}

void test_beryllium_analyze_gcode_empty_file(void) {
    BerylliumConfig config = create_test_config();
    FILE* file = create_temp_file("");
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    // Should handle empty file
    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(0, stats.total_lines);
    TEST_ASSERT_EQUAL_INT(0, stats.gcode_lines);
    TEST_ASSERT_EQUAL(0.0, stats.print_time);  // Use integer comparison for 0.0
    TEST_ASSERT_EQUAL(0.0, stats.extrusion);   // Use integer comparison for 0.0

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Simple G-code Analysis Tests
//=============================================================================

void test_beryllium_analyze_gcode_simple_gcode(void) {
    BerylliumConfig config = create_test_config();
    const char* simple_gcode =
        "G21 ; Set units to millimeters\n"
        "G90 ; Absolute positioning\n"
        "G1 X10 Y10 Z0.5 F3000\n"
        "G1 X20 Y10\n"
        "G1 X20 Y20\n"
        "G1 X10 Y20\n"
        "G1 X10 Y10\n";

    FILE* file = create_temp_file(simple_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    // Basic checks
    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(7, stats.total_lines);
    TEST_ASSERT_EQUAL_INT(7, stats.gcode_lines); // 7 G-code lines (G21, G90, and 5 G1 commands)
    TEST_ASSERT_GREATER_THAN(0.0, stats.print_time);
    TEST_ASSERT_EQUAL(0.0, stats.extrusion); // No E parameters = no extrusion

    // File size should be reasonable
    TEST_ASSERT_GREATER_THAN(0, stats.file_size);

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

void test_beryllium_analyze_gcode_with_layer_changes(void) {
    BerylliumConfig config = create_test_config();
    const char* layer_gcode =
        "G1 X0 Y0 Z0.2\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=0\n"
        "G1 X10 Y0\n"
        "G1 X10 Y10\n"
        "G1 Z0.4\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=1\n"
        "G1 X0 Y10\n"
        "G1 X0 Y0\n";

    FILE* file = create_temp_file(layer_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(2, stats.layer_count_slicer); // 2 layers detected
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, stats.layer_times[0]);
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, stats.layer_times[1]);
    TEST_ASSERT_EQUAL(0.0, stats.extrusion); // No E parameters = no extrusion

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Object Tracking Tests
//=============================================================================

void test_beryllium_analyze_gcode_with_objects(void) {
    BerylliumConfig config = create_test_config();
    const char* object_gcode =
        "EXCLUDE_OBJECT_DEFINE NAME=cube\n"
        "EXCLUDE_OBJECT_DEFINE NAME=sphere\n"
        "G1 X0 Y0 Z0.2\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=0\n"
        "EXCLUDE_OBJECT_START NAME=cube\n"
        "G1 X10 Y0\n"
        "G1 X10 Y10\n"
        "EXCLUDE_OBJECT_END\n"
        "EXCLUDE_OBJECT_START NAME=sphere\n"
        "G1 X20 Y10\n"
        "G1 X20 Y20\n"
        "EXCLUDE_OBJECT_END\n";

    FILE* file = create_temp_file(object_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(2, stats.num_objects);
    TEST_ASSERT_NOT_NULL(stats.object_infos);

    // Check object names
    TEST_ASSERT_EQUAL_STRING("cube", stats.object_infos[0].name);
    TEST_ASSERT_EQUAL_STRING("sphere", stats.object_infos[1].name);

    // Check object times are allocated and have values
    TEST_ASSERT_NOT_NULL(stats.object_times);
    TEST_ASSERT_NOT_NULL(stats.object_times[0]);

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Movement Analysis Tests
//=============================================================================

void test_beryllium_analyze_gcode_movement_analysis(void) {
    BerylliumConfig config = create_test_config();
    const char* movement_gcode =
        "G1 X0 Y0 Z0.5 F3000 ; Move to start\n"
        "G1 X100 Y0 ; Long X movement\n"
        "G1 X100 Y100 ; Long Y movement\n"
        "G0 X50 Y50 ; Rapid move (no extrusion)\n"
        "G1 Z1.0 ; Z movement (slower)\n";

    FILE* file = create_temp_file(movement_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_GREATER_THAN(0.0, stats.print_time);
    TEST_ASSERT_EQUAL(0.0, stats.extrusion); // No E parameters = no extrusion

    // Z movement should be accounted for
    TEST_ASSERT_EQUAL_INT(2, stats.layer_count_height); // Z values tracked (0.5 and 1.0)

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

void test_beryllium_analyze_gcode_dwell_commands(void) {
    BerylliumConfig config = create_test_config();
    const char* dwell_gcode =
        "G1 X10 Y10 Z0.5\n"
        "G4 P1000 ; Dwell for 1 second\n"
        "G1 X20 Y10\n"
        "G4 S2 ; Dwell for 2 seconds\n"
        "G1 X20 Y20\n";

    FILE* file = create_temp_file(dwell_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_GREATER_OR_EQUAL(3.0, stats.print_time); // At least 3 seconds from dwells

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Extrusion Tracking Tests
//=============================================================================

void test_beryllium_analyze_gcode_extrusion_tracking(void) {
    BerylliumConfig config = create_test_config();
    const char* extrusion_gcode =
        "G92 E0 ; Reset extrusion\n"
        "G1 X10 Y10 Z0.5 E5.0 ; Extrude 5mm\n"
        "G1 X20 Y10 E10.0 ; Extrude 5mm more\n"
        "M83 ; Relative extrusion\n"
        "G1 X20 Y20 E2.5 ; Extrude 2.5mm more\n"
        "G1 X10 Y20 E3.0 ; Extrude 3mm more\n";

    FILE* file = create_temp_file(extrusion_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    printf("EXTRUSION_TEST: extrusion=%.6f, filament_volume=%.6f, filament_weight=%.6f\n", 
           stats.extrusion, stats.filament_volume, stats.filament_weight);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_GREATER_THAN_DOUBLE(10.0, stats.extrusion); // Should be at least 10.5 (5 + 5 + 2.5 + 3)

    // Check filament calculations
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, stats.filament_volume);
    TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, stats.filament_weight);

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Complex Print Tests
//=============================================================================

void test_beryllium_analyze_gcode_complex_print(void) {
    BerylliumConfig config = create_test_config();
    const char* complex_gcode =
        "; Complex multi-object print\n"
        "EXCLUDE_OBJECT_DEFINE NAME=base\n"
        "EXCLUDE_OBJECT_DEFINE NAME=tower\n"
        "G21\n"
        "G90\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=0\n"
        "EXCLUDE_OBJECT_START NAME=base\n"
        "G1 X0 Y0 Z0.2 F3000 E0\n"
        "G1 X50 Y0 E2.5\n"
        "G1 X50 Y50 E5.0\n"
        "G1 X0 Y50 E7.5\n"
        "G1 X0 Y0 E10.0\n"
        "EXCLUDE_OBJECT_END\n"
        "G1 Z0.4\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=1\n"
        "EXCLUDE_OBJECT_START NAME=tower\n"
        "G1 X25 Y25 Z0.4 E12.0\n"
        "G1 X30 Y25 E14.0\n"
        "G1 X30 Y30 E16.0\n"
        "G1 X25 Y30 E18.0\n"
        "EXCLUDE_OBJECT_END\n";

    FILE* file = create_temp_file(complex_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(2, stats.num_objects);
    TEST_ASSERT_EQUAL_INT(2, stats.layer_count_slicer);
    TEST_ASSERT_GREATER_THAN(17.0, stats.extrusion); // Should be at least 18.0

    // Check object times are tracked
    TEST_ASSERT_NOT_NULL(stats.object_times);
    TEST_ASSERT_NOT_NULL(stats.object_times[0]);
    TEST_ASSERT_GREATER_THAN(0.0, stats.object_times[0][0]); // base object time
    TEST_ASSERT_GREATER_THAN(0.0, stats.object_times[1][1]); // tower object time

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Error Handling and Memory Tests
//=============================================================================

void test_beryllium_analyze_gcode_memory_management(void) {
    BerylliumConfig config = create_test_config();

    // Test with a larger G-code file to stress memory management
    char* large_gcode = calloc(10000, sizeof(char));
    TEST_ASSERT_NOT_NULL(large_gcode);

    // Generate repetitive G-code commands
    char* ptr = large_gcode;
    for (int i = 0; i < 100; i++) {
        sprintf(ptr, "G1 X%d Y%d Z0.2\n", i % 10, i / 10);
        ptr += strlen(ptr);
    }

    FILE* file = create_temp_file(large_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    // Should handle large files without memory issues
    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(100, stats.gcode_lines);

    beryllium_free_stats(&stats);
    if (file) fclose(file);
    free(large_gcode);
}

void test_beryllium_analyze_gcode_error_recovery(void) {
    BerylliumConfig config = create_test_config();
    const char* error_gcode =
        "G1 X0 Y0 Z0.2\n"
        "INVALID_COMMAND\n"
        "G1 X10 Y0\n"
        "SET_PRINT_STATS_INFO CURRENT_LAYER=0\n"
        "EXCLUDE_OBJECT_DEFINE NAME=test\n"
        "G1 X10 Y10\n";

    FILE* file = create_temp_file(error_gcode);
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);

    // Should handle invalid commands gracefully
    TEST_ASSERT_TRUE(stats.success);
    TEST_ASSERT_EQUAL_INT(6, stats.total_lines);
    TEST_ASSERT_EQUAL_INT(3, stats.gcode_lines); // Only valid G/M commands (G1 commands)

    beryllium_free_stats(&stats);
    if (file) fclose(file);
}

//=============================================================================
// Test Main Function
//=============================================================================

int main(void) {
    UNITY_BEGIN();

    // Basic parameter tests
    RUN_TEST(test_beryllium_analyze_gcode_null_file);
    RUN_TEST(test_beryllium_analyze_gcode_null_config);
    RUN_TEST(test_beryllium_analyze_gcode_empty_file);

    // Simple analysis tests
    RUN_TEST(test_beryllium_analyze_gcode_simple_gcode);
    RUN_TEST(test_beryllium_analyze_gcode_with_layer_changes);

    // Object tracking tests
    RUN_TEST(test_beryllium_analyze_gcode_with_objects);

    // Movement analysis tests
    RUN_TEST(test_beryllium_analyze_gcode_movement_analysis);
    RUN_TEST(test_beryllium_analyze_gcode_dwell_commands);

    // Extrusion tracking tests
    RUN_TEST(test_beryllium_analyze_gcode_extrusion_tracking);

    // Complex scenario tests
    RUN_TEST(test_beryllium_analyze_gcode_complex_print);

    // Error handling tests
    RUN_TEST(test_beryllium_analyze_gcode_memory_management);
    RUN_TEST(test_beryllium_analyze_gcode_error_recovery);

    return UNITY_END();
}
