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
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include <src/mailrelay/mailrelay_repository.h>
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
int H_lua_system_info(lua_State* L);

int H_lua_gc_collect(lua_State* L);
int H_lua_gc_step(lua_State* L);
int H_lua_gc_count(lua_State* L);
int H_lua_gc_isrunning(lua_State* L);

int H_lua_set_current_state(lua_State* L);
int H_lua_set_result(lua_State* L);
int H_lua_set_result_json(lua_State* L);
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
int H_lua_scoreboard_prune_terminal(lua_State* L);

bool split_module_name(const char* name, char** group_out, char** script_out);
int H_lua_package_searcher(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_json.c
// ----------------------------------------------------------------------------

char* H_lua_params_to_json(lua_State* L, int arg);
char* H_lua_table_to_json_string(lua_State* L, int arg);
json_t* H_lua_value_to_json(lua_State* L, int idx, int depth);
void push_json_value_as_lua(lua_State* L, json_t* val);
void push_json_object_as_table(lua_State* L, json_t* obj);
void push_json_array_as_table(lua_State* L, json_t* arr);
void H_lua_inject_job_params(lua_State* L, const char* params_json);
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
int H_lua_finish_sync_wait(lua_State* L, int n_pushed,
                           const char* alloc_err, const char* create_err);
int H_lua_query_sync(lua_State* L);
int H_lua_altquery_sync(lua_State* L);
int H_lua_authquery_sync(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_mail_repo.c
// ----------------------------------------------------------------------------

typedef struct {
    MailRelayRepoStatus status;
    char error[256];
    json_t* data;
    int affected_rows;
} MailRepoLuaCtx;

void mail_repo_lua_callback(MailRelayRepoResult* result, void* user_data);
const char* mail_repo_status_message(MailRelayRepoStatus status, const char* detail);
int mail_repo_push_result(lua_State* L, MailRepoLuaCtx* ctx);
int H_lua_mail_template_list(lua_State* L);
int H_lua_mail_template_get(lua_State* L);
int H_lua_mail_route_list(lua_State* L);
int H_lua_mail_queue_get(lua_State* L);
int H_lua_mail_cleanup_queue(lua_State* L);
int H_lua_mail_cleanup_events(lua_State* L);
int H_lua_mail_cleanup_attempts(lua_State* L);
int H_lua_mail_cleanup_otp(lua_State* L);
int H_lua_mail_event_list_pending(lua_State* L);
int H_lua_mail_event_insert(lua_State* L);
void H_lua_install_mail_repo(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_http.c
// ----------------------------------------------------------------------------

struct curl_slist* H_lua_headers_to_slist(lua_State* L, int idx);
int H_lua_opts_timeout(lua_State* L, int idx);
int H_lua_http_get(lua_State* L);
int H_lua_http_post(lua_State* L);
int H_lua_http_wait_one(lua_State* L, H_Handle* h);
int H_lua_http_push_inline_result(lua_State* L, OidcRpHttpResponse* resp, long elapsed_ms);
int H_lua_http_push_pool_result(lua_State* L, H_Handle* h);
int H_lua_http_get_sync(lua_State* L);
int H_lua_http_post_sync(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_llm.c
// ----------------------------------------------------------------------------

int H_lua_llm_call(lua_State* L);
int H_lua_llm_list(lua_State* L);
int H_lua_llm_wait_one(lua_State* L, H_Handle* h);
int H_lua_llm_call_sync(lua_State* L);
int H_lua_llm_list_sync(lua_State* L);

// ----------------------------------------------------------------------------
// scripting_api_mcp.c
// ----------------------------------------------------------------------------

typedef char* (*H_lua_mcp_list_rows_fn)(void);
typedef char* (*H_lua_mcp_fetch_source_fn)(const char* group, const char* name);
typedef char* (*H_lua_mcp_submit_job_fn)(const char* script_name,
                                         const char* source,
                                         const char* params_json);
typedef int (*H_lua_mcp_wait_job_fn)(lua_State* L, H_Handle* h);

extern H_lua_mcp_list_rows_fn H_lua_mcp_list_rows_hook;
extern H_lua_mcp_fetch_source_fn H_lua_mcp_fetch_source_hook;
extern H_lua_mcp_submit_job_fn H_lua_mcp_submit_job_hook;
extern H_lua_mcp_wait_job_fn H_lua_mcp_wait_job_hook;
extern int H_lua_mcp_submit_count;

void H_lua_mcp_clear_hooks(void);
char* H_lua_mcp_fetch_list_rows_json(void);
void H_lua_mcp_push_decoded_json_field(lua_State* L, json_t* row, const char* key);
void H_lua_mcp_push_row(lua_State* L, json_t* row);
char* H_lua_mcp_parent_hydrogen_json(lua_State* L);
char* H_lua_mcp_build_tool_params(lua_State* L, int args_idx, char** err_out);
char* H_lua_mcp_load_tool_source(const char* group, const char* script);
int H_lua_mcp_list(lua_State* L);
int H_lua_mcp_call(lua_State* L);
int H_lua_mcp_call_async(lua_State* L);
int H_lua_mcp_wait_one(lua_State* L, H_Handle* h);
int H_lua_mcp_capture_result_json(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_API_INTERNAL_H */
