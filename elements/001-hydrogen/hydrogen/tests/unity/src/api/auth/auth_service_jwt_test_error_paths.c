/*
 * Unity Unit Tests for auth_service_jwt.c - Error Path Testing
 *
 * Tests error conditions and failure paths in JWT functions
 *
 * CHANGELOG:
 * 2026-07-26: Updated with call-counter-based mock failures for precise
 *             error-path coverage; added no-database and signature-mismatch
 *             tests for validate_jwt; added generate_new_jwt error paths.
 * 2026-01-15: Initial version - Tests for JWT error paths using mocks
 *
 * TEST_VERSION: 1.1.0
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_crypto.h>

// Include necessary headers for the module being tested
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>
#include <src/api/auth/auth_service_database.h>
#include <string.h>
#include <time.h>

// Function prototypes for test functions
void test_generate_jwt_random_bytes_failure(void);
void test_generate_jwt_config_failure(void);
void test_generate_jwt_header_asprintf_failure(void);
void test_generate_jwt_payload_asprintf_failure(void);
void test_generate_jwt_header_encoding_failure(void);
void test_generate_jwt_payload_encoding_failure(void);
void test_generate_jwt_signing_input_asprintf_failure(void);
void test_generate_jwt_signature_encoding_failure(void);
void test_generate_jwt_final_jwt_asprintf_failure(void);
void test_validate_jwt_no_database_in_token(void);
void test_validate_jwt_signature_mismatch(void);
void test_validate_jwt_config_failure(void);
void test_validate_jwt_signing_input_asprintf_failure(void);
void test_validate_jwt_claims_allocation_failure(void);
void test_generate_new_jwt_missing_secret(void);
void test_generate_new_jwt_header_asprintf_failure(void);
void test_generate_new_jwt_payload_asprintf_failure(void);
void test_generate_new_jwt_header_encoding_failure(void);
void test_generate_new_jwt_payload_encoding_failure(void);
void test_generate_new_jwt_signing_input_asprintf_failure(void);
void test_generate_new_jwt_signature_encoding_failure(void);
void test_generate_new_jwt_final_jwt_asprintf_failure(void);
void test_compute_token_hash(void);
void test_compute_token_hash_null(void);
void test_compute_password_hash(void);
void test_get_jwt_config(void);
void test_validate_jwt_expired(void);
void test_validate_jwt_for_logout(void);
void test_free_functions(void);
void test_generate_jwt_null_parameters(void);
void test_validate_jwt_null_token(void);
void test_validate_jwt_payload_decode_failure(void);
void test_validate_jwt_invalid_json_payload(void);
void test_validate_jwt_missing_exp_field(void);
void test_generate_new_jwt_null_claims(void);

// Helper function prototypes
account_info_t* create_test_account(void);
system_info_t* create_test_system(void);
void free_test_account(account_info_t* account);
void free_test_system(system_info_t* system);

// Helper: test-seam query function that returns a "token is active" result
static QueryResult* seam_query_active(int query_ref, const char* database, json_t* params) {
    (void)query_ref;
    (void)database;
    (void)params;

    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) return NULL;
    result->success = true;
    result->row_count = 1;
    result->data_json = strdup("[]");
    return result;
}

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
    mock_system_reset_all();
    mock_crypto_reset_all();
    auth_service_database_test_clear_query_fn();
}

void tearDown(void) {
    auth_service_database_test_clear_query_fn();
}

// ============================================================
// generate_jwt_with_oidc error paths
// ============================================================

/* Test: generate_jwt fails when random bytes generation fails */
void test_generate_jwt_random_bytes_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_crypto_set_random_bytes_failure(1);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when config retrieval fails */
void test_generate_jwt_config_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_system_set_malloc_failure(1);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when header asprintf fails (line 118) */
void test_generate_jwt_header_asprintf_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_system_set_asprintf_failure(1);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when payload asprintf fails (line 142) */
void test_generate_jwt_payload_asprintf_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_system_set_asprintf_failure(2);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when header encoding fails (line 172) */
void test_generate_jwt_header_encoding_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    // 1st encode = jti, 2nd encode = header
    mock_crypto_set_base64url_encode_failure(2);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when payload encoding fails (line 173) */
void test_generate_jwt_payload_encoding_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    // 1st encode = jti, 2nd = header, 3rd = payload
    mock_crypto_set_base64url_encode_failure(3);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when signing input asprintf fails (line 188) */
