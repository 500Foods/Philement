/*
 * Unity Test File: nats_registry_copy
 *
 * Snapshot of the peer table. This test does not start the retry
 * thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#include "mock_logging.h"

#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;

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

void test_nats_registry_copy_empty(void);
void test_nats_registry_copy_states(void);
void test_nats_registry_copy_cap_and_null(void);
void test_nats_registry_copy_snapshot_survives_reset(void);
void test_nats_registry_copy_stopping_removed(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    mock_logging_reset_all();
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Presence.Enabled = true;
    app_config = &cfg;
}

void tearDown(void) {
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_registry_copy_empty(void) {
    NatsRegistryCopy copied[4];

    TEST_ASSERT_EQUAL(0, nats_registry_copy(copied, 4));
    TEST_ASSERT_EQUAL(0, nats_registry_copy(NULL, 4));
    TEST_ASSERT_EQUAL(0, nats_registry_copy(copied, 0));
}

void test_nats_registry_copy_states(void) {
    NatsRegistryCopy copied[4];
    int n;

    apply_peer("peer-a", "Starting", -1);
    n = nats_registry_copy(copied, 4);
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_EQUAL_STRING("peer-a", copied[0].id);
    TEST_ASSERT_EQUAL_STRING("Starting", copied[0].state);
    TEST_ASSERT_EQUAL(0, copied[0].websocket_connections);

    apply_peer("peer-a", "Alive", 4);
    apply_peer("peer-b", "Alive", 2);
    mock_logging_reset_all();
    n = nats_registry_copy(copied, 4);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_STRING("peer-a", copied[0].id);
    TEST_ASSERT_EQUAL_STRING("Alive", copied[0].state);
    TEST_ASSERT_EQUAL(4, copied[0].websocket_connections);
    TEST_ASSERT_EQUAL_STRING("peer-b", copied[1].id);
    TEST_ASSERT_EQUAL_STRING("Alive", copied[1].state);
    TEST_ASSERT_EQUAL(2, copied[1].websocket_connections);
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-a"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("peer-b"));
}

void test_nats_registry_copy_cap_and_null(void) {
    NatsRegistryCopy copied[4];

    apply_peer("peer-a", "Alive", 1);
    apply_peer("peer-b", "Starting", -1);
    TEST_ASSERT_EQUAL(1, nats_registry_copy(copied, 1));
    TEST_ASSERT_EQUAL_STRING("peer-a", copied[0].id);
    TEST_ASSERT_EQUAL(0, nats_registry_copy(NULL, 2));
    TEST_ASSERT_EQUAL(2, nats_registry_peer_count());
}

void test_nats_registry_copy_snapshot_survives_reset(void) {
    NatsRegistryCopy copied[4];

    apply_peer("peer-a", "Alive", 3);
    TEST_ASSERT_EQUAL(1, nats_registry_copy(copied, 4));
    nats_registry_reset();
    TEST_ASSERT_EQUAL(0, nats_registry_peer_count());
    TEST_ASSERT_EQUAL_STRING("peer-a", copied[0].id);
    TEST_ASSERT_EQUAL_STRING("Alive", copied[0].state);
    TEST_ASSERT_EQUAL(3, copied[0].websocket_connections);
}

void test_nats_registry_copy_stopping_removed(void) {
    NatsRegistryCopy copied[4];

    apply_peer("peer-a", "Alive", 1);
    apply_peer("peer-a", "Stopping", -1);
    TEST_ASSERT_EQUAL(0, nats_registry_copy(copied, 4));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_registry_copy_empty);
    RUN_TEST(test_nats_registry_copy_states);
    RUN_TEST(test_nats_registry_copy_cap_and_null);
    RUN_TEST(test_nats_registry_copy_snapshot_survives_reset);
    RUN_TEST(test_nats_registry_copy_stopping_removed);
    return UNITY_END();
}
