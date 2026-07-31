/*
 * Unity Test File: scripting_api_mail_repo_test.c
 *
 * Covers H.mail repository helpers: callback, status messages, push_result,
 * Lua wrappers, and H_lua_install_mail_repo.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <jansson.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/mailrelay/mailrelay_repository.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>

void test_callback_null_ctx(void);
void test_callback_null_result(void);
void test_callback_copies_error_and_data(void);
void test_callback_empty_error_message(void);
void test_status_message_detail_preferred(void);
void test_status_message_all_enums(void);
void test_push_result_null_ctx(void);
void test_push_result_error_with_data(void);
void test_push_result_error_without_data(void);
void test_push_result_ok_array(void);
void test_push_result_ok_object(void);
void test_push_result_ok_null_data(void);
void test_template_list_success(void);
void test_template_list_submit_failed_ok_status(void);
void test_template_get_success(void);
void test_template_get_submit_failed_ok_status(void);
void test_route_list_success(void);
void test_route_list_submit_failed_ok_status(void);
void test_queue_get_success(void);
void test_queue_get_submit_failed_ok_status(void);
void test_cleanup_queue_success(void);
void test_cleanup_queue_submit_failed_ok_status(void);
void test_cleanup_events_success(void);
void test_cleanup_events_submit_failed_ok_status(void);
void test_cleanup_attempts_success(void);
void test_cleanup_attempts_submit_failed_ok_status(void);
void test_cleanup_otp_success(void);
void test_cleanup_otp_submit_failed_ok_status(void);
void test_event_list_pending_success(void);
void test_event_list_pending_submit_failed_ok_status(void);
void test_event_insert_missing_event_key(void);
void test_event_insert_empty_event_key(void);
void test_event_insert_success(void);
void test_event_insert_submit_failed_ok_status(void);
void test_install_null_L(void);
void test_install_missing_h(void);
void test_install_missing_mail(void);
void test_install_success(void);

static lua_State* L = NULL;
static MailRelayRepoStatus g_mock_status = MAILRELAY_REPO_OK;
static const char* g_mock_error = NULL;
static json_t* g_mock_data = NULL;
static int g_mock_affected = 0;
static bool g_mock_return = true;
static bool g_mock_invoke_callback = true;
static bool g_executor_called = false;

static bool mock_executor(int query_ref, const char* params_json,
                          mailrelay_repo_callback_fn callback, void* user_data) {
    (void)query_ref;
    (void)params_json;
    g_executor_called = true;
    if (g_mock_invoke_callback && callback) {
        MailRelayRepoResult result = {
            .status = g_mock_status,
            .error_message = g_mock_error,
            .data = g_mock_data,
            .affected_rows = g_mock_affected
        };
        callback(&result, user_data);
    }
    return g_mock_return;
}

static void reset_mock(void) {
    g_mock_status = MAILRELAY_REPO_OK;
    g_mock_error = NULL;
    if (g_mock_data) {
        json_decref(g_mock_data);
        g_mock_data = NULL;
    }
    g_mock_affected = 0;
    g_mock_return = true;
    g_mock_invoke_callback = true;
    g_executor_called = false;
}

void setUp(void) {
    reset_mock();
    mailrelay_repo_set_executor(mock_executor);
    L = luaL_newstate();
    TEST_ASSERT_NOT_NULL(L);
    luaL_openlibs(L);
    H_Handle_install_metatable(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_setfield(L, -2, "mail");
    lua_setglobal(L, "H");
}

void tearDown(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
    reset_mock();
    mailrelay_repo_set_executor(NULL);
}

void test_callback_null_ctx(void) {
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .error_message = NULL,
        .data = NULL,
        .affected_rows = 0
    };
    mail_repo_lua_callback(&result, NULL);
}

void test_callback_null_result(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    mail_repo_lua_callback(NULL, &ctx);
    TEST_ASSERT_EQUAL_INT(0, (int)ctx.status);
}

void test_callback_copies_error_and_data(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    json_t* data = json_pack("{s:s}", "k", "v");
    TEST_ASSERT_NOT_NULL(data);
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_QUERY_ERROR,
        .error_message = "boom",
        .data = data,
        .affected_rows = 3
    };
    mail_repo_lua_callback(&result, &ctx);
    TEST_ASSERT_EQUAL_INT(MAILRELAY_REPO_QUERY_ERROR, ctx.status);
    TEST_ASSERT_EQUAL_STRING("boom", ctx.error);
    TEST_ASSERT_EQUAL_INT(3, ctx.affected_rows);
    TEST_ASSERT_NOT_NULL(ctx.data);
    json_decref(ctx.data);
    json_decref(data);
}

void test_callback_empty_error_message(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.error[0] = 'x';
    MailRelayRepoResult result = {
        .status = MAILRELAY_REPO_OK,
        .error_message = "",
        .data = NULL,
        .affected_rows = 0
    };
    mail_repo_lua_callback(&result, &ctx);
    TEST_ASSERT_EQUAL_STRING("", ctx.error);
}

void test_status_message_detail_preferred(void) {
    TEST_ASSERT_EQUAL_STRING("custom",
        mail_repo_status_message(MAILRELAY_REPO_OK, "custom"));
}

void test_status_message_all_enums(void) {
    TEST_ASSERT_EQUAL_STRING("ok",
        mail_repo_status_message(MAILRELAY_REPO_OK, NULL));
    TEST_ASSERT_EQUAL_STRING("ok",
        mail_repo_status_message(MAILRELAY_REPO_OK, ""));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: invalid repository arguments",
        mail_repo_status_message(MAILRELAY_REPO_INVALID_ARGS, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: Mail Relay database not configured",
        mail_repo_status_message(MAILRELAY_REPO_NO_DATABASE, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: QueryRef not found in cache",
        mail_repo_status_message(MAILRELAY_REPO_QUERY_NOT_FOUND, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: query submit failed",
        mail_repo_status_message(MAILRELAY_REPO_SUBMIT_FAILED, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: query timed out",
        mail_repo_status_message(MAILRELAY_REPO_TIMEOUT, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: query error",
        mail_repo_status_message(MAILRELAY_REPO_QUERY_ERROR, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: result parse error",
        mail_repo_status_message(MAILRELAY_REPO_PARSE_ERROR, NULL));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: unknown repository error",
        mail_repo_status_message((MailRelayRepoStatus)99, NULL));
}

void test_push_result_null_ctx(void) {
    int n = mail_repo_push_result(L, NULL);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: null repository context",
                             lua_tostring(L, -1));
    lua_pop(L, 2);
}

void test_push_result_error_with_data(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_TIMEOUT;
    ctx.data = json_pack("{s:i}", "n", 1);
    TEST_ASSERT_NOT_NULL(ctx.data);
    int n = mail_repo_push_result(L, &ctx);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: query timed out",
                             lua_tostring(L, -1));
    TEST_ASSERT_NULL(ctx.data);
    lua_pop(L, 2);
}

void test_push_result_error_without_data(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_NO_DATABASE;
    snprintf(ctx.error, sizeof(ctx.error), "%s", "db gone");
    int n = mail_repo_push_result(L, &ctx);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("db gone", lua_tostring(L, -1));
    lua_pop(L, 2);
}

void test_push_result_ok_array(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_OK;
    ctx.affected_rows = 2;
    ctx.data = json_pack("[{s:s}]", "id", "a");
    TEST_ASSERT_NOT_NULL(ctx.data);
    int n = mail_repo_push_result(L, &ctx);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_istable(L, -2));
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_getfield(L, -2, "affected_rows");
    TEST_ASSERT_EQUAL_INT(2, (int)lua_tointeger(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, -2, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 3);
    TEST_ASSERT_NULL(ctx.data);
}

void test_push_result_ok_object(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_OK;
    ctx.affected_rows = 1;
    ctx.data = json_pack("{s:s}", "name", "t1");
    TEST_ASSERT_NOT_NULL(ctx.data);
    int n = mail_repo_push_result(L, &ctx);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_istable(L, -2));
    lua_getfield(L, -2, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_rawgeti(L, -1, 1);
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 4);
    TEST_ASSERT_NULL(ctx.data);
}

void test_push_result_ok_null_data(void) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_OK;
    ctx.affected_rows = 0;
    int n = mail_repo_push_result(L, &ctx);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_istable(L, -2));
    lua_getfield(L, -2, "rows");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    TEST_ASSERT_EQUAL_INT(0, (int)lua_rawlen(L, -1));
    lua_pop(L, 3);
}

static void assert_lua_ok_result(int n) {
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_istable(L, -2));
    TEST_ASSERT_TRUE(lua_isnil(L, -1));
    lua_pop(L, 2);
}

static void assert_lua_err_submit_failed(int n) {
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("MAIL_PERSIST_FAILED: query submit failed",
                             lua_tostring(L, -1));
    lua_pop(L, 2);
}

void test_template_list_success(void) {
    g_mock_data = json_array();
    int n = H_lua_mail_template_list(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_template_list_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_invoke_callback = true;
    g_mock_status = MAILRELAY_REPO_OK;
    int n = H_lua_mail_template_list(L);
    assert_lua_err_submit_failed(n);
}

void test_template_get_success(void) {
    lua_pushstring(L, "welcome");
    int n = H_lua_mail_template_get(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_template_get_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "welcome");
    int n = H_lua_mail_template_get(L);
    assert_lua_err_submit_failed(n);
}

void test_route_list_success(void) {
    int n = H_lua_mail_route_list(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_route_list_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    int n = H_lua_mail_route_list(L);
    assert_lua_err_submit_failed(n);
}

void test_queue_get_success(void) {
    lua_pushstring(L, "msg-uuid-1");
    int n = H_lua_mail_queue_get(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_queue_get_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "msg-uuid-1");
    int n = H_lua_mail_queue_get(L);
    assert_lua_err_submit_failed(n);
}

void test_cleanup_queue_success(void) {
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_queue(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_cleanup_queue_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_queue(L);
    assert_lua_err_submit_failed(n);
}

void test_cleanup_events_success(void) {
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_events(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_cleanup_events_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_events(L);
    assert_lua_err_submit_failed(n);
}

void test_cleanup_attempts_success(void) {
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_attempts(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_cleanup_attempts_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_attempts(L);
    assert_lua_err_submit_failed(n);
}

void test_cleanup_otp_success(void) {
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_otp(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_cleanup_otp_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_pushstring(L, "2020-01-01T00:00:00Z");
    int n = H_lua_mail_cleanup_otp(L);
    assert_lua_err_submit_failed(n);
}

void test_event_list_pending_success(void) {
    int n = H_lua_mail_event_list_pending(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_event_list_pending_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    int n = H_lua_mail_event_list_pending(L);
    assert_lua_err_submit_failed(n);
}

void test_event_insert_missing_event_key(void) {
    lua_newtable(L);
    int n = H_lua_mail_event_insert(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: event_key required",
                             lua_tostring(L, -1));
    lua_pop(L, 2);
}

void test_event_insert_empty_event_key(void) {
    lua_newtable(L);
    lua_pushstring(L, "");
    lua_setfield(L, -2, "event_key");
    int n = H_lua_mail_event_insert(L);
    TEST_ASSERT_EQUAL_INT(2, n);
    TEST_ASSERT_TRUE(lua_isnil(L, -2));
    TEST_ASSERT_EQUAL_STRING("MAIL_PARAM_MISSING: event_key required",
                             lua_tostring(L, -1));
    lua_pop(L, 2);
}

void test_event_insert_success(void) {
    lua_newtable(L);
    lua_pushstring(L, "evt-1");
    lua_setfield(L, -2, "event_key");
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "status_a65");
    lua_pushstring(L, "tpl");
    lua_setfield(L, -2, "template_key");
    lua_pushstring(L, "from@ex.com");
    lua_setfield(L, -2, "from_addr");
    lua_pushstring(L, "reply@ex.com");
    lua_setfield(L, -2, "reply_to");
    lua_pushstring(L, "[]");
    lua_setfield(L, -2, "recipients_json");
    lua_pushstring(L, "subj");
    lua_setfield(L, -2, "subject");
    lua_pushstring(L, "text");
    lua_setfield(L, -2, "body_text");
    lua_pushstring(L, "<b>h</b>");
    lua_setfield(L, -2, "body_html");
    lua_pushstring(L, "{}");
    lua_setfield(L, -2, "headers_json");
    lua_pushstring(L, "{}");
    lua_setfield(L, -2, "params_json");
    lua_pushstring(L, "dk");
    lua_setfield(L, -2, "debounce_key");
    lua_pushstring(L, "ik");
    lua_setfield(L, -2, "idempotency_key");
    lua_pushinteger(L, 5);
    lua_setfield(L, -2, "priority");
    int n = H_lua_mail_event_insert(L);
    TEST_ASSERT_TRUE(g_executor_called);
    assert_lua_ok_result(n);
}

void test_event_insert_submit_failed_ok_status(void) {
    g_mock_return = false;
    g_mock_status = MAILRELAY_REPO_OK;
    lua_newtable(L);
    lua_pushstring(L, "evt-1");
    lua_setfield(L, -2, "event_key");
    int n = H_lua_mail_event_insert(L);
    assert_lua_err_submit_failed(n);
}

void test_install_null_L(void) {
    H_lua_install_mail_repo(NULL);
}

void test_install_missing_h(void) {
    lua_pushnil(L);
    lua_setglobal(L, "H");
    H_lua_install_mail_repo(L);
    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));
}

void test_install_missing_mail(void) {
    lua_newtable(L);
    lua_setglobal(L, "H");
    H_lua_install_mail_repo(L);
    TEST_ASSERT_EQUAL_INT(0, lua_gettop(L));
}

void test_install_success(void) {
    H_lua_install_mail_repo(L);
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "mail");
    TEST_ASSERT_TRUE(lua_istable(L, -1));

    const char* names[] = {
        "template_list", "template_get", "route_list", "queue_get",
        "cleanup_queue", "cleanup_events", "cleanup_attempts", "cleanup_otp",
        "event_list_pending", "event_insert"
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        lua_getfield(L, -1, names[i]);
        TEST_ASSERT_EQUAL(LUA_TFUNCTION, lua_type(L, -1));
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_callback_null_ctx);
    RUN_TEST(test_callback_null_result);
    RUN_TEST(test_callback_copies_error_and_data);
    RUN_TEST(test_callback_empty_error_message);
    RUN_TEST(test_status_message_detail_preferred);
    RUN_TEST(test_status_message_all_enums);
    RUN_TEST(test_push_result_null_ctx);
    RUN_TEST(test_push_result_error_with_data);
    RUN_TEST(test_push_result_error_without_data);
    RUN_TEST(test_push_result_ok_array);
    RUN_TEST(test_push_result_ok_object);
    RUN_TEST(test_push_result_ok_null_data);
    RUN_TEST(test_template_list_success);
    RUN_TEST(test_template_list_submit_failed_ok_status);
    RUN_TEST(test_template_get_success);
    RUN_TEST(test_template_get_submit_failed_ok_status);
    RUN_TEST(test_route_list_success);
    RUN_TEST(test_route_list_submit_failed_ok_status);
    RUN_TEST(test_queue_get_success);
    RUN_TEST(test_queue_get_submit_failed_ok_status);
    RUN_TEST(test_cleanup_queue_success);
    RUN_TEST(test_cleanup_queue_submit_failed_ok_status);
    RUN_TEST(test_cleanup_events_success);
    RUN_TEST(test_cleanup_events_submit_failed_ok_status);
    RUN_TEST(test_cleanup_attempts_success);
    RUN_TEST(test_cleanup_attempts_submit_failed_ok_status);
    RUN_TEST(test_cleanup_otp_success);
    RUN_TEST(test_cleanup_otp_submit_failed_ok_status);
    RUN_TEST(test_event_list_pending_success);
    RUN_TEST(test_event_list_pending_submit_failed_ok_status);
    RUN_TEST(test_event_insert_missing_event_key);
    RUN_TEST(test_event_insert_empty_event_key);
    RUN_TEST(test_event_insert_success);
    RUN_TEST(test_event_insert_submit_failed_ok_status);
    RUN_TEST(test_install_null_L);
    RUN_TEST(test_install_missing_h);
    RUN_TEST(test_install_missing_mail);
    RUN_TEST(test_install_success);

    return UNITY_END();
}