void test_generate_jwt_signing_input_asprintf_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_system_set_asprintf_failure(3);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when signature encoding fails (line 212) */
void test_generate_jwt_signature_encoding_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    // 1st = jti, 2nd = header, 3rd = payload, 4th = signature
    mock_crypto_set_base64url_encode_failure(4);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: generate_jwt fails when final JWT asprintf fails (line 224) */
void test_generate_jwt_final_jwt_asprintf_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    mock_system_set_asprintf_failure(4);

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));

    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

// ============================================================
// validate_jwt error paths
// ============================================================

/* Test: validate_jwt with no database in token and NULL database param (lines 361-364) */
void test_validate_jwt_no_database_in_token(void) {
    // Build a JWT manually: valid header + payload with exp but no database field
    const char* header_b64 = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    // {"sub":"123","exp":<future>,"ip":"1.2.3.4"} - no "database" field
    time_t future = time(NULL) + 3600;
    char payload_json[256];
    snprintf(payload_json, sizeof(payload_json),
             "{\"sub\":\"123\",\"exp\":%ld,\"ip\":\"1.2.3.4\"}", (long)future);

    // Base64url-encode the payload
    size_t plen = strlen(payload_json);
    char* payload_b64 = utils_base64url_encode((const unsigned char*)payload_json, plen);
    TEST_ASSERT_NOT_NULL(payload_b64);

    // Dummy signature (doesn't matter - we never reach signature verification)
    const char* dummy_sig = "dummysignature";

    char* jwt = NULL;
    asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, dummy_sig);
    free(payload_b64);
    TEST_ASSERT_NOT_NULL(jwt);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);

    free(jwt);
}

/* Test: validate_jwt with signature mismatch (lines 401-406) */
void test_validate_jwt_signature_mismatch(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NOT_NULL(jwt);

    // Modify the signature
    char* sig_start = strrchr(jwt, '.');
    TEST_ASSERT_NOT_NULL(sig_start);
    if (sig_start[1]) {
        sig_start[1] = (sig_start[1] == 'A') ? 'B' : 'A';
    }

    // Use test seam so is_token_revoked returns false (token is "active")
    auth_service_database_test_set_query_fn(seam_query_active);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_SIGNATURE, result.error);

    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test: validate_jwt when get_jwt_config fails (lines 370-372) */
void test_validate_jwt_config_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NOT_NULL(jwt);

    // Use test seam so is_token_revoked returns false
    auth_service_database_test_set_query_fn(seam_query_active);

    // strdup(token)=1, calloc(seam)=2, strdup(seam)=3, calloc(config)=4
    mock_system_set_malloc_failure(4);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_SIGNATURE, result.error);

    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test: validate_jwt signing input asprintf failure (lines 378-381) */
void test_validate_jwt_signing_input_asprintf_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NOT_NULL(jwt);

    // Use test seam so is_token_revoked returns false
    auth_service_database_test_set_query_fn(seam_query_active);

    // First asprintf in validate_jwt is the signing input
    mock_system_set_asprintf_failure(1);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_SIGNATURE, result.error);

    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

/* Test: validate_jwt claims allocation failure (lines 418-422) */
void test_validate_jwt_claims_allocation_failure(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NOT_NULL(jwt);

    // Use test seam so is_token_revoked returns false
    auth_service_database_test_set_query_fn(seam_query_active);

    // strdup(token)=1, calloc(seam)=2, strdup(seam)=3, calloc(config)=4,
    // strdup(secret)=5, calloc(claims)=6
    mock_system_set_malloc_failure(6);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);

    free(jwt);
    free_test_account(account);
    free_test_system(system);
}

// ============================================================
// generate_new_jwt error paths
// ============================================================

