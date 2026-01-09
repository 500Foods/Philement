/*
 * Unity Unit Tests for auth_service_jwt.c - generate_jwt()
 *
 * Tests the JWT generation functionality
 *
 * CHANGELOG:
 * 2026-01-09: Initial version - Tests for JWT generation
 *
 * TEST_VERSION: 1.0.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <string.h>
#include <time.h>

// Function prototypes for test functions
void test_generate_jwt_null_account(void);
void test_generate_jwt_null_system(void);
void test_generate_jwt_null_client_ip(void);
void test_generate_jwt_valid_all_params(void);
void test_generate_jwt_proper_format(void);
void test_generate_jwt_contains_three_parts(void);
void test_generate_jwt_unique_tokens(void);
void test_generate_jwt_base64url_encoded(void);

// Helper function prototypes
account_info_t* create_test_account(void);
system_info_t* create_test_system(void);
void free_test_account(account_info_t* account);
void free_test_system(system_info_t* system);

// Helper function to create test account
account_info_t* create_test_account(void) {
    account_info_t* account = calloc(1, sizeof(account_info_t));
    if (!account) return NULL;
    
    account->id = 123;
    account->username = strdup("testuser");
    account->email = strdup("test@example.com");
    account->roles = strdup("user,admin");
    
    return account;
}

// Helper function to create test system
system_info_t* create_test_system(void) {
    system_info_t* system = calloc(1, sizeof(system_info_t));
    if (!system) return NULL;
    
    system->system_id = 456;
    system->app_id = 789;
    
    return system;
}

// Helper function to free test account
void free_test_account(account_info_t* account) {
    if (account) {
        free(account->username);
        free(account->email);
        free(account->roles);
        free(account);
    }
}

// Helper function to free test system
void free_test_system(system_info_t* system) {
    if (system) {
        free(system);
    }
}

/* Test Setup and Teardown */
void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

/* Test 1: NULL account returns NULL */
void test_generate_jwt_null_account(void) {
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(NULL, system, "192.168.1.1", time(NULL));
    
    TEST_ASSERT_NULL(jwt);
    
    free_test_system(system);
}

/* Test 2: NULL system returns NULL */
void test_generate_jwt_null_system(void) {
    account_info_t* account = create_test_account();
    
    char* jwt = generate_jwt(account, NULL, "192.168.1.1", time(NULL));
    
    TEST_ASSERT_NULL(jwt);
    
    free_test_account(account);
}

/* Test 3: NULL client_ip returns NULL */
void test_generate_jwt_null_client_ip(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, NULL, time(NULL));
    
    TEST_ASSERT_NULL(jwt);
    
    free_test_account(account);
    free_test_system(system);
}

/* Test 4: Valid parameters generate JWT */
void test_generate_jwt_valid_all_params(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, "192.168.1.1", time(NULL));
    
    TEST_ASSERT_NOT_NULL(jwt);
    TEST_ASSERT_GREATER_THAN(0, strlen(jwt));
    
    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test 5: JWT has proper format (header.payload.signature) */
void test_generate_jwt_proper_format(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, "10.0.0.1", time(NULL));
    
    TEST_ASSERT_NOT_NULL(jwt);
    
    // JWT should contain exactly 2 dots (header.payload.signature)
    int dot_count = 0;
    for (char* p = jwt; *p; p++) {
        if (*p == '.') dot_count++;
    }
    TEST_ASSERT_EQUAL(2, dot_count);
    
    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test 6: JWT contains three parts */
void test_generate_jwt_contains_three_parts(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, "172.16.0.1", time(NULL));
    
    TEST_ASSERT_NOT_NULL(jwt);
    
    // Make a copy for tokenizing
    char* jwt_copy = strdup(jwt);
    TEST_ASSERT_NOT_NULL(jwt_copy);
    
    char* header = strtok(jwt_copy, ".");
    char* payload = strtok(NULL, ".");
    char* signature = strtok(NULL, ".");
    
    TEST_ASSERT_NOT_NULL(header);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_NOT_NULL(signature);
    
    // All parts should be non-empty
    TEST_ASSERT_GREATER_THAN(0, strlen(header));
    TEST_ASSERT_GREATER_THAN(0, strlen(payload));
    TEST_ASSERT_GREATER_THAN(0, strlen(signature));
    
    free(jwt_copy);
    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test 7: Multiple calls generate unique tokens */
void test_generate_jwt_unique_tokens(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    time_t now = time(NULL);
    
    char* jwt1 = generate_jwt(account, system, "192.168.1.1", now);
    char* jwt2 = generate_jwt(account, system, "192.168.1.1", now);
    
    TEST_ASSERT_NOT_NULL(jwt1);
    TEST_ASSERT_NOT_NULL(jwt2);
    
    // JWTs should be different due to unique JTI
    TEST_ASSERT_NOT_EQUAL(0, strcmp(jwt1, jwt2));
    
    free(jwt1);
    free(jwt2);
    free_test_account(account);
    free_test_system(system);
}

/* Test 8: JWT parts are base64url encoded (no + or / characters) */
void test_generate_jwt_base64url_encoded(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();
    
    char* jwt = generate_jwt(account, system, "192.168.1.1", time(NULL));
    
    TEST_ASSERT_NOT_NULL(jwt);
    
    // Base64url should not contain '+' or '/' (standard base64 chars)
    TEST_ASSERT_NULL(strchr(jwt, '+'));
    TEST_ASSERT_NULL(strchr(jwt, '/'));
    
    // Should not have '=' padding either
    TEST_ASSERT_NULL(strchr(jwt, '='));
    
    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_generate_jwt_null_account);
    RUN_TEST(test_generate_jwt_null_system);
    RUN_TEST(test_generate_jwt_null_client_ip);
    RUN_TEST(test_generate_jwt_valid_all_params);
    RUN_TEST(test_generate_jwt_proper_format);
    RUN_TEST(test_generate_jwt_contains_three_parts);
    RUN_TEST(test_generate_jwt_unique_tokens);
    RUN_TEST(test_generate_jwt_base64url_encoded);
    
    return UNITY_END();
}
