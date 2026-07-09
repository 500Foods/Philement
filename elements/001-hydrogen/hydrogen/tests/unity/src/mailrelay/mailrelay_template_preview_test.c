/*
 * Unity Test File: mailrelay_template_preview_test.c
 *
 * Tests for the Mail Relay template preview function.
 * Covers template lookup, rendering of subject/text/html, template not found,
 * missing macros, and repository failure. Uses the repository executor seam
 * to avoid requiring a live database.
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/mailrelay/mailrelay_template.h>
#include <src/mailrelay/mailrelay_repository.h>

// Third-party includes
#include <jansson.h>

// Forward declarations
void test_mailrelay_template_preview_all_fields(void);
void test_mailrelay_template_preview_html_only(void);
void test_mailrelay_template_preview_not_found(void);
void test_mailrelay_template_preview_missing_macro(void);
void test_mailrelay_template_preview_repository_failure(void);
void test_mailrelay_template_preview_macros_used_basic(void);
void test_mailrelay_template_preview_macros_used_dedup(void);
void test_mailrelay_template_preview_macros_used_builtins(void);

static mailrelay_repo_execute_fn g_original_executor = NULL;
static json_t* g_mock_result = NULL;

static bool mock_executor(int query_ref,
                          const char* params_json,
                          mailrelay_repo_callback_fn callback,
                          void* user_data) {
    (void)params_json;

    TEST_ASSERT_EQUAL(MAILRELAY_QREF_TEMPLATE_GET_BY_KEY, query_ref);
    TEST_ASSERT_NOT_NULL(callback);

    MailRelayRepoResult result = { 0 };
    result.status = MAILRELAY_REPO_OK;
    result.data = g_mock_result;
    result.affected_rows = 1;
    callback(&result, user_data);
    return true;
}

static void set_mock_result(const char* subject, const char* text, const char* html) {
    if (g_mock_result) {
        json_decref(g_mock_result);
        g_mock_result = NULL;
    }
    g_mock_result = json_array();
    json_t* row = json_object();
    json_object_set_new(row, "subject_template", json_string(subject));
    if (text) {
        json_object_set_new(row, "text_template", json_string(text));
    }
    if (html) {
        json_object_set_new(row, "html_template", json_string(html));
    }
    json_array_append_new(g_mock_result, row);
}

void setUp(void) {
    g_original_executor = mailrelay_repo_get_executor();
    mailrelay_repo_set_executor(mock_executor);
    g_mock_result = NULL;
}

void tearDown(void) {
    mailrelay_repo_set_executor(g_original_executor);
    if (g_mock_result) {
        json_decref(g_mock_result);
        g_mock_result = NULL;
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mailrelay_template_preview_all_fields);
    RUN_TEST(test_mailrelay_template_preview_html_only);
    RUN_TEST(test_mailrelay_template_preview_not_found);
    RUN_TEST(test_mailrelay_template_preview_missing_macro);
    RUN_TEST(test_mailrelay_template_preview_repository_failure);
    RUN_TEST(test_mailrelay_template_preview_macros_used_basic);
    RUN_TEST(test_mailrelay_template_preview_macros_used_dedup);
    RUN_TEST(test_mailrelay_template_preview_macros_used_builtins);

    return UNITY_END();
}

void test_mailrelay_template_preview_all_fields(void) {
    set_mock_result("Hello %NAME% from %APP_NAME%", "Text: %NAME%", "<p>HTML: %NAME%</p>");

    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_preview("mail.test", &params,
                                                "Hydrogen", "srv1", "2026-07-08T12:00:00Z",
                                                "req-1", "user@example.com", NULL,
                                                &subject, &text, &html, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Hello World from Hydrogen", subject);
    TEST_ASSERT_EQUAL_STRING("Text: World", text);
    TEST_ASSERT_EQUAL_STRING("<p>HTML: World</p>", html);

    free(subject);
    free(text);
    free(html);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_preview_html_only(void) {
    set_mock_result("Server %SERVER_NAME% started", NULL,
                    "<p>Server <strong>%SERVER_NAME%</strong> started at %TIMESTAMP%.</p>");

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    TEST_ASSERT_TRUE(mailrelay_template_preview("system.server_started", NULL,
                                                NULL, "test-server", "2026-07-08T12:00:00Z",
                                                NULL, NULL, NULL,
                                                &subject, &text, &html, err, sizeof(err)));
    TEST_ASSERT_EQUAL_STRING("Server test-server started", subject);
    TEST_ASSERT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("<p>Server <strong>test-server</strong> started at 2026-07-08T12:00:00Z.</p>", html);

    free(subject);
    free(html);
}

void test_mailrelay_template_preview_not_found(void) {
    if (g_mock_result) {
        json_decref(g_mock_result);
    }
    g_mock_result = json_array();

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_preview("missing.template", NULL,
                                                 NULL, NULL, NULL, NULL, NULL, NULL,
                                                 &subject, &text, &html, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_TEMPLATE_NOT_FOUND"));
    TEST_ASSERT_NULL(subject);
    TEST_ASSERT_NULL(text);
    TEST_ASSERT_NULL(html);
}

void test_mailrelay_template_preview_missing_macro(void) {
    set_mock_result("Hello %NAME%", "Text", NULL);

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_preview("mail.test", NULL,
                                                 NULL, NULL, NULL, NULL, NULL, NULL,
                                                 &subject, &text, &html, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_PARAM_MISSING"));
    TEST_ASSERT_NULL(subject);
    TEST_ASSERT_NULL(text);
    TEST_ASSERT_NULL(html);
}

static bool failure_executor(int query_ref,
                             const char* params_json,
                             mailrelay_repo_callback_fn callback,
                             void* user_data) {
    (void)query_ref;
    (void)params_json;

    MailRelayRepoResult result = { 0 };
    result.status = MAILRELAY_REPO_QUERY_ERROR;
    result.error_message = "mock database error";
    callback(&result, user_data);
    return true;
}

void test_mailrelay_template_preview_repository_failure(void) {
    mailrelay_repo_set_executor(failure_executor);

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    TEST_ASSERT_FALSE(mailrelay_template_preview("mail.test", NULL,
                                                 NULL, NULL, NULL, NULL, NULL, NULL,
                                                 &subject, &text, &html, err, sizeof(err)));
    TEST_ASSERT_NOT_NULL(strstr(err, "mock database error"));
    TEST_ASSERT_NULL(subject);
    TEST_ASSERT_NULL(text);
    TEST_ASSERT_NULL(html);
}

static bool macro_list_contains(const MailRelayMacroList* list, const char* name) {
    if (!list || !name) {
        return false;
    }
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

void test_mailrelay_template_preview_macros_used_basic(void) {
    set_mock_result("Hello %NAME% from %APP_NAME%", "Text: %NAME%", NULL);

    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    MailRelayMacroList macros = { 0 };
    mailrelay_macro_list_init(&macros);

    TEST_ASSERT_TRUE(mailrelay_template_preview_with_macros("mail.test", &params,
                                                            "Hydrogen", "srv1", NULL,
                                                            NULL, NULL, NULL,
                                                            &subject, &text, &html, &macros,
                                                            err, sizeof(err)));
    TEST_ASSERT_EQUAL(2, macros.count);
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "NAME"));
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "APP_NAME"));
    TEST_ASSERT_FALSE(macro_list_contains(&macros, "MISSING"));

    free(subject);
    free(text);
    free(html);
    mailrelay_macro_list_free(&macros);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_preview_macros_used_dedup(void) {
    set_mock_result("%NAME% %NAME% from %APP_NAME%", NULL, NULL);

    MailRelayTemplateParams params;
    mailrelay_template_params_init(&params);
    TEST_ASSERT_TRUE(mailrelay_template_params_add(&params, "NAME", "World"));

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    MailRelayMacroList macros = { 0 };
    mailrelay_macro_list_init(&macros);

    TEST_ASSERT_TRUE(mailrelay_template_preview_with_macros("mail.test", &params,
                                                            "Hydrogen", NULL, NULL,
                                                            NULL, NULL, NULL,
                                                            &subject, &text, &html, &macros,
                                                            err, sizeof(err)));
    TEST_ASSERT_EQUAL(2, macros.count);
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "NAME"));
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "APP_NAME"));

    free(subject);
    free(text);
    free(html);
    mailrelay_macro_list_free(&macros);
    mailrelay_template_params_free(&params);
}

void test_mailrelay_template_preview_macros_used_builtins(void) {
    set_mock_result("Server %SERVER_NAME% at %TIMESTAMP%", NULL, NULL);

    char err[256] = { 0 };
    char* subject = NULL;
    char* text = NULL;
    char* html = NULL;
    MailRelayMacroList macros = { 0 };
    mailrelay_macro_list_init(&macros);

    TEST_ASSERT_TRUE(mailrelay_template_preview_with_macros("system.server_started", NULL,
                                                            NULL, "test-server", "2026-07-08T12:00:00Z",
                                                            NULL, NULL, NULL,
                                                            &subject, &text, &html, &macros,
                                                            err, sizeof(err)));
    TEST_ASSERT_EQUAL(2, macros.count);
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "SERVER_NAME"));
    TEST_ASSERT_TRUE(macro_list_contains(&macros, "TIMESTAMP"));

    free(subject);
    free(text);
    free(html);
    mailrelay_macro_list_free(&macros);
}
