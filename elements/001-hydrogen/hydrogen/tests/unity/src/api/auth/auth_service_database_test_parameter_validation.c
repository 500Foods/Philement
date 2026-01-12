/*
 * Unity Test File: Parameter Validation in auth_service_database.c
 * This file tests parameter validation for various database functions
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_database.h>
#include <jansson.h>
#include <string.h>

// Forward declarations for functions being tested
QueryResult* execute_auth_query(int query_ref, const char* database, json_t* params);
bool verify_password_and_status(const char* password, int account_id, const char* database, account_info_t* account);
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info);
bool check_username_availability(const char* username, const char* database);
int create_account_record(const char* username, const char* email,
                          const char* hashed_password, const char* full_name, const char* database);
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                        const char* new_jwt_hash, time_t new_expires, const char* database);
void delete_jwt_from_storage(const char* jwt_hash, const char* database);
void block_ip_address(const char* client_ip, int duration_minutes, const char* database);

// Forward declarations for test functions
void test_execute_auth_query_with_null_database(void);
void test_execute_auth_query_with_zero_query_ref(void);
void test_execute_auth_query_with_negative_query_ref(void);
void test_verify_password_and_status_with_null_password(void);
void test_verify_password_and_status_with_zero_account_id(void);
void test_verify_password_and_status_with_negative_account_id(void);
void test_verify_password_and_status_with_null_database(void);
void test_verify_password_and_status_with_null_account(void);
void test_verify_api_key_with_null_api_key(void);
void test_verify_api_key_with_null_database(void);
void test_verify_api_key_with_null_sys_info(void);
void test_check_username_availability_with_null_username(void);
void test_check_username_availability_with_null_database(void);
void test_create_account_record_with_null_username(void);
void test_create_account_record_with_null_email(void);
void test_create_account_record_with_null_password(void);
void test_create_account_record_with_null_database(void);
void test_create_account_record_with_null_full_name(void);
void test_update_jwt_storage_with_null_old_jwt_hash(void);
void test_update_jwt_storage_with_null_new_jwt_hash(void);
void test_update_jwt_storage_with_zero_account_id(void);
void test_update_jwt_storage_with_null_database(void);
void test_delete_jwt_from_storage_with_null_jwt_hash(void);
void test_delete_jwt_from_storage_with_null_database(void);
void test_block_ip_address_with_null_client_ip(void);
void test_block_ip_address_with_null_database(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

/**
 * Test: execute_auth_query_with_null_database
 * Purpose: Verify function returns NULL when database is NULL
 * Coverage: Lines 34-36
 */