/* Test: generate_new_jwt fails when config missing secret (lines 541-543) */
void test_generate_new_jwt_missing_secret(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    // calloc(config)=1, strdup(secret)=2 → fails, config->hmac_secret is NULL
    mock_system_set_malloc_failure(2);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when header asprintf fails (line 548) */
void test_generate_new_jwt_header_asprintf_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    mock_system_set_asprintf_failure(1);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when payload asprintf fails (line 570) */
void test_generate_new_jwt_payload_asprintf_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    mock_system_set_asprintf_failure(2);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when header encoding fails (line 610) */
void test_generate_new_jwt_header_encoding_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    // 1st = jti, 2nd = header
    mock_crypto_set_base64url_encode_failure(2);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when payload encoding fails (line 611) */
void test_generate_new_jwt_payload_encoding_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    // 1st = jti, 2nd = header, 3rd = payload
    mock_crypto_set_base64url_encode_failure(3);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when signing input asprintf fails (line 623) */
void test_generate_new_jwt_signing_input_asprintf_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    mock_system_set_asprintf_failure(3);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when signature encoding fails (line 647) */
void test_generate_new_jwt_signature_encoding_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    // 1st = jti, 2nd = header, 3rd = payload, 4th = signature
    mock_crypto_set_base64url_encode_failure(4);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

/* Test: generate_new_jwt fails when final JWT asprintf fails (line 664) */
void test_generate_new_jwt_final_jwt_asprintf_failure(void) {
    jwt_claims_t claims = {0};
    claims.user_id = 123;
    claims.username = strdup("testuser");

    mock_system_set_asprintf_failure(4);

    char* jwt = generate_new_jwt(&claims);

    TEST_ASSERT_NULL(jwt);

    free(claims.username);
}

// ============================================================
// Existing tests (kept for regression)
// ============================================================

/* Test: compute_token_hash works correctly */
void test_compute_token_hash(void) {
    const char* token = "test.jwt.token";
    char* hash = compute_token_hash(token);

    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);

    free(hash);
}

/* Test: compute_token_hash handles NULL */
void test_compute_token_hash_null(void) {
    char* hash = compute_token_hash(NULL);
    TEST_ASSERT_NULL(hash);
}

/* Test: compute_password_hash works correctly */
void test_compute_password_hash(void) {
    const char* password = "testpassword";
    int account_id = 123;
    char* hash = compute_password_hash(password, account_id);

    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);

    free(hash);
}

/* Test: get_jwt_config returns valid config */
void test_get_jwt_config(void) {
    jwt_config_t* config = get_jwt_config();

    TEST_ASSERT_NOT_NULL(config);
    TEST_ASSERT_NOT_NULL(config->hmac_secret);
    TEST_ASSERT_TRUE(strlen(config->hmac_secret) > 0);
    TEST_ASSERT_FALSE(config->use_rsa);
    TEST_ASSERT_EQUAL(90, config->rotation_interval_days);

    free_jwt_config(config);
}

/* Test: validate_jwt expired token */
void test_validate_jwt_expired(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    time_t past = time(NULL) - 3601;
    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "testdb", past);

    TEST_ASSERT_NOT_NULL(jwt);

    jwt_validation_result_t result = validate_jwt(jwt, NULL);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_EXPIRED, result.error);

    free(jwt);
    free_jwt_validation_result(&result);
    free_test_account(account);
    free_test_system(system);
}

/* Test: validate_jwt_for_logout allows expired tokens */
void test_validate_jwt_for_logout(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    time_t past = time(NULL) - 3601;
    char* jwt = generate_jwt(account, system, "192.168.1.1", "UTC", "testdb", past);

    TEST_ASSERT_NOT_NULL(jwt);

    jwt_validation_result_t result = validate_jwt_for_logout(jwt, NULL);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_NONE, result.error);

    free(jwt);
    free_jwt_validation_result(&result);
    free_test_account(account);
    free_test_system(system);
}

/* Test: free functions work correctly */
void test_free_functions(void) {
    jwt_config_t* config = get_jwt_config();
    TEST_ASSERT_NOT_NULL(config);
    free_jwt_config(config);

    jwt_claims_t* claims = calloc(1, sizeof(jwt_claims_t));
    if (claims) {
        claims->username = strdup("test");
        claims->email = strdup("test@example.com");
    }
    free_jwt_claims(claims);

    jwt_validation_result_t result = {0};
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        result.claims->username = strdup("test");
    }
    free_jwt_validation_result(&result);
}

