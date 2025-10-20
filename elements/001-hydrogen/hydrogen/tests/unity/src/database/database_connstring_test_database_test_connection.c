/*
 * Unity Test File: database_connstring_test_database_test_connection
 * This file contains unit tests for database_test_connection function
 * from src/database/database_connstring.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_connstring.h>
#include <src/database/dbqueue/dbqueue.h>

// Mock global variables for testing
extern DatabaseSubsystem* database_subsystem;
extern DatabaseQueueManager* global_queue_manager;

// Function prototypes for test functions
void test_database_test_connection_null_subsystem(void);
void test_database_test_connection_null_database_name(void);
void test_database_test_connection_null_queue_manager(void);
void test_database_test_connection_mutex_lock_failure(void);
void test_database_test_connection_database_not_found(void);
void test_database_test_connection_connected_database(void);
void test_database_test_connection_disconnected_database(void);

void setUp(void) {
    // Reset global state before each test
    database_subsystem = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    // Clean up after each test
    database_subsystem = NULL;
    global_queue_manager = NULL;
}

// Test NULL database subsystem
void test_database_test_connection_null_subsystem(void) {
    bool result = database_test_connection("test_db");
    TEST_ASSERT_FALSE(result);
}

// Test NULL database name
void test_database_test_connection_null_database_name(void) {
    // Initialize minimal subsystem
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    TEST_ASSERT_NOT_NULL(database_subsystem);

    bool result = database_test_connection(NULL);
    TEST_ASSERT_FALSE(result);

    free(database_subsystem);
    database_subsystem = NULL;
}

// Test NULL queue manager
void test_database_test_connection_null_queue_manager(void) {
    // Initialize minimal subsystem
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    TEST_ASSERT_NOT_NULL(database_subsystem);

    bool result = database_test_connection("test_db");
    TEST_ASSERT_FALSE(result);

    free(database_subsystem);
    database_subsystem = NULL;
}

// Test mutex lock failure (this is hard to test directly, but we can test the path)
void test_database_test_connection_mutex_lock_failure(void) {
    // Initialize minimal subsystem and queue manager
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(database_subsystem);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Initialize queue manager with zero database count
    global_queue_manager->database_count = 0;
    global_queue_manager->databases = NULL;

    // This should work since we have valid structures
    bool result = database_test_connection("test_db");
    TEST_ASSERT_FALSE(result); // Database not found

    free(global_queue_manager);
    free(database_subsystem);
    global_queue_manager = NULL;
    database_subsystem = NULL;
}

// Test database not found
void test_database_test_connection_database_not_found(void) {
    // Initialize minimal subsystem and queue manager
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(database_subsystem);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Initialize queue manager with one database that doesn't match
    global_queue_manager->database_count = 1;
    global_queue_manager->databases = calloc(1, sizeof(DatabaseQueue*));
    global_queue_manager->databases[0] = calloc(1, sizeof(DatabaseQueue));
    if (global_queue_manager->databases[0]) {
        global_queue_manager->databases[0]->database_name = strdup("other_db");
    }

    bool result = database_test_connection("test_db");
    TEST_ASSERT_FALSE(result);

    free(global_queue_manager->databases[0]);
    free(global_queue_manager->databases);
    free(global_queue_manager);
    free(database_subsystem);
    global_queue_manager = NULL;
    database_subsystem = NULL;
}

// Test connected database
void test_database_test_connection_connected_database(void) {
    // Initialize minimal subsystem and queue manager
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(database_subsystem);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Initialize queue manager with one connected database
    global_queue_manager->database_count = 1;
    global_queue_manager->databases = calloc(1, sizeof(DatabaseQueue*));
    global_queue_manager->databases[0] = calloc(1, sizeof(DatabaseQueue));
    if (global_queue_manager->databases[0]) {
        global_queue_manager->databases[0]->database_name = strdup("test_db");
        global_queue_manager->databases[0]->is_connected = true;
        global_queue_manager->databases[0]->shutdown_requested = false;
    }

    bool result = database_test_connection("test_db");
    TEST_ASSERT_TRUE(result);

    free(global_queue_manager->databases[0]);
    free(global_queue_manager->databases);
    free(global_queue_manager);
    free(database_subsystem);
    global_queue_manager = NULL;
    database_subsystem = NULL;
}

// Test disconnected database
void test_database_test_connection_disconnected_database(void) {
    // Initialize minimal subsystem and queue manager
    database_subsystem = calloc(1, sizeof(DatabaseSubsystem));
    global_queue_manager = calloc(1, sizeof(DatabaseQueueManager));
    TEST_ASSERT_NOT_NULL(database_subsystem);
    TEST_ASSERT_NOT_NULL(global_queue_manager);

    // Initialize queue manager with one disconnected database
    global_queue_manager->database_count = 1;
    global_queue_manager->databases = calloc(1, sizeof(DatabaseQueue*));
    global_queue_manager->databases[0] = calloc(1, sizeof(DatabaseQueue));
    if (global_queue_manager->databases[0]) {
        global_queue_manager->databases[0]->database_name = strdup("test_db");
        global_queue_manager->databases[0]->is_connected = false;
        global_queue_manager->databases[0]->shutdown_requested = false;
    }

    bool result = database_test_connection("test_db");
    TEST_ASSERT_FALSE(result);

    free(global_queue_manager->databases[0]);
    free(global_queue_manager->databases);
    free(global_queue_manager);
    free(database_subsystem);
    global_queue_manager = NULL;
    database_subsystem = NULL;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_test_connection_null_subsystem);
    RUN_TEST(test_database_test_connection_null_database_name);
    RUN_TEST(test_database_test_connection_null_queue_manager);
    RUN_TEST(test_database_test_connection_mutex_lock_failure);
    RUN_TEST(test_database_test_connection_database_not_found);
    RUN_TEST(test_database_test_connection_connected_database);
    RUN_TEST(test_database_test_connection_disconnected_database);

    return UNITY_END();
}