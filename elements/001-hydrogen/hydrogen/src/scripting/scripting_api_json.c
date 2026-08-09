/*
 * Scripting Subsystem - Host API: JSON/Lua Conversion Helpers
 *
 * Shared helpers for converting Lua parameter tables to Hydrogen's
 * typed parameter_json format and for converting database result JSON
 * into Lua tables.
 */

// Project includes
#include <src/hydrogen.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"

// Forward declarations for the recursive JSON-to-Lua helpers.
void push_json_value_as_lua(lua_State* L, json_t* val);

/*
 * Convert a Lua table at `arg` to Hydrogen's parameter_json format.
 */
char* H_lua_params_to_json(lua_State* L, int arg) {
    if (!lua_istable(L, arg)) {
        return NULL;
    }

    json_t* int_obj = json_object();
    json_t* str_obj = json_object();
    json_t* bool_obj = json_object();
    json_t* float_obj = json_object();
    if (!int_obj || !str_obj || !bool_obj || !float_obj) {
        if (int_obj) json_decref(int_obj);
        if (str_obj) json_decref(str_obj);
        if (bool_obj) json_decref(bool_obj);
        if (float_obj) json_decref(float_obj);
        return NULL;
    }

    int idx = 0;
    lua_pushnil(L);
    while (lua_next(L, arg) != 0) {
        const char* key = lua_tostring(L, -2);
        if (!key) {
            lua_pop(L, 1);
            continue;
        }
        int t = lua_type(L, -1);
        if (t == LUA_TNUMBER) {
            double d = lua_tonumber(L, -1);
            int is_int = 0;
            lua_Integer li = lua_tointegerx(L, -1, &is_int);
            if (is_int) {
                json_object_set_new(int_obj, key, json_integer(li));
            } else {
                json_object_set_new(float_obj, key, json_real(d));
            }
        } else if (t == LUA_TSTRING) {
            const char* s = lua_tostring(L, -1);
            json_object_set_new(str_obj, key, json_string(s ? s : ""));
        } else if (t == LUA_TBOOLEAN) {
            json_object_set_new(bool_obj, key, json_boolean(lua_toboolean(L, -1)));
        } else if (t == LUA_TNIL) {
            // omit
        } else {
            log_this(SR_SCRIPTING,
                     "H.*query: skipping unsupported param type for key '%s'",
                     LOG_LEVEL_ALERT, 1, key);
        }
        lua_pop(L, 1);
        idx++;
        if (idx > 1000) {
            log_this(SR_SCRIPTING, "H.*query: params table too large (>1000 entries)",
                     LOG_LEVEL_ALERT, 0);
            break;
        }
    }

    json_t* root = json_object();
    if (json_object_size(int_obj) > 0) {
        json_object_set_new(root, "INTEGER", int_obj);
    } else {
        json_decref(int_obj);
    }
    if (json_object_size(str_obj) > 0) {
        json_object_set_new(root, "STRING", str_obj);
    } else {
        json_decref(str_obj);
    }
    if (json_object_size(bool_obj) > 0) {
        json_object_set_new(root, "BOOLEAN", bool_obj);
    } else {
        json_decref(bool_obj);
    }
    if (json_object_size(float_obj) > 0) {
        json_object_set_new(root, "FLOAT", float_obj);
    } else {
        json_decref(float_obj);
    }

    char* out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

void push_json_object_as_table(lua_State* L, json_t* obj) {
    if (!json_is_object(obj)) {
        lua_pushnil(L);
        return;
    }
    lua_newtable(L);
    const char* key = NULL;
    json_t* val = NULL;
    json_object_foreach((json_t*)obj, key, val) {
        push_json_value_as_lua(L, val);
        lua_setfield(L, -2, key);
    }
}

void push_json_array_as_table(lua_State* L, json_t* arr) {
    lua_newtable(L);
    size_t idx = 0;
    json_t* val = NULL;
    size_t i = 1;
    json_array_foreach((json_t*)arr, i, val) {
        push_json_value_as_lua(L, val);
        lua_rawseti(L, -2, (lua_Integer)idx + 1);
        idx++;
    }
}

void push_json_value_as_lua(lua_State* L, json_t* val) {
    if (!val) {
        lua_pushnil(L);
        return;
    }
    switch (json_typeof(val)) {
    case JSON_NULL:
        lua_pushnil(L);
        break;
    case JSON_TRUE:
        lua_pushboolean(L, 1);
        break;
    case JSON_FALSE:
        lua_pushboolean(L, 0);
        break;
    case JSON_INTEGER:
        lua_pushinteger(L, (lua_Integer)json_integer_value(val));
        break;
    case JSON_REAL:
        lua_pushnumber(L, (lua_Number)json_real_value(val));
        break;
    case JSON_STRING:
        lua_pushstring(L, json_string_value(val));
        break;
    case JSON_ARRAY:
        push_json_array_as_table(L, val);
        break;
    case JSON_OBJECT:
        push_json_object_as_table(L, val);
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

/*
 * Convert a Lua value at `idx` to a jansson value (caller owns).
 * Tables with sequential integer keys 1..n become arrays; otherwise objects.
 * Depth is capped to avoid hostile graphs.
 */
json_t* H_lua_value_to_json(lua_State* L, int idx, int depth) {
    if (!L || depth > 64) {
        return NULL;
    }
    idx = lua_absindex(L, idx);
    int t = lua_type(L, idx);
    switch (t) {
    case LUA_TNIL:
        return json_null();
    case LUA_TBOOLEAN:
        return json_boolean(lua_toboolean(L, idx));
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            return json_integer((json_int_t)lua_tointeger(L, idx));
        }
        return json_real((double)lua_tonumber(L, idx));
    case LUA_TSTRING: {
        const char* s = lua_tostring(L, idx);
        return json_string(s ? s : "");
    }
    case LUA_TTABLE: {
        /* Detect array: keys are exactly 1..n consecutive integers. */
        size_t n = lua_rawlen(L, idx);
        bool is_array = (n > 0);
        if (is_array) {
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                if (!lua_isinteger(L, -2)) {
                    is_array = false;
                    lua_pop(L, 2);
                    break;
                }
                lua_Integer k = lua_tointeger(L, -2);
                if (k < 1 || (size_t)k > n) {
                    is_array = false;
                    lua_pop(L, 2);
                    break;
                }
                lua_pop(L, 1);
            }
            /* Count keys: must equal n for a pure array. */
            if (is_array) {
                size_t key_count = 0;
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    key_count++;
                    lua_pop(L, 1);
                }
                if (key_count != n) {
                    is_array = false;
                }
            }
        } else if (n == 0) {
            /* Empty table or map: treat empty as object {}. */
            is_array = false;
        }

        if (is_array) {
            json_t* arr = json_array();
            if (!arr) {
                return NULL;
            }
            for (size_t i = 1; i <= n; i++) {
                lua_rawgeti(L, idx, (lua_Integer)i);
                json_t* elem = H_lua_value_to_json(L, -1, depth + 1);
                lua_pop(L, 1);
                if (!elem) {
                    json_decref(arr);
                    return NULL;
                }
                if (json_array_append_new(arr, elem) != 0) {
                    json_decref(arr);
                    return NULL;
                }
            }
            return arr;
        }

        json_t* obj = json_object();
        if (!obj) {
            return NULL;
        }
        lua_pushnil(L);
        while (lua_next(L, idx) != 0) {
            char keybuf[64];
            const char* key = NULL;
            if (lua_type(L, -2) == LUA_TSTRING) {
                key = lua_tostring(L, -2);
            } else if (lua_isinteger(L, -2)) {
                snprintf(keybuf, sizeof(keybuf), "%lld",
                         (long long)lua_tointeger(L, -2));
                key = keybuf;
            } else {
                lua_pop(L, 1);
                continue;
            }
            if (!key) {
                lua_pop(L, 1);
                continue;
            }
            json_t* val = H_lua_value_to_json(L, -1, depth + 1);
            lua_pop(L, 1);
            if (!val) {
                json_decref(obj);
                return NULL;
            }
            if (json_object_set_new(obj, key, val) != 0) {
                json_decref(obj);
                return NULL;
            }
        }
        return obj;
    }
    default:
        return NULL;
    }
}