/* Test: generate_jwt null parameter validation */
void test_generate_jwt_null_parameters(void) {
    account_info_t* account = create_test_account();
    system_info_t* system = create_test_system();

    char* jwt = generate_jwt(NULL, system, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NULL(jwt);

    jwt = generate_jwt(account, NULL, "192.168.1.1", "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NULL(jwt);

    jwt = generate_jwt(account, system, NULL, "UTC", "Acuranzo", time(NULL));
    TEST_ASSERT_NULL(jwt);

    jwt = generate_jwt(account, system, "192.168.1.1", NULL, "Acuranzo", time(NULL));
    TEST_ASSERT_NULL(jwt);

    jwt = generate_jwt(account, system, "192.168.1.1", "UTC", NULL, time(NULL));
    TEST_ASSERT_NULL(jwt);

    free_test_account(account);
    free_test_system(system);
}

/* Test: validate_jwt null token */
void test_validate_jwt_null_token(void) {
    jwt_validation_result_t result = validate_jwt(NULL, "Acuranzo");

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test: validate_jwt payload decode failure */
void test_validate_jwt_payload_decode_failure(void) {
    const char* invalid_jwt = "header. invalid_base64_payload .signature";

    jwt_validation_result_t result = validate_jwt(invalid_jwt, "Acuranzo");

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);
}

/* Test: validate_jwt invalid JSON payload */
void test_validate_jwt_invalid_json_payload(void) {
    const char* header = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    const char* invalid_payload = "eyJpbnZhbGlkIGpzb24";
    const char* signature = "signature";

    char* invalid_jwt = NULL;
    asprintf(&invalid_jwt, "%s.%s.%s", header, invalid_payload, signature);

    jwt_validation_result_t result = validate_jwt(invalid_jwt, "Acuranzo");

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);

    free(invalid_jwt);
}

/* Test: validate_jwt missing exp field */
void test_validate_jwt_missing_exp_field(void) {
    const char* header = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    const char* payload_no_exp = "eyJzdWIiOiIxMjMifQ";
    const char* signature = "signature";

    char* invalid_jwt = NULL;
    asprintf(&invalid_jwt, "%s.%s.%s", header, payload_no_exp, signature);

    jwt_validation_result_t result = validate_jwt(invalid_jwt, "Acuranzo");

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL(JWT_ERROR_INVALID_FORMAT, result.error);

    free(invalid_jwt);
}

/* Test: generate_new_jwt null claims */
void test_generate_new_jwt_null_claims(void) {
    char* jwt = generate_new_jwt(NULL);
    TEST_ASSERT_NULL(jwt);
}

/* Main test runner */
int main(void) {
    UNITY_BEGIN();

    // generate_jwt_with_oidc error paths
    RUN_TEST(test_generate_jwt_random_bytes_failure);
    RUN_TEST(test_generate_jwt_config_failure);
    RUN_TEST(test_generate_jwt_header_asprintf_failure);
    RUN_TEST(test_generate_jwt_payload_asprintf_failure);
    RUN_TEST(test_generate_jwt_header_encoding_failure);
    RUN_TEST(test_generate_jwt_payload_encoding_failure);
    RUN_TEST(test_generate_jwt_signing_input_asprintf_failure);
    RUN_TEST(test_generate_jwt_signature_encoding_failure);
    RUN_TEST(test_generate_jwt_final_jwt_asprintf_failure);

    // validate_jwt error paths
    RUN_TEST(test_validate_jwt_no_database_in_token);
    RUN_TEST(test_validate_jwt_signature_mismatch);
    RUN_TEST(test_validate_jwt_config_failure);
    RUN_TEST(test_validate_jwt_signing_input_asprintf_failure);
    RUN_TEST(test_validate_jwt_claims_allocation_failure);

    // generate_new_jwt error paths
    RUN_TEST(test_generate_new_jwt_missing_secret);
    RUN_TEST(test_generate_new_jwt_header_asprintf_failure);
    RUN_TEST(test_generate_new_jwt_payload_asprintf_failure);
    RUN_TEST(test_generate_new_jwt_header_encoding_failure);
    RUN_TEST(test_generate_new_jwt_payload_encoding_failure);
    RUN_TEST(test_generate_new_jwt_signing_input_asprintf_failure);
    RUN_TEST(test_generate_new_jwt_signature_encoding_failure);
    RUN_TEST(test_generate_new_jwt_final_jwt_asprintf_failure);

    // Existing tests
    RUN_TEST(test_compute_token_hash);
    RUN_TEST(test_compute_token_hash_null);
    RUN_TEST(test_compute_password_hash);
    RUN_TEST(test_get_jwt_config);
    RUN_TEST(test_validate_jwt_expired);
    RUN_TEST(test_validate_jwt_for_logout);
    RUN_TEST(test_free_functions);
    RUN_TEST(test_generate_jwt_null_parameters);
    RUN_TEST(test_validate_jwt_null_token);
    RUN_TEST(test_validate_jwt_payload_decode_failure);
    RUN_TEST(test_validate_jwt_invalid_json_payload);
    RUN_TEST(test_validate_jwt_missing_exp_field);
    RUN_TEST(test_generate_new_jwt_null_claims);

    return UNITY_END();
}
