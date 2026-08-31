#include <src/hydrogen.h>

#include <arpa/inet.h>
#include <lua.h>

#include "scripting_api.h"
#include <src/mdns/mdns_client.h>

int H_lua_mdns_list(lua_State *L)
{
    mdns_client_service_t *snap = NULL;
    size_t n = 0;
    size_t i;
    const char *type = NULL;

    if (lua_isstring(L, 1)) {
        type = lua_tostring(L, 1);
    }
    if (type && type[0] != '\0') {
        n = mdns_client_lookup_by_type(type, &snap);
    } else {
        snap = mdns_client_snapshot(&n);
    }
    lua_newtable(L);
    for (i = 0; i < n; i++) {
        size_t e;
        lua_newtable(L);
        lua_pushstring(L, snap[i].instance);
        lua_setfield(L, -2, "name");
        lua_pushstring(L, snap[i].type);
        lua_setfield(L, -2, "type");
        lua_pushinteger(L, (lua_Integer)snap[i].port);
        lua_setfield(L, -2, "port");
        lua_pushboolean(L, snap[i].healthy != 0);
        lua_setfield(L, -2, "healthy");
        lua_newtable(L);
        for (e = 0; e < snap[i].nendpoints; e++) {
            char buf[INET6_ADDRSTRLEN];
            const mdns_client_endpoint_t *ep = &snap[i].endpoints[e];
            if (ep->family == AF_INET) {
                inet_ntop(AF_INET, ep->addr, buf, sizeof buf);
            } else {
                inet_ntop(AF_INET6, ep->addr, buf, sizeof buf);
            }
            lua_pushstring(L, buf);
            lua_rawseti(L, -2, (lua_Integer)(e + 1));
        }
        lua_setfield(L, -2, "addrs");
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }
    mdns_client_snapshot_free(snap);
    return 1;
}

void H_lua_install_mdns(lua_State *L)
{
    if (!L) {
        return;
    }
    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_SCRIPTING, "H_lua_install_mdns: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_newtable(L);
    lua_pushcfunction(L, H_lua_mdns_list);
    lua_setfield(L, -2, "list");
    lua_setfield(L, -2, "mdns");
    lua_pop(L, 1);
}
