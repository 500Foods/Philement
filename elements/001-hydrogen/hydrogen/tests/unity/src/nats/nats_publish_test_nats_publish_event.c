/*
 * Unity Test File: nats_publish_event
 *
 * One JSON envelope for events other than cache invalidation.
 * The socket is a fake NatsIo. This test does not start the retry
 * thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/dbqueue/query_result_cache.h>
#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>

#include "mock_logging.h"

#include <stdlib.h>
#include <string.h>

typedef struct FakeNats {
    char written[8192];
    size_t written_len;
} FakeNats;

static FakeNats fake;
static AppConfig cfg;
static AppConfig *saved_config;
static json_t *held_data;
static json_t *held_env;
static DatabaseQueueManager *saved_manager;
static DatabaseQueueManager *test_manager;
static DatabaseQueue *test_queue;
static bool manager_installed;
static bool result_cache_used;

static int fake_connect(void *ctx, const char *host, int port, int timeout_seconds) {
    (void)ctx;
    (void)host;
    (void)port;
    (void)timeout_seconds;
    return 0;
}

static int fake_read(void *ctx, void *buf, size_t len) {
    (void)ctx;
    (void)buf;
    (void)len;
    return 0;
}

static int fake_write(void *ctx, const void *buf, size_t len) {
    FakeNats *io = ctx;

    if (!io || (!buf && len > 0)) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }
    if (io->written_len + len >= sizeof(io->written)) {
        return -1;
    }
    memcpy(io->written + io->written_len, buf, len);
    io->written_len += len;
    io->written[io->written_len] = '\0';
    return 0;
}

static void fake_close(void *ctx) {
    (void)ctx;
}

static void install_fake(void) {
    NatsIo io;

    memset(&fake, 0, sizeof(fake));
    io.connect_fn = fake_connect;
    io.read_fn = fake_read;
    io.write_fn = fake_write;
    io.close_fn = fake_close;
    io.ctx = &fake;
    nats_io_install(&io);
}

static void prime_server(void) {
    nats_config_apply_defaults(&cfg.nats);
    cfg.nats.Enabled = true;
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("hydrogen-01");
    app_config = &cfg;
}

static json_t *event_data(void) {
    json_t *data = json_object();

    json_object_set_new(data, "database", json_string("Acuranzo"));
    json_object_set_new(data, "query_ref", json_integer(127));
    json_object_set_new(data, "note", json_string("secret-token"));
    return data;
}

static json_t *take_envelope(const char *header) {
    const char *at;
    char *end;
    unsigned long size;
    char *copy;
    json_t *obj;

    at = strstr(fake.written, header);
    if (!at) {
        return NULL;
    }
    at += strlen(header);
    size = strtoul(at, &end, 10);
    if (!end || end == at || end[0] != '\r' || end[1] != '\n' || size == 0 || size > 8000) {
        return NULL;
    }
    at = end + 2;
    if (strlen(at) < size + 2 || at[size] != '\r' || at[size + 1] != '\n') {
        return NULL;
    }
    copy = malloc(size + 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, at, size);
    copy[size] = '\0';
    obj = json_loads(copy, 0, NULL);
    free(copy);
    return obj;
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

void test_nats_publish_event_envelope(void);
void test_nats_publish_event_rejects(void);
void test_nats_publish_event_does_not_evict(void);
void test_nats_publish_event_queues_while_down(void);
void test_nats_publish_event_fail_next_once(void);
void test_nats_publish_event_relays_without_payload(void);
void test_nats_publish_event_empty_instance_stays_empty(void);

void setUp(void) {
    saved_config = app_config;
    held_data = NULL;
    held_env = NULL;
    memset(&cfg, 0, sizeof(cfg));
    mock_logging_reset_all();
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    install_fake();
    app_config = &cfg;
}

void tearDown(void) {
    release_fixture();
    json_decref(held_data);
    json_decref(held_env);
    held_data = NULL;
    held_env = NULL;
    nats_link_set(NATS_LINK_DOWN);
    nats_io_install(NULL);
    nats_client_reset();
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_publish_event_envelope(void) {
    const json_t *body;
    const char *stamp;

    prime_server();
    nats_link_set(NATS_LINK_UP);
    held_data = event_data();
    TEST_ASSERT_EQUAL(0, nats_publish_event("jobs.done", held_data));
    TEST_ASSERT_FALSE(mock_logging_message_contains("secret-token"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("jobs.done"));
    held_env = take_envelope("PUB cluster.philement.jobs.done ");
    TEST_ASSERT_NOT_NULL(held_env);
    TEST_ASSERT_EQUAL_STRING("jobs.done", json_string_value(json_object_get(held_env, "event")));
    TEST_ASSERT_EQUAL_STRING("cluster.philement.jobs.done",
                             json_string_value(json_object_get(held_env, "subject")));
    stamp = json_string_value(json_object_get(held_env, "timestamp"));
    TEST_ASSERT_NOT_NULL(stamp);
    TEST_ASSERT_EQUAL(20, (int)strlen(stamp));
    TEST_ASSERT_EQUAL('T', stamp[10]);
    TEST_ASSERT_EQUAL('Z', stamp[19]);
    TEST_ASSERT_EQUAL_STRING("hydrogen-01", json_string_value(json_object_get(held_env, "source")));
    TEST_ASSERT_EQUAL_STRING("hydrogen-01",
                             json_string_value(json_object_get(held_env, "instance_id")));
    body = json_object_get(held_env, "data");
    TEST_ASSERT_EQUAL_STRING("Acuranzo", json_string_value(json_object_get(body, "database")));
    TEST_ASSERT_TRUE(json_integer_value(json_object_get(body, "query_ref")) == 127);
    TEST_ASSERT_EQUAL_STRING("secret-token", json_string_value(json_object_get(body, "note")));
}

void test_nats_publish_event_rejects(void) {
    json_t *array;
    json_t *text;

    prime_server();
    nats_link_set(NATS_LINK_UP);
    held_data = event_data();
    TEST_ASSERT_EQUAL(-1, nats_publish_event(NULL, held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("app_state", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("cluster.philement.jobs", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("bad event", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("a*b", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("a>b", held_data));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("jobs.done", NULL));
    array = json_array();
    json_array_append_new(array, json_string("x"));
    TEST_ASSERT_EQUAL(-1, nats_publish_event("jobs.done", array));
    json_decref(array);
    text = json_string("nope");
    TEST_ASSERT_EQUAL(-1, nats_publish_event("jobs.done", text));
    json_decref(text);
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_nats_publish_event_does_not_evict(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";
    json_t *inv;

    prime_server();
    nats_link_set(NATS_LINK_UP);
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    held_data = event_data();
    TEST_ASSERT_EQUAL(0, nats_publish_event("jobs.done", held_data));
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
    inv = json_object();
    json_object_set_new(inv, "database", json_string("Acuranzo"));
    json_object_set_new(inv, "query_ref", json_integer(127));
    json_object_set_new(inv, "reason", json_string("mutation"));
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", inv));
    json_decref(inv);
    TEST_ASSERT_FALSE(cached("Acuranzo", sql, "{\"id\":1}"));
}

void test_nats_publish_event_queues_while_down(void) {
    prime_server();
    nats_link_set(NATS_LINK_DOWN);
    held_data = event_data();
    TEST_ASSERT_EQUAL(0, nats_publish_event("not-an-event", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.not-an-event "));
}

void test_nats_publish_event_fail_next_once(void) {
    prime_server();
    nats_link_set(NATS_LINK_UP);
    cfg.nats.Test.FailNextPublishOnLaunch = true;
    held_data = event_data();
    TEST_ASSERT_EQUAL(-1, nats_publish_event("jobs.done", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_FALSE(cfg.nats.Test.FailNextPublishOnLaunch);
    TEST_ASSERT_EQUAL(0, nats_publish_event("jobs.done", held_data));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.jobs.done "));
}

void test_nats_publish_event_relays_without_payload(void) {
    prime_server();
    nats_link_set(NATS_LINK_UP);
    cfg.nats.WebSocketRelay.Enabled = true;
    cfg.nats.WebSocketRelay.Events[0] = strdup("jobs.done");
    cfg.nats.WebSocketRelay.EventCount = 1;
    mock_logging_reset_all();
    held_data = event_data();
    TEST_ASSERT_EQUAL(0, nats_publish_event("jobs.done", held_data));
    TEST_ASSERT_TRUE(mock_logging_message_contains("NATS relay jobs.done"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("secret-token"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
}

void test_nats_publish_event_empty_instance_stays_empty(void) {
    prime_server();
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("");
    nats_link_set(NATS_LINK_UP);
    held_data = event_data();
    TEST_ASSERT_EQUAL(0, nats_publish_event("jobs.done", held_data));
    held_env = take_envelope("PUB cluster.philement.jobs.done ");
    TEST_ASSERT_NOT_NULL(held_env);
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(held_env, "source")));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(held_env, "instance_id")));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_publish_event_envelope);
    RUN_TEST(test_nats_publish_event_rejects);
    RUN_TEST(test_nats_publish_event_does_not_evict);
    RUN_TEST(test_nats_publish_event_queues_while_down);
    RUN_TEST(test_nats_publish_event_fail_next_once);
    RUN_TEST(test_nats_publish_event_relays_without_payload);
    RUN_TEST(test_nats_publish_event_empty_instance_stays_empty);
    return UNITY_END();
}
