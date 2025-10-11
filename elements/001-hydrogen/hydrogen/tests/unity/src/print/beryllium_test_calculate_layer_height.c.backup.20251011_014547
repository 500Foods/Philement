/*
 * Unity Test File: Beryllium calculate_layer_height Function Tests
 * This file contains unit tests for the calculate_layer_height() function
 * from src/print/beryllium.c
 *
 * Coverage Goals:
 * - Test layer height calculation from Z values
 * - Test edge cases with insufficient data
 * - Test various Z value distributions
 * - Test memory allocation handling
 * - Test calculation accuracy
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"
#include <math.h>

// Include necessary headers for the beryllium module
#include "../../../../src/print/beryllium.h"

void setUp(void) {
    // No setup needed for layer height calculation tests
}

void tearDown(void) {
    // No teardown needed for layer height calculation tests
}

// Forward declaration for the function being tested
double calculate_layer_height(const double *z_values, int z_values_count);

// Test functions
void test_calculate_layer_height_null_values(void);
void test_calculate_layer_height_insufficient_data(void);
void test_calculate_layer_height_single_layer_height(void);
void test_calculate_layer_height_multiple_same_heights(void);
void test_calculate_layer_height_variable_layer_heights(void);
void test_calculate_layer_height_out_of_order_values(void);
void test_calculate_layer_height_with_noise(void);
void test_calculate_layer_height_precision(void);
void test_calculate_layer_height_large_dataset(void);

void test_calculate_layer_height_null_values(void) {
    double result = calculate_layer_height(NULL, 5);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_calculate_layer_height_insufficient_data(void) {
    const double z_values[] = {0.2};
    double result = calculate_layer_height(z_values, 1);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
}

void test_calculate_layer_height_single_layer_height(void) {
    const double z_values[] = {0.2, 0.4, 0.6, 0.8};
    double result = calculate_layer_height(z_values, 4);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, result);
}

void test_calculate_layer_height_multiple_same_heights(void) {
    const double z_values[] = {0.2, 0.4, 0.6, 0.8, 1.0, 1.2};
    double result = calculate_layer_height(z_values, 6);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, result);
}

void test_calculate_layer_height_variable_layer_heights(void) {
    const double z_values[] = {0.2, 0.4, 0.65, 0.9, 1.15, 1.4};
    double result = calculate_layer_height(z_values, 6);
    // Should find the most common difference (0.25)
    TEST_ASSERT_TRUE(fabs(result - 0.25) < 0.001);
}

void test_calculate_layer_height_out_of_order_values(void) {
    const double z_values[] = {0.8, 0.2, 0.6, 0.4, 1.0};
    double result = calculate_layer_height(z_values, 5);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, result);
}

void test_calculate_layer_height_with_noise(void) {
    const double z_values[] = {0.2, 0.4, 0.6, 0.8, 0.9, 1.0, 1.2}; // 0.9 is noise
    double result = calculate_layer_height(z_values, 7);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, result);
}

void test_calculate_layer_height_precision(void) {
    const double z_values[] = {0.0, 0.15, 0.3, 0.45, 0.6};
    double result = calculate_layer_height(z_values, 5);
    TEST_ASSERT_EQUAL_DOUBLE(0.15, result);
}

void test_calculate_layer_height_large_dataset(void) {
    const int num_values = 20;
    const double z_values[20] = {
        0.0, 0.2, 0.4, 0.6, 0.8,
        1.01, 1.2, 1.4, 1.6, 1.8,
        2.0, 2.2, 2.4, 2.6, 2.8,
        3.01, 3.2, 3.4, 3.6, 3.8
    };

    double result = calculate_layer_height(z_values, num_values);
    TEST_ASSERT_TRUE(fabs(result - 0.2) < 0.01);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_calculate_layer_height_null_values);
    RUN_TEST(test_calculate_layer_height_insufficient_data);
    RUN_TEST(test_calculate_layer_height_single_layer_height);
    RUN_TEST(test_calculate_layer_height_multiple_same_heights);
    RUN_TEST(test_calculate_layer_height_variable_layer_heights);
    RUN_TEST(test_calculate_layer_height_out_of_order_values);
    RUN_TEST(test_calculate_layer_height_with_noise);
    RUN_TEST(test_calculate_layer_height_precision);
    RUN_TEST(test_calculate_layer_height_large_dataset);

    return UNITY_END();
}