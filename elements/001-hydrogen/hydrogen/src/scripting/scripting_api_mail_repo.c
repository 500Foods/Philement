/*
 * Scripting Subsystem - Host API: H.mail repository helpers
 *
 * Synchronous wrappers around mailrelay_repo_* QueryRefs so trusted Lua
 * can list templates/routes, look up queue rows, run cleanup, and insert
 * durable mail_events rows without embedding SQL.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>

#include <jansson.h>
#include <lua.h>
#include <lauxlib.h>

#include <src/mailrelay/mailrelay_repository.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_api_mail_notify.h"

void mail_repo_lua_callback(MailRelayRepoResult* result, void* user_data) {
    MailRepoLuaCtx* ctx = (MailRepoLuaCtx*)user_data;
    if (!ctx || !result) {
        return;
    }
    ctx->status = result->status;
    ctx->affected_rows = result->affected_rows;
    ctx->error[0] = '\0';
    if (result->error_message && result->error_message[0] != '\0') {
        snprintf(ctx->error, sizeof(ctx->error), "%s", result->error_message);
    }
    if (result->data) {
        ctx->data = json_incref(result->data);
    }
}

const char* mail_repo_status_message(MailRelayRepoStatus status,
                                     const char* detail) {
    if (detail && detail[0] != '\0') {
        return detail;
    }
    switch (status) {
    case MAILRELAY_REPO_OK:
        return "ok";
    case MAILRELAY_REPO_INVALID_ARGS:
        return "MAIL_PARAM_MISSING: invalid repository arguments";
    case MAILRELAY_REPO_NO_DATABASE:
        return "MAIL_PERSIST_FAILED: Mail Relay database not configured";
    case MAILRELAY_REPO_QUERY_NOT_FOUND:
        return "MAIL_PERSIST_FAILED: QueryRef not found in cache";
    case MAILRELAY_REPO_SUBMIT_FAILED:
        return "MAIL_PERSIST_FAILED: query submit failed";
    case MAILRELAY_REPO_TIMEOUT:
        return "MAIL_PERSIST_FAILED: query timed out";
    case MAILRELAY_REPO_QUERY_ERROR:
        return "MAIL_PERSIST_FAILED: query error";
    case MAILRELAY_REPO_PARSE_ERROR:
        return "MAIL_PERSIST_FAILED: result parse error";
    default:
        return "MAIL_PERSIST_FAILED: unknown repository error";
    }
}

/*
 * Push result, err (2 values). Owns and releases ctx->data.
 */
int mail_repo_push_result(lua_State* L, MailRepoLuaCtx* ctx) {
    if (!ctx) {
        lua_pushnil(L);
        lua_pushstring(L, "MAIL_PARAM_MISSING: null repository context");
        return 2;
    }
    if (ctx->status != MAILRELAY_REPO_OK) {
        lua_pushnil(L);
        lua_pushstring(L, mail_repo_status_message(ctx->status, ctx->error));
        if (ctx->data) {
            json_decref(ctx->data);
            ctx->data = NULL;
        }
        return 2;
    }

    lua_newtable(L);
    lua_pushstring(L, "rows");
    if (ctx->data && json_is_array(ctx->data)) {
        push_json_value_as_lua(L, ctx->data);
    } else if (ctx->data && json_is_object(ctx->data)) {
        lua_newtable(L);
        push_json_value_as_lua(L, ctx->data);
        lua_rawseti(L, -2, 1);
    } else {
        lua_newtable(L);
    }
    lua_settable(L, -3);

    lua_pushstring(L, "affected_rows");
    lua_pushinteger(L, (lua_Integer)ctx->affected_rows);
    lua_settable(L, -3);

    if (ctx->data) {
        json_decref(ctx->data);
        ctx->data = NULL;
    }
    lua_pushnil(L);
    return 2;
}

