/*
 * Unity Test File: GET /api/nats/instances
 *
 * JWT, presence off, and one copied peer. The MHD mock drops the
 * body, so the JSON is checked through nats_instances_build_json.
 * This test does not start the retry thread and does not dial.
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
#include <src/api/nats/instances/instances.h>
#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>

#include "mock_logging.h"

static AppConfig cfg;
static AppConfig *saved_config;

void test_instances_missing_auth(void);
void test_instances_presence_off(void);
void test_instances_one_peer(void);

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

static json_t *make_peer(const char *id, const char *state, int connections) {
    json_t *root = json_object();
    json_t *body = json_object();

    if (!root || !body) {
        json_decref(root);
        json_decref(body);
        return NULL;
    }
    json_object_set_new(body, "state", json_string(state));
    if (connections >= 0) {
        json_object_set_new(body, "websocket_connections", json_integer(connections));
    }
    json_object_set_new(root, "event", json_string("app_state"));
    json_object_set_new(root, "subject",
                        json_string("cluster.philement.instance.app_state"));
    json_object_set_new(root, "timestamp", json_string("2026-10-05T00:00:00Z"));
    json_object_set_new(root, "source", json_string(id));
    json_object_set_new(root, "instance_id", json_string(id));
    json_object_set_new(root, "data", body);
    return root;
}

static void apply_peer(const char *id, const char *state, int connections) {
    json_t *root = make_peer(id, state, connections);

    TEST_ASSERT_NOT_NULL(root);
    nats_registry_apply(root);
    json_decref(root);
}

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_logging_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    mock_mhd_set_lookup_result("Bearer valid.token.here");
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    nats_stats_reset();
    nats_config_apply_defaults(&cfg.nats);
    app_config = &cfg;
}

void tearDown(void) {
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    nats_stats_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
}

void test_instances_missing_auth(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;

    mock_mhd_set_lookup_result(NULL);
    set_jwt_result(false, JWT_ERROR_INVALID_FORMAT, false);
    result = handle_nats_instances_request(conn, "/api/nats/instances",
                                           "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
}

void test_instances_presence_off(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;
    json_t *body;
    json_t *peers;

    cfg.nats.Presence.Enabled = true;
    apply_peer("peer-a", "Alive", 3);
    cfg.nats.Presence.Enabled = false;
    set_jwt_result(true, JWT_ERROR_NONE, true);
    mock_logging_reset_all();
    result = handle_nats_instances_request(conn, "/api/nats/instances",
                                           "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-a"));

    body = nats_instances_build_json();
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(body, "success")));
    TEST_ASSERT_TRUE(json_is_false(json_object_get(body, "singleton")));
    TEST_ASSERT_EQUAL_STRING("none", json_string_value(json_object_get(body, "self")));
    peers = json_object_get(body, "peers");
    TEST_ASSERT_NOT_NULL(peers);
    TEST_ASSERT_EQUAL_INT(0, (int)json_array_size(peers));
    json_decref(body);
}

void test_instances_one_peer(void) {
    struct MHD_Connection *conn = (struct MHD_Connection *)0x123;
    enum MHD_Result result;
    json_t *body;
    json_t *peers;
    json_t *peer;

    cfg.nats.Presence.Enabled = true;
    apply_peer("peer-a", "Alive", 3);
    set_jwt_result(true, JWT_ERROR_NONE, true);
    mock_logging_reset_all();
    result = handle_nats_instances_request(conn, "/api/nats/instances",
                                           "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(MHD_HTTP_OK, mock_mhd_get_last_status_code());
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-a"));

    body = nats_instances_build_json();
    TEST_ASSERT_NOT_NULL(body);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(body, "success")));
    TEST_ASSERT_TRUE(json_is_false(json_object_get(body, "singleton")));
    TEST_ASSERT_EQUAL_STRING("none", json_string_value(json_object_get(body, "self")));
    peers = json_object_get(body, "peers");
    TEST_ASSERT_EQUAL_INT(1, (int)json_array_size(peers));
    peer = json_array_get(peers, 0);
    TEST_ASSERT_EQUAL_STRING("peer-a", json_string_value(json_object_get(peer, "id")));
    TEST_ASSERT_EQUAL_STRING("Alive", json_string_value(json_object_get(peer, "state")));
    TEST_ASSERT_EQUAL_INT(3,
        (int)json_integer_value(json_object_get(peer, "websocket_connections")));
    json_decref(body);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_instances_missing_auth);
    RUN_TEST(test_instances_presence_off);
    RUN_TEST(test_instances_one_peer);
    return UNITY_END();
}
