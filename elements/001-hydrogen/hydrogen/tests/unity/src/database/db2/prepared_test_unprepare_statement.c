/*
 * Unity Test File: db2_unprepare_statement
 * Tests for DB2 prepared statement cleanup
 */

#include <src/hydrogen.h>
#include <unity.h>

// Mock libdb2 (enabled by CMake via -DUSE_MOCK_LIBDB2)
#include <unity/mocks/mock_libdb2.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>
#include <src/database/database.h>

// Forward declaration
bool db2_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

// External function pointers that need to be set
extern SQLFreeHandle_t SQLFreeHandle_ptr;

// Function prototypes
void test_unprepare_statement_null_connection(void);
void test_unprepare_statement_null_statement(void);
void test_unprepare_statement_wrong_engine(void);
void test_unprepare_statement_null_db2_connection(void);
void test_unprepare_statement_null_db2_connection_field(void);
void test_unprepare_statement_no_function_pointer(void);
void test_unprepare_statement_success(void);
void test_unprepare_statement_first_of_multiple(void);
void test_unprepare_statement_middle_of_multiple(void);
void test_unprepare_statement_last_of_multiple(void);
void test_unprepare_statement_null_engine_handle(void);

void setUp(void) {
    // Reset mocks before each test
    mock_libdb2_reset_all();
    
    // Set function pointer to mock implementation
    SQLFreeHandle_ptr = mock_SQLFreeHandle;
}

void tearDown(void) {
    // Cleanup after each test
    mock_libdb2_reset_all();
}

// Test: NULL connection parameter
void test_unprepare_statement_null_connection(void) {
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(NULL, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: NULL statement parameter
void test_unprepare_statement_null_statement(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    bool result = db2_unprepare_statement(&connection, NULL);
    TEST_ASSERT_FALSE(result);
}

// Test: Wrong engine type
void test_unprepare_statement_wrong_engine(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_POSTGRESQL;
    
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: NULL DB2 connection handle
void test_unprepare_statement_null_db2_connection(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    connection.connection_handle = NULL;
    
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: NULL DB2 connection field
void test_unprepare_statement_null_db2_connection_field(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = NULL;
    connection.connection_handle = &db2_conn;
    
    PreparedStatement stmt = {0};
    bool result = db2_unprepare_statement(&connection, &stmt);
    TEST_ASSERT_FALSE(result);
}

// Test: Function pointer not available - cleanup without DB2 API
void test_unprepare_statement_no_function_pointer(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;
    
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt;
    
    // Clear function pointer
    SQLFreeHandle_ptr = NULL;
    
    bool result = db2_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
}

// Test: Successful unprepare with SQLFreeHandle
void test_unprepare_statement_success(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;
    
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = (void*)0x5678;
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt;
    
    // Mock SQLFreeHandle to succeed
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);
    
    bool result = db2_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
}

// Test: Unprepare first of multiple statements
void test_unprepare_statement_first_of_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 3;
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->sql_template = strdup("SELECT 1");
    stmt1->engine_specific_handle = (void*)0x1111;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->sql_template = strdup("SELECT 2");
    stmt2->engine_specific_handle = (void*)0x2222;
    
    PreparedStatement* stmt3 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt3);
    stmt3->name = strdup("stmt_3");
    stmt3->sql_template = strdup("SELECT 3");
    stmt3->engine_specific_handle = (void*)0x3333;
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt1;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[1] = stmt2;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[2] = stmt3;
    
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);
    
    bool result = db2_unprepare_statement(&connection, stmt1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);
    
    // Cleanup remaining statements
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(stmt3->name);
    free(stmt3->sql_template);
    free(stmt3);
    free(connection.prepared_statements);
}

// Test: Unprepare middle of multiple statements
void test_unprepare_statement_middle_of_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 3;
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->sql_template = strdup("SELECT 1");
    stmt1->engine_specific_handle = (void*)0x1111;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->sql_template = strdup("SELECT 2");
    stmt2->engine_specific_handle = (void*)0x2222;
    
    PreparedStatement* stmt3 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt3);
    stmt3->name = strdup("stmt_3");
    stmt3->sql_template = strdup("SELECT 3");
    stmt3->engine_specific_handle = (void*)0x3333;
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt1;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[1] = stmt2;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[2] = stmt3;
    
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);
    
    bool result = db2_unprepare_statement(&connection, stmt2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt1, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt3, connection.prepared_statements[1]);
    
    // Cleanup remaining statements
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt3->name);
    free(stmt3->sql_template);
    free(stmt3);
    free(connection.prepared_statements);
}

// Test: Unprepare last of multiple statements
void test_unprepare_statement_last_of_multiple(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 3;
    
    PreparedStatement* stmt1 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt1);
    stmt1->name = strdup("stmt_1");
    stmt1->sql_template = strdup("SELECT 1");
    stmt1->engine_specific_handle = (void*)0x1111;
    
    PreparedStatement* stmt2 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt2);
    stmt2->name = strdup("stmt_2");
    stmt2->sql_template = strdup("SELECT 2");
    stmt2->engine_specific_handle = (void*)0x2222;
    
    PreparedStatement* stmt3 = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt3);
    stmt3->name = strdup("stmt_3");
    stmt3->sql_template = strdup("SELECT 3");
    stmt3->engine_specific_handle = (void*)0x3333;
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt1;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[1] = stmt2;
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[2] = stmt3;
    
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);
    
    bool result = db2_unprepare_statement(&connection, stmt3);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, connection.prepared_statement_count);
    TEST_ASSERT_EQUAL(stmt1, connection.prepared_statements[0]);
    TEST_ASSERT_EQUAL(stmt2, connection.prepared_statements[1]);
    
    // Cleanup remaining statements
    free(stmt1->name);
    free(stmt1->sql_template);
    free(stmt1);
    free(stmt2->name);
    free(stmt2->sql_template);
    free(stmt2);
    free(connection.prepared_statements);
}

// Test: Unprepare with NULL engine_specific_handle
void test_unprepare_statement_null_engine_handle(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_DB2;
    
    DB2Connection db2_conn = {0};
    db2_conn.connection = (void*)0x1234;
    connection.connection_handle = &db2_conn;
    
    // Setup prepared statements array
    connection.prepared_statements = calloc(10, sizeof(PreparedStatement*));
    connection.prepared_statement_count = 1;
    
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("test_stmt");
    stmt->sql_template = strdup("SELECT 1");
    stmt->engine_specific_handle = NULL; // NULL handle
    
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: Test will fail if allocation fails, which is acceptable in test environment
    connection.prepared_statements[0] = stmt;
    
    mock_libdb2_set_SQLFreeHandle_result(SQL_SUCCESS);
    
    bool result = db2_unprepare_statement(&connection, stmt);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, connection.prepared_statement_count);
    
    // Cleanup
    free(connection.prepared_statements);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_unprepare_statement_null_connection);
    RUN_TEST(test_unprepare_statement_null_statement);
    RUN_TEST(test_unprepare_statement_wrong_engine);
    RUN_TEST(test_unprepare_statement_null_db2_connection);
    RUN_TEST(test_unprepare_statement_null_db2_connection_field);
    RUN_TEST(test_unprepare_statement_no_function_pointer);
    RUN_TEST(test_unprepare_statement_success);
    RUN_TEST(test_unprepare_statement_first_of_multiple);
    RUN_TEST(test_unprepare_statement_middle_of_multiple);
    RUN_TEST(test_unprepare_statement_last_of_multiple);
    RUN_TEST(test_unprepare_statement_null_engine_handle);
    
    return UNITY_END();
}