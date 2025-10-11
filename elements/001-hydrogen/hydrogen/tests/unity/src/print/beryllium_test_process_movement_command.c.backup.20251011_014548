/*
 * Unity Test File: Beryllium process_movement_command Function Tests
 * This file contains unit tests for the process_movement_command() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test G0/G1 movement command processing
 * - Test G4 dwell command processing
 * - Test G92 position reset command processing
 * - Test movement time calculations
 * - Test extrusion tracking
 * - Test position updates
 * - Test edge cases and error conditions
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <math.h>

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

void setUp(void) {
    // No setup needed for movement command processing tests
}

void tearDown(void) {
    // No teardown needed for movement command processing tests
}

// Forward declaration for the function being tested
double process_movement_command(const char *line, BerylliumConfig *config,
                                double *current_x, double *current_y, double *current_z,
                                double *extrusion, double *current_extrusion_pos,
                                bool *relative_mode, bool *relative_extrusion,
                                double *current_feedrate, double *z_values, int *z_values_count,
                                int *z_values_capacity, int current_layer, int current_object,
                                int num_objects, double ***object_times);

// Test functions
void test_process_movement_command_null_line(void);
void test_process_movement_command_no_movement(void);
void test_process_movement_command_g0_move(void);
void test_process_movement_command_g1_move(void);
void test_process_movement_command_g4_dwell_p(void);
void test_process_movement_command_g4_dwell_s(void);
void test_process_movement_command_g92_reset(void);
void test_process_movement_command_relative_mode(void);
void test_process_movement_command_extrusion_absolute(void);
void test_process_movement_command_extrusion_relative(void);
void test_process_movement_command_feedrate_update(void);
void test_process_movement_command_z_tracking(void);

void test_process_movement_command_null_line(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command(NULL, &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_process_movement_command_no_movement(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_process_movement_command_g0_move(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;
    config.acceleration = 1000.0;
    config.max_speed_xy = 100.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G0 X10 Y10", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_x);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_y);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, current_z);
}

void test_process_movement_command_g1_move(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;
    config.acceleration = 1000.0;
    config.max_speed_xy = 100.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1 X10 Y10 E5", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_TRUE(result > 0.0);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_x);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_y);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, extrusion);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, current_extrusion_pos);
}

void test_process_movement_command_g4_dwell_p(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G4 P1000", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(1.0, result); // 1000ms = 1 second
}

void test_process_movement_command_g4_dwell_s(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G4 S2", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(2.0, result); // 2 seconds
}

void test_process_movement_command_g92_reset(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 10.0, current_extrusion_pos = 10.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G92 E0", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, extrusion); // Total extrusion should remain unchanged
    TEST_ASSERT_EQUAL_DOUBLE(0.0, current_extrusion_pos); // Position should be reset
}

void test_process_movement_command_relative_mode(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;
    config.acceleration = 1000.0;
    config.max_speed_xy = 100.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    // First set relative mode
    process_movement_command("G91", &config, &current_x, &current_y, &current_z,
                           &extrusion, &current_extrusion_pos, &relative_mode,
                           &relative_extrusion, &current_feedrate, z_values,
                           &z_values_count, &z_values_capacity, current_layer,
                           current_object, num_objects, &object_times);

    TEST_ASSERT_TRUE(relative_mode);
    TEST_ASSERT_TRUE(relative_extrusion);

    // Then make a relative move
    double result = process_movement_command("G1 X10 Y10", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_x);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, current_y);
}

void test_process_movement_command_extrusion_absolute(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1 E5", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, extrusion);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, current_extrusion_pos);
}

void test_process_movement_command_extrusion_relative(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = true;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1 E5", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, extrusion);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, current_extrusion_pos);
}

void test_process_movement_command_feedrate_update(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1 X10 F1500", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(1500.0, current_feedrate);
}

void test_process_movement_command_z_tracking(void) {
    BerylliumConfig config = {0};
    config.default_feedrate = 3000.0;

    double current_x = 0.0, current_y = 0.0, current_z = 0.0;
    double extrusion = 0.0, current_extrusion_pos = 0.0;
    bool relative_mode = false, relative_extrusion = false;
    double current_feedrate = 3000.0;
    double z_values[100];
    int z_values_count = 0, z_values_capacity = 100;
    int current_layer = 0, current_object = 0, num_objects = 1;
    double **object_times = NULL;

    double result = process_movement_command("G1 Z0.5", &config, &current_x, &current_y, &current_z,
                                           &extrusion, &current_extrusion_pos, &relative_mode,
                                           &relative_extrusion, &current_feedrate, z_values,
                                           &z_values_count, &z_values_capacity, current_layer,
                                           current_object, num_objects, &object_times);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, current_z);
    TEST_ASSERT_EQUAL_INT(1, z_values_count);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, z_values[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_process_movement_command_null_line);
    RUN_TEST(test_process_movement_command_no_movement);
    RUN_TEST(test_process_movement_command_g0_move);
    RUN_TEST(test_process_movement_command_g1_move);
    RUN_TEST(test_process_movement_command_g4_dwell_p);
    RUN_TEST(test_process_movement_command_g4_dwell_s);
    RUN_TEST(test_process_movement_command_g92_reset);
    RUN_TEST(test_process_movement_command_relative_mode);
    RUN_TEST(test_process_movement_command_extrusion_absolute);
    RUN_TEST(test_process_movement_command_extrusion_relative);
    RUN_TEST(test_process_movement_command_feedrate_update);
    RUN_TEST(test_process_movement_command_z_tracking);

    return UNITY_END();
}