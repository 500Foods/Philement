/*
 * Unity Test File: victoria_logs_worker / send_http_post
 * Deterministic coverage for the worker loop and HTTP POST paths.
 *
 * Intermittent dual-coverage dips (~70%) were caused by:
 * 1) init/cleanup races: cleanup often sets shutdown before the worker
 *    enters the loop body, so most worker lines only hit occasionally.
 * 2) HTTP success path only runs when connect() to a real peer succeeds
 *    (rare against localhost:9428 during the suite).
 *
 * These tests drive the worker on a dedicated thread with controlled
 * timers and a loopback HTTP sink so coverage is stable.
 *
 * NOTE: Do not include mock_system.h here. CMake defines USE_MOCK_SYSTEM
 * globally for Unity tests; including the header would remap poll/close
 * and break the embedded HTTP sink (same pattern as proxy_test_network.c).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/logging/victoria_logs.h>
#include <src/logging/logging.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

extern VictoriaLogsConfig victoria_logs_config;
extern VLThreadState victoria_logs_thread;

void test_victoria_logs_send_http_post_success_204(void);
void test_victoria_logs_send_http_post_success_200(void);
void test_victoria_logs_send_http_post_bad_status(void);
void test_victoria_logs_send_http_post_connect_fail(void);
void test_victoria_logs_flush_batch_success(void);
void test_victoria_logs_flush_batch_stale_drop(void);
void test_victoria_logs_add_to_batch_message_too_large(void);
void test_victoria_logs_worker_first_message_flush_fail(void);
void test_victoria_logs_worker_batch_full_flush(void);
void test_victoria_logs_worker_short_timer_flush(void);
void test_victoria_logs_worker_long_timer_idle_reset(void);
void test_victoria_logs_worker_long_timer_with_batch(void);
void test_victoria_logs_worker_preexpired_timer_dequeue(void);
void test_victoria_logs_worker_batch_full_flush_fail(void);

/* ---- loopback HTTP sink ------------------------------------------------- */

static volatile int g_sink_stop;
static int g_sink_listen_fd = -1;
static pthread_t g_sink_thread;
static int g_sink_status = 204;
static int g_sink_port;

static void sink_handle_client(int client_fd) {
    char buf[2048];
    (void)recv(client_fd, buf, sizeof(buf) - 1, 0);

    char resp[128];
    int n = snprintf(resp, sizeof(resp),
                     "HTTP/1.1 %d OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                     g_sink_status);
    if (n > 0) {
        (void)send(client_fd, resp, (size_t)n, MSG_NOSIGNAL);
    }
    close(client_fd);
}

static void* sink_thread_func(void* arg) {
    (void)arg;
    while (!g_sink_stop) {
        struct pollfd pfd;
        pfd.fd = g_sink_listen_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, 50);
        if (pr <= 0) {
            continue;
        }
        int client_fd = accept(g_sink_listen_fd, NULL, NULL);
        if (client_fd >= 0) {
            sink_handle_client(client_fd);
        }
    }
    return NULL;
}

static bool sink_start(int status) {
    g_sink_stop = 0;
    g_sink_status = status;
    g_sink_listen_fd = -1;
    g_sink_port = 0;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return false;
    }
    if (listen(fd, 8) != 0) {
        close(fd);
        return false;
    }

    socklen_t alen = sizeof(addr);
    if (getsockname(fd, (struct sockaddr*)&addr, &alen) != 0) {
        close(fd);
        return false;
    }
    g_sink_port = (int)ntohs(addr.sin_port);
    g_sink_listen_fd = fd;

    if (pthread_create(&g_sink_thread, NULL, sink_thread_func, NULL) != 0) {
        close(fd);
        g_sink_listen_fd = -1;
        return false;
    }
    usleep(20000);
    return true;
}

static void sink_stop(void) {
    g_sink_stop = 1;
    if (g_sink_listen_fd >= 0) {
        close(g_sink_listen_fd);
        g_sink_listen_fd = -1;
    }
    pthread_join(g_sink_thread, NULL);
}

/* ---- worker harness (no init_victoria_logs thread) ---------------------- */

