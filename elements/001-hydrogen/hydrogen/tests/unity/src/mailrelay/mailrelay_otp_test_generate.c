/*
 * Unity Test File: mailrelay_otp_test_generate.c
 *
 * Phase 8.3: OTP generate, hash, constant-time equal, wipe.
 * Does not exercise DB insert or template send.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <src/mailrelay/mailrelay_otp.h>

void test_generate_code_default_digits(void);
void test_generate_code_custom_digits(void);
void test_generate_code_rejects_bad_digits(void);
void test_generate_code_rejects_small_buffer(void);
void test_generate_code_uses_random_seam(void);
void test_generate_code_default_random_source(void);
void test_hash_code_stable(void);
void test_hash_code_rejects_null_empty(void);
void test_hash_code_rejects_small_buffer(void);
void test_constant_time_equal(void);
void test_wipe_clears_buffer(void);

static unsigned char g_fake_random_fill = 0;
static int g_fake_random_calls = 0;

static bool fake_random_fill(unsigned char* buffer, size_t length) {
    g_fake_random_calls = g_fake_random_calls + 1;
    for (size_t i = 0; i < length; i++) {
        buffer[i] = g_fake_random_fill;
    }
    return true;
}

static bool fake_random_fail(unsigned char* buffer, size_t length) {
    (void)buffer;
    (void)length;
    g_fake_random_calls = g_fake_random_calls + 1;
    return false;
}

void setUp(void) {
    g_fake_random_fill = 0;
    g_fake_random_calls = 0;
    mailrelay_otp_set_random_fn(NULL);
}

void tearDown(void) {
    mailrelay_otp_set_random_fn(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_generate_code_default_digits);
    RUN_TEST(test_generate_code_custom_digits);
    RUN_TEST(test_generate_code_rejects_bad_digits);
    RUN_TEST(test_generate_code_rejects_small_buffer);
    RUN_TEST(test_generate_code_uses_random_seam);
    RUN_TEST(test_generate_code_default_random_source);
    RUN_TEST(test_hash_code_stable);
    RUN_TEST(test_hash_code_rejects_null_empty);
    RUN_TEST(test_hash_code_rejects_small_buffer);
    RUN_TEST(test_constant_time_equal);
    RUN_TEST(test_wipe_clears_buffer);
    return UNITY_END();
}

void test_generate_code_default_digits(void) {
    char code[MAIL_OTP_MAX_DIGITS + 1];
    memset(code, 0xAA, sizeof(code));

    mailrelay_otp_set_random_fn(fake_random_fill);
    g_fake_random_fill = 7;

    TEST_ASSERT_TRUE(mailrelay_otp_generate_code(MAIL_OTP_DEFAULT_DIGITS, code, sizeof(code)));
    TEST_ASSERT_EQUAL(6, (int)strlen(code));
    TEST_ASSERT_EQUAL_STRING("777777", code);
    TEST_ASSERT_EQUAL(1, g_fake_random_calls);
}

void test_generate_code_custom_digits(void) {
    char code[MAIL_OTP_MAX_DIGITS + 1];
    mailrelay_otp_set_random_fn(fake_random_fill);
    g_fake_random_fill = 3;

    TEST_ASSERT_TRUE(mailrelay_otp_generate_code(8, code, sizeof(code)));
    TEST_ASSERT_EQUAL(8, (int)strlen(code));
    TEST_ASSERT_EQUAL_STRING("33333333", code);
}

void test_generate_code_rejects_bad_digits(void) {
    char code[MAIL_OTP_MAX_DIGITS + 1];
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(3, code, sizeof(code)));
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(11, code, sizeof(code)));
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(0, code, sizeof(code)));
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(6, NULL, 16));
}

void test_generate_code_rejects_small_buffer(void) {
    char tiny[4];
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(6, tiny, sizeof(tiny)));
}

void test_generate_code_uses_random_seam(void) {
    char code[MAIL_OTP_MAX_DIGITS + 1];
    mailrelay_otp_set_random_fn(fake_random_fail);
    TEST_ASSERT_FALSE(mailrelay_otp_generate_code(6, code, sizeof(code)));
    TEST_ASSERT_EQUAL(1, g_fake_random_calls);

    mailrelay_otp_set_random_fn(fake_random_fill);
    g_fake_random_fill = 42;
    TEST_ASSERT_TRUE(mailrelay_otp_generate_code(4, code, sizeof(code)));
    TEST_ASSERT_EQUAL_STRING("2222", code);
}

void test_generate_code_default_random_source(void) {
    /* No seam installed -> falls back to utils_random_bytes. */
    char code[MAIL_OTP_MAX_DIGITS + 1];
    memset(code, 0, sizeof(code));

    mailrelay_otp_set_random_fn(NULL);
    TEST_ASSERT_TRUE(mailrelay_otp_generate_code(MAIL_OTP_DEFAULT_DIGITS, code, sizeof(code)));
    TEST_ASSERT_EQUAL(MAIL_OTP_DEFAULT_DIGITS, (int)strlen(code));
    for (int i = 0; i < MAIL_OTP_DEFAULT_DIGITS; i++) {
        TEST_ASSERT_TRUE(code[i] >= '0' && code[i] <= '9');
    }
}

