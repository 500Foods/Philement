/*
 * Scripting Subsystem - Host API: H.mail, H.notify (Stubs)
 *
 * Phase 19 of the LUA_PLAN. Provides stub implementations for mail and
 * notify send functions. Real implementations depend on Mail Relay /
 * Notify subsystems landing later.
 *
 * The stubs always return an error handle so the API surface is stable
 * and scripts can be written now.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "scripting_api.h"
#include "scripting_handle.h"

static const char* MAIL_NOT_IMPLEMENTED_ERROR = "mail: not implemented";
static const char* NOTIFY_NOT_IMPLEMENTED_ERROR = "notify: not implemented";

/*
 * H.mail.send(message, opts?) -> handle
 *
 * Stub implementation that always returns an error handle.
 * message is a Lua table with fields:
 *   to       = string or array of strings (email addresses)
 *   subject  = string
 *   body     = string
 *   template = optional string (name of a mail template)
 */
int H_lua_mail_send(lua_State* L) {
    H_Handle* h = H_Handle_new(L, H_HK_MAIL);
    if (!h) {
        return 0;
    }
    h->mail_error = strdup(MAIL_NOT_IMPLEMENTED_ERROR);
    return 1;
}

/*
 * H.mail.send_sync(message, opts?) -> result, err
 *
 * Convenience wrapper: submit + wait in one call.
 * For stubs, always returns nil + error.
 */
int H_lua_mail_send_sync(lua_State* L) {
    int n_pushed = H_lua_mail_send(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.mail.send_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.mail.send_sync: handle creation failed");
        return 2;
    }
    lua_pushnil(L);
    lua_pushstring(L, h->mail_error);
    return 2;
}

/*
 * H.notify.send(message, opts?) -> handle
 *
 * Stub implementation that always returns an error handle.
 * message is a Lua table with fields:
 *   channel = "sms", "push", etc.
 *   to      = string or array
 *   body    = string
 */
int H_lua_notify_send(lua_State* L) {
    H_Handle* h = H_Handle_new(L, H_HK_NOTIFY);
    if (!h) {
        return 0;
    }
    h->notify_error = strdup(NOTIFY_NOT_IMPLEMENTED_ERROR);
    return 1;
}

/*
 * H.notify.send_sync(message, opts?) -> result, err
 *
 * Convenience wrapper: submit + wait in one call.
 * For stubs, always returns nil + error.
 */
int H_lua_notify_send_sync(lua_State* L) {
    int n_pushed = H_lua_notify_send(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.notify.send_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.notify.send_sync: handle creation failed");
        return 2;
    }
    lua_pushnil(L);
    lua_pushstring(L, h->notify_error);
    return 2;
}

/*
 * Install H.mail and H.notify stub functions.
 *
 * Called from H_lua_install_api in lua_context.c.
 */
void H_lua_install_mail_notify(lua_State* L) {
    if (!L) {
        return;
    }

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_mail_notify: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    // Install H.mail sub-table
    lua_getfield(L, -1, "mail");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushcfunction(L, H_lua_mail_send);
        lua_setfield(L, -2, "send");
        lua_pushcfunction(L, H_lua_mail_send_sync);
        lua_setfield(L, -2, "send_sync");
        lua_setfield(L, -3, "mail");
    } else {
        lua_pop(L, 1);
    }

    // Install H.notify sub-table
    lua_getfield(L, -1, "notify");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushcfunction(L, H_lua_notify_send);
        lua_setfield(L, -2, "send");
        lua_pushcfunction(L, H_lua_notify_send_sync);
        lua_setfield(L, -2, "send_sync");
        lua_setfield(L, -3, "notify");
    } else {
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
}