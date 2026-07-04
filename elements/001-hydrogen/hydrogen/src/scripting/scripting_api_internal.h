/*
 * Scripting Subsystem - Host API Internal Prototypes
 *
 * This header declares the implementation functions that back the H.*
 * host API. The declarations are kept separate from scripting_api.h so
 * the public header only exposes the install entry points, while every
 * function remains non-static and directly testable from Unity tests.
 *
 * Functions are grouped by the source file that defines them. Include
 * this header from any implementation file that needs cross-file
 * prototypes and from Unity tests that exercise individual helpers.
 */

#ifndef HYDROGEN_SCRIPTING_SCRIPTING_API_INTERNAL_H
#define HYDROGEN_SCRIPTING_SCRIPTING_API_INTERNAL_H

// Third-party headers
#include <lua.h>
#include <jansson.h>

// Project headers
#include <src/database/dbqueue/dbqueue.h>
#include "scoreboard.h"
#include "scripting_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// scripting_api_log.c
// ----------------------------------------------------------------------------

int H_lua_log_at_level(lua_State* L, int priority);
int H_lua_log_trace(lua_State* L);
int H_lua_log_debug(lua_State* L);
int H_lua_log_info(lua_State* L);
int H_lua_log_warn(lua_State* L);
int H_lua_log_error(lua_State* L);
int H_lua_log_fatal(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_system.c
// ----------------------------------------------------------------------------

int H_lua_system_uptime(lua_State* L);
int H_lua_system_now(lua_State* L);
int H_lua_system_now_iso(lua_State* L);
int H_lua_system_instance_id(lua_State* L);
int H_lua_system_version(lua_State* L);

int H_lua_gc_collect(lua_State* L);
int H_lua_gc_step(lua_State* L);
int H_lua_gc_count(lua_State* L);
int H_lua_gc_isrunning(lua_State* L);

int H_lua_set_current_state(lua_State* L);
int H_lua_sleep(lua_State* L);
int H_lua_shutdown_requested(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_scoreboard.c
// ----------------------------------------------------------------------------

Scoreboard* resolve_active_scoreboard(lua_State* L);
void push_scoreboard_entry_as_table(lua_State* L, const ScoreboardEntry* e);
int H_lua_scoreboard_list(lua_State* L);
int H_lua_scoreboard_get(lua_State* L);
int H_lua_scoreboard_submit(lua_State* L);
int H_lua_scoreboard_cancel(lua_State* L);

bool split_module_name(const char* name, char** group_out, char** script_out);
int H_lua_package_searcher(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_json.c
// ----------------------------------------------------------------------------

char* H_lua_params_to_json(lua_State* L, int arg);
void push_json_value_as_lua(lua_State* L, json_t* val);
void push_json_object_as_table(lua_State* L, json_t* obj);
void push_json_array_as_table(lua_State* L, json_t* arr);
int H_lua_build_result_table(lua_State* L, const char* data_json, int affected_rows);

// ----------------------------------------------------------------------------
// scripting_api_query.c
// ----------------------------------------------------------------------------

DatabaseQueue* resolve_db_queue(const char* db_name, char** err_out);
int H_lua_submit_query(lua_State* L,
                       const char* db_name,
                       const char* sql,
                       const char* params_json,
                       int timeout_seconds,
                       const char* call_label);
int get_default_query_timeout(void);
int H_lua_query(lua_State* L);
int H_lua_altquery(lua_State* L);
char* validate_jwt_and_get_db(const char* token, char** err_out);
int H_lua_authquery(lua_State* L);
int H_lua_wait_one(lua_State* L, H_Handle* h);
int H_lua_wait(lua_State* L);
int H_lua_query_sync(lua_State* L);
int H_lua_altquery_sync(lua_State* L);
int H_lua_authquery_sync(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_http.c
// ----------------------------------------------------------------------------

struct curl_slist* H_lua_headers_to_slist(lua_State* L, int idx);
int H_lua_opts_timeout(lua_State* L, int idx);
int H_lua_http_get(lua_State* L);
int H_lua_http_post(lua_State* L);
int H_lua_http_wait_one(lua_State* L, H_Handle* h);
int H_lua_http_get_sync(lua_State* L);
int H_lua_http_post_sync(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_INTERNAL_H */
