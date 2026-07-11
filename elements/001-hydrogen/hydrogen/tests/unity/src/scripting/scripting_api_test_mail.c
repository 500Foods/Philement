/*
 * Unity Test File: scripting_api_test_mail.c
 *
 * H.mail.send / send_sync / H.wait for MAIL handles (template + freeform).
 * Uses mailrelay_send_template_set_fn and mailrelay_send_direct_set_fn seams.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/scripting_api.h>
#include <src/mailrelay/mailrelay.h>

static AppConfig mock_app_config_storage = {0};
static char last_template_key[256];
static char last_idempotency_key[512];
static char last_subject[256];
static char last_text_body[512];
static int last_to_count;
static int mock_template_calls;
static int mock_direct_calls;
static MailRelayStatus mock_status = MAILRELAY_OK;
static const char* mock_err = NULL;

void test_mail_send_returns_handle(void);
void test_mail_send_success_wait(void);
void test_mail_send_auto_idempotency_key(void);
void test_mail_send_explicit_idempotency_key(void);
void test_mail_send_missing_mode(void);
void test_mail_send_freeform_success(void);
void test_mail_send_freeform_body_alias(void);
void test_mail_send_mixed_mode_rejected(void);
void test_mail_send_freeform_missing_body(void);
void test_mail_send_missing_recipient(void);
void test_mail_send_disabled(void);
void test_mail_send_template_not_found(void);
void test_mail_send_sync_success(void);
void test_mail_send_sync_freeform(void);
void test_mail_wait_already_consumed(void);
void test_notify_deferred_error(void);
void test_notify_send_sync_deferred(void);
void test_notify_does_not_enqueue(void);

static MailRelayStatus mock_send_template(const MailRelaySendTemplateRequest* req,
                                          MailRelaySendTemplateResponse* resp,
                                          char* err,
                                          size_t err_cap) {
    mock_template_calls++;
    last_template_key[0] = '\0';
    last_idempotency_key[0] = '\0';
    last_to_count = 0;

    if (!req || !resp || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_send_template_response_init(resp);
    err[0] = '\0';

    if (req->template_key) {
        snprintf(last_template_key, sizeof(last_template_key), "%s", req->template_key);
    }
    if (req->idempotency_key) {
        snprintf(last_idempotency_key, sizeof(last_idempotency_key), "%s", req->idempotency_key);
    }
    last_to_count = req->to_count;

    if (mock_status != MAILRELAY_OK) {
        if (mock_err && mock_err[0] != '\0') {
            snprintf(err, err_cap, "%s", mock_err);
        }
        return mock_status;
    }

    resp->message_id = strdup("msg-test-uuid-001");
    resp->status = strdup("queued");
    if (!resp->message_id || !resp->status) {
        free(resp->message_id);
        free(resp->status);
        resp->message_id = NULL;
        resp->status = NULL;
        snprintf(err, err_cap, "memory allocation failed");
        return MAILRELAY_INVALID_ARGS;
    }
    return MAILRELAY_OK;
}

static MailRelayStatus mock_send_direct(const MailRelaySendDirectRequest* req,
                                        MailRelaySendTemplateResponse* resp,
                                        char* err,
                                        size_t err_cap) {
    mock_direct_calls++;
    last_subject[0] = '\0';
    last_text_body[0] = '\0';
    last_idempotency_key[0] = '\0';
    last_to_count = 0;

    if (!req || !resp || !err || err_cap == 0) {
        if (err && err_cap > 0) {
            snprintf(err, err_cap, "invalid arguments");
        }
        return MAILRELAY_INVALID_ARGS;
    }

    mailrelay_send_template_response_init(resp);
    err[0] = '\0';

    if (req->subject) {
        snprintf(last_subject, sizeof(last_subject), "%s", req->subject);
    }
    if (req->text_body) {
        snprintf(last_text_body, sizeof(last_text_body), "%s", req->text_body);
    }
    if (req->idempotency_key) {
        snprintf(last_idempotency_key, sizeof(last_idempotency_key), "%s", req->idempotency_key);
    }
    last_to_count = req->to_count;

    if (mock_status != MAILRELAY_OK) {
        if (mock_err && mock_err[0] != '\0') {
            snprintf(err, err_cap, "%s", mock_err);
        }
        return mock_status;
    }

    resp->message_id = strdup("msg-direct-uuid-001");
    resp->status = strdup("queued");
    if (!resp->message_id || !resp->status) {
        free(resp->message_id);
        free(resp->status);
        resp->message_id = NULL;
        resp->status = NULL;
        snprintf(err, err_cap, "memory allocation failed");
        return MAILRELAY_INVALID_ARGS;
    }
    return MAILRELAY_OK;
}

void setUp(void) {
    static char server_name_buf[] = "TestServer";
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    mock_app_config_storage.server.server_name = server_name_buf;
    mock_app_config_storage.mail_relay.Enabled = true;
    app_config = &mock_app_config_storage;
    mock_status = MAILRELAY_OK;
    mock_err = NULL;
    mock_template_calls = 0;
    mock_direct_calls = 0;
    last_template_key[0] = '\0';
    last_idempotency_key[0] = '\0';
    last_subject[0] = '\0';
    last_text_body[0] = '\0';
    last_to_count = 0;
    mailrelay_send_template_set_fn(mock_send_template);
    mailrelay_send_direct_set_fn(mock_send_direct);
}

void tearDown(void) {
    mailrelay_send_template_set_fn(NULL);
    mailrelay_send_direct_set_fn(NULL);
    app_config = NULL;
}

static void run_lua(lua_State* L, const char* code) {
    int rc = luaL_loadbuffer(L, code, strlen(code), "test");
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, "luaL_loadbuffer failed");
    rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    TEST_ASSERT_EQUAL_MESSAGE(LUA_OK, rc, lua_tostring(L, -1));
}

static lua_State* make_ctx(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);
    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_getfield(L, -1, "mail");
    TEST_ASSERT_TRUE_MESSAGE(lua_istable(L, -1), "H.mail must be a table");
    lua_getfield(L, -1, "send");
    TEST_ASSERT_TRUE_MESSAGE(lua_isfunction(L, -1), "H.mail.send must be a function");
    lua_pop(L, 3);
    return L;
}

void test_mail_send_returns_handle(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "return H.mail.send({ template = 'mail.test', to = 'a@x.com' })");
    TEST_ASSERT_EQUAL(1, lua_gettop(L));
    TEST_ASSERT_TRUE(lua_isuserdata(L, -1));
    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL(H_HK_MAIL, h->kind);
    TEST_ASSERT_EQUAL(1, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_success_wait(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test', to = 'a@x.com', params = { NAME = 'Ada' } })\n"
        "r, e = H.wait(h)\n"
        "_ok = (type(r) == 'table' and r.message_id == 'msg-test-uuid-001' and r.status == 'queued' and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL_STRING("mail.test", last_template_key);
    TEST_ASSERT_EQUAL(1, last_to_count);
    H_lua_destroy_context(L);
}

void test_mail_send_auto_idempotency_key(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template_key = 'mail.test', to = { 'a@x.com' } })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_TRUE(strlen(last_idempotency_key) > 0);
    H_lua_destroy_context(L);
}

void test_mail_send_explicit_idempotency_key(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test', to = 'a@x.com', idempotency_key = 'idem-abc' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL_STRING("idem-abc", last_idempotency_key);
    H_lua_destroy_context(L);
}

void test_mail_send_missing_mode(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ to = 'a@x.com' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_PARAM_MISSING') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_freeform_success(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ to = 'a@x.com', subject = 'Alert', text_body = 'Detail line' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (type(r) == 'table' and r.message_id == 'msg-direct-uuid-001' and r.status == 'queued' and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(1, mock_direct_calls);
    TEST_ASSERT_EQUAL_STRING("Alert", last_subject);
    TEST_ASSERT_EQUAL_STRING("Detail line", last_text_body);
    TEST_ASSERT_EQUAL(1, last_to_count);
    H_lua_destroy_context(L);
}

void test_mail_send_freeform_body_alias(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ to = 'a@x.com', subject = 'Hi', body = 'via body alias' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r ~= nil and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(1, mock_direct_calls);
    TEST_ASSERT_EQUAL_STRING("via body alias", last_text_body);
    H_lua_destroy_context(L);
}

void test_mail_send_mixed_mode_rejected(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test', to = 'a@x.com', subject = 'x', body = 'y' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_PARAM_MISSING') ~= nil and e:find('either') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_freeform_missing_body(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ to = 'a@x.com', subject = 'No body' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_PARAM_MISSING') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_missing_recipient(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_RECIPIENT_INVALID') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_disabled(void) {
    mock_status = MAILRELAY_DISABLED;
    mock_err = "MAIL_DISABLED: mail relay is disabled";
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test', to = 'a@x.com' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_DISABLED') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(1, mock_template_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_template_not_found(void) {
    mock_status = MAILRELAY_INVALID_ARGS;
    mock_err = "MAIL_TEMPLATE_NOT_FOUND: Template not found or inactive";
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'missing', to = 'a@x.com' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('MAIL_TEMPLATE_NOT_FOUND') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_mail_send_sync_success(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.mail.send_sync({ template = 'mail.test', to = 'a@x.com' })\n"
        "_ok = (type(r) == 'table' and r.status == 'queued' and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(1, mock_template_calls);
    H_lua_destroy_context(L);
}

void test_mail_send_sync_freeform(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.mail.send_sync({ to = 'a@x.com', subject = 'S', text_body = 'B' })\n"
        "_ok = (type(r) == 'table' and r.status == 'queued' and e == nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(1, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_mail_wait_already_consumed(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.mail.send({ template = 'mail.test', to = 'a@x.com' })\n"
        "r1, e1 = H.wait(h)\n"
        "r2, e2 = H.wait(h)\n"
        "_ok = (r1 ~= nil and r2 == nil and type(e2) == 'string' and e2:find('consumed') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    H_lua_destroy_context(L);
}

void test_notify_deferred_error(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.notify.send({ channel = 'sms', to = '1', body = 'x' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('deferred to mailrelay rules') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_notify_send_sync_deferred(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "r, e = H.notify.send_sync({ channel = 'push', to = 'u', body = 'x' })\n"
        "_ok = (r == nil and type(e) == 'string' and e:find('deferred to mailrelay rules') ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL(0, mock_template_calls);
    TEST_ASSERT_EQUAL(0, mock_direct_calls);
    H_lua_destroy_context(L);
}

void test_notify_does_not_enqueue(void) {
    lua_State* L = make_ctx();
    run_lua(L,
        "h = H.notify.send({ channel = 'email', to = 'a@x.com', body = 'hello' })\n"
        "r, e = H.wait(h)\n"
        "_ok = (r == nil and e ~= nil)\n");
    lua_getglobal(L, "_ok");
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);
    TEST_ASSERT_EQUAL_MESSAGE(0, mock_template_calls, "notify must not call mailrelay_send_template");
    TEST_ASSERT_EQUAL_MESSAGE(0, mock_direct_calls, "notify must not call mailrelay_send_direct");
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mail_send_returns_handle);
    RUN_TEST(test_mail_send_success_wait);
    RUN_TEST(test_mail_send_auto_idempotency_key);
    RUN_TEST(test_mail_send_explicit_idempotency_key);
    RUN_TEST(test_mail_send_missing_mode);
    RUN_TEST(test_mail_send_freeform_success);
    RUN_TEST(test_mail_send_freeform_body_alias);
    RUN_TEST(test_mail_send_mixed_mode_rejected);
    RUN_TEST(test_mail_send_freeform_missing_body);
    RUN_TEST(test_mail_send_missing_recipient);
    RUN_TEST(test_mail_send_disabled);
    RUN_TEST(test_mail_send_template_not_found);
    RUN_TEST(test_mail_send_sync_success);
    RUN_TEST(test_mail_send_sync_freeform);
    RUN_TEST(test_mail_wait_already_consumed);
    RUN_TEST(test_notify_deferred_error);
    RUN_TEST(test_notify_send_sync_deferred);
    RUN_TEST(test_notify_does_not_enqueue);

    return UNITY_END();
}