static void harness_setup(const char* url) {
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));

    victoria_logs_config.enabled = true;
    victoria_logs_config.url = strdup(url);
    victoria_logs_config.min_level = LOG_LEVEL_DEBUG;
    victoria_logs_config.k8s_namespace = strdup("local");
    victoria_logs_config.k8s_pod_name = strdup("pod");
    victoria_logs_config.k8s_container_name = strdup("hydrogen");
    victoria_logs_config.k8s_node_name = strdup("node");
    victoria_logs_config.host = strdup("node");

    victoria_logs_thread.batch_buffer = malloc(VICTORIA_LOGS_MAX_BATCH_BUFFER);
    TEST_ASSERT_NOT_NULL(victoria_logs_thread.batch_buffer);
    victoria_logs_thread.batch_buffer[0] = '\0';
    victoria_logs_thread.batch_buffer_size = 0;
    victoria_logs_thread.batch_count = 0;

    TEST_ASSERT_TRUE(victoria_logs_queue_init());
    victoria_logs_thread.running = true;
    victoria_logs_thread.shutdown = false;
    victoria_logs_thread.short_timer_active = false;
    victoria_logs_thread.first_log_sent = false;
}

static void harness_teardown(void) {
    victoria_logs_thread.shutdown = true;
    if (victoria_logs_thread.running) {
        pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
        pthread_cond_signal(&victoria_logs_thread.queue.cond);
        pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
    }
    victoria_logs_queue_cleanup();
    free(victoria_logs_thread.batch_buffer);
    victoria_logs_thread.batch_buffer = NULL;
    free(victoria_logs_config.url);
    free(victoria_logs_config.k8s_namespace);
    free(victoria_logs_config.k8s_pod_name);
    free(victoria_logs_config.k8s_container_name);
    free(victoria_logs_config.k8s_node_name);
    free(victoria_logs_config.host);
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

static void wake_worker(void) {
    pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
    pthread_cond_signal(&victoria_logs_thread.queue.cond);
    pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
}

static void stop_worker(pthread_t worker) {
    victoria_logs_thread.shutdown = true;
    wake_worker();
    pthread_join(worker, NULL);
    victoria_logs_thread.running = false;
}

static bool wait_for(bool (*pred)(void), int max_ms) {
    for (int i = 0; i < max_ms / 5; i++) {
        if (pred()) {
            return true;
        }
        usleep(5000);
    }
    return pred();
}

static bool short_timer_is_active(void) {
    return victoria_logs_thread.short_timer_active;
}

static bool short_timer_is_inactive(void) {
    return !victoria_logs_thread.short_timer_active;
}

static bool batch_is_empty(void) {
    return victoria_logs_thread.batch_count == 0;
}

static bool first_log_sent_true(void) {
    return victoria_logs_thread.first_log_sent;
}

static bool batch_has_messages(void) {
    return victoria_logs_thread.batch_count > 0;
}

/* Closed local port for fast connect-fail (no DNS, RST/timeout short). */
static char g_closed_url[64];

static void make_closed_url(void) {
    /* Bind then close so the port is unused; connect fails quickly with ECONNREFUSED */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT_TRUE(fd >= 0);
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);
    TEST_ASSERT_EQUAL(0, bind(fd, (struct sockaddr*)&addr, sizeof(addr)));
    socklen_t alen = sizeof(addr);
    TEST_ASSERT_EQUAL(0, getsockname(fd, (struct sockaddr*)&addr, &alen));
    int port = (int)ntohs(addr.sin_port);
    close(fd);
    snprintf(g_closed_url, sizeof(g_closed_url), "http://127.0.0.1:%d/insert", port);
}