void test_hash_code_stable(void) {
    char hash1[MAIL_OTP_HASH_HEX_LEN];
    char hash2[MAIL_OTP_HASH_HEX_LEN];

    TEST_ASSERT_TRUE(mailrelay_otp_hash_code("123456", hash1, sizeof(hash1)));
    TEST_ASSERT_TRUE(mailrelay_otp_hash_code("123456", hash2, sizeof(hash2)));
    TEST_ASSERT_EQUAL(64, (int)strlen(hash1));
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);

    /* Known SHA-256("123456") lowercase hex */
    TEST_ASSERT_EQUAL_STRING(
        "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92",
        hash1);

    char other[MAIL_OTP_HASH_HEX_LEN];
    TEST_ASSERT_TRUE(mailrelay_otp_hash_code("654321", other, sizeof(other)));
    TEST_ASSERT_TRUE(strcmp(hash1, other) != 0);
}

void test_hash_code_rejects_null_empty(void) {
    char hash[MAIL_OTP_HASH_HEX_LEN];
    TEST_ASSERT_FALSE(mailrelay_otp_hash_code(NULL, hash, sizeof(hash)));
    TEST_ASSERT_FALSE(mailrelay_otp_hash_code("", hash, sizeof(hash)));
    TEST_ASSERT_FALSE(mailrelay_otp_hash_code("123456", NULL, sizeof(hash)));
}

void test_hash_code_rejects_small_buffer(void) {
    char tiny[16];
    TEST_ASSERT_FALSE(mailrelay_otp_hash_code("123456", tiny, sizeof(tiny)));
}

void test_constant_time_equal(void) {
    TEST_ASSERT_TRUE(mailrelay_otp_constant_time_equal("abc", "abc"));
    TEST_ASSERT_FALSE(mailrelay_otp_constant_time_equal("abc", "abd"));
    TEST_ASSERT_FALSE(mailrelay_otp_constant_time_equal("abc", "ab"));
    TEST_ASSERT_FALSE(mailrelay_otp_constant_time_equal(NULL, "abc"));
    TEST_ASSERT_FALSE(mailrelay_otp_constant_time_equal("abc", NULL));
    TEST_ASSERT_FALSE(mailrelay_otp_constant_time_equal("", ""));
}

void test_wipe_clears_buffer(void) {
    char buf[8];
    memset(buf, '9', sizeof(buf));
    mailrelay_otp_wipe(buf, sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        TEST_ASSERT_EQUAL(0, (int)(unsigned char)buf[i]);
    }
    mailrelay_otp_wipe(NULL, 8);
}
