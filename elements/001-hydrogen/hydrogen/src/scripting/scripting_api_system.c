/*
 * Scripting Subsystem - Host API: H.system, H.gc, H.set_current_state,
 * H.sleep, H.shutdown_requested
 *
 * C-side implementations of the utility and lifecycle host functions.
 */

// Project includes
#include <src/hydrogen.h>

// System includes
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "lua_context.h"
#include "scripting.h"

// H.system.* implementations ///////////////////////////////////////////////

int H_lua_system_uptime(lua_State* L) {
    (void)L;
    time_t start = get_server_start_time();
    time_t now = time(NULL);
    lua_Number elapsed = (start == 0) ? 0.0 : (lua_Number)(now - start);
    lua_pushnumber(L, elapsed);
    return 1;
}

int H_lua_system_now(lua_State* L) {
    lua_pushnumber(L, (lua_Number)time(NULL));
    return 1;
}

int H_lua_system_now_iso(lua_State* L) {
    time_t now = time(NULL);
    struct tm tm_buf;
    gmtime_r(&now, &tm_buf);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    lua_pushstring(L, buf);
    return 1;
}

int H_lua_system_instance_id(lua_State* L) {
    static char cached[UUID_STR_LEN];
    static bool initialized = false;

    if (!initialized) {
        unsigned char bytes[16] = {0};
        bool got_random = false;

        int fd = open("/dev/urandom", O_RDONLY);
        if (fd >= 0) {
            ssize_t n = read(fd, bytes, sizeof(bytes));
            close(fd);
            if (n == (ssize_t)sizeof(bytes)) {
                got_random = true;
            }
        }

        if (!got_random) {
            unsigned long seed = (unsigned long)time(NULL) ^
                                 ((unsigned long)getpid() << 16);
            for (int i = 0; i < 16; i++) {
                seed = seed * 1103515245UL + 12345UL;
                bytes[i] = (unsigned char)((seed >> 16) & 0xff);
            }
        }

        bytes[6] = (unsigned char)((bytes[6] & 0x0f) | 0x40);
        bytes[8] = (unsigned char)((bytes[8] & 0x3f) | 0x80);

        snprintf(cached, sizeof(cached),
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 bytes[0], bytes[1], bytes[2], bytes[3],
                 bytes[4], bytes[5], bytes[6], bytes[7],
                 bytes[8], bytes[9], bytes[10], bytes[11],
                 bytes[12], bytes[13], bytes[14], bytes[15]);
        initialized = true;
    }

    lua_pushstring(L, cached);
    return 1;
}

int H_lua_system_version(lua_State* L) {
    lua_pushstring(L, VERSION);
    return 1;
}

// H.gc.* implementations ///////////////////////////////////////////////////

int H_lua_gc_collect(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCCOLLECT, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

int H_lua_gc_step(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCSTEP, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

int H_lua_gc_count(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCCOUNT, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

int H_lua_gc_isrunning(lua_State* L) {
    int running = lua_gc(L, LUA_GCISRUNNING, 0);
    lua_pushboolean(L, running != 0);
    return 1;
}

// H.set_current_state implementation ///////////////////////////////////////

int H_lua_set_current_state(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        log_this(SR_SCRIPTING, "H.set_current_state: missing string argument",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }
    if (!lua_isstring(L, 1)) {
        log_this(SR_SCRIPTING, "H.set_current_state: argument must be a string",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }

    H_lua_job_context* ctx = H_lua_get_job_context(L);
    if (!ctx || ctx->job_id[0] == '\0' || !ctx->scoreboard) {
        return 0;
    }

    const char* state = lua_tostring(L, 1);
    if (!state) {
        return 0;
    }

    if (!scoreboard_update_current_state(ctx->scoreboard, ctx->job_id, state)) {
        log_this(SR_SCRIPTING,
                 "H.set_current_state: scoreboard update failed for job %s",
                 LOG_LEVEL_ERROR, 1, ctx->job_id);
    }

    return 0;
}

// H.sleep / H.shutdown_requested implementations ///////////////////////////

int H_lua_sleep(lua_State* L) {
    (void)L;
    int nargs = lua_gettop(L);
    int ms = 0;
    if (nargs < 1) {
        return 0;
    }
    if (lua_isnumber(L, 1)) {
        double v = lua_tonumber(L, 1);
        if (v <= 0.0) {
            return 0;
        }
        ms = (int)v;
    } else {
        log_this(SR_SCRIPTING, "H.sleep: argument must be a number",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }

    #define H_SLEEP_POLL_USEC (100 * 1000)
    int slept_us = 0;
    int total_us = ms * 1000;
    while (slept_us < total_us) {
        if (scripting_system_shutdown) {
            return 0;
        }
        int slice = total_us - slept_us;
        if (slice > H_SLEEP_POLL_USEC) {
            slice = H_SLEEP_POLL_USEC;
        }
        usleep((useconds_t)slice);
        slept_us += slice;
    }
    return 0;
    #undef H_SLEEP_POLL_USEC
}

int H_lua_shutdown_requested(lua_State* L) {
    (void)L;
    lua_pushboolean(L, scripting_system_shutdown != 0);
    return 1;
}

// Install functions ////////////////////////////////////////////////////////

void H_lua_install_system(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_system: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "system");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_system: H.system not a table", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_system_uptime);      lua_setfield(L, -2, "uptime");
    lua_pushcfunction(L, H_lua_system_now);         lua_setfield(L, -2, "now");
    lua_pushcfunction(L, H_lua_system_now_iso);     lua_setfield(L, -2, "now_iso");
    lua_pushcfunction(L, H_lua_system_instance_id); lua_setfield(L, -2, "instance_id");
    lua_pushcfunction(L, H_lua_system_version);     lua_setfield(L, -2, "version");

    lua_pop(L, 2);
}

void H_lua_install_gc(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_gc: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "gc");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_gc: H.gc not a table", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_gc_collect);   lua_setfield(L, -2, "collect");
    lua_pushcfunction(L, H_lua_gc_step);      lua_setfield(L, -2, "step");
    lua_pushcfunction(L, H_lua_gc_count);     lua_setfield(L, -2, "count");
    lua_pushcfunction(L, H_lua_gc_isrunning); lua_setfield(L, -2, "isrunning");

    lua_pop(L, 2);
}

void H_lua_install_set_current_state(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_set_current_state: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_pushcfunction(L, H_lua_set_current_state);
    lua_setfield(L, -2, "set_current_state");

    lua_pop(L, 1);
}

void H_lua_install_sleep_shutdown(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_sleep_shutdown: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_pushcfunction(L, H_lua_sleep);
    lua_setfield(L, -2, "sleep");

    lua_pushcfunction(L, H_lua_shutdown_requested);
    lua_setfield(L, -2, "shutdown_requested");

    lua_pop(L, 1);
}
