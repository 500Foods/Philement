/*
 * Unity Test File: nats_parser_feed
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/nats/nats.h>

#include <string.h>

static char got_subject[256];
static char got_sid[32];
static char got_reply[256];
static char got_body[64];
static size_t got_len;
static int got_count;

static void capture_msg(const char *subject, const char *sid, const char *reply,
                        const void *data, size_t len) {
    got_count++;
    snprintf(got_subject, sizeof(got_subject), "%s", subject ? subject : "");
    snprintf(got_sid, sizeof(got_sid), "%s", sid ? sid : "");
    snprintf(got_reply, sizeof(got_reply), "%s", reply ? reply : "");
    got_len = len < sizeof(got_body) ? len : sizeof(got_body) - 1;
    if (data && got_len > 0) {
        memcpy(got_body, data, got_len);
    }
    got_body[got_len] = '\0';
}

void test_nats_parser_feed_split_msg(void);
void test_nats_parser_feed_two_ops(void);
void test_nats_parser_feed_empty_payload(void);
void test_nats_parser_feed_reply_token(void);
void test_nats_parser_feed_bad_trailer(void);
void test_nats_parser_feed_err(void);
void test_nats_parser_feed_unknown_op(void);
void test_nats_parser_feed_max_payload(void);
void test_nats_parser_take_ctrl_keeps_buffer_when_short(void);

void setUp(void) {
    nats_client_reset();
    nats_msg_set_handler(capture_msg);
    got_count = 0;
    got_len = 0;
    got_subject[0] = '\0';
    got_sid[0] = '\0';
    got_reply[0] = '\0';
    got_body[0] = '\0';
}

void tearDown(void) {
    nats_client_reset();
}

void test_nats_parser_feed_split_msg(void) {
    const char *head = "MSG alpha 7 5\r\n";
    const char *tail = "hello\r\n";

    TEST_ASSERT_EQUAL(0, nats_parser_feed(head, strlen(head)));
    TEST_ASSERT_EQUAL(0, got_count);
    TEST_ASSERT_EQUAL(0, nats_parser_feed(tail, strlen(tail)));
    TEST_ASSERT_EQUAL(1, got_count);
    TEST_ASSERT_EQUAL_STRING("alpha", got_subject);
    TEST_ASSERT_EQUAL_STRING("7", got_sid);
    TEST_ASSERT_EQUAL_STRING("hello", got_body);
    TEST_ASSERT_EQUAL(5, got_len);
}

void test_nats_parser_feed_two_ops(void) {
    const char *buf = "PING\r\n+OK\r\n";
    char ctrl[64];
    int n;

    TEST_ASSERT_EQUAL(0, nats_parser_feed(buf, strlen(buf)));
    n = nats_parser_take_ctrl(ctrl, sizeof(ctrl));
    TEST_ASSERT_EQUAL(6, n);
    TEST_ASSERT_EQUAL_STRING("PONG\r\n", ctrl);
}

void test_nats_parser_feed_empty_payload(void) {
    const char *buf = "MSG demo 1 0\r\n\r\n";

    TEST_ASSERT_EQUAL(0, nats_parser_feed(buf, strlen(buf)));
    TEST_ASSERT_EQUAL(1, got_count);
    TEST_ASSERT_EQUAL(0, got_len);
    TEST_ASSERT_EQUAL_STRING("demo", got_subject);
}

void test_nats_parser_feed_reply_token(void) {
    const char *buf = "MSG demo 1 reply.inbox 3\r\nabc\r\n";

    TEST_ASSERT_EQUAL(0, nats_parser_feed(buf, strlen(buf)));
    TEST_ASSERT_EQUAL(1, got_count);
    TEST_ASSERT_EQUAL_STRING("demo", got_subject);
    TEST_ASSERT_EQUAL_STRING("reply.inbox", got_reply);
    TEST_ASSERT_EQUAL_STRING("abc", got_body);
}

void test_nats_parser_feed_bad_trailer(void) {
    const char *buf = "MSG demo 1 1\r\nX\nZ";

    TEST_ASSERT_EQUAL(-1, nats_parser_feed(buf, strlen(buf)));
    TEST_ASSERT_EQUAL(0, got_count);
}

void test_nats_parser_feed_err(void) {
    const char *buf = "-ERR 'Permissions Violation'\r\n";

    TEST_ASSERT_EQUAL(-1, nats_parser_feed(buf, strlen(buf)));
}

void test_nats_parser_feed_unknown_op(void) {
    const char *buf = "HMSG demo 1 0\r\n\r\n";

    TEST_ASSERT_EQUAL(-1, nats_parser_feed(buf, strlen(buf)));
}

void test_nats_parser_feed_max_payload(void) {
    const char *buf = "INFO {\"max_payload\":4}\r\nMSG demo 1 5\r\nhello\r\n";

    TEST_ASSERT_EQUAL(-1, nats_parser_feed(buf, strlen(buf)));
    TEST_ASSERT_EQUAL(0, got_count);
}

void test_nats_parser_take_ctrl_keeps_buffer_when_short(void) {
    char tiny[2];
    char full[16];

    TEST_ASSERT_EQUAL(0, nats_parser_feed("PING\r\n", 6));
    TEST_ASSERT_EQUAL(-1, nats_parser_take_ctrl(tiny, sizeof(tiny)));
    TEST_ASSERT_EQUAL(6, nats_parser_take_ctrl(full, sizeof(full)));
    TEST_ASSERT_EQUAL_STRING("PONG\r\n", full);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_parser_feed_split_msg);
    RUN_TEST(test_nats_parser_feed_two_ops);
    RUN_TEST(test_nats_parser_feed_empty_payload);
    RUN_TEST(test_nats_parser_feed_reply_token);
    RUN_TEST(test_nats_parser_feed_bad_trailer);
    RUN_TEST(test_nats_parser_feed_err);
    RUN_TEST(test_nats_parser_feed_unknown_op);
    RUN_TEST(test_nats_parser_feed_max_payload);
    RUN_TEST(test_nats_parser_take_ctrl_keeps_buffer_when_short);
    return UNITY_END();
}
