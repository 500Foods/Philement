/*
 * Lua host API for NATS.
 *
 * H.nats.broadcast and H.nats.broadcast_sync publish one envelope.
 * cache.invalidate_by_ref stays on nats_broadcast. Every other event
 * uses nats_publish_event and does not evict. app_state is reserved.
 * status and instances read the link and the registry. This file
 * does not register subscribe or unsubscribe, and the NATS thread
 * does not call Lua. The payload is not logged.
 */

#include <src/hydrogen.h>

#include <string.h>

#include <lua.h>
#include <jansson.h>

#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_subject.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"

int H_lua_nats_fail(lua_State* L, const char* message) {
    lua_pushnil(L);
    lua_pushstring(L, message);
    return 2;
}

int H_lua_nats_publish(lua_State* L, bool sync) {
    const char* event;
    json_t* data = NULL;

    if (!app_config || !app_config->nats.Enabled) {
        return H_lua_nats_fail(L, "disabled");
    }
    if (lua_type(L, 1) != LUA_TSTRING) {
        return H_lua_nats_fail(L, "bad event");
    }
    event = lua_tostring(L, 1);
    if (!nats_event_suffix_ok(event)) {
        return H_lua_nats_fail(L, "bad event");
    }
    if (strcmp(event, "app_state") == 0) {
        return H_lua_nats_fail(L, "reserved");
    }
    {
        char* text = H_lua_table_to_json_string(L, 2);

        if (text) {
            data = json_loads(text, 0, NULL);
            free(text);
        }
    }
    if (!json_is_object(data)) {
        json_decref(data);
        return H_lua_nats_fail(L, "bad event");
    }
    if (sync && nats_link_get() != NATS_LINK_UP) {
        json_decref(data);
        return H_lua_nats_fail(L, "link down");
    }
    {
        int rc;

        if (strcmp(event, "cache.invalidate_by_ref") == 0) {
            rc = nats_broadcast(event, data);
        } else {
            rc = nats_publish_event(event, data);
        }
        json_decref(data);
        if (rc != 0) {
            return H_lua_nats_fail(L, "publish failed");
        }
    }
    lua_pushboolean(L, 1);
    return 1;
}

int H_lua_nats_broadcast(lua_State* L) {
    return H_lua_nats_publish(L, false);
}

int H_lua_nats_broadcast_sync(lua_State* L) {
    return H_lua_nats_publish(L, true);
}

int H_lua_nats_status(lua_State* L) {
    bool enabled = app_config && app_config->nats.Enabled;
    bool presence = app_config && app_config->nats.Presence.Enabled;

    lua_newtable(L);
    lua_pushboolean(L, enabled);
    lua_setfield(L, -2, "enabled");
    lua_pushstring(L, nats_link_state_name());
    lua_setfield(L, -2, "link");
    lua_pushboolean(L, presence);
    lua_setfield(L, -2, "presence");
    lua_pushboolean(L, nats_registry_is_singleton());
    lua_setfield(L, -2, "singleton");
    lua_pushinteger(L, nats_registry_peer_count());
    lua_setfield(L, -2, "peers");
    lua_pushinteger(L, nats_registry_alive_count());
    lua_setfield(L, -2, "alive");
    lua_pushstring(L, nats_registry_self_state());
    lua_setfield(L, -2, "self");
    return 1;
}

int H_lua_nats_instances(lua_State* L) {
    bool presence = app_config && app_config->nats.Presence.Enabled;
    int count = 0;
    NatsRegistryCopy copied[NATS_REGISTRY_COPY_CAP];

    memset(copied, 0, sizeof(copied));
    if (presence) {
        count = nats_registry_copy(copied, NATS_REGISTRY_COPY_CAP);
    }
    lua_newtable(L);
    lua_pushboolean(L, nats_registry_is_singleton());
    lua_setfield(L, -2, "singleton");
    lua_pushstring(L, nats_registry_self_state());
    lua_setfield(L, -2, "self");
    lua_newtable(L);
    {
        int i;

        for (i = 0; i < count; i++) {
            lua_newtable(L);
            lua_pushstring(L, copied[i].id);
            lua_setfield(L, -2, "id");
            lua_pushstring(L, copied[i].state);
            lua_setfield(L, -2, "state");
            lua_pushinteger(L, copied[i].websocket_connections);
            lua_setfield(L, -2, "websocket_connections");
            lua_rawseti(L, -2, i + 1);
        }
    }
    lua_setfield(L, -2, "peers");
    return 1;
}

void H_lua_install_nats(lua_State* L) {
    if (!L) {
        return;
    }
    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_SCRIPTING, "H_lua_install_nats: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_newtable(L);
    lua_pushcfunction(L, H_lua_nats_broadcast);
    lua_setfield(L, -2, "broadcast");
    lua_pushcfunction(L, H_lua_nats_broadcast_sync);
    lua_setfield(L, -2, "broadcast_sync");
    lua_pushcfunction(L, H_lua_nats_status);
    lua_setfield(L, -2, "status");
    lua_pushcfunction(L, H_lua_nats_instances);
    lua_setfield(L, -2, "instances");
    lua_setfield(L, -2, "nats");
    lua_pop(L, 1);
}