int H_lua_mail_template_list(lua_State* L) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_template_list_active(mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_template_get(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    MailRelayRepoTemplateGetByKey params = {
        .template_key = key
    };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_template_get_by_key(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_route_list(lua_State* L) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_route_list_active(mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_queue_get(lua_State* L) {
    const char* uuid = luaL_checkstring(L, 1);
    MailRelayRepoQueueGetByUuid params = {
        .message_uuid = uuid
    };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_queue_get_by_uuid(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_cleanup_queue(lua_State* L) {
    const char* cutoff = luaL_checkstring(L, 1);
    MailRelayRepoCleanupQueue params = { .cutoff_at = cutoff };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_cleanup_queue(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_cleanup_events(lua_State* L) {
    const char* cutoff = luaL_checkstring(L, 1);
    MailRelayRepoCleanupEvents params = { .cutoff_at = cutoff };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_cleanup_events(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_cleanup_attempts(lua_State* L) {
    const char* cutoff = luaL_checkstring(L, 1);
    MailRelayRepoCleanupAttempts params = { .cutoff_at = cutoff };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_cleanup_attempts(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_cleanup_otp(lua_State* L) {
    const char* cutoff = luaL_checkstring(L, 1);
    MailRelayRepoCleanupOtp params = { .cutoff_at = cutoff };
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_cleanup_otp(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

int H_lua_mail_event_list_pending(lua_State* L) {
    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_event_list_pending(mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }
    return mail_repo_push_result(L, &ctx);
}

/*
 * H.mail.event_insert({ event_key, template_key?, recipients_json?, ... })
 * Builds MailRelayRepoEventInsert from a Lua table.
 */
int H_lua_mail_event_insert(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "event_key");
    const char* event_key = lua_tostring(L, -1);
    if (!event_key || event_key[0] == '\0') {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "MAIL_PARAM_MISSING: event_key required");
        return 2;
    }

    lua_getfield(L, 1, "status_a65");
    int status_a65 = (int)luaL_optinteger(L, -1, 0);
    lua_getfield(L, 1, "template_key");
    const char* template_key = lua_tostring(L, -1);
    lua_getfield(L, 1, "from_addr");
    const char* from_addr = lua_tostring(L, -1);
    lua_getfield(L, 1, "reply_to");
    const char* reply_to = lua_tostring(L, -1);
    lua_getfield(L, 1, "recipients_json");
    const char* recipients_json = lua_tostring(L, -1);
    lua_getfield(L, 1, "subject");
    const char* subject = lua_tostring(L, -1);
    lua_getfield(L, 1, "body_text");
    const char* body_text = lua_tostring(L, -1);
    lua_getfield(L, 1, "body_html");
    const char* body_html = lua_tostring(L, -1);
    lua_getfield(L, 1, "headers_json");
    const char* headers_json = lua_tostring(L, -1);
    lua_getfield(L, 1, "params_json");
    const char* params_json = lua_tostring(L, -1);
    lua_getfield(L, 1, "debounce_key");
    const char* debounce_key = lua_tostring(L, -1);
    lua_getfield(L, 1, "idempotency_key");
    const char* idempotency_key = lua_tostring(L, -1);
    lua_getfield(L, 1, "priority");
    int priority = (int)luaL_optinteger(L, -1, 0);

    MailRelayRepoEventInsert params = {
        .event_key = event_key,
        .status_a65 = status_a65,
        .template_key = template_key,
        .from_addr = from_addr,
        .reply_to = reply_to,
        .recipients_json = recipients_json,
        .subject = subject,
        .body_text = body_text,
        .body_html = body_html,
        .headers_json = headers_json,
        .params_json = params_json,
        .debounce_key = debounce_key,
        .idempotency_key = idempotency_key,
        .priority = priority
    };

    MailRepoLuaCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.status = MAILRELAY_REPO_INVALID_ARGS;
    if (!mailrelay_repo_event_insert(&params, mail_repo_lua_callback, &ctx)) {
        if (ctx.status == MAILRELAY_REPO_OK) {
            ctx.status = MAILRELAY_REPO_SUBMIT_FAILED;
        }
    }

    lua_pop(L, 14);
    return mail_repo_push_result(L, &ctx);
}

void H_lua_install_mail_repo(lua_State* L) {
    if (!L) {
        return;
    }
    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "mail");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_mail_template_list);
    lua_setfield(L, -2, "template_list");
    lua_pushcfunction(L, H_lua_mail_template_get);
    lua_setfield(L, -2, "template_get");
    lua_pushcfunction(L, H_lua_mail_route_list);
    lua_setfield(L, -2, "route_list");
    lua_pushcfunction(L, H_lua_mail_queue_get);
    lua_setfield(L, -2, "queue_get");
    lua_pushcfunction(L, H_lua_mail_cleanup_queue);
    lua_setfield(L, -2, "cleanup_queue");
    lua_pushcfunction(L, H_lua_mail_cleanup_events);
    lua_setfield(L, -2, "cleanup_events");
    lua_pushcfunction(L, H_lua_mail_cleanup_attempts);
    lua_setfield(L, -2, "cleanup_attempts");
    lua_pushcfunction(L, H_lua_mail_cleanup_otp);
    lua_setfield(L, -2, "cleanup_otp");
    lua_pushcfunction(L, H_lua_mail_event_list_pending);
    lua_setfield(L, -2, "event_list_pending");
    lua_pushcfunction(L, H_lua_mail_event_insert);
    lua_setfield(L, -2, "event_insert");

    lua_pop(L, 2);
}
