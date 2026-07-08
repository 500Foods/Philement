/*
 * Unity Test File: mailrelay_template_test.c
 *
 * Tests for the Mail Relay template macro engine.
 * Covers macro replacement, optional defaults, built-ins, literal escapes,
 * missing-macro errors, and parameter map behavior.
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay_template.h>

// Forward declarations
void test_mailrelay_template_params_add_and_get(void);
void test_mailrelay_template_params_override_latest_wins(void);
void test_mailrelay_template_simple_replacement(void);
void test_mailrelay_template_default_value(void);
void test_mailrelay_template_missing_macro_fails(void);
void test_mailrelay_template_missing_builtin_fails(void);
void test_mailrelay_template_default_used_for_missing_builtin(void);
void test_mailrelay_template_literal_percent(void);
void test_mailrelay_template_literal_percent_with_macro(void);
void test_mailrelay_template_repeated_macro(void);
void test_mailrelay_template_builtin_app_name(void);
void test_mailrelay_template_builtin_server_name(void);
void test_mailrelay_template_builtin_timestamp(void);
void test_mailrelay_template_builtin_user_email(void);
void test_mailrelay_template_builtin_request_id(void);
void test_mailrelay_template_builtin_otp_code(void);
void test_mailrelay_template_empty_template(void);
void test_mailrelay_template_large_value(void);
void test_mailrelay_template_invalid_macro_name(void);
void test_mailrelay_template_unclosed_macro(void);
void test_mailrelay_template_empty_macro_name(void);
void test_mailrelay_template_null_params_with_builtins(void);

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mailrelay_template_params_add_and_get);
    RUN_TEST(test_mailrelay_template_params_override_latest_wins);
    RUN_TEST(test_mailrelay_template_simple_replacement);
    RUN_TEST(test_mailrelay_template_default_value);
    RUN_TEST(test_mailrelay_template_missing_macro_fails);
    RUN_TEST(test_mailrelay_template_missing_builtin_fails);
    RUN_TEST(test_mailrelay_template_default_used_for_missing_builtin);
    RUN_TEST(test_mailrelay_template_literal_percent);
    RUN_TEST(test_mailrelay_template_literal_percent_with_macro);
    RUN_TEST(test_mailrelay_template_repeated_macro);
    RUN_TEST(test_mailrelay_template_builtin_app_name);
    RUN_TEST(test_mailrelay_template_builtin_server_name);
    RUN_TEST(test_mailrelay_template_builtin_timestamp);
    RUN_TEST(test_mailrelay_template_builtin_user_email);
    RUN_TEST(test_mailrelay_template_builtin_request_id);
    RUN_TEST(test_mailrelay_template_builtin_otp_code);
    RUN_TEST(test_mailrelay_template_empty_template);
    RUN_TEST(test_mailrelay_template_large_value);
    RUN_TEST(test_mailrelay_template_invalid_macro_name);
    RUN_TEST(test_mailrelay_template_unclosed_macro);
    RUN_TEST(test_mailrelay_template_empty_macro_name);
    RUN_TEST(test_mailrelay_template_null_params_with_builtins);

    return UNITY_END();
}

// ===== PARAMETER MAP =====

void test_mailrelay_template_params_add_and_get(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);

    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "Alice"));
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "GREETING", "Hello"));
    TEST_ASSERT_EQUAL_STRING("Alice", mailrelay_template_params_get(&params, "NAME"));
    TEST_ASSERT_EQUAL_STRING("Hello", mailrelay_template_params_get(&params, "GREETING"));
    TEST_ASSERT_NULL(mailrelay_template_params_get(&params, "MISSING"));

    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_params_override_latest_wins(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);

    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "Alice"));
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "Bob"));
    TEST_ASSERT_EQUAL_STRING("Bob", mailrelay_template_params_get(&params, "NAME"));

    mailrelay_template_params_free(&params);
}

// ===== SIMPLE REPLACEMENT =====

void test_mailrelay_template_simple_replacement(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Hello %NAME%!", &params,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Hello World!", out);

    free(out);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_default_value(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Hello %NAME|Friend%!", &params,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Hello Friend!", out);

    free(out);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_missing_macro_fails(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_render("Hello %NAME%!", &params,
                                                NULL, NULL, NULL, NULL, NULL, NULL,
                                                &out, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_PARAM_MISSING"));
    TEST_ASSERT_NOT_NULL(strstr(err, "NAME"));
    TEST_ASSERT_NULL(out);

    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_missing_builtin_fails(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_render("App: %APP_NAME%", NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL,
                                                &out, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_PARAM_MISSING"));
    TEST_ASSERT_NOT_NULL(strstr(err, "APP_NAME"));
    TEST_ASSERT_NULL(out);
}

void test_mailrelay_template_default_used_for_missing_builtin(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("App: %APP_NAME|Unknown%", NULL,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("App: Unknown", out);

    free(out);
}

// ===== ESCAPING AND SPECIAL CASES =====

void test_mailrelay_template_literal_percent(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Discount: 50%%", NULL,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Discount: 50%", out);

    free(out);
}

void test_mailrelay_template_literal_percent_with_macro(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "RATE", "50"));

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Rate: %RATE%%% -> %RATE%%%", &params,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Rate: 50% -> 50%", out);

    free(out);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_repeated_macro(void) {
    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "X", "A"));

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("%X% %X% %X%", &params,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("A A A", out);

    free(out);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_empty_template(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("", NULL,
                                              NULL, NULL, NULL, NULL, NULL, NULL,
                                              &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("", out);

    free(out);
}

void test_mailrelay_template_large_value(void) {
    size_t large_len = 10000;
    char* large_value = malloc(large_len + 1);
    TEST_ASSERT_NOT_NULL(large_value);
    for (size_t i = 0; i < large_len; i++) {
        large_value[i] = 'x';
    }
    large_value[large_len] = '\0';

    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "BIG", large_value));

    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("prefix %BIG% suffix", &params,
                                               NULL, NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_size_t(large_len + 14, strlen(out)); /* prefix  + suffix + spaces */
    TEST_ASSERT_EQUAL_STRING_LEN(large_value, out + 7, large_len);

    free(out);
    free(large_value);
    mailrelay_template_params_free(&params);
}