void setUp(void) {
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

void tearDown(void) {
    if (victoria_logs_thread.batch_buffer || victoria_logs_config.url) {
        harness_teardown();
    }
}

/* ---- HTTP POST ---------------------------------------------------------- */

void test_victoria_logs_send_http_post_success_204(void) {
    TEST_ASSERT_TRUE(sink_start(204));
    const char* body = "{\"msg\":\"hello\"}";
    bool ok = victoria_logs_send_http_post("127.0.0.1", g_sink_port, "/insert",
                                          body, strlen(body), false);
    sink_stop();
    TEST_ASSERT_TRUE(ok);
}

void test_victoria_logs_send_http_post_success_200(void) {
    TEST_ASSERT_TRUE(sink_start(200));
    const char* body = "{\"msg\":\"hello\"}";
    bool ok = victoria_logs_send_http_post("127.0.0.1", g_sink_port, "/insert",
                                          body, strlen(body), false);
    sink_stop();
    TEST_ASSERT_TRUE(ok);
}

void test_victoria_logs_send_http_post_bad_status(void) {
    TEST_ASSERT_TRUE(sink_start(500));
    const char* body = "{\"msg\":\"hello\"}";
    bool ok = victoria_logs_send_http_post("127.0.0.1", g_sink_port, "/insert",
                                          body, strlen(body), false);
    sink_stop();
    TEST_ASSERT_FALSE(ok);
}

void test_victoria_logs_send_http_post_connect_fail(void) {
    make_closed_url();
    int port = 0;
    sscanf(g_closed_url, "http://127.0.0.1:%d/", &port);
    bool ok = victoria_logs_send_http_post("127.0.0.1", port, "/insert", "x", 1, false);
    TEST_ASSERT_FALSE(ok);
}

/* ---- flush_batch_internal ----------------------------------------------- */

void test_victoria_logs_flush_batch_success(void) {
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    TEST_ASSERT_TRUE(victoria_logs_add_to_batch("{\"a\":1}"));
    TEST_ASSERT_EQUAL(1, victoria_logs_thread.batch_count);

    bool ok = victoria_logs_flush_batch_internal();
    sink_stop();
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_count);

    harness_teardown();
}

void test_victoria_logs_flush_batch_stale_drop(void) {
    make_closed_url();
    harness_setup(g_closed_url);
    TEST_ASSERT_TRUE(victoria_logs_add_to_batch("{\"stale\":1}"));
    victoria_logs_thread.first_message_time = time(NULL) - (VICTORIA_LOGS_MAX_RETRY_SEC + 5);

    bool ok = victoria_logs_flush_batch_internal();
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_count);

    harness_teardown();
}

void test_victoria_logs_add_to_batch_message_too_large(void) {
    harness_setup("http://127.0.0.1:1/");
    size_t len = VICTORIA_LOGS_MAX_BATCH_BUFFER;
    char* big = malloc(len + 1);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'x', len);
    big[len] = '\0';

    TEST_ASSERT_FALSE(victoria_logs_add_to_batch(big));
    TEST_ASSERT_EQUAL(0, victoria_logs_thread.batch_count);
    free(big);
    harness_teardown();
}

/* ---- worker ------------------------------------------------------------- */

void test_victoria_logs_worker_first_message_flush_fail(void) {
    make_closed_url();
    harness_setup(g_closed_url);

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"first\":1}"));
    /* connect-fail may take up to VICTORIA_LOGS_TIMEOUT_SEC */
    TEST_ASSERT_TRUE(wait_for(short_timer_is_active, 3000));
    TEST_ASSERT_TRUE(wait_for(batch_has_messages, 500));
    TEST_ASSERT_FALSE(victoria_logs_thread.first_log_sent);

    stop_worker(worker);
    harness_teardown();
}

void test_victoria_logs_worker_batch_full_flush(void) {
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    /* Preload the queue before starting the worker so the batch-full
     * check is hit in a single drain burst (avoids the 1s short timer
     * flushing a partial batch under UNITY_TEST_MODE). */
    victoria_logs_thread.running = true;
    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":0}"));
    for (int i = 1; i <= VICTORIA_LOGS_BATCH_SIZE; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "{\"n\":%d}", i);
        TEST_ASSERT_TRUE(victoria_logs_queue_enqueue(msg));
    }

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    TEST_ASSERT_TRUE(wait_for(first_log_sent_true, 3000));
    TEST_ASSERT_TRUE(wait_for(batch_is_empty, 3000));

    stop_worker(worker);
    sink_stop();
    harness_teardown();
}

void test_victoria_logs_worker_short_timer_flush(void) {
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":0}"));
    TEST_ASSERT_TRUE(wait_for(first_log_sent_true, 2000));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":1}"));
    TEST_ASSERT_TRUE(wait_for(short_timer_is_active, 2000));
    TEST_ASSERT_TRUE(wait_for(batch_has_messages, 500));

    /* Push long timer far ahead so only the short timer can flush. */
    clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
    victoria_logs_thread.long_timer.tv_sec += 3600;

    /* Short timer is 1s; wait for natural expiry. */
    TEST_ASSERT_TRUE(wait_for(short_timer_is_inactive, 3000));
    TEST_ASSERT_TRUE(wait_for(batch_is_empty, 2000));

    stop_worker(worker);
    sink_stop();
    harness_teardown();
}

