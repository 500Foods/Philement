/*
 * Unity Test File: chat_proxy_send_request
 * This file contains unit tests for chat_proxy_send_request() in
 * src/api/wschat/helpers/proxy.c
 *
 * CHANGELOG:
 * 2026-09-05: Cover invalid params, connect failure, and HTTP status classes
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

void test_send_request_invalid_params(void);
void test_send_request_empty_url(void);
void test_send_request_connect_fail(void);
void test_send_request_http_200(void);
void test_send_request_http_400(void);
void test_send_request_http_500(void);
void test_send_request_anthropic_headers(void);

typedef struct {
    int status;
    const char *body;
    volatile int port;
    volatile int ready;
    volatile int stop;
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
    if (getsockname(s, (struct sockaddr *)&addr, &alen) != 0) {
        close(s);
        stub->ready = -1;
        return NULL;
    }
    stub->port = ntohs(addr.sin_port);
    stub->ready = 1;
    int c = accept(s, NULL, NULL);
    if (c >= 0) {
        char req[1024];
        char resp[512];
        (void)recv(c, req, sizeof(req), 0);
        snprintf(resp, sizeof(resp),
                 "HTTP/1.1 %d X\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
                 stub->status, stub->body ? strlen(stub->body) : 0,
                 stub->body ? stub->body : "");
        (void)send(c, resp, strlen(resp), 0);
        close(c);
    }
    close(s);
    stub->stop = 1;
    return NULL;
}

static void fill_engine(ChatEngineConfig *engine, const char *url, ChatEngineProvider provider) {
    memset(engine, 0, sizeof(*engine));
    snprintf(engine->name, sizeof(engine->name), "test-engine");
    snprintf(engine->api_url, sizeof(engine->api_url), "%s", url);
    snprintf(engine->api_key, sizeof(engine->api_key), "secret");
    engine->provider = provider;
}

static ChatProxyResult *request_against_stub(int status, ChatEngineProvider provider) {
    HttpStub stub;
    pthread_t tid;
    ChatEngineConfig engine;
    ChatProxyConfig config;
    char url[128];
    int spins = 0;

    memset(&stub, 0, sizeof(stub));
    stub.status = status;
    stub.body = "{\"ok\":true}";
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&tid, NULL, http_stub_thread, &stub));
    while (!stub.ready && spins < 1000) {
        usleep(1000);
        spins++;
    }
    TEST_ASSERT_EQUAL_INT(1, stub.ready);
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/v1", stub.port);
    fill_engine(&engine, url, provider);
    config = chat_proxy_get_default_config();
    config.verify_ssl = false;
    config.connect_timeout_seconds = 2;
    config.request_timeout_seconds = 5;
    ChatProxyResult *result = chat_proxy_send_request(&engine, "{\"stream\":false}", &config);
    pthread_join(tid, NULL);
    return result;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_send_request_invalid_params(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatEngineConfig engine;
    fill_engine(&engine, "http://127.0.0.1:1", CEC_PROVIDER_OPENAI);
    ChatProxyResult *r1 = chat_proxy_send_request(NULL, "{}", &config);
    ChatProxyResult *r2 = chat_proxy_send_request(&engine, NULL, &config);
    ChatProxyResult *r3 = chat_proxy_send_request(&engine, "{}", NULL);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r1->code);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r2->code);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, r3->code);
    chat_proxy_result_destroy(r1);
    chat_proxy_result_destroy(r2);
    chat_proxy_result_destroy(r3);
}

void test_send_request_empty_url(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatEngineConfig engine;
    fill_engine(&engine, "", CEC_PROVIDER_OPENAI);
    ChatProxyResult *result = chat_proxy_send_request(&engine, "{}", &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_INIT, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_request_connect_fail(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    ChatEngineConfig engine;
    fill_engine(&engine, "http://127.0.0.1:1/", CEC_PROVIDER_OPENAI);
    config.connect_timeout_seconds = 1;
    config.request_timeout_seconds = 1;
    config.verify_ssl = false;
    ChatProxyResult *result = chat_proxy_send_request(&engine, "{}", &config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_EQUAL(CHAT_PROXY_OK, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_request_http_200(void) {
    ChatProxyResult *result = request_against_stub(200, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, result->code);
    TEST_ASSERT_EQUAL_INT(200, result->http_status);
    chat_proxy_result_destroy(result);
}

void test_send_request_http_400(void) {
    ChatProxyResult *result = request_against_stub(400, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_4XX, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_request_http_500(void) {
    ChatProxyResult *result = request_against_stub(500, CEC_PROVIDER_OPENAI);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_HTTP_5XX, result->code);
    chat_proxy_result_destroy(result);
}

void test_send_request_anthropic_headers(void) {
    ChatProxyResult *result = request_against_stub(200, CEC_PROVIDER_ANTHROPIC);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_OK, result->code);
    chat_proxy_result_destroy(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_send_request_invalid_params);
    RUN_TEST(test_send_request_empty_url);
    RUN_TEST(test_send_request_connect_fail);
    RUN_TEST(test_send_request_http_200);
    RUN_TEST(test_send_request_http_400);
    RUN_TEST(test_send_request_http_500);
    RUN_TEST(test_send_request_anthropic_headers);
    return UNITY_END();
}
