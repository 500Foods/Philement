/*
 * Unity Test File: Beryllium accelerated_move Function Tests
 * This file contains unit tests for the accelerated_move() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test motion profile calculations
 * - Test triangle vs trapezoidal profiles
 * - Test edge cases (zero length, zero acceleration)
 * - Test boundary conditions
 * - Test calculation accuracy
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <math.h>

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

void setUp(void) {
    // No setup needed for motion calculation tests
}

void tearDown(void) {
    // No teardown needed for motion calculation tests
}

// Forward declaration for the function being tested
double accelerated_move(double length, double acceleration, double max_velocity);

// Test functions
void test_accelerated_move_zero_length(void);
void test_accelerated_move_zero_acceleration(void);
void test_accelerated_move_zero_velocity(void);
void test_accelerated_move_triangle_profile(void);
void test_accelerated_move_trapezoidal_profile(void);
void test_accelerated_move_precision_calculation(void);
void test_accelerated_move_boundary_conditions(void);
void test_accelerated_move_realistic_values(void);

void test_accelerated_move_zero_length(void) {
    double result = accelerated_move(0.0, 1000.0, 100.0);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_accelerated_move_zero_acceleration(void) {
    double result = accelerated_move(10.0, 0.0, 100.0);
    // With zero acceleration, should return 0 (can't move)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_accelerated_move_zero_velocity(void) {
    double result = accelerated_move(10.0, 1000.0, 0.0);
    // With zero velocity, should return 0 (can't move)
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_accelerated_move_triangle_profile(void) {
    // Short distance that should use triangle profile
    // accel_distance = v^2 / (2a) = 100^2 / (2*1000) = 10000 / 2000 = 5.0
    // 2 * accel_distance = 10.0, so distance < 10.0 should use triangle profile
    double result = accelerated_move(5.0, 1000.0, 100.0);

    // For triangle profile: peak_velocity = sqrt(a * d) = sqrt(1000 * 5) = sqrt(5000) ≈ 70.71
    // time = 2 * peak_velocity / a = 2 * 70.71 / 1000 ≈ 0.1414
    double expected = 2.0 * sqrt(1000.0 * 5.0) / 1000.0;

    TEST_ASSERT_TRUE(fabs(result - expected) < 0.001);
}

void test_accelerated_move_trapezoidal_profile(void) {
    // Long distance that should use trapezoidal profile
    // accel_distance = v^2 / (2a) = 100^2 / (2*1000) = 5.0
    // 2 * accel_distance = 10.0, so distance > 10.0 should use trapezoidal profile
    double result = accelerated_move(20.0, 1000.0, 100.0);

    // For trapezoidal profile:
    // accel_time = v / a = 100 / 1000 = 0.1
    // const_time = (d - 2*accel_distance) / v = (20 - 10) / 100 = 0.1
    // total_time = 2*accel_time + const_time = 0.2 + 0.1 = 0.3
    double expected = 2.0 * (100.0 / 1000.0) + (20.0 - 2.0 * 5.0) / 100.0;

    TEST_ASSERT_TRUE(fabs(result - expected) < 0.001);
}

void test_accelerated_move_precision_calculation(void) {
    double result = accelerated_move(1.0, 500.0, 50.0);

    // accel_distance = v^2 / (2a) = 2500 / 1000 = 2.5
    // Since 1.0 < 2*2.5=5.0, should use triangle profile
    // peak_velocity = sqrt(a*d) = sqrt(500*1) = sqrt(500) ≈ 22.36
    // time = 2 * peak_velocity / a = 2 * 22.36 / 500 ≈ 0.0894
    double expected = 2.0 * sqrt(500.0 * 1.0) / 500.0;

    TEST_ASSERT_TRUE(fabs(result - expected) < 0.001);
}

void test_accelerated_move_boundary_conditions(void) {
    // Test exactly at the boundary between triangle and trapezoidal
    double accel_distance = 100.0 * 100.0 / (2.0 * 1000.0); // 5.0
    double boundary_distance = 2.0 * accel_distance; // 10.0

    double result = accelerated_move(boundary_distance, 1000.0, 100.0);

    // Should be the same for both profiles at boundary
    double expected_triangle = 2.0 * sqrt(1000.0 * boundary_distance) / 1000.0;
    double expected_trapezoidal = 2.0 * (100.0 / 1000.0) + (boundary_distance - 2.0 * accel_distance) / 100.0;

    TEST_ASSERT_TRUE(fabs(result - expected_triangle) < 0.001);
    TEST_ASSERT_TRUE(fabs(result - expected_trapezoidal) < 0.001);
}

void test_accelerated_move_realistic_values(void) {
    // Realistic 3D printer values
    double result = accelerated_move(100.0, 1500.0, 120.0); // 100mm move, 1500mm/s² accel, 120mm/s max vel

    // accel_distance = v^2 / (2a) = 14400 / 3000 = 4.8
    // Since 100 > 2*4.8=9.6, should use trapezoidal profile
    // accel_time = v/a = 120/1500 = 0.08
    // const_time = (100 - 9.6) / 120 = 90.4 / 120 ≈ 0.7533
    // total_time = 2*0.08 + 0.7533 = 0.9133
    double expected = 2.0 * (120.0 / 1500.0) + (100.0 - 2.0 * (120.0 * 120.0 / (2.0 * 1500.0))) / 120.0;

    TEST_ASSERT_TRUE(fabs(result - expected) < 0.001);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_accelerated_move_zero_length);
    RUN_TEST(test_accelerated_move_zero_acceleration);
    RUN_TEST(test_accelerated_move_zero_velocity);
    RUN_TEST(test_accelerated_move_triangle_profile);
    RUN_TEST(test_accelerated_move_trapezoidal_profile);
    RUN_TEST(test_accelerated_move_precision_calculation);
    RUN_TEST(test_accelerated_move_boundary_conditions);
    RUN_TEST(test_accelerated_move_realistic_values);

    return UNITY_END();
}