/*
 * Unity Unit Tests for register.c - extract_and_validate_parameters function
 *
 * Tests the extract_and_validate_parameters helper function which extracts
 * and validates registration parameters from a JSON request object.
 *
 * CHANGELOG:
 * 2026-07-24: Initial version
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/register/register.h>

// Forward declarations for functions being tested
bool extract_and_validate_parameters(
    json_t *request,
    const char **username,
    const char **password,
    const char **email,
    const char **full_name,
    const char **api_key,
    const char **database
);

// Forward declarations for test functions
void test_extract_and_validate_parameters_all_present(void);
void test_extract_and_validate_parameters_null_output_pointers(void);
void test_extract_and_validate_parameters_missing_username(void);
void test_extract_and_validate_parameters_missing_password(void);
void test_extract_and_validate_parameters_missing_email(void);
void test_extract_and_validate_parameters_missing_api_key(void);
void test_extract_and_validate_parameters_missing_database(void);
void test_extract_and_validate_parameters_optional_full_name_null(void);
void test_extract_and_validate_parameters_optional_full_name_present(void);
void test_extract_and_validate_parameters_null_request(void);
void test_extract_and_validate_parameters_multiple_missing(void);

// ============================================================================
// Test Setup/Teardown
// ============================================================================

void setUp(void) {
}

void tearDown(void) {
}

// ============================================================================
// Helper to create a complete valid request
// ============================================================================

static json_t *create_valid_request(void) {
    json_t *request = json_object();
    json_object_set_new(request, "username", json_string("testuser"));
    json_object_set_new(request, "password", json_string("password123"));
    json_object_set_new(request, "email", json_string("test@example.com"));
    json_object_set_new(request, "api_key", json_string("key123"));
    json_object_set_new(request, "database", json_string("testdb"));
    return request;
}

// ============================================================================
// Test Functions
// ============================================================================

// Test: All required parameters present
void test_extract_and_validate_parameters_all_present(void) {
    json_t *request = create_valid_request();
    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("testuser", username);
    TEST_ASSERT_EQUAL_STRING("password123", password);
    TEST_ASSERT_EQUAL_STRING("test@example.com", email);
    TEST_ASSERT_EQUAL_STRING("key123", api_key);
    TEST_ASSERT_EQUAL_STRING("testdb", database);

    json_decref(request);
}

// Test: NULL output pointers (covers line 49: return false)
void test_extract_and_validate_parameters_null_output_pointers(void) {
    json_t *request = create_valid_request();
    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    // Pass NULL for username output pointer
    bool result = extract_and_validate_parameters(
        request, NULL, &password, &email, &full_name, &api_key, &database
    );
    TEST_ASSERT_FALSE(result);

    // Pass NULL for password output pointer
    result = extract_and_validate_parameters(
        request, &username, NULL, &email, &full_name, &api_key, &database
    );
    TEST_ASSERT_FALSE(result);

    // Pass NULL for email output pointer
    result = extract_and_validate_parameters(
        request, &username, &password, NULL, &full_name, &api_key, &database
    );
    TEST_ASSERT_FALSE(result);

    // Pass NULL for api_key output pointer
    result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, NULL, &database
    );
    TEST_ASSERT_FALSE(result);

    // Pass NULL for database output pointer
    result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, NULL
    );
    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Missing username (covers line 59: return false)
void test_extract_and_validate_parameters_missing_username(void) {
    json_t *request = create_valid_request();
    json_object_del(request, "username");

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Missing password
void test_extract_and_validate_parameters_missing_password(void) {
    json_t *request = create_valid_request();
    json_object_del(request, "password");

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Missing email
void test_extract_and_validate_parameters_missing_email(void) {
    json_t *request = create_valid_request();
    json_object_del(request, "email");

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Missing api_key
void test_extract_and_validate_parameters_missing_api_key(void) {
    json_t *request = create_valid_request();
    json_object_del(request, "api_key");

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Missing database
void test_extract_and_validate_parameters_missing_database(void) {
    json_t *request = create_valid_request();
    json_object_del(request, "database");

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// Test: Optional full_name is NULL (not present in request)
void test_extract_and_validate_parameters_optional_full_name_null(void) {
    json_t *request = create_valid_request();
    // full_name is not set in create_valid_request

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(username);
    TEST_ASSERT_NOT_NULL(password);
    TEST_ASSERT_NOT_NULL(email);
    TEST_ASSERT_NOT_NULL(api_key);
    TEST_ASSERT_NOT_NULL(database);
    // full_name should be NULL since it's optional and not present
    TEST_ASSERT_NULL(full_name);

    json_decref(request);
}

// Test: Optional full_name is present
void test_extract_and_validate_parameters_optional_full_name_present(void) {
    json_t *request = create_valid_request();
    json_object_set_new(request, "full_name", json_string("Test User"));

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test User", full_name);

    json_decref(request);
}

// Test: NULL request
void test_extract_and_validate_parameters_null_request(void) {
    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        NULL, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);
}

// Test: Multiple parameters missing
void test_extract_and_validate_parameters_multiple_missing(void) {
    json_t *request = json_object();
    json_object_set_new(request, "username", json_string("testuser"));
    // Missing password, email, api_key, database

    const char *username = NULL, *password = NULL, *email = NULL;
    const char *full_name = NULL, *api_key = NULL, *database = NULL;

    bool result = extract_and_validate_parameters(
        request, &username, &password, &email, &full_name, &api_key, &database
    );

    TEST_ASSERT_FALSE(result);

    json_decref(request);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_extract_and_validate_parameters_all_present);
    RUN_TEST(test_extract_and_validate_parameters_null_output_pointers);
    RUN_TEST(test_extract_and_validate_parameters_missing_username);
    RUN_TEST(test_extract_and_validate_parameters_missing_password);
    RUN_TEST(test_extract_and_validate_parameters_missing_email);
    RUN_TEST(test_extract_and_validate_parameters_missing_api_key);
    RUN_TEST(test_extract_and_validate_parameters_missing_database);
    RUN_TEST(test_extract_and_validate_parameters_optional_full_name_null);
    RUN_TEST(test_extract_and_validate_parameters_optional_full_name_present);
    RUN_TEST(test_extract_and_validate_parameters_null_request);
    RUN_TEST(test_extract_and_validate_parameters_multiple_missing);

    return UNITY_END();
}
