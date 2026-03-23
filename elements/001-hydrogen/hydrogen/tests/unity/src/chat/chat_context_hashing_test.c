/*
 * Unity Tests for Chat Context Hashing Module (Phase 8)
 *
 * Tests the context hashing functionality for client-server bandwidth optimization.
 * Includes tests for hash generation, validation, parsing, and bandwidth calculation.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>

#include <src/api/conduit/chat_common/chat_context_hashing.h>
#include <src/api/conduit/chat_common/chat_storage.h>

// Forward declarations for internal functions tested via public API
char* chat_context_hash_content(const char* content, size_t content_length);
char* chat_context_hash_json(const char* json_string);
bool chat_context_validate_hash(const char* hash);
double chat_context_calculate_bandwidth_savings(const ContextHashResult* result, size_t original_request_size);
size_t chat_context_estimate_size_savings(json_t* messages, size_t hash_count);
void chat_context_free_hash(char* hash);
void chat_context_free_hash_array(char** hashes, size_t count);
void chat_context_free_result(ContextHashResult* result);

// Test function declarations
void test_context_hash_content_basic(void);
void test_context_hash_content_null(void);
void test_context_hash_content_empty(void);
void test_context_hash_json_basic(void);
void test_context_hash_json_null(void);
void test_context_hash_consistency(void);
void test_context_hash_uniqueness(void);
void test_context_validate_hash_valid(void);
void test_context_validate_hash_invalid_length(void);
void test_context_validate_hash_invalid_chars(void);
void test_context_validate_hash_null(void);
void test_context_parse_request_hashes_valid(void);
void test_context_parse_request_hashes_missing(void);
void test_context_parse_request_hashes_invalid_type(void);
void test_context_parse_request_hashes_invalid_format(void);
void test_context_parse_request_hashes_null(void);
void test_context_calculate_bandwidth_savings_basic(void);
void test_context_calculate_bandwidth_savings_no_savings(void);
void test_context_calculate_bandwidth_savings_null(void);
void test_context_estimate_size_savings_basic(void);
void test_context_estimate_size_savings_null(void);
void test_context_free_functions(void);
void test_context_resolve_hashes_null_params(void);

// Test data
static const char* test_content = "Hello, this is a test message for context hashing.";
static const char* test_content_2 = "A different message for uniqueness testing.";
static const char* test_json = "{\"role\": \"user\", \"content\": \"Hello, World!\"}";
static const char* test_database = "testdb";

void setUp(void) {
}

void tearDown(void) {
}

void test_context_hash_content_basic(void) {
    char* hash = chat_context_hash_content(test_content, strlen(test_content));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_INT(43, (int)strlen(hash));  // base64url SHA-256 (without padding) is 43 chars
    chat_context_free_hash(hash);
}

void test_context_hash_content_null(void) {
    char* hash = chat_context_hash_content(NULL, 100);
    TEST_ASSERT_NULL(hash);
}

void test_context_hash_content_empty(void) {
    char* hash = chat_context_hash_content("", 0);
    TEST_ASSERT_NULL(hash);
}

void test_context_hash_json_basic(void) {
    char* hash = chat_context_hash_json(test_json);
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_EQUAL_INT(43, (int)strlen(hash));
    chat_context_free_hash(hash);
}

void test_context_hash_json_null(void) {
    char* hash = chat_context_hash_json(NULL);
    TEST_ASSERT_NULL(hash);
}

void test_context_hash_consistency(void) {
    char* hash1 = chat_context_hash_content(test_content, strlen(test_content));
    char* hash2 = chat_context_hash_content(test_content, strlen(test_content));
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);
    chat_context_free_hash(hash1);
    chat_context_free_hash(hash2);
}

void test_context_hash_uniqueness(void) {
    char* hash1 = chat_context_hash_content(test_content, strlen(test_content));
    char* hash2 = chat_context_hash_content(test_content_2, strlen(test_content_2));
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_FALSE(strcmp(hash1, hash2) == 0);
    chat_context_free_hash(hash1);
    chat_context_free_hash(hash2);
}

void test_context_validate_hash_valid(void) {
    // Generate a valid hash and validate it
    char* hash = chat_context_hash_content(test_content, strlen(test_content));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(chat_context_validate_hash(hash));
    chat_context_free_hash(hash);
}

void test_context_validate_hash_invalid_length(void) {
    TEST_ASSERT_FALSE(chat_context_validate_hash("tooshort"));
    TEST_ASSERT_FALSE(chat_context_validate_hash("this_is_a_way_too_long_hash_value_that_is_not_valid_for_sha256"));
}

void test_context_validate_hash_invalid_chars(void) {
    // Valid length but invalid characters
    char invalid_hash[45];
    memset(invalid_hash, 'A', 44);
    invalid_hash[10] = '+';  // Invalid base64url char
    invalid_hash[44] = '\0';
    TEST_ASSERT_FALSE(chat_context_validate_hash(invalid_hash));

    memset(invalid_hash, 'A', 44);
    invalid_hash[20] = '/';
    invalid_hash[44] = '\0';
    TEST_ASSERT_FALSE(chat_context_validate_hash(invalid_hash));
}

void test_context_validate_hash_null(void) {
    TEST_ASSERT_FALSE(chat_context_validate_hash(NULL));
}

void test_context_parse_request_hashes_valid(void) {
    json_t* request = json_object();
    json_t* hashes = json_array();
    // Use valid 43-character base64url strings (no padding)
    json_array_append_new(hashes, json_string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq"));
    json_array_append_new(hashes, json_string("1234567890ABCDEFGHIJKLMNOPQRSTUVWXabcdefghi"));
    json_object_set_new(request, "context_hashes", hashes);

    char* error_message = NULL;
    char** result = chat_context_parse_request_hashes(request, &error_message);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NULL(error_message);

    // Verify hashes were parsed
    TEST_ASSERT_NOT_NULL(result[0]);
    TEST_ASSERT_NOT_NULL(result[1]);
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopq", result[0]);
    TEST_ASSERT_EQUAL_STRING("1234567890ABCDEFGHIJKLMNOPQRSTUVWXabcdefghi", result[1]);

    // Cleanup
    chat_context_free_hash_array(result, 2);
    json_decref(request);
}

void test_context_parse_request_hashes_missing(void) {
    json_t* request = json_object();
    json_object_set_new(request, "messages", json_array());

    char* error_message = NULL;
    char** result = chat_context_parse_request_hashes(request, &error_message);

    // No hashes is valid - returns NULL without error
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(error_message);
    json_decref(request);
}

void test_context_parse_request_hashes_invalid_type(void) {
    json_t* request = json_object();
    json_object_set_new(request, "context_hashes", json_string("not_an_array"));

    char* error_message = NULL;
    char** result = chat_context_parse_request_hashes(request, &error_message);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(error_message);
    TEST_ASSERT_TRUE(strstr(error_message, "array") != NULL);

    free(error_message);
    json_decref(request);
}

void test_context_parse_request_hashes_invalid_format(void) {
    json_t* request = json_object();
    json_t* hashes = json_array();
    json_array_append_new(hashes, json_string("invalid_hash_too_short"));
    json_array_append_new(hashes, json_integer(123));  // Not a string
    json_object_set_new(request, "context_hashes", hashes);

    char* error_message = NULL;
    char** result = chat_context_parse_request_hashes(request, &error_message);

    // Invalid hashes are skipped, returns NULL if none valid
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(error_message);  // No error for skipped invalid entries
    json_decref(request);
}

void test_context_parse_request_hashes_null(void) {
    char* error_message = NULL;
    char** result = chat_context_parse_request_hashes(NULL, &error_message);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(error_message);
    free(error_message);
}

void test_context_calculate_bandwidth_savings_basic(void) {
    // Create a mock result with savings
    ContextHashResult result = {0};
    result.hashes_found = 5;
    result.total_content_size = 5000;  // 5000 bytes of content
    result.bandwidth_saved_bytes = 4700;  // Hash overhead ~300 bytes
    result.bandwidth_saved_percent = 94.0;

    double savings = chat_context_calculate_bandwidth_savings(&result, 5000);
    TEST_ASSERT_TRUE(savings > 0.0);
    TEST_ASSERT_TRUE(savings <= 100.0);
}

void test_context_calculate_bandwidth_savings_no_savings(void) {
    ContextHashResult result = {0};
    result.hashes_found = 0;
    result.total_content_size = 0;

    double savings = chat_context_calculate_bandwidth_savings(&result, 5000);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, savings);
}

void test_context_calculate_bandwidth_savings_null(void) {
    double savings = chat_context_calculate_bandwidth_savings(NULL, 5000);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, savings);

    ContextHashResult result = {0};
    savings = chat_context_calculate_bandwidth_savings(&result, 0);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, savings);
}

void test_context_estimate_size_savings_basic(void) {
    json_t* messages = json_array();
    json_t* msg = json_object();
    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string("Hello, World! This is a longer message to test bandwidth savings calculation."));
    json_array_append_new(messages, msg);

    size_t savings = chat_context_estimate_size_savings(messages, 1);
    // Should estimate some savings when replacing with hash
    TEST_ASSERT_TRUE(savings > 0);

    json_decref(messages);
}

void test_context_estimate_size_savings_null(void) {
    size_t savings = chat_context_estimate_size_savings(NULL, 1);
    TEST_ASSERT_EQUAL_INT(0, (int)savings);

    json_t* messages = json_array();
    savings = chat_context_estimate_size_savings(messages, 0);
    TEST_ASSERT_EQUAL_INT(0, (int)savings);
    json_decref(messages);
}

void test_context_free_functions(void) {
    // Test that free functions handle NULL gracefully
    chat_context_free_hash(NULL);
    chat_context_free_hash_array(NULL, 0);
    chat_context_free_result(NULL);

    // Test with actual allocation
    char* hash = strdup("test");
    TEST_ASSERT_NOT_NULL(hash);
    chat_context_free_hash(hash);

    char** hashes = calloc(2, sizeof(char*));
    TEST_ASSERT_NOT_NULL(hashes);
    hashes[0] = strdup("hash1");
    hashes[1] = strdup("hash2");
    chat_context_free_hash_array(hashes, 2);
}

void test_context_resolve_hashes_null_params(void) {
    const char* hashes[] = {"hash1", "hash2"};
    ContextHashResult* result = chat_context_resolve_hashes(NULL, hashes, 2);
    TEST_ASSERT_NULL(result);

    result = chat_context_resolve_hashes(test_database, NULL, 2);
    TEST_ASSERT_NULL(result);

    result = chat_context_resolve_hashes(test_database, hashes, 0);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();

    // Hash generation tests
    RUN_TEST(test_context_hash_content_basic);
    RUN_TEST(test_context_hash_content_null);
    RUN_TEST(test_context_hash_content_empty);
    RUN_TEST(test_context_hash_json_basic);
    RUN_TEST(test_context_hash_json_null);
    RUN_TEST(test_context_hash_consistency);
    RUN_TEST(test_context_hash_uniqueness);

    // Hash validation tests
    RUN_TEST(test_context_validate_hash_valid);
    RUN_TEST(test_context_validate_hash_invalid_length);
    RUN_TEST(test_context_validate_hash_invalid_chars);
    RUN_TEST(test_context_validate_hash_null);

    // Request parsing tests
    RUN_TEST(test_context_parse_request_hashes_valid);
    RUN_TEST(test_context_parse_request_hashes_missing);
    RUN_TEST(test_context_parse_request_hashes_invalid_type);
    RUN_TEST(test_context_parse_request_hashes_invalid_format);
    RUN_TEST(test_context_parse_request_hashes_null);

    // Bandwidth calculation tests
    RUN_TEST(test_context_calculate_bandwidth_savings_basic);
    RUN_TEST(test_context_calculate_bandwidth_savings_no_savings);
    RUN_TEST(test_context_calculate_bandwidth_savings_null);
    RUN_TEST(test_context_estimate_size_savings_basic);
    RUN_TEST(test_context_estimate_size_savings_null);

    // Free function tests
    RUN_TEST(test_context_free_functions);

    // Resolve hashes tests
    RUN_TEST(test_context_resolve_hashes_null_params);

    return UNITY_END();
}