void test_victoria_logs_worker_long_timer_idle_reset(void) {
    make_closed_url();
    harness_setup(g_closed_url);

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    /* Snapshot the long timer after the worker initializes it. Empty-queue
     * cond signals do not break out of timedwait (worker re-waits until the
     * local deadline), so we must let the real long timer expire. Under
     * UNITY_TEST_MODE the long interval is 1s. */
    usleep(50000);
    time_t initial = victoria_logs_thread.long_timer.tv_sec;
    TEST_ASSERT_TRUE(initial > 0);

    bool advanced = false;
    for (int i = 0; i < 500; i++) {
        if (victoria_logs_thread.long_timer.tv_sec > initial) {
            advanced = true;
            break;
        }
        usleep(10000);
    }
    TEST_ASSERT_TRUE(advanced);

    stop_worker(worker);
    harness_teardown();
}

void test_victoria_logs_worker_long_timer_with_batch(void) {
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":0}"));
    TEST_ASSERT_TRUE(wait_for(first_log_sent_true, 2000));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":1}"));
    TEST_ASSERT_TRUE(wait_for(batch_has_messages, 2000));

    /* Short or long timer (1s in UNITY_TEST_MODE) flushes the batch */
    TEST_ASSERT_TRUE(wait_for(batch_is_empty, 3000));

    stop_worker(worker);
    sink_stop();
    harness_teardown();
}

void test_victoria_logs_worker_preexpired_timer_dequeue(void) {
    /* Start with long_timer already expired so the first loop iteration
     * takes the timeout_expired branch (queue_dequeue, not timedwait). */
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    victoria_logs_thread.running = true;
    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"pre\":1}"));

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    /* Immediately force long timer into the past before the worker can
     * finish reset_long_timer + first wait — race a few times. */
    for (int i = 0; i < 50; i++) {
        victoria_logs_thread.long_timer.tv_sec = 0;
        victoria_logs_thread.long_timer.tv_nsec = 0;
        usleep(1000);
    }

    TEST_ASSERT_TRUE(wait_for(first_log_sent_true, 3000));

    stop_worker(worker);
    sink_stop();
    harness_teardown();
}

void test_victoria_logs_worker_batch_full_flush_fail(void) {
    /* Succeed first flush, then kill the sink and fill the batch so the
     * batch-full flush-failure retry path is exercised. */
    TEST_ASSERT_TRUE(sink_start(204));
    char url[128];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/insert", g_sink_port);
    harness_setup(url);

    pthread_t worker;
    TEST_ASSERT_EQUAL(0, pthread_create(&worker, NULL, victoria_logs_worker, NULL));

    TEST_ASSERT_TRUE(victoria_logs_queue_enqueue("{\"n\":0}"));
    TEST_ASSERT_TRUE(wait_for(first_log_sent_true, 2000));

    sink_stop();

    for (int i = 1; i <= VICTORIA_LOGS_BATCH_SIZE; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "{\"n\":%d}", i);
        TEST_ASSERT_TRUE(victoria_logs_queue_enqueue(msg));
    }

    /* Flush failure keeps batch and arms short timer / retry long timer */
    TEST_ASSERT_TRUE(wait_for(short_timer_is_active, 5000));
    TEST_ASSERT_TRUE(wait_for(batch_has_messages, 500));

    stop_worker(worker);
    harness_teardown();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_victoria_logs_send_http_post_success_204);
    RUN_TEST(test_victoria_logs_send_http_post_success_200);
    RUN_TEST(test_victoria_logs_send_http_post_bad_status);
    RUN_TEST(test_victoria_logs_send_http_post_connect_fail);
    RUN_TEST(test_victoria_logs_flush_batch_success);
    RUN_TEST(test_victoria_logs_flush_batch_stale_drop);
    RUN_TEST(test_victoria_logs_add_to_batch_message_too_large);
    RUN_TEST(test_victoria_logs_worker_first_message_flush_fail);
    RUN_TEST(test_victoria_logs_worker_batch_full_flush);
    RUN_TEST(test_victoria_logs_worker_short_timer_flush);
    RUN_TEST(test_victoria_logs_worker_long_timer_idle_reset);
    RUN_TEST(test_victoria_logs_worker_long_timer_with_batch);
    RUN_TEST(test_victoria_logs_worker_preexpired_timer_dequeue);
    RUN_TEST(test_victoria_logs_worker_batch_full_flush_fail);

    return UNITY_END();
}
