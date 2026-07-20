/*
 * Unity Test File: Chat Proxy - Network Paths
 *
 * Exercises the real libcurl code paths in src/api/wschat/helpers/proxy.c that
 * the blackbox suite cannot reach (no reachable AI API in CI):
 *   - chat_proxy_init / chat_proxy_cleanup
 *   - chat_proxy_send_request (invalid params, 200, 4xx, 5xx, connect failure,
 *     response-buffer allocation failure via USE_MOCK_SYSTEM)
 *   - chat_proxy_send_with_retry (no-retry break + exponential backoff path)
 *   - chat_proxy_send_multi (parallel success + connect failure)
 *   - chat_proxy_send_stream + chat_proxy_stream_worker_thread (SSE)
 *
 * A tiny self-contained HTTP server (real TCP socket, runs in a thread) serves
 * configurable responses on 127.0.0.1 so libcurl talks to a genuine peer.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>

// We deliberately do NOT enable USE_MOCK_SYSTEM in this test file: the proxy
// code under test (proxy.c.o) is already compiled with the global
// USE_MOCK_SYSTEM, so malloc/calloc/strdup/realloc are intercepted there and
// allocation failures can be injected. Enabling it HERE would also remap
// write()/read()/close()/poll() used by our embedded HTTP server to the mocks
// (which return 0), silently sending zero bytes or never accepting. We only
// need the mock control-function entry points, so declare them directly.
void mock_system_set_malloc_failure(int should_fail);
void mock_system_set_realloc_failure(int should_fail);
void mock_system_reset_all(void);

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Embedded HTTP server
// ---------------------------------------------------------------------------

typedef enum {
    SRV_MODE_200,        // 200 OK with small JSON body
    SRV_MODE_STATUS,     // configurable status code + body
    SRV_MODE_SSE,        // 200 text/event-stream with a couple of SSE lines
    SRV_MODE_SSE_NOTRAIL // final SSE line sent WITHOUT a trailing newline
} ServerMode;

static volatile int      g_srv_stop = 0;
static int               g_srv_port = 0;
static ServerMode        g_srv_mode = SRV_MODE_200;
static int               g_srv_status = 200;
static int               g_srv_listen_fd = -1;
static pthread_t         g_srv_thread;

static void server_send(int client_fd, const char* data, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(client_fd, data + off, len - off);
        if (n <= 0) break;
        off += (size_t)n;
    }
}

static void server_handle_client(int client_fd) {
    // Drain the request (we don't care about its contents)
    char buf[1024];
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (1) {
        ssize_t n = read(client_fd, buf, sizeof(buf));
        if (n <= 0) break;
        // Stop once we have read a complete request (blank line)
        if (memmem(buf, (size_t)n, "\r\n\r\n", 4) ||
            memmem(buf, (size_t)n, "\n\n", 2)) {
            break;
        }
    }

    if (g_srv_mode == SRV_MODE_SSE || g_srv_mode == SRV_MODE_SSE_NOTRAIL) {
        static const char sse_head[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Connection: close\r\n\r\n";
        server_send(client_fd, sse_head, sizeof(sse_head) - 1);
        server_send(client_fd,
            "data: {\"choices\":[{\"delta\":{\"content\":\"hello\"}}]}\n\n",
            strlen("data: {\"choices\":[{\"delta\":{\"content\":\"hello\"}}]}\n\n"));
        server_send(client_fd,
            "data: {\"choices\":[{\"delta\":{\"content\":\" world\"}}]}\n\n",
            strlen("data: {\"choices\":[{\"delta\":{\"content\":\" world\"}}]}\n\n"));
        if (g_srv_mode == SRV_MODE_SSE_NOTRAIL) {
            // Send final line WITHOUT a trailing newline so the worker must
            // flush the lingering line buffer as a final chunk.
            server_send(client_fd,
                "data: [DONE]",
                strlen("data: [DONE]"));
        } else {
            server_send(client_fd, "data: [DONE]\n\n", strlen("data: [DONE]\n\n"));
        }
    } else if (g_srv_mode == SRV_MODE_STATUS) {
        char header[256];
        int hl = snprintf(header, sizeof(header),
            "HTTP/1.1 %d Result\r\nContent-Length: 9\r\nConnection: close\r\n\r\nerrorbody",
            g_srv_status);
        server_send(client_fd, header, (size_t)hl);
    } else {
        const char* body =
            "{\"id\":\"x\",\"choices\":[{\"message\":{\"content\":\"ok\"}}]}";
        char header[256];
        int hl = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
            strlen(body), body);
        server_send(client_fd, header, (size_t)hl);
    }
    // Close write side so libcurl sees EOF and completes
    shutdown(client_fd, SHUT_WR);
    usleep(50000);
    close(client_fd);
}

static void* server_thread_func(void* arg) {
    (void)arg;
    while (!g_srv_stop) {
        // Poll the listen fd so the loop re-checks g_srv_stop periodically
        // instead of blocking forever in accept() (a concurrent close() of the
        // listen fd does not reliably interrupt a blocked accept() here).
        struct pollfd pfd;
        pfd.fd = g_srv_listen_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = poll(&pfd, 1, 50);
        if (pr < 0) {
            if (!g_srv_stop) usleep(10000);
            continue;
        }
        if (pr == 0) continue;  // timeout: re-check g_srv_stop

        int client_fd = accept(g_srv_listen_fd, NULL, NULL);
        if (client_fd < 0) {
            if (!g_srv_stop) usleep(10000);
            continue;
        }
        server_handle_client(client_fd);
    }
    return NULL;
}

static bool server_start(int port, ServerMode mode, int status) {
    g_srv_stop = 0;
    g_srv_port = port;
    g_srv_mode = mode;
    g_srv_status = status;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(fd);
        return false;
    }
    if (listen(fd, 8) != 0) {
        close(fd);
        return false;
    }
    g_srv_listen_fd = fd;
    if (pthread_create(&g_srv_thread, NULL, server_thread_func, NULL) != 0) {
        close(fd);
        g_srv_listen_fd = -1;
        return false;
    }
    usleep(50000); // let the listener come up
    return true;
}

static void server_stop(void) {
    g_srv_stop = 1;
    if (g_srv_listen_fd >= 0) {
        close(g_srv_listen_fd);
        g_srv_listen_fd = -1;
    }
    pthread_join(g_srv_thread, NULL);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ChatEngineConfig make_engine(const char* url) {
    ChatEngineConfig engine;
    memset(&engine, 0, sizeof(engine));
    engine.provider = CEC_PROVIDER_OPENAI;
    strncpy(engine.name, "test-engine", sizeof(engine.name) - 1);
    strncpy(engine.api_key, "test-key", sizeof(engine.api_key) - 1);
    strncpy(engine.api_url, url, sizeof(engine.api_url) - 1);
    return engine;
}

static ChatEngineConfig make_engine_p(ChatEngineProvider provider, const char* url) {
    ChatEngineConfig engine = make_engine(url);
    engine.provider = provider;
    return engine;
}

static char g_url_buf[256];
static char* local_url(int port) {
    snprintf(g_url_buf, sizeof(g_url_buf), "http://127.0.0.1:%d/api", port);
    return g_url_buf;
}

// SSE stream callback state
static volatile int g_stream_chunks = 0;
static void stream_chunk_cb(const ChatStreamChunk* chunk, void* user_data) {
    (void)chunk;
    (void)user_data;
    g_stream_chunks++;
}

// ---------------------------------------------------------------------------
// Test function prototypes
// ---------------------------------------------------------------------------
void test_init_cleanup(void);
void test_send_request_invalid_params(void);
void test_send_request_success(void);
void test_send_request_http_4xx(void);
void test_send_request_http_5xx(void);
void test_send_request_connect_failure(void);
void test_send_request_buffer_alloc_failure(void);
void test_send_with_retry_no_retry_break(void);
void test_send_with_retry_backoff(void);
void test_send_multi_success(void);
void test_send_multi_connect_failure(void);
void test_send_stream_success(void);
// Additional coverage
void test_result_code_to_string(void);
void test_result_is_success(void);
void test_result_should_retry(void);
void test_get_multi_manager(void);
void test_write_callback_basic(void);
void test_write_callback_max_size(void);
void test_write_callback_realloc_failure(void);
void test_send_request_anthropic_success(void);
void test_send_request_anthropic_4xx(void);
void test_send_multi_anthropic_success(void);
void test_send_multi_invalid_params(void);
void test_send_multi_4xx(void);
void test_send_multi_5xx(void);
void test_multi_result_create_failures(void);
void test_send_multi_null_args(void);
void test_send_stream_anthropic_success(void);
void test_send_stream_null_params(void);
void test_send_stream_malloc_failure(void);
void test_send_stream_connect_error(void);
void test_send_stream_trailing_flush(void);
void test_cleanup_completed_streams_empty(void);
void test_send_request_result_create_fail(void);
void test_send_request_unknown_http(void);
void test_send_multi_contexts_alloc_fail(void);
void test_send_multi_buffer_alloc_fail(void);
void test_send_stream_strdup_fail(void);

void setUp(void) {
    mock_system_reset_all();
    g_stream_chunks = 0;
    chat_proxy_init();
}

void tearDown(void) {
    chat_proxy_cleanup();
    mock_system_reset_all();
}

// ---------------------------------------------------------------------------

void test_init_cleanup(void) {
    // chat_proxy_init already called in setUp; re-init is safe (ref-counted)
    TEST_ASSERT_TRUE(chat_proxy_init());
    chat_proxy_cleanup();
    // Re-initialize for any later tests in the run
    TEST_ASSERT_TRUE(chat_proxy_init());
}

void test_send_request_invalid_params(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatEngineConfig engine = make_engine("http://127.0.0.1:1/x");

    ChatProxyResult* r1 = chat_proxy_send_request(NULL, "{}", &config);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r1->code);
    chat_proxy_result_destroy(r1);

    ChatProxyResult* r2 = chat_proxy_send_request(&engine, NULL, &config);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r2->code);
    chat_proxy_result_destroy(r2);

    ChatProxyResult* r3 = chat_proxy_send_request(&engine, "{}", NULL);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r3->code);
    chat_proxy_result_destroy(r3);
}

void test_send_request_success(void) {
    TEST_ASSERT_TRUE(server_start(19011, SRV_MODE_200, 200));
    ChatEngineConfig engine = make_engine(local_url(19011));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{\"model\":\"x\"}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, result->code);
    TEST_ASSERT_EQUAL_INT(200, result->http_status);
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(result));
    TEST_ASSERT_NOT_NULL(result->response_body);
    TEST_ASSERT_EQUAL_size_t(strlen("{\"id\":\"x\",\"choices\":[{\"message\":{\"content\":\"ok\"}}]}"),
                              result->response_size);
    chat_proxy_result_destroy(result);
}

void test_send_request_http_4xx(void) {
    TEST_ASSERT_TRUE(server_start(19012, SRV_MODE_STATUS, 404));
    ChatEngineConfig engine = make_engine(local_url(19012));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_4XX, result->code);
    TEST_ASSERT_EQUAL_INT(404, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_request_http_5xx(void) {
    TEST_ASSERT_TRUE(server_start(19013, SRV_MODE_STATUS, 503));
    ChatEngineConfig engine = make_engine(local_url(19013));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_5XX, result->code);
    TEST_ASSERT_EQUAL_INT(503, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_request_connect_failure(void) {
    // Nothing listening on this port -> CURLE_COULDNT_CONNECT
    ChatEngineConfig engine = make_engine("http://127.0.0.1:19999/api");
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_CONNECT, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_request_buffer_alloc_failure(void) {
    // Make the 2nd intercepted allocation (the response buffer malloc) fail.
    // chat_proxy_result_create()'s calloc is allocation #1; the buffer
    // malloc(INITIAL_RESPONSE_BUFFER_SIZE) is #2. (proxy.c is compiled with
    // USE_MOCK_SYSTEM, so its malloc is intercepted by the global mock.)
    mock_system_set_malloc_failure(2);

    ChatEngineConfig engine = make_engine("http://127.0.0.1:19999/api");
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_MEMORY, result->code);
    chat_proxy_result_destroy(result);
    mock_system_reset_all();
}

void test_send_with_retry_no_retry_break(void) {
    // Unreachable endpoint with zero retries: the loop runs once, sees a
    // retryable connect error, but max_attempts == 1 so it breaks without sleeping.
    ChatEngineConfig engine = make_engine("http://127.0.0.1:19998/api");
    ChatProxyConfig config = chat_proxy_get_default_config();
    config.max_retries = 0;

    ChatProxyResult* result = chat_proxy_send_with_retry(&engine, "{}", &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_CONNECT, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_with_retry_backoff(void) {
    // Server returns 503 (retryable). With max_retries=1 we exercise the
    // exponential-backoff+sleep path on the first failure, then give up.
    TEST_ASSERT_TRUE(server_start(19014, SRV_MODE_STATUS, 503));
    ChatEngineConfig engine = make_engine(local_url(19014));
    ChatProxyConfig config = chat_proxy_get_default_config();
    config.max_retries = 1;

    ChatProxyResult* result = chat_proxy_send_with_retry(&engine, "{}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_5XX, result->code);
    TEST_ASSERT_EQUAL_INT(503, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_multi_success(void) {
    TEST_ASSERT_TRUE(server_start(19015, SRV_MODE_200, 200));
    ChatEngineConfig engine = make_engine(local_url(19015));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatMultiRequest reqs[2];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = &engine;
    reqs[0].request_json = "{\"model\":\"x\"}";
    reqs[1].engine = &engine;
    reqs[1].request_json = "{\"model\":\"y\"}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 2, &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(2, multi->count);
    TEST_ASSERT_EQUAL_size_t(2, multi->success_count);
    TEST_ASSERT_EQUAL_size_t(0, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, multi->results[0]->code);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, multi->results[1]->code);
    chat_multi_result_destroy(multi);
}

void test_send_multi_connect_failure(void) {
    ChatEngineConfig engine = make_engine("http://127.0.0.1:19997/api");
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = &engine;
    reqs[0].request_json = "{}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->count);
    TEST_ASSERT_EQUAL_size_t(0, multi->success_count);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_NETWORK, multi->results[0]->code);
    chat_multi_result_destroy(multi);
}

void test_send_stream_success(void) {
    TEST_ASSERT_TRUE(server_start(19016, SRV_MODE_SSE, 200));
    ChatEngineConfig engine = make_engine(local_url(19016));
    ChatProxyConfig config = chat_proxy_get_streaming_config();

    bool started = chat_proxy_send_stream(&engine, "{\"stream\":true}", &config,
                                           stream_chunk_cb, NULL);
    TEST_ASSERT_TRUE(started);

    // Wait for the worker thread to deliver at least one chunk (bounded poll)
    int waited = 0;
    while (g_stream_chunks == 0 && waited < 100) {
        usleep(20000);
        waited++;
    }
    TEST_ASSERT_TRUE(g_stream_chunks > 0);

    // Give the worker a moment to finish, then reap the completed stream
    usleep(100000);
    chat_proxy_cleanup_completed_streams();
    server_stop();
}

// ---------------------------------------------------------------------------
// Additional coverage tests
// ---------------------------------------------------------------------------

void test_result_code_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("Success", chat_proxy_result_code_to_string(CHAT_PROXY_OK));
    TEST_ASSERT_EQUAL_STRING("CURL initialization failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_INIT));
    TEST_ASSERT_EQUAL_STRING("Connection failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_CONNECT));
    TEST_ASSERT_EQUAL_STRING("Request timeout", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("HTTP client error (4xx)", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_HTTP_4XX));
    TEST_ASSERT_EQUAL_STRING("HTTP server error (5xx)", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_HTTP_5XX));
    TEST_ASSERT_EQUAL_STRING("Memory allocation failed", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_MEMORY));
    TEST_ASSERT_EQUAL_STRING("Network error", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_NETWORK));
    TEST_ASSERT_EQUAL_STRING("Unknown error", chat_proxy_result_code_to_string(CHAT_PROXY_ERROR_UNKNOWN));
    TEST_ASSERT_EQUAL_STRING("Unknown", chat_proxy_result_code_to_string((ChatProxyResultCode)999));
}

void test_result_is_success(void) {
    ChatProxyResult* r = chat_proxy_result_create();
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(NULL));
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(r));  // code=UNKNOWN, status=0
    r->code = CHAT_PROXY_OK;
    r->http_status = 200;
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(r));
    r->http_status = 199;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(r));
    r->http_status = 300;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(r));
    r->http_status = 299;
    r->code = CHAT_PROXY_ERROR_INIT;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(r));
    chat_proxy_result_destroy(r);
}

void test_result_should_retry(void) {
    ChatProxyResult* r = chat_proxy_result_create();
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(NULL));
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(r));

    r->code = CHAT_PROXY_ERROR_TIMEOUT;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));
    r->code = CHAT_PROXY_ERROR_NETWORK;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));
    r->code = CHAT_PROXY_ERROR_CONNECT;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));

    r->code = CHAT_PROXY_ERROR_INIT;
    r->http_status = 429;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));
    r->http_status = 502;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));
    r->http_status = 503;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));
    r->http_status = 504;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(r));

    r->code = CHAT_PROXY_OK;
    r->http_status = 200;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(r));
    r->http_status = 500;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(r));
    chat_proxy_result_destroy(r);
}

void test_get_multi_manager(void) {
    MultiStreamManager* mgr = chat_proxy_get_multi_manager();
    TEST_ASSERT_NOT_NULL(mgr);
}

void test_write_callback_basic(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(64);
    buffer.size = 0;
    buffer.capacity = 64;

    const char* chunk = "hello";
    size_t got = chat_proxy_write_callback(chunk, 5, 1, &buffer);
    TEST_ASSERT_EQUAL_size_t(5, got);
    TEST_ASSERT_EQUAL_size_t(5, buffer.size);
    TEST_ASSERT_EQUAL_STRING("hello", buffer.data);

    got = chat_proxy_write_callback(chunk, 5, 1, &buffer);
    TEST_ASSERT_EQUAL_size_t(5, got);
    TEST_ASSERT_EQUAL_size_t(10, buffer.size);
    TEST_ASSERT_EQUAL_STRING("hellohello", buffer.data);

    free(buffer.data);
}

void test_write_callback_max_size(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(16);
    buffer.size = 0;
    buffer.capacity = 16;

    // Implied size far exceeds MAX_RESPONSE_SIZE: triggers the size guard.
    char dummy = 'x';
    size_t n = chat_proxy_write_callback(&dummy, 9 * 1024 * 1024, 1, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, n);  // signals CURL to abort

    free(buffer.data);
}

void test_write_callback_realloc_failure(void) {
    ChatResponseBuffer buffer;
    buffer.data = (char*)malloc(16);
    buffer.size = 0;
    buffer.capacity = 16;

    mock_system_set_realloc_failure(1);
    char dummy = 'x';
    // 32 bytes exceeds capacity(16) -> realloc attempted -> mocked to fail
    size_t n = chat_proxy_write_callback(&dummy, 32, 1, &buffer);
    TEST_ASSERT_EQUAL_size_t(0, n);

    mock_system_reset_all();
    free(buffer.data);
}

void test_send_request_anthropic_success(void) {
    TEST_ASSERT_TRUE(server_start(19121, SRV_MODE_200, 200));
    ChatEngineConfig engine = make_engine_p(CEC_PROVIDER_ANTHROPIC, local_url(19121));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{\"model\":\"x\"}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, result->code);
    TEST_ASSERT_EQUAL_INT(200, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_request_anthropic_4xx(void) {
    TEST_ASSERT_TRUE(server_start(19122, SRV_MODE_STATUS, 400));
    ChatEngineConfig engine = make_engine_p(CEC_PROVIDER_ANTHROPIC, local_url(19122));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_4XX, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_multi_anthropic_success(void) {
    TEST_ASSERT_TRUE(server_start(19123, SRV_MODE_200, 200));
    ChatEngineConfig engine = make_engine_p(CEC_PROVIDER_ANTHROPIC, local_url(19123));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatMultiRequest reqs[2];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = &engine;
    reqs[0].request_json = "{\"model\":\"x\"}";
    reqs[1].engine = &engine;
    reqs[1].request_json = "{\"model\":\"y\"}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 2, &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(2, multi->count);
    TEST_ASSERT_EQUAL_size_t(2, multi->success_count);
    TEST_ASSERT_EQUAL_size_t(0, multi->failure_count);
    chat_multi_result_destroy(multi);
}

void test_send_multi_invalid_params(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    // Per-request engine is NULL -> error during setup (invalid parameters)
    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = NULL;
    reqs[0].request_json = "{}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->count);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, multi->results[0]->code);
    chat_multi_result_destroy(multi);
}

void test_send_multi_4xx(void) {
    TEST_ASSERT_TRUE(server_start(19124, SRV_MODE_STATUS, 404));
    ChatEngineConfig engine = make_engine(local_url(19124));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = &engine;
    reqs[0].request_json = "{}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_4XX, multi->results[0]->code);
    chat_multi_result_destroy(multi);
}

void test_send_multi_5xx(void) {
    TEST_ASSERT_TRUE(server_start(19125, SRV_MODE_STATUS, 503));
    ChatEngineConfig engine = make_engine(local_url(19125));
    ChatProxyConfig config = chat_proxy_get_default_config();

    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = &engine;
    reqs[0].request_json = "{}";

    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    server_stop();

    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_5XX, multi->results[0]->code);
    chat_multi_result_destroy(multi);
}

void test_multi_result_create_failures(void) {
    // calloc(multi) fails -> returns NULL from chat_multi_result_create
    mock_system_set_malloc_failure(1);
    ChatMultiResult* m1 = chat_multi_result_create(2);
    TEST_ASSERT_NULL(m1);
    mock_system_reset_all();

    // calloc(results array) fails -> frees multi, returns NULL
    mock_system_set_malloc_failure(2);
    ChatMultiResult* m2 = chat_multi_result_create(2);
    TEST_ASSERT_NULL(m2);
    mock_system_reset_all();
}

void test_send_multi_null_args(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    TEST_ASSERT_NULL(chat_proxy_send_multi(NULL, 0, &config));
    TEST_ASSERT_NULL(chat_proxy_send_multi(NULL, 5, &config));
}

void test_send_stream_anthropic_success(void) {
    TEST_ASSERT_TRUE(server_start(19126, SRV_MODE_SSE, 200));
    ChatEngineConfig engine = make_engine_p(CEC_PROVIDER_ANTHROPIC, local_url(19126));
    ChatProxyConfig config = chat_proxy_get_streaming_config();

    bool started = chat_proxy_send_stream(&engine, "{\"stream\":true}", &config,
                                          stream_chunk_cb, NULL);
    TEST_ASSERT_TRUE(started);

    int waited = 0;
    while (g_stream_chunks == 0 && waited < 100) {
        usleep(20000);
        waited++;
    }
    TEST_ASSERT_TRUE(g_stream_chunks > 0);

    usleep(100000);
    chat_proxy_cleanup_completed_streams();
    server_stop();
}

void test_send_stream_null_params(void) {
    ChatEngineConfig engine = make_engine("http://127.0.0.1:1/x");
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    TEST_ASSERT_FALSE(chat_proxy_send_stream(NULL, "{}", &config, stream_chunk_cb, NULL));
    TEST_ASSERT_FALSE(chat_proxy_send_stream(&engine, NULL, &config, stream_chunk_cb, NULL));
    TEST_ASSERT_FALSE(chat_proxy_send_stream(&engine, "{}", &config, NULL, NULL));
}

void test_send_stream_malloc_failure(void) {
    // The StreamRequest calloc is the first allocation in chat_proxy_send_stream
    mock_system_set_malloc_failure(1);
    ChatEngineConfig engine = make_engine("http://127.0.0.1:1/x");
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    bool started = chat_proxy_send_stream(&engine, "{}", &config, stream_chunk_cb, NULL);
    TEST_ASSERT_FALSE(started);
    mock_system_reset_all();
}

void test_send_stream_connect_error(void) {
    // Worker thread fails to connect; exercises the streaming CURL-error path.
    ChatEngineConfig engine = make_engine("http://127.0.0.1:19996/api");
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    bool started = chat_proxy_send_stream(&engine, "{}", &config, stream_chunk_cb, NULL);
    TEST_ASSERT_TRUE(started);
    int waited = 0;
    while (waited < 100) {
        chat_proxy_cleanup_completed_streams();
        usleep(20000);
        waited++;
    }
}

void test_send_stream_trailing_flush(void) {
    // SSE server sends a final line WITHOUT a trailing newline: the worker must
    // flush the lingering line buffer and parse it as a final chunk.
    TEST_ASSERT_TRUE(server_start(19127, SRV_MODE_SSE_NOTRAIL, 200));
    ChatEngineConfig engine = make_engine(local_url(19127));
    ChatProxyConfig config = chat_proxy_get_streaming_config();

    bool started = chat_proxy_send_stream(&engine, "{\"stream\":true}", &config,
                                          stream_chunk_cb, NULL);
    TEST_ASSERT_TRUE(started);

    int waited = 0;
    while (g_stream_chunks == 0 && waited < 100) {
        usleep(20000);
        waited++;
    }
    TEST_ASSERT_TRUE(g_stream_chunks > 0);

    usleep(100000);
    chat_proxy_cleanup_completed_streams();
    server_stop();
}

void test_cleanup_completed_streams_empty(void) {
    // No active streams -> function simply returns; exercises the entry path.
    chat_proxy_cleanup_completed_streams();
}

void test_send_request_result_create_fail(void) {
    // First allocation (chat_proxy_result_create's calloc) fails -> NULL result
    mock_system_set_malloc_failure(1);
    ChatEngineConfig engine = make_engine("http://127.0.0.1:19994/api");
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    TEST_ASSERT_NULL(result);
    mock_system_reset_all();
}

void test_send_request_unknown_http(void) {
    // 302 (redirect, no Location) is neither success (2xx) nor 4xx/5xx and is
    // not followed (no Location) -> ERROR_UNKNOWN
    TEST_ASSERT_TRUE(server_start(19128, SRV_MODE_STATUS, 302));
    ChatEngineConfig engine = make_engine(local_url(19128));
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatProxyResult* result = chat_proxy_send_request(&engine, "{}", &config);
    server_stop();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_UNKNOWN, result->code);
    TEST_ASSERT_EQUAL_INT(302, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_multi_contexts_alloc_fail(void) {
    // 3rd allocation (the contexts array calloc) fails -> send_multi returns NULL
    mock_system_set_malloc_failure(3);
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    reqs[0].engine = NULL;  // value irrelevant; fails before use
    reqs[0].request_json = "{}";
    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    TEST_ASSERT_NULL(multi);
    mock_system_reset_all();
}

void test_send_multi_buffer_alloc_fail(void) {
    // 4th allocation (per-request response buffer malloc) fails -> that request
    // is recorded as a memory failure; the call still returns a multi result.
    mock_system_set_malloc_failure(4);
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatMultiRequest reqs[1];
    memset(reqs, 0, sizeof(reqs));
    ChatEngineConfig eng = make_engine_p(CEC_PROVIDER_OPENAI, "http://127.0.0.1:19993/api");
    reqs[0].engine = &eng;
    reqs[0].request_json = "{}";
    ChatMultiResult* multi = chat_proxy_send_multi(reqs, 1, &config);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->count);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_MEMORY, multi->results[0]->code);
    chat_multi_result_destroy(multi);
    mock_system_reset_all();
}

void test_send_stream_strdup_fail(void) {
    // The request_json strdup is the 2nd allocation in chat_proxy_send_stream;
    // if it fails the function returns false.
    mock_system_set_malloc_failure(2);
    ChatEngineConfig engine = make_engine("http://127.0.0.1:1/x");
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    bool started = chat_proxy_send_stream(&engine, "{}", &config, stream_chunk_cb, NULL);
    TEST_ASSERT_FALSE(started);
    mock_system_reset_all();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_init_cleanup);
    RUN_TEST(test_send_request_invalid_params);
    RUN_TEST(test_send_request_success);
    RUN_TEST(test_send_request_http_4xx);
    RUN_TEST(test_send_request_http_5xx);
    RUN_TEST(test_send_request_connect_failure);
    RUN_TEST(test_send_request_buffer_alloc_failure);
    RUN_TEST(test_send_with_retry_no_retry_break);
    RUN_TEST(test_send_with_retry_backoff);
    RUN_TEST(test_send_multi_success);
    RUN_TEST(test_send_multi_connect_failure);
    RUN_TEST(test_send_stream_success);
    RUN_TEST(test_result_code_to_string);
    RUN_TEST(test_result_is_success);
    RUN_TEST(test_result_should_retry);
    RUN_TEST(test_get_multi_manager);
    RUN_TEST(test_write_callback_basic);
    RUN_TEST(test_write_callback_max_size);
    RUN_TEST(test_write_callback_realloc_failure);
    RUN_TEST(test_send_request_anthropic_success);
    RUN_TEST(test_send_request_anthropic_4xx);
    RUN_TEST(test_send_multi_anthropic_success);
    RUN_TEST(test_send_multi_invalid_params);
    RUN_TEST(test_send_multi_4xx);
    RUN_TEST(test_send_multi_5xx);
    RUN_TEST(test_multi_result_create_failures);
    RUN_TEST(test_send_multi_null_args);
    RUN_TEST(test_send_stream_anthropic_success);
    RUN_TEST(test_send_stream_null_params);
    RUN_TEST(test_send_stream_malloc_failure);
    RUN_TEST(test_send_stream_connect_error);
    RUN_TEST(test_send_stream_trailing_flush);
    RUN_TEST(test_cleanup_completed_streams_empty);
    RUN_TEST(test_send_request_result_create_fail);
    RUN_TEST(test_send_request_unknown_http);
    RUN_TEST(test_send_multi_contexts_alloc_fail);
    RUN_TEST(test_send_multi_buffer_alloc_fail);
    RUN_TEST(test_send_stream_strdup_fail);

    return UNITY_END();
}
