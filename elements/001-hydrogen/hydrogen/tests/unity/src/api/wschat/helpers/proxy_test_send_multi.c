/*
 * Unity Test File: chat_proxy_send_multi
 * This file contains unit tests for chat_proxy_send_multi() in
 * src/api/wschat/helpers/proxy.c
 *
 * CHANGELOG:
 * 2026-09-05: Cover guards, invalid engine, connect fail, HTTP 200
 *
 * TEST_VERSION: 1.0.0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/engine_cache.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

void test_send_multi_null_or_empty(void);
void test_send_multi_invalid_engine(void);
void test_send_multi_connect_fail(void);
void test_send_multi_http_200(void);

typedef struct {
    volatile int port;
    volatile int ready;
} HttpStub;

static void *http_stub_thread(void *arg) {
    HttpStub *stub = (HttpStub *)arg;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    struct sockaddr_in addr;
    socklen_t alen = sizeof(addr);
    if (s < 0) {
        stub->ready = -1;
        return NULL;
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(s, 1) != 0) {
        close(s);
        stub->ready = -1;
        return NULL;
    }
    getsockname(s, (struct sockaddr *)&addr, &alen);
    stub->port = ntohs(addr.sin_port);
    stub->ready = 1;
    int c = accept(s, NULL, NULL);
    if (c >= 0) {
        char req[1024];
        char resp[256];
        (void)recv(c, req, sizeof(req), 0);
        snprintf(resp, sizeof(resp),
                 "HTTP/1.1 200 X\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
        (void)send(c, resp, strlen(resp), 0);
        close(c);
    }
    close(s);
    return NULL;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_send_multi_null_or_empty(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatMultiRequest req;
    memset(&req, 0, sizeof(req));
    TEST_ASSERT_NULL(chat_proxy_send_multi(NULL, 1, &config));
    TEST_ASSERT_NULL(chat_proxy_send_multi(&req, 0, &config));
}

void test_send_multi_invalid_engine(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatMultiRequest req;
    memset(&req, 0, sizeof(req));
    req.engine = NULL;
    req.request_json = "{}";
    ChatMultiResult *multi = chat_proxy_send_multi(&req, 1, &config);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    chat_multi_result_destroy(multi);
}

void test_send_multi_connect_fail(void) {
    ChatEngineConfig engine;
    ChatProxyConfig config;
    ChatMultiRequest req;
    memset(&engine, 0, sizeof(engine));
    snprintf(engine.name, sizeof(engine.name), "multi");
    snprintf(engine.api_url, sizeof(engine.api_url), "http://127.0.0.1:1/");
    snprintf(engine.api_key, sizeof(engine.api_key), "k");
    config = chat_proxy_get_default_config();
    config.connect_timeout_seconds = 1;
    config.request_timeout_seconds = 1;
    config.verify_ssl = false;
    memset(&req, 0, sizeof(req));
    req.engine = &engine;
    req.request_json = "{}";
    ChatMultiResult *multi = chat_proxy_send_multi(&req, 1, &config);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->failure_count);
    chat_multi_result_destroy(multi);
}

void test_send_multi_http_200(void) {
    HttpStub stub;
    pthread_t tid;
    ChatEngineConfig engine;
    ChatProxyConfig config;
    ChatMultiRequest req;
    char url[128];
    int spins = 0;

    memset(&stub, 0, sizeof(stub));
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, http_stub_thread, &stub));
    while (!stub.ready && spins < 1000) {
        usleep(1000);
        spins++;
    }
    TEST_ASSERT_EQUAL_INT(1, stub.ready);
    memset(&engine, 0, sizeof(engine));
    snprintf(engine.name, sizeof(engine.name), "multi");
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/", stub.port);
    snprintf(engine.api_url, sizeof(engine.api_url), "%s", url);
    snprintf(engine.api_key, sizeof(engine.api_key), "k");
    config = chat_proxy_get_default_config();
    config.verify_ssl = false;
    memset(&req, 0, sizeof(req));
    req.engine = &engine;
    req.request_json = "{}";
    ChatMultiResult *multi = chat_proxy_send_multi(&req, 1, &config);
    pthread_join(tid, NULL);
    TEST_ASSERT_NOT_NULL(multi);
    TEST_ASSERT_EQUAL_size_t(1, multi->success_count);
    chat_multi_result_destroy(multi);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_multi_null_or_empty);
    RUN_TEST(test_send_multi_invalid_engine);
    RUN_TEST(test_send_multi_connect_fail);
    RUN_TEST(test_send_multi_http_200);
    return UNITY_END();
}
