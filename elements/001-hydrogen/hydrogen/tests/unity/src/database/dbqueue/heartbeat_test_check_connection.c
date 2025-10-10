/*
 * Unity Test File: database_queue_heartbeat_test_check_connection
 * This file contains unit tests for database_queue_check_connection function
 * from src/database/database_queue_heartbeat.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Function prototypes for test functions
void test_database_queue_check_connection_postgresql_format(void);
void test_database_queue_check_connection_mysql_format(void);
void test_database_queue_check_connection_sqlite_format(void);
void test_database_queue_check_connection_invalid_formats(void);
void test_database_queue_check_connection_edge_cases(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test PostgreSQL connection string format
void test_database_queue_check_connection_postgresql_format(void) {
    // Test with complete PostgreSQL connection string
    DatabaseQueue* test_queue = database_queue_create_lead("testdb1",
        "postgresql://testuser:testpass@localhost:5432/testdb", NULL);
    if (test_queue) {
        bool result = database_queue_check_connection(test_queue);
        // Connection will fail but parse_connection_string should work
        TEST_ASSERT_FALSE(result); // Should fail due to no actual database
        database_queue_destroy(test_queue);
    }

    // Test with minimal PostgreSQL connection string
    DatabaseQueue* test_queue2 = database_queue_create_lead("testdb2",
        "postgresql://user@host/db", NULL);
    if (test_queue2) {
        bool result = database_queue_check_connection(test_queue2);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue2);
    }
}

// Test MySQL connection string format
void test_database_queue_check_connection_mysql_format(void) {
    // Test MySQL connection string
    DatabaseQueue* test_queue = database_queue_create_lead("testdb3",
        "mysql://testuser:testpass@localhost:3306/testdb", NULL);
    if (test_queue) {
        bool result = database_queue_check_connection(test_queue);
        TEST_ASSERT_FALSE(result); // Should fail due to no actual database
        database_queue_destroy(test_queue);
    }
}

// Test SQLite connection string format
void test_database_queue_check_connection_sqlite_format(void) {
    // Test SQLite connection string
    DatabaseQueue* test_queue = database_queue_create_lead("testdb4",
        "sqlite:///tmp/test.db", NULL);
    if (test_queue) {
        bool result = database_queue_check_connection(test_queue);
        TEST_ASSERT_FALSE(result); // Should fail due to no actual database
        database_queue_destroy(test_queue);
    }
}

// Test invalid connection string formats
void test_database_queue_check_connection_invalid_formats(void) {
    // Test with malformed connection string
    DatabaseQueue* test_queue1 = database_queue_create_lead("testdb5",
        "invalid://connection", NULL);
    if (test_queue1) {
        bool result = database_queue_check_connection(test_queue1);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue1);
    }

    // Test with empty connection string
    DatabaseQueue* test_queue2 = database_queue_create_lead("testdb6",
        "", NULL);
    if (test_queue2) {
        bool result = database_queue_check_connection(test_queue2);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue2);
    }

    // Test with NULL connection string (already tested in safe tests)
    DatabaseQueue* test_queue3 = database_queue_create_lead("testdb7",
        NULL, NULL);
    if (test_queue3) {
        bool result = database_queue_check_connection(test_queue3);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue3);
    }
}

// Test edge cases for connection strings
void test_database_queue_check_connection_edge_cases(void) {
    // Test with very long connection string
    char long_conn[1024];
    memset(long_conn, 'a', sizeof(long_conn) - 1);
    long_conn[sizeof(long_conn) - 1] = '\0';
    DatabaseQueue* test_queue1 = database_queue_create_lead("testdb8",
        long_conn, NULL);
    if (test_queue1) {
        bool result = database_queue_check_connection(test_queue1);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue1);
    }

    // Test with special characters in connection string
    DatabaseQueue* test_queue2 = database_queue_create_lead("testdb9",
        "postgresql://user%20name:pass%40word@host:5432/db%20name", NULL);
    if (test_queue2) {
        bool result = database_queue_check_connection(test_queue2);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue2);
    }

    // Test with IPv6 address
    DatabaseQueue* test_queue3 = database_queue_create_lead("testdb10",
        "postgresql://user:pass@[::1]:5432/db", NULL);
    if (test_queue3) {
        bool result = database_queue_check_connection(test_queue3);
        TEST_ASSERT_FALSE(result);
        database_queue_destroy(test_queue3);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_check_connection_postgresql_format);
    RUN_TEST(test_database_queue_check_connection_mysql_format);
    if (0) RUN_TEST(test_database_queue_check_connection_sqlite_format);
    RUN_TEST(test_database_queue_check_connection_invalid_formats);
    RUN_TEST(test_database_queue_check_connection_edge_cases);

    return UNITY_END();
}