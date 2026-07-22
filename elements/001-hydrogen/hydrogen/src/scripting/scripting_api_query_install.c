/*
 * Scripting Subsystem - Host API Installation
 *
 * Phase 13 of the LUA_PLAN: installs H.query, H.altquery, H.authquery,
 * H.wait, and sync wrappers into the H table.
 */

#include <src/hydrogen.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"

#include <src/api/conduit/conduit_helpers.h>
#include <src/api/auth/auth_service.h>

void H_lua_install_query(lua_State* L) {
    if (!L) return;

    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_query: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_pushcfunction(L, H_lua_query);        lua_setfield(L, -2, "query");
    lua_pushcfunction(L, H_lua_altquery);     lua_setfield(L, -2, "altquery");
    lua_pushcfunction(L, H_lua_authquery);    lua_setfield(L, -2, "authquery");
    lua_pushcfunction(L, H_lua_wait);         lua_setfield(L, -2, "wait");

    lua_pushcfunction(L, H_lua_query_sync);        lua_setfield(L, -2, "query_sync");
    lua_pushcfunction(L, H_lua_altquery_sync);     lua_setfield(L, -2, "altquery_sync");
    lua_pushcfunction(L, H_lua_authquery_sync);    lua_setfield(L, -2, "authquery_sync");

    H_lua_install_http(L);

    lua_pop(L, 1);
}