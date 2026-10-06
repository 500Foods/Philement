/*
 * Unity Test File: GET /api/nats/status
 *
 * Method, JWT, and the two 200 shapes. The MHD mock drops the body,
 * so the JSON is checked through nats_status_build_json. This test
 * does not start the retry thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>

#include <src/api/api_utils.h>
#include <src/api/nats/status/status.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>

static AppConfig cfg;
static AppConfig *saved_config;

void test_status_wrong_method(void);
void test_status_missing_auth(void);
void test_status_invalid_claims(void);
void test_status_disabled(void);
void test_status_enabled_degraded(void);

static void set_jwt_result(bool valid, jwt_error_t error, bool with_database) {
    jwt_validation_result_t result = {0};
    result.valid = valid;
    result.error = error;
    result.claims = calloc(1, sizeof(jwt_claims_t));
    if (result.claims) {
        if (with_database) {
            result.claims->database = strdup("testdb");
        }
        result.claims->roles = strdup("1");
        result.claims->email = strdup("user@example.com");
        result.claims->jti = strdup("request-123");
        result.claims->username = strdup("testuser");
        result.claims->user_id = 123;
    }
    mock_auth_service_jwt_set_validation_result(result);

    if (valid && result.claims) {
        free(result.claims->database);
        free(result.claims->roles);
        free(result.claims->email);
        free(result.claims->jti);
        free(result.claims->username);
        free(result.claims);
    }
}

static void assert_status_body(bool enabled, const char *link, const char *state,
                               int published) {
    NatsMetrics snap;
    json_t *body;

    nats_stats_collect(&snap);
    body = nats_status_build_json(&snap);
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(body, "success")));
    if (enabled) {
        TEST_ASSERT_TRUE(json_is_true(json_object_get(body, "enabled")));
    } else {
        TEST_ASSERT_TRUE(json_is_false(json_object_get(body, "enabled")));
    }
    TEST_ASSERT_EQUAL_STRING(link, json_string_value(json_object_get(body, "link")));
    TEST_ASSERT_EQUAL_STRING(state, json_string_value(json_object_get(body, "state")));
    TEST_ASSERT_EQUAL_STRING("none", json_string_value(json_object_get(body, "self")));
    TEST_ASSERT_EQUAL_INT(0, (int)json_integer_value(json_object_get(body, "peers")));
    TEST_ASSERT_EQUAL_INT(0, (int)json_integer_value(json_object_get(body, "alive")));
    TEST_ASSERT_EQUAL_INT(published,
        (int)json_integer_value(json_object_get(body, "published")));
    TEST_ASSERT_EQUAL_INT(0, (int)json_integer_value(json_object_get(body, "received")));
    TEST_ASSERT_EQUAL_INT(0, (int)json_integer_value(json_object_get(body, "reconnects")));
    json_decref(body);
}

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    app_config = &cfg;
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    mock_mhd_set_lookup_result("Bearer valid.token.here");
    nats_link_set(NATS_LINK_DOWN);
    nats_registry_reset();
    nats_stats_reset();
}

void tearDown(void) {
    nats_link_set(NATS_LINK_DOWN);
    nats_registry_reset();
    nats_stats_reset();
    app_config = saved_config;
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
}

void test_status_wrong_method(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result = handle_nats_status_request(conn, "/api/nats/status",
                                                        "POST", NULL, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_METHOD_NOT_ALLOWED, mock_mhd_get_last_status_code());
}

void test_status_missing_auth(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;

    mock_mhd_set_lookup_result(NULL);
    set_jwt_result(false, JWT_ERROR_INVALID_FORMAT, false);
    result = handle_nats_status_request(conn, "/api/nats/status",
                                        "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
}

void test_status_invalid_claims(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;

    set_jwt_result(true, JWT_ERROR_NONE, false);
    result = handle_nats_status_request(conn, "/api/nats/status",
                                        "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
}

void test_status_disabled(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;

    set_jwt_result(true, JWT_ERROR_NONE, true);
    result = handle_nats_status_request(conn, "/api/nats/status",
                                        "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    assert_status_body(false, "down", "down", 0);
}

void test_status_enabled_degraded(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;

    cfg.nats.Enabled = true;
    nats_link_set(NATS_LINK_DEGRADED);
    nats_stats_inc_published();
    set_jwt_result(true, JWT_ERROR_NONE, true);
    result = handle_nats_status_request(conn, "/api/nats/status",
                                        "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    assert_status_body(true, "degraded", "degraded", 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_status_wrong_method);
    RUN_TEST(test_status_missing_auth);
    RUN_TEST(test_status_invalid_claims);
    RUN_TEST(test_status_disabled);
    RUN_TEST(test_status_enabled_degraded);
    return UNITY_END();
}