/*
 * Encode Lua table at `arg` to a compact JSON string (caller frees).
 * Returns NULL on type/encode failure.
 */
char* H_lua_table_to_json_string(lua_State* L, int arg) {
    if (!L || !lua_istable(L, arg)) {
        return NULL;
    }
    json_t* root = H_lua_value_to_json(L, arg, 0);
    if (!root) {
        return NULL;
    }
    char* out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

/*
 * LUA_CLIENT: expose job params_json as the global `params` table before
 * the worker runs script source. NULL/empty/invalid JSON → empty table.
 */
void H_lua_inject_job_params(lua_State* L, const char* params_json) {
    if (!L) {
        return;
    }
    if (!params_json || params_json[0] == '\0') {
        lua_newtable(L);
        lua_setglobal(L, "params");
        return;
    }
    json_error_t err;
    json_t* root = json_loads(params_json, 0, &err);
    if (!root) {
        log_this(SR_SCRIPTING,
                 "H_lua_inject_job_params: parse failed: %s",
                 LOG_LEVEL_ALERT, 1, err.text);
        lua_newtable(L);
        lua_setglobal(L, "params");
        return;
    }
    if (!json_is_object(root) && !json_is_array(root)) {
        log_this(SR_SCRIPTING,
                 "H_lua_inject_job_params: root must be object or array",
                 LOG_LEVEL_ALERT, 0);
        json_decref(root);
        lua_newtable(L);
        lua_setglobal(L, "params");
        return;
    }
    push_json_value_as_lua(L, root);
    json_decref(root);
    lua_setglobal(L, "params");
}

/*
 * Parse data_json and push a result table onto the Lua stack.
 */
int H_lua_build_result_table(lua_State* L, const char* data_json, int affected_rows) {
    lua_newtable(L);
    lua_pushstring(L, "rows");
    if (data_json && *data_json) {
        json_error_t err;
        json_t* arr = json_loads(data_json, 0, &err);
        if (arr && json_is_array(arr)) {
            size_t n = json_array_size(arr);
            lua_newtable(L);
            for (size_t i = 0; i < n; i++) {
                push_json_value_as_lua(L, json_array_get(arr, i));
                lua_rawseti(L, -2, (lua_Integer)(i + 1));
            }
            json_decref(arr);
        } else {
            log_this(SR_SCRIPTING, "H.*query: data_json parse failed: %s",
                     LOG_LEVEL_ALERT, 1,
                     arr ? "not a JSON array" : err.text);
            if (arr) json_decref(arr);
            lua_newtable(L);
        }
    } else {
        lua_newtable(L);
    }
    lua_settable(L, -3);

    lua_pushstring(L, "affected_rows");
    lua_pushinteger(L, (lua_Integer)affected_rows);
    lua_settable(L, -3);

    return 1;
}
