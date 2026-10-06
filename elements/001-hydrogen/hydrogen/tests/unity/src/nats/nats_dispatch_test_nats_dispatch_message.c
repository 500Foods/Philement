/*
 * Unity Test File: nats_dispatch_message
 *
 * One incoming envelope. The socket is a fake NatsIo. This test does
 * not start the retry thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/dbqueue/query_result_cache.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#include "mock_logging.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const char *k_wire = "cluster.philement.cache.invalidate";

static AppConfig cfg;
static AppConfig *saved_config;
static char *held_json;
static char *held_frame;
static int connect_count;
static int got_count;
static DatabaseQueueManager *saved_manager;
static DatabaseQueueManager *test_manager;
static DatabaseQueue *test_queue;
static bool manager_installed;
static bool result_cache_used;
static char got_database[64];
static json_int_t got_ref;
static char got_reason[64];

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    connect_count++;
    return 0;
}

static int fake_read(void *ctx, void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static int fake_write(void *ctx, const void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static void fake_close(void *ctx) {
    (void)ctx;
}

static void install_fake(void) {
    NatsIo io;

    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = NULL;
    nats_io_install(&io);
}

static void prime_server(const char *instance_id) {
    nats_config_apply_defaults(&cfg.nats);
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup(instance_id);
    app_config = &cfg;
}

static void capture_invalidate(const char *database, json_int_t query_ref,
                               const char *reason) {
    got_count++;
    snprintf(got_database, sizeof(got_database), "%s", database ? database : "");
    got_ref = query_ref;
    snprintf(got_reason, sizeof(got_reason), "%s", reason ? reason : "");
}

static void release_fixture(void) {
    if (result_cache_used) {
        query_result_cache_clear(query_result_cache_get_global());
        result_cache_used = false;
    }
    if (!manager_installed) {
        return;
    }
    if (test_manager) {
        for (size_t i = 0; i < test_manager->max_databases; i++) {
            test_manager->databases[i] = NULL;
        }
        test_manager->database_count = 0;
        database_queue_manager_destroy(test_manager);
        test_manager = NULL;
    }
    if (test_queue) {
        query_cache_destroy(test_queue->query_cache, SR_NATS);
        free(test_queue->database_name);
        free(test_queue);
        test_queue = NULL;
    }
    global_queue_manager = saved_manager;
    saved_manager = NULL;
    manager_installed = false;
}

static void install_template(const char *sql) {
    saved_manager = global_queue_manager;
    test_manager = database_queue_manager_create(4);
    manager_installed = true;
    TEST_ASSERT_NOT_NULL(test_manager);
    global_queue_manager = test_manager;
    test_queue = calloc(1, sizeof(*test_queue));
    TEST_ASSERT_NOT_NULL(test_queue);
    test_queue->database_name = strdup("Acuranzo");
    TEST_ASSERT_NOT_NULL(test_queue->database_name);
    test_queue->query_cache = query_cache_create(SR_NATS);
    TEST_ASSERT_NOT_NULL(test_queue->query_cache);
    {
        QueryCacheEntry *entry = query_cache_entry_create(127, 1, sql, "cached read",
                                                          "cache", 30, SR_NATS);

        TEST_ASSERT_NOT_NULL(entry);
        TEST_ASSERT_TRUE(query_cache_add_entry(test_queue->query_cache, entry, SR_NATS));
    }
    TEST_ASSERT_TRUE(database_queue_manager_add_database(test_manager, test_queue));
    mock_logging_reset_all();
}

static bool put_row(const char *database, const char *sql, const char *params) {
    result_cache_used = true;
    return query_result_cache_put(query_result_cache_get_global(), database, sql, params,
                                  "[{\"id\":1}]", 1, 1, 0, 1);
}

static bool cached(const char *database, const char *sql, const char *params) {
    json_t *data = NULL;
    bool hit = query_result_cache_get(query_result_cache_get_global(), database, sql, params,
                                       &data, NULL, NULL, NULL, NULL);

    result_cache_used = true;
    json_decref(data);
    return hit;
}

static char *envelope_json_ref(const char *event, const char *subject,
                               const char *instance_id, json_int_t ref,
                               bool with_query_ref) {
    json_t *root = json_object();
    json_t *body = json_object();
    char *text;

    if (!root || !body) {
        json_decref(root);
        json_decref(body);
        return NULL;
    }
    json_object_set_new(body, "database", json_string("Acuranzo"));
    if (with_query_ref) {
        json_object_set_new(body, "query_ref", json_integer(ref));
    }
    json_object_set_new(body, "reason", json_string("mutation"));
    json_object_set_new(root, "event", json_string(event));
    json_object_set_new(root, "subject", json_string(subject));
    json_object_set_new(root, "timestamp", json_string("2026-10-05T00:00:00Z"));
    json_object_set_new(root, "source", json_string(instance_id));
    json_object_set_new(root, "instance_id", json_string(instance_id));
    json_object_set_new(root, "data", body);
    text = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return text;
}

static char *envelope_json(const char *event, const char *subject,
                           const char *instance_id, bool with_query_ref) {
    return envelope_json_ref(event, subject, instance_id, 127, with_query_ref);
}

static char *build_frame(const char *msg_subject, const char *payload, size_t *out_len) {
    size_t payload_len = strlen(payload);
    size_t cap = strlen(msg_subject) + payload_len + 64;
    char *frame = malloc(cap);
    int written;

    if (!frame) {
        return NULL;
    }
    written = snprintf(frame, cap, "MSG %s 1 %zu\r\n", msg_subject, payload_len);
    if (written < 0 || (size_t)written + payload_len + 2 >= cap) {
        free(frame);
        return NULL;
    }
    memcpy(frame + (size_t)written, payload, payload_len);
    memcpy(frame + (size_t)written + payload_len, "\r\n", 2);
    *out_len = (size_t)written + payload_len + 2;
    return frame;
}

static int feed_event(const char *msg_subject, const char *event,
                      const char *env_subject, const char *instance_id,
                      bool with_query_ref) {
    size_t len = 0;
    char *json;
    char *frame;
    int rc;

    json = envelope_json(event, env_subject, instance_id, with_query_ref);
    free(held_json);
    held_json = json;
    if (!json) {
        return -1;
    }
    frame = build_frame(msg_subject, json, &len);
    free(held_frame);
    held_frame = frame;
    if (!frame) {
        return -1;
    }
    rc = nats_parser_feed(frame, len);
    return rc;
}

static int feed_event_ref(json_int_t ref) {
    size_t len = 0;
    char *json = envelope_json_ref("cache.invalidate_by_ref", k_wire,
                                   "hydrogen-02", ref, true);

    free(held_json);
    held_json = json;
    if (!json) {
        return -1;
    }
    {
        char *frame = build_frame(k_wire, json, &len);

        free(held_frame);
        held_frame = frame;
        if (!frame) {
            return -1;
        }
        return nats_parser_feed(frame, len);
    }
}

void test_nats_dispatch_message_peer(void);
void test_nats_dispatch_message_skip_self(void);
void test_nats_dispatch_message_empty_instance(void);
void test_nats_dispatch_message_bad_json(void);
void test_nats_dispatch_message_ignores_app_state(void);
void test_nats_dispatch_message_bad_data(void);
void test_nats_dispatch_message_subject_mismatch(void);
void test_nats_dispatch_message_log_omits_payload(void);
void test_nats_dispatch_message_evicts_template(void);
void test_nats_dispatch_message_skip_self_keeps_rows(void);
void test_nats_dispatch_message_drops_wide_ref(void);

void setUp(void) {
    saved_config = app_config;
    held_json = NULL;
    held_frame = NULL;
    connect_count = 0;
    got_count = 0;
    got_ref = 0;
    got_database[0] = '\0';
    got_reason[0] = '\0';
    memset(&cfg, 0, sizeof(cfg));
    mock_logging_reset_all();
    nats_client_reset();
    nats_dispatch_set_invalidate(NULL);
    install_fake();
}

void tearDown(void) {
    release_fixture();
    free(held_json);
    free(held_frame);
    held_json = NULL;
    held_frame = NULL;
    nats_dispatch_set_invalidate(NULL);
    nats_io_install(NULL);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_dispatch_message_peer(void) {
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(k_wire, "cache.invalidate_by_ref", k_wire, "hydrogen-02", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, connect_count);
    TEST_ASSERT_EQUAL(1, got_count);
    TEST_ASSERT_EQUAL_STRING("Acuranzo", got_database);
    TEST_ASSERT_TRUE(got_ref == 127);
    TEST_ASSERT_EQUAL_STRING("mutation", got_reason);
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
}

void test_nats_dispatch_message_skip_self(void) {
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(k_wire, "cache.invalidate_by_ref", k_wire, "hydrogen-01", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS skip self"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS dispatch cache.invalidate_by_ref"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_empty_instance(void) {
    int rc;

    prime_server("");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(k_wire, "cache.invalidate_by_ref", k_wire, "", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS skip self"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_bad_json(void) {
    size_t len = 0;
    char *frame;
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    frame = build_frame(k_wire, "NOT-JSON", &len);
    free(held_frame);
    held_frame = frame;
    TEST_ASSERT_NOT_NULL(frame);
    rc = nats_parser_feed(frame, len);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("NOT-JSON"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_ignores_app_state(void) {
    const char *subject = "cluster.philement.instance.app_state";
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(subject, "app_state", subject, "hydrogen-02", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_bad_data(void) {
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(k_wire, "cache.invalidate_by_ref", k_wire, "hydrogen-02", false);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_subject_mismatch(void) {
    int rc;

    prime_server("hydrogen-01");
    nats_dispatch_set_invalidate(capture_invalidate);
    rc = feed_event(k_wire, "cache.invalidate_by_ref", "cluster.philement.other",
                    "hydrogen-02", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_dispatch_message_log_omits_payload(void) {
    int rc;

    prime_server("hydrogen-01");
    rc = feed_event(k_wire, "cache.invalidate_by_ref", k_wire, "hydrogen-02", true);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_EQUAL(0, connect_count);
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS dispatch cache.invalidate_by_ref"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
}

void test_nats_dispatch_message_evicts_template(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";
    const char *other = "SELECT id FROM sessions WHERE id = :id";

    prime_server("hydrogen-01");
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":2}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", other, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("OtherDB", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    TEST_ASSERT_EQUAL(0, feed_event(k_wire, "cache.invalidate_by_ref", k_wire,
                                    "hydrogen-02", true));
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS dispatch cache.invalidate_by_ref"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
    TEST_ASSERT_FALSE(cached("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_FALSE(cached("Acuranzo", sql, "{\"id\":2}"));
    TEST_ASSERT_TRUE(cached("Acuranzo", other, "{\"id\":1}"));
    TEST_ASSERT_TRUE(cached("OtherDB", sql, "{\"id\":1}"));
}

void test_nats_dispatch_message_skip_self_keeps_rows(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";

    prime_server("hydrogen-01");
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    TEST_ASSERT_EQUAL(0, feed_event(k_wire, "cache.invalidate_by_ref", k_wire,
                                    "hydrogen-01", true));
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS skip self"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
}

void test_nats_dispatch_message_drops_wide_ref(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";

    prime_server("hydrogen-01");
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    TEST_ASSERT_EQUAL(0, feed_event_ref((json_int_t)INT_MAX + 1));
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS envelope dropped"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("NATS dispatch cache.invalidate_by_ref"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_dispatch_message_peer);
    RUN_TEST(test_nats_dispatch_message_skip_self);
    RUN_TEST(test_nats_dispatch_message_empty_instance);
    RUN_TEST(test_nats_dispatch_message_bad_json);
    RUN_TEST(test_nats_dispatch_message_ignores_app_state);
    RUN_TEST(test_nats_dispatch_message_bad_data);
    RUN_TEST(test_nats_dispatch_message_subject_mismatch);
    RUN_TEST(test_nats_dispatch_message_log_omits_payload);
    RUN_TEST(test_nats_dispatch_message_evicts_template);
    RUN_TEST(test_nats_dispatch_message_skip_self_keeps_rows);
    RUN_TEST(test_nats_dispatch_message_drops_wide_ref);
    return UNITY_END();
}
