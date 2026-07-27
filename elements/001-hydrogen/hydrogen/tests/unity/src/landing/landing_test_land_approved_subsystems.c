/*
 * Unity Test File: landing_test_land_approved_subsystems.c
 * This file contains unit tests for the land_approved_subsystems function
 * from src/landing/landing.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/landing/landing.h>
#include <src/state/state_types.h>
#include <src/globals.h>
#include <src/registry/registry.h>

bool land_approved_subsystems(ReadinessResults* results);

void test_land_approved_subsystems_null_results(void);
void test_land_approved_subsystems_empty_results(void);
void test_land_approved_subsystems_single_ready_subsystem(void);
void test_land_approved_subsystems_multiple_ready_subsystems(void);
void test_land_approved_subsystems_registry_skipped(void);
void test_land_approved_subsystems_not_ready_subsystems_skipped(void);
void test_land_approved_subsystems_unknown_subsystem_skipped(void);

void setUp(void) {
    init_registry();
}

void tearDown(void) {
    init_registry();
}

void test_land_approved_subsystems_null_results(void) {
    TEST_ASSERT_FALSE(land_approved_subsystems(NULL));
}

void test_land_approved_subsystems_empty_results(void) {
    ReadinessResults results = {0};
    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
}

void test_land_approved_subsystems_single_ready_subsystem(void) {
    int id = register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(id >= 0);
    update_subsystem_state(id, SUBSYSTEM_RUNNING);

    ReadinessResults results = {
        .total_checked = 1,
        .results = {
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(id));
}

void test_land_approved_subsystems_multiple_ready_subsystems(void) {
    int print_id = register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    int api_id = register_subsystem(SR_API, NULL, NULL, NULL, NULL, NULL);
    int db_id = register_subsystem(SR_DATABASE, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(print_id >= 0 && api_id >= 0 && db_id >= 0);
    update_subsystem_state(print_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(api_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(db_id, SUBSYSTEM_RUNNING);

    ReadinessResults results = {
        .total_checked = 3,
        .results = {
            {.subsystem = SR_PRINT, .ready = true},
            {.subsystem = SR_API, .ready = true},
            {.subsystem = SR_DATABASE, .ready = true}
        }
    };

    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(print_id));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(api_id));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(db_id));
}

void test_land_approved_subsystems_registry_skipped(void) {
    int reg_id = register_subsystem(SR_REGISTRY, NULL, NULL, NULL, NULL, NULL);
    int print_id = register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(reg_id >= 0 && print_id >= 0);
    update_subsystem_state(reg_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(print_id, SUBSYSTEM_RUNNING);

    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = SR_REGISTRY, .ready = true},
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
    /* Registry is never landed by land_approved_subsystems */
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(reg_id));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(print_id));
}

void test_land_approved_subsystems_not_ready_subsystems_skipped(void) {
    int print_id = register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    int api_id = register_subsystem(SR_API, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(print_id >= 0 && api_id >= 0);
    update_subsystem_state(print_id, SUBSYSTEM_RUNNING);
    update_subsystem_state(api_id, SUBSYSTEM_RUNNING);

    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = SR_PRINT, .ready = false},
            {.subsystem = SR_API, .ready = true}
        }
    };

    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
    TEST_ASSERT_EQUAL(SUBSYSTEM_RUNNING, get_subsystem_state(print_id));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(api_id));
}

void test_land_approved_subsystems_unknown_subsystem_skipped(void) {
    int print_id = register_subsystem(SR_PRINT, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(print_id >= 0);
    update_subsystem_state(print_id, SUBSYSTEM_RUNNING);

    ReadinessResults results = {
        .total_checked = 2,
        .results = {
            {.subsystem = "UnknownSubsystem", .ready = true},
            {.subsystem = SR_PRINT, .ready = true}
        }
    };

    TEST_ASSERT_TRUE(land_approved_subsystems(&results));
    TEST_ASSERT_EQUAL(SUBSYSTEM_INACTIVE, get_subsystem_state(print_id));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_approved_subsystems_null_results);
    RUN_TEST(test_land_approved_subsystems_empty_results);
    RUN_TEST(test_land_approved_subsystems_single_ready_subsystem);
    RUN_TEST(test_land_approved_subsystems_multiple_ready_subsystems);
    RUN_TEST(test_land_approved_subsystems_registry_skipped);
    RUN_TEST(test_land_approved_subsystems_not_ready_subsystems_skipped);
    RUN_TEST(test_land_approved_subsystems_unknown_subsystem_skipped);

    return UNITY_END();
}
