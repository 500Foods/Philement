/*
 * Unity Test File: nats_broadcast
 *
 * One JSON envelope on the publish queue. The socket is a fake NatsIo.
 * This test does not start the retry thread and does not dial.
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
    cfg.nats.Servers[0] = strdup("nats://127.0.0.1:4222");
    cfg.nats.ServerCount = 1;
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("hydrogen-01");
    app_config = &cfg;
}

static json_t *data_with_ref(json_int_t ref) {
    json_t *data = json_object();

    json_object_set_new(data, "database", json_string("Acuranzo"));
    json_object_set_new(data, "query_ref", json_integer(ref));
    json_object_set_new(data, "reason", json_string("mutation"));
    return data;
}

static json_t *invalidation_data(void) {
    return data_with_ref(127);
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

static void add_template(int query_ref, const char *sql) {
    QueryCacheEntry *entry = query_cache_entry_create(query_ref, 1, sql, "cached read",
                                                      "cache", 30, SR_NATS);

    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(test_queue->query_cache, entry, SR_NATS));
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

static bool iso_utc_timestamp(const char *text) {
    size_t i;

    if (!text || strlen(text) != 20) {
        return false;
    }
    for (i = 0; i < 20; i++) {
        char c = text[i];

        if (i == 4 || i == 7) {
            if (c != '-') {
                return false;
            }
        } else if (i == 10) {
            if (c != 'T') {
                return false;
            }
        } else if (i == 13 || i == 16) {
            if (c != ':') {
                return false;
            }
        } else if (i == 19) {
            if (c != 'Z') {
                return false;
            }
        } else if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

static json_t *take_envelope(void) {
    const char *header = "PUB cluster.philement.cache.invalidate ";
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

void test_nats_broadcast_envelope(void);
void test_nats_broadcast_queues_while_down(void);
void test_nats_broadcast_fail_next_once(void);
void test_nats_broadcast_rejects_bad_event(void);
void test_nats_broadcast_empty_instance_stays_empty(void);
void test_nats_broadcast_rejects_bad_data(void);
void test_nats_broadcast_evicts_template(void);
void test_nats_broadcast_unknown_ref_still_publishes(void);
void test_nats_broadcast_drops_wide_ref(void);
void test_nats_broadcast_uses_first_template(void);

void setUp(void) {
    saved_config = app_config;
    held_data = NULL;
    held_env = NULL;
    memset(&cfg, 0, sizeof(cfg));
    memset(&fake, 0, sizeof(fake));
    mock_logging_reset_all();
    nats_link_set(NATS_LINK_DOWN);
    nats_client_reset();
    nats_io_install(NULL);
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

void test_nats_broadcast_envelope(void) {
    int rc;
    const json_t *body;

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    held_data = invalidation_data();
    rc = nats_broadcast("cache.invalidate_by_ref", held_data);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("cache.invalidate_by_ref"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
    held_env = take_envelope();
    TEST_ASSERT_NOT_NULL(held_env);
    TEST_ASSERT_EQUAL_STRING("cache.invalidate_by_ref",
                             json_string_value(json_object_get(held_env, "event")));
    TEST_ASSERT_EQUAL_STRING("cluster.philement.cache.invalidate",
                             json_string_value(json_object_get(held_env, "subject")));
    TEST_ASSERT_TRUE(iso_utc_timestamp(json_string_value(json_object_get(held_env, "timestamp"))));
    TEST_ASSERT_EQUAL_STRING("hydrogen-01", json_string_value(json_object_get(held_env, "source")));
    TEST_ASSERT_EQUAL_STRING("hydrogen-01",
                             json_string_value(json_object_get(held_env, "instance_id")));
    body = json_object_get(held_env, "data");
    TEST_ASSERT_TRUE(json_is_object(body));
    TEST_ASSERT_EQUAL_STRING("Acuranzo", json_string_value(json_object_get(body, "database")));
    TEST_ASSERT_TRUE(json_integer_value(json_object_get(body, "query_ref")) == 127);
    TEST_ASSERT_EQUAL_STRING("mutation", json_string_value(json_object_get(body, "reason")));
}

void test_nats_broadcast_queues_while_down(void) {
    int rc;

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_DOWN);
    held_data = invalidation_data();
    rc = nats_broadcast("cache.invalidate_by_ref", held_data);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(0, fake.written_len);
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(0, nats_client_flush_outbound());
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_nats_broadcast_fail_next_once(void) {
    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    cfg.nats.Test.FailNextPublishOnLaunch = true;
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(-1, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_FALSE(cfg.nats.Test.FailNextPublishOnLaunch);
    json_decref(held_data);
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_nats_broadcast_rejects_bad_event(void) {
    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(-1, nats_broadcast("cluster.philement.cache.invalidate", held_data));
    TEST_ASSERT_EQUAL(-1, nats_broadcast("not-an-event", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_nats_broadcast_empty_instance_stays_empty(void) {
    prime_server();
    free(cfg.nats.InstanceId);
    cfg.nats.InstanceId = strdup("");
    install_fake();
    nats_link_set(NATS_LINK_UP);
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    held_env = take_envelope();
    TEST_ASSERT_NOT_NULL(held_env);
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(held_env, "source")));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(held_env, "instance_id")));
}

void test_nats_broadcast_rejects_bad_data(void) {
    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    TEST_ASSERT_EQUAL(-1, nats_broadcast("cache.invalidate_by_ref", NULL));
    held_data = json_object();
    json_object_set_new(held_data, "database", json_string("Acuranzo"));
    TEST_ASSERT_EQUAL(-1, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
}

void test_nats_broadcast_evicts_template(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";
    const char *other = "SELECT id FROM sessions WHERE id = :id";

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":2}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", other, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("OtherDB", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_FALSE(mock_logging_message_contains("Acuranzo"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("mutation"));
    TEST_ASSERT_FALSE(mock_logging_message_contains("cache.invalidate_by_ref"));
    TEST_ASSERT_FALSE(cached("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_FALSE(cached("Acuranzo", sql, "{\"id\":2}"));
    TEST_ASSERT_TRUE(cached("Acuranzo", other, "{\"id\":1}"));
    TEST_ASSERT_TRUE(cached("OtherDB", sql, "{\"id\":1}"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_nats_broadcast_unknown_ref_still_publishes(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    held_data = data_with_ref(50);
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_nats_broadcast_drops_wide_ref(void) {
    const char *sql = "SELECT id FROM accounts WHERE id = :id";

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    install_template(sql);
    TEST_ASSERT_TRUE(put_row("Acuranzo", sql, "{\"id\":1}"));
    mock_logging_reset_all();
    held_data = data_with_ref((json_int_t)INT_MAX + 1);
    TEST_ASSERT_EQUAL(-1, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_EQUAL(0, fake.written_len);
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
    json_decref(held_data);
    held_data = data_with_ref(INT_MAX);
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_TRUE(cached("Acuranzo", sql, "{\"id\":1}"));
    TEST_ASSERT_NOT_NULL(strstr(fake.written, "PUB cluster.philement.cache.invalidate "));
}

void test_nats_broadcast_uses_first_template(void) {
    const char *first = "SELECT id FROM accounts WHERE id = :id";
    const char *second = "SELECT id FROM sessions WHERE id = :id";

    prime_server();
    install_fake();
    nats_link_set(NATS_LINK_UP);
    install_template(first);
    add_template(127, second);
    TEST_ASSERT_TRUE(put_row("Acuranzo", first, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", second, "{\"id\":1}"));
    mock_logging_reset_all();
    held_data = invalidation_data();
    TEST_ASSERT_EQUAL(0, nats_broadcast("cache.invalidate_by_ref", held_data));
    TEST_ASSERT_FALSE(cached("Acuranzo", first, "{\"id\":1}"));
    TEST_ASSERT_TRUE(cached("Acuranzo", second, "{\"id\":1}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_broadcast_envelope);
    RUN_TEST(test_nats_broadcast_queues_while_down);
    RUN_TEST(test_nats_broadcast_fail_next_once);
    RUN_TEST(test_nats_broadcast_rejects_bad_event);
    RUN_TEST(test_nats_broadcast_empty_instance_stays_empty);
    RUN_TEST(test_nats_broadcast_rejects_bad_data);
    RUN_TEST(test_nats_broadcast_evicts_template);
    RUN_TEST(test_nats_broadcast_unknown_ref_still_publishes);
    RUN_TEST(test_nats_broadcast_drops_wide_ref);
    RUN_TEST(test_nats_broadcast_uses_first_template);
    return UNITY_END();
}