// ===== BUILT-IN MACROS =====

void test_mailrelay_template_builtin_app_name(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("App: %APP_NAME%", NULL,
                                               "Hydrogen", NULL, NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("App: Hydrogen", out);

    free(out);
}

void test_mailrelay_template_builtin_server_name(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Server: %SERVER_NAME%", NULL,
                                               NULL, "test-server", NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Server: test-server", out);

    free(out);
}

void test_mailrelay_template_builtin_timestamp(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("At: %TIMESTAMP%", NULL,
                                               NULL, NULL, "2026-07-08T12:00:00Z", NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("At: 2026-07-08T12:00:00Z", out);

    free(out);
}

void test_mailrelay_template_builtin_user_email(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("User: %USER_EMAIL%", NULL,
                                               NULL, NULL, NULL, NULL, "user@example.com", NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("User: user@example.com", out);

    free(out);
}

void test_mailrelay_template_builtin_request_id(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Req: %REQUEST_ID%", NULL,
                                               NULL, NULL, NULL, "req-123", NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Req: req-123", out);

    free(out);
}

void test_mailrelay_template_builtin_otp_code(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("Code: %OTP_CODE%", NULL,
                                               NULL, NULL, NULL, NULL, NULL, "123456",
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Code: 123456", out);

    free(out);
}

void test_mailrelay_template_null_params_with_builtins(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_render("%APP_NAME%/%SERVER_NAME%", NULL,
                                               "H", "S", NULL, NULL, NULL, NULL,
                                               &out, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("H/S", out);

    free(out);
}

// ===== ERROR CASES =====

void test_mailrelay_template_invalid_macro_name(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_render("Bad: %BAD NAME%", NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL,
                                                &out, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "invalid macro name"));
    TEST_ASSERT_NULL(out);
}

void test_mailrelay_template_unclosed_macro(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_render("Bad: %UNCLOSED", NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL,
                                                &out, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "unclosed macro"));
    TEST_ASSERT_NULL(out);
}

void test_mailrelay_template_empty_macro_name(void) {
    char err[256] = { 0 };
    char* out = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_render("Bad: %|default%", NULL,
                                                NULL, NULL, NULL, NULL, NULL, NULL,
                                                &out, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "empty macro name"));
    TEST_ASSERT_NULL(out);
}