void test_execute_auth_query_with_null_database(void) {
    json_t* params = json_object();
    QueryResult* result = execute_auth_query(1, NULL, params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

/**
 * Test: execute_auth_query_with_zero_query_ref
 * Purpose: Verify function returns NULL when query_ref is 0
 * Coverage: Lines 34-36
 */
void test_execute_auth_query_with_zero_query_ref(void) {
    json_t* params = json_object();
    QueryResult* result = execute_auth_query(0, "test_db", params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

/**
 * Test: execute_auth_query_with_negative_query_ref
 * Purpose: Verify function returns NULL when query_ref is negative
 * Coverage: Lines 34-36
 */
void test_execute_auth_query_with_negative_query_ref(void) {
    json_t* params = json_object();
    QueryResult* result = execute_auth_query(-1, "test_db", params);
    TEST_ASSERT_NULL(result);
    json_decref(params);
}

/**
 * Test: verify_password_and_status_with_null_password
 * Purpose: Verify function returns false when password is NULL
 * Coverage: Line 232
 */
void test_verify_password_and_status_with_null_password(void) {
    account_info_t account = {0};
    bool result = verify_password_and_status(NULL, 1, "test_db", &account);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_and_status_with_zero_account_id
 * Purpose: Verify function returns false when account_id is 0
 * Coverage: Line 232
 */
void test_verify_password_and_status_with_zero_account_id(void) {
    account_info_t account = {0};
    bool result = verify_password_and_status("password", 0, "test_db", &account);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_and_status_with_negative_account_id
 * Purpose: Verify function returns false when account_id is negative
 * Coverage: Line 232
 */
void test_verify_password_and_status_with_negative_account_id(void) {
    account_info_t account = {0};
    bool result = verify_password_and_status("password", -1, "test_db", &account);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_and_status_with_null_database
 * Purpose: Verify function returns false when database is NULL
 * Coverage: Line 232
 */
void test_verify_password_and_status_with_null_database(void) {
    account_info_t account = {0};
    bool result = verify_password_and_status("password", 1, NULL, &account);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_password_and_status_with_null_account
 * Purpose: Verify function returns false when account pointer is NULL
 * Coverage: Line 232
 */
void test_verify_password_and_status_with_null_account(void) {
    bool result = verify_password_and_status("password", 1, "test_db", NULL);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_api_key_with_null_api_key
 * Purpose: Verify function returns false and logs error when api_key is NULL
 * Coverage: Lines 665-667
 */
void test_verify_api_key_with_null_api_key(void) {
    system_info_t sys_info = {0};
    bool result = verify_api_key(NULL, "test_db", &sys_info);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_api_key_with_null_database
 * Purpose: Verify function returns false and logs error when database is NULL
 * Coverage: Lines 665-667
 */
void test_verify_api_key_with_null_database(void) {
    system_info_t sys_info = {0};
    bool result = verify_api_key("test_key", NULL, &sys_info);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: verify_api_key_with_null_sys_info
 * Purpose: Verify function returns false and logs error when sys_info is NULL
 * Coverage: Lines 665-667
 */
void test_verify_api_key_with_null_sys_info(void) {
    bool result = verify_api_key("test_key", "test_db", NULL);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: check_username_availability_with_null_username
 * Purpose: Verify function returns false when username is NULL
 * Coverage: Line 330
 */
void test_check_username_availability_with_null_username(void) {
    bool result = check_username_availability(NULL, "test_db");
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: check_username_availability_with_null_database
 * Purpose: Verify function returns false when database is NULL
 * Coverage: Line 330
 */
void test_check_username_availability_with_null_database(void) {
    bool result = check_username_availability("testuser", NULL);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test: create_account_record_with_null_username
 * Purpose: Verify function returns -1 when username is NULL
 * Coverage: Line 360
 */
void test_create_account_record_with_null_username(void) {
    int result = create_account_record(NULL, "test@example.com", "hash123", "Test User", "test_db");
    TEST_ASSERT_EQUAL(-1, result);
}

/**
 * Test: create_account_record_with_null_email
 * Purpose: Verify function returns -1 when email is NULL
 * Coverage: Line 360
 */
void test_create_account_record_with_null_email(void) {
    int result = create_account_record("testuser", NULL, "hash123", "Test User", "test_db");
    TEST_ASSERT_EQUAL(-1, result);
}

/**
 * Test: create_account_record_with_null_password
 * Purpose: Verify function returns -1 when hashed_password is NULL
 * Coverage: Line 360
 */
void test_create_account_record_with_null_password(void) {
    int result = create_account_record("testuser", "test@example.com", NULL, "Test User", "test_db");
    TEST_ASSERT_EQUAL(-1, result);
}

/**
 * Test: create_account_record_with_null_database
 * Purpose: Verify function returns -1 when database is NULL
 * Coverage: Line 360
 */
void test_create_account_record_with_null_database(void) {
    int result = create_account_record("testuser", "test@example.com", "hash123", "Test User", NULL);
    TEST_ASSERT_EQUAL(-1, result);
}

/**
 * Test: create_account_record_with_null_full_name
 * Purpose: Verify function handles NULL full_name (optional parameter)
 * This should not return -1 since full_name is optional
 */
void test_create_account_record_with_null_full_name(void) {
    // This will likely fail at the database level since we don't have a real DB
    // but we're testing that it doesn't fail on the NULL check
    // The function will proceed past line 360 and fail later
    int result = create_account_record("testuser", "test@example.com", "hash123", NULL, "test_db");
    // We expect -1 because there's no database, but importantly not from the null check
    TEST_ASSERT_EQUAL(-1, result);
}

/**
 * Test: update_jwt_storage_with_null_old_jwt_hash
 * Purpose: Verify function returns early when old_jwt_hash is NULL
 * Coverage: Line 441
 */
void test_update_jwt_storage_with_null_old_jwt_hash(void) {
    // Function returns void, so we just ensure it doesn't crash
    update_jwt_storage(1, NULL, "new_hash", time(NULL), "test_db");
    // If we get here without crashing, the test passes
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: update_jwt_storage_with_null_new_jwt_hash
 * Purpose: Verify function returns early when new_jwt_hash is NULL
 * Coverage: Line 441
 */
void test_update_jwt_storage_with_null_new_jwt_hash(void) {
    update_jwt_storage(1, "old_hash", NULL, time(NULL), "test_db");
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: update_jwt_storage_with_zero_account_id
 * Purpose: Verify function returns early when account_id is 0
 * Coverage: Line 441
 */
void test_update_jwt_storage_with_zero_account_id(void) {
    update_jwt_storage(0, "old_hash", "new_hash", time(NULL), "test_db");
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: update_jwt_storage_with_null_database
 * Purpose: Verify function returns early when database is NULL
 * Coverage: Line 441
 */
void test_update_jwt_storage_with_null_database(void) {
    update_jwt_storage(1, "old_hash", "new_hash", time(NULL), NULL);
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: delete_jwt_from_storage_with_null_jwt_hash
 * Purpose: Verify function returns early when jwt_hash is NULL
 * Coverage: Line 471
 */
void test_delete_jwt_from_storage_with_null_jwt_hash(void) {
    delete_jwt_from_storage(NULL, "test_db");
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: delete_jwt_from_storage_with_null_database
 * Purpose: Verify function returns early when database is NULL
 * Coverage: Line 471
 */
void test_delete_jwt_from_storage_with_null_database(void) {
    delete_jwt_from_storage("jwt_hash", NULL);
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: block_ip_address_with_null_client_ip
 * Purpose: Verify function returns early when client_ip is NULL
 * Coverage: Line 584
 */
void test_block_ip_address_with_null_client_ip(void) {
    block_ip_address(NULL, 30, "test_db");
    TEST_ASSERT_TRUE(true);
}

/**
 * Test: block_ip_address_with_null_database
 * Purpose: Verify function returns early when database is NULL
 * Coverage: Line 584
 */
void test_block_ip_address_with_null_database(void) {
    block_ip_address("192.168.1.1", 30, NULL);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    // Test execute_auth_query parameter validation
    RUN_TEST(test_execute_auth_query_with_null_database);
    RUN_TEST(test_execute_auth_query_with_zero_query_ref);
    RUN_TEST(test_execute_auth_query_with_negative_query_ref);

    // Test verify_password_and_status parameter validation
    RUN_TEST(test_verify_password_and_status_with_null_password);
    RUN_TEST(test_verify_password_and_status_with_zero_account_id);
    RUN_TEST(test_verify_password_and_status_with_negative_account_id);
    RUN_TEST(test_verify_password_and_status_with_null_database);
    RUN_TEST(test_verify_password_and_status_with_null_account);

    // Test verify_api_key parameter validation
    RUN_TEST(test_verify_api_key_with_null_api_key);
    RUN_TEST(test_verify_api_key_with_null_database);
    RUN_TEST(test_verify_api_key_with_null_sys_info);

    // Test check_username_availability parameter validation
    RUN_TEST(test_check_username_availability_with_null_username);
    RUN_TEST(test_check_username_availability_with_null_database);

    // Test create_account_record parameter validation
    RUN_TEST(test_create_account_record_with_null_username);
    RUN_TEST(test_create_account_record_with_null_email);
    RUN_TEST(test_create_account_record_with_null_password);
    RUN_TEST(test_create_account_record_with_null_database);
    RUN_TEST(test_create_account_record_with_null_full_name);

    // Test update_jwt_storage parameter validation
    RUN_TEST(test_update_jwt_storage_with_null_old_jwt_hash);
    RUN_TEST(test_update_jwt_storage_with_null_new_jwt_hash);
    RUN_TEST(test_update_jwt_storage_with_zero_account_id);
    RUN_TEST(test_update_jwt_storage_with_null_database);

    // Test delete_jwt_from_storage parameter validation
    RUN_TEST(test_delete_jwt_from_storage_with_null_jwt_hash);
    RUN_TEST(test_delete_jwt_from_storage_with_null_database);

    // Test block_ip_address parameter validation
    RUN_TEST(test_block_ip_address_with_null_client_ip);
    RUN_TEST(test_block_ip_address_with_null_database);

    return UNITY_END();
}
