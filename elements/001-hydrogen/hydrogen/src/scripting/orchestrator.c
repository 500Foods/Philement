/*
 * Scripting Subsystem - Orchestrator
 *
 * Phase 11d/f of the LUA_PLAN. See orchestrator.h for the design.
 *
 * Implementation notes:
 *   - The long-lived lua_State lives in the existing
 *     scripting_orchestrator_state global (declared in scripting.h) so
 *     the Phase 3b landing readiness check and scripting_cleanup_state
 *     see the same pointer.
 *   - The pthread is the only thread that touches the lua_State
 *     (matches the Phase 1 two-tier architecture).
 *   - UAF discipline: the source string and chunk name are heap-
 *     allocated and owned by the orchestrator module. They are passed
 *     by pointer to the pthread, which frees them after the lua_State
 *     is destroyed. scripting_orchestrator_destroy() must not free
 *     them concurrently; it only signals shutdown, joins the thread
 *     (which frees the buffers as part of its exit path), and clears
 *     the module-level state.
 *   - 11f: scripting_orchestrator_start_from_db fetches the Orchestrator
 *     source from the scripts table via QueryRef #087, using
 *     config->scripting.DefaultDatabase to resolve the database queue.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

// Local includes
#include "orchestrator.h"
#include "lua_context.h"
#include "scripting.h"
#include <src/threads/threads.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_cache.h>
#include <src/api/conduit/query/query.h>
#include <src/utils/utils_logging.h>
#include <src/config/config.h>

// Module-level state. The long-lived lua_State lives in
// scripting_orchestrator_state (existing Phase 3b global). The rest
// of the module-level state below is the threading + bookkeeping
// that the global doesn't carry.
static pthread_mutex_t   orchestrator_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t         orchestrator_thread;
static bool              orchestrator_thread_valid = false;
static char*             orchestrator_source = NULL;
static char*             orchestrator_chunk_name = NULL;
static volatile sig_atomic_t orchestrator_shutting_down = 0;

extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;
extern lua_State* scripting_orchestrator_state;

// Forward declarations
static void* orchestrator_thread_main(void* arg);
static void   orchestrator_set_shutdown_and_join(void);

// ----------------------------------------------------------------------------
// Internal: pthread entry point
// ----------------------------------------------------------------------------

/*
 * The orchestrator pthread's main function. Reads the source and
 * chunk name from module-level state, compiles and runs the script
 * on the lua_State, tears the state down, and frees the buffers.
 *
 * Compile + run are in two steps: luaL_loadbufferx, then lua_pcall.
 * The two-tier architecture compiles the orchestrator chunk once;
 * the migration engine's per-compile corruption history does not
 * apply here because there is exactly one compilation per state and
 * the state is destroyed at thread exit.
 */
static void* orchestrator_thread_main(void* arg) {
    (void)arg;

    char* source = NULL;
    char* chunk_name = NULL;
    lua_State* L = NULL;

    pthread_mutex_lock(&orchestrator_mutex);
    source = orchestrator_source;
    chunk_name = orchestrator_chunk_name;
    L = scripting_orchestrator_state;
    pthread_mutex_unlock(&orchestrator_mutex);

    if (!L || !source || !chunk_name) {
        // start() raced with destroy() and lost; clean exit.
        free(source);
        free(chunk_name);
        return NULL;
    }

    int load_rc = luaL_loadbufferx(L, source, strlen(source),
                                    chunk_name, NULL);
    if (load_rc != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_this(SR_SCRIPTING,
                 "Orchestrator: failed to compile %s: %s",
                 LOG_LEVEL_ERROR, 2, chunk_name, err ? err : "(no message)");
        lua_pop(L, 1);
    } else {
        int call_rc = lua_pcall(L, 0, 0, 0);
        if (call_rc != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            log_this(SR_SCRIPTING,
                     "Orchestrator: %s returned error: %s",
                     LOG_LEVEL_ERROR, 2, chunk_name, err ? err : "(no message)");
            lua_pop(L, 1);
        } else {
            log_this(SR_SCRIPTING, "Orchestrator: %s returned",
                     LOG_LEVEL_STATE, 1, chunk_name);
        }
    }

    // Tear down the state, then free the source and chunk name.
    H_lua_destroy_context(L);

    pthread_mutex_lock(&orchestrator_mutex);
    scripting_orchestrator_state = NULL;
    if (orchestrator_source == source) {
        orchestrator_source = NULL;
    }
    if (orchestrator_chunk_name == chunk_name) {
        orchestrator_chunk_name = NULL;
    }
    pthread_mutex_unlock(&orchestrator_mutex);

    free(source);
    free(chunk_name);

    return NULL;
}

/*
 * Idempotent shutdown + join helper used by both start_from_db (on
 * failure) and destroy(). Sets the shutdown flag, joins the thread,
 * and frees any remaining buffers. The lua_State is destroyed by
 * the thread itself as part of its exit path.
 */
static void orchestrator_set_shutdown_and_join(void) {
    orchestrator_shutting_down = 1;
    scripting_system_shutdown = 1;

    if (orchestrator_thread_valid) {
        pthread_join(orchestrator_thread, NULL);
        orchestrator_thread_valid = false;
    }

    pthread_mutex_lock(&orchestrator_mutex);
    if (scripting_orchestrator_state) {
        H_lua_destroy_context(scripting_orchestrator_state);
        scripting_orchestrator_state = NULL;
    }
    free(orchestrator_source); orchestrator_source = NULL;
    free(orchestrator_chunk_name); orchestrator_chunk_name = NULL;
    orchestrator_shutting_down = 0;
    pthread_mutex_unlock(&orchestrator_mutex);
}

// ----------------------------------------------------------------------------
// Public: start_with_source
// ----------------------------------------------------------------------------

bool scripting_orchestrator_start_with_source(const char* source,
                                              const char* name) {
    if (!source || !name) {
        log_this(SR_SCRIPTING,
                 "scripting_orchestrator_start_with_source: NULL source or name",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    pthread_mutex_lock(&orchestrator_mutex);
    if (orchestrator_thread_valid || scripting_orchestrator_state) {
        pthread_mutex_unlock(&orchestrator_mutex);
        return true; // Idempotent: already running.
    }
    pthread_mutex_unlock(&orchestrator_mutex);

    char* source_copy = strdup(source);
    char* name_copy = strdup(name);
    if (!source_copy || !name_copy) {
        free(source_copy);
        free(name_copy);
        return false;
    }

    lua_State* L = H_lua_create_context();
    if (!L) {
        log_this(SR_SCRIPTING,
                 "scripting_orchestrator_start_with_source: H_lua_create_context failed",
                 LOG_LEVEL_ERROR, 0);
        free(source_copy);
        free(name_copy);
        return false;
    }

    pthread_mutex_lock(&orchestrator_mutex);
    if (orchestrator_thread_valid || scripting_orchestrator_state) {
        // Raced with a concurrent start: clean up and report
        // "already running" as success.
        pthread_mutex_unlock(&orchestrator_mutex);
        H_lua_destroy_context(L);
        free(source_copy);
        free(name_copy);
        return true;
    }
    orchestrator_source = source_copy;
    orchestrator_chunk_name = name_copy;
    scripting_orchestrator_state = L;
    orchestrator_shutting_down = 0;
    pthread_mutex_unlock(&orchestrator_mutex);

    if (pthread_create(&orchestrator_thread, NULL,
                        orchestrator_thread_main, NULL) != 0) {
        log_this(SR_SCRIPTING,
                 "scripting_orchestrator_start_with_source: pthread_create failed",
                 LOG_LEVEL_ERROR, 0);
        pthread_mutex_lock(&orchestrator_mutex);
        scripting_orchestrator_state = NULL;
        orchestrator_source = NULL;
        orchestrator_chunk_name = NULL;
        pthread_mutex_unlock(&orchestrator_mutex);
        H_lua_destroy_context(L);
        free(source_copy);
        free(name_copy);
        return false;
    }
    orchestrator_thread_valid = true;
    add_service_thread_with_description(&scripting_threads,
                                        orchestrator_thread,
                                        "orchestrator");
    log_this(SR_SCRIPTING, "Orchestrator: started %s",
             LOG_LEVEL_STATE, 1, name);
    return true;
}

// ----------------------------------------------------------------------------
// Public: start_from_db
// ----------------------------------------------------------------------------

/*
 * QueryRef #087 - Get Script by Group/Name (with code). The hard-
 * coded 87 here matches the QueryRef number registered in
 * acuranzo_1204.lua. Keeping the number adjacent to the SQL
 * parameter names makes the lookup obvious during code review.
 */
#define H_SCRIPTING_ORCHESTRATOR_QUERYREF 87

/*
 * Build a typed-JSON parameter object for QueryRef #087:
 * { "STRING": { "GROUP_NAME": "...", "SCRIPT_NAME": "..." } }
 * Returns a heap-allocated string. Caller frees.
 */
static char* orchestrator_build_params_json(const char* group_name,
                                            const char* script_name) {
    json_t* root = json_object();
    if (!root) {
        return NULL;
    }
    json_t* string_params = json_object();
    if (!string_params) {
        json_decref(root);
        return NULL;
    }
    if (group_name) {
        json_object_set_new(string_params, "GROUP_NAME", json_string(group_name));
    } else {
        json_object_set_new(string_params, "GROUP_NAME", json_null());
    }
    if (script_name) {
        json_object_set_new(string_params, "SCRIPT_NAME", json_string(script_name));
    } else {
        json_object_set_new(string_params, "SCRIPT_NAME", json_null());
    }
    json_object_set_new(root, "STRING", string_params);
    char* out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

/*
 * Extract the `code` column from the first row of a conduit result.
 * The result data is a JSON array-of-objects stored in
 * result_query->query_template (see database_queue_await_result).
 * Returns a heap-allocated copy of the code string, or NULL if the
 * result is empty or has no `code` string. Caller frees.
 */
static char* orchestrator_extract_code_from_result(const DatabaseQuery* result_query) {
    if (!result_query || !result_query->query_template) {
        return NULL;
    }
    json_error_t err;
    json_t* root = json_loads(result_query->query_template, 0, &err);
    if (!root) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: failed to parse query result JSON: %s",
                 LOG_LEVEL_ERROR, 1, err.text);
        return NULL;
    }
    if (!json_is_array(root)) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: query result is not an array",
                 LOG_LEVEL_ERROR, 0);
        json_decref(root);
        return NULL;
    }
    if (json_array_size(root) == 0) {
        json_decref(root);
        return NULL;
    }
    json_t* row = json_array_get(root, 0);
    if (!json_is_object(row)) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: query result row is not an object",
                 LOG_LEVEL_ERROR, 0);
        json_decref(root);
        return NULL;
    }
    json_t* code_obj = json_object_get(row, "code");
    if (!code_obj || !json_is_string(code_obj)) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: query result row has no 'code' string",
                 LOG_LEVEL_ERROR, 0);
        json_decref(root);
        return NULL;
    }
    const char* code = json_string_value(code_obj);
    char* code_copy = strdup(code ? code : "");
    json_decref(root);
    return code_copy;
}

/*
 * Fetch a script source from the `scripts` table via QueryRef #087.
 * Returns a heap-allocated copy of the `code` column, or NULL if the
 * row is missing, the query fails, or any DB infrastructure is
 * unavailable. Caller frees the returned string.
 *
 * This helper is shared by the Orchestrator load path (11f) and the
 * DB-backed `require` searcher (11g).
 */
char* scripting_fetch_script_source(const char* group_name,
                                    const char* script_name,
                                    const char* database,
                                    int timeout_seconds) {
    if (!group_name || !script_name || !database || timeout_seconds <= 0) {
        log_this(SR_SCRIPTING,
                 "scripting_fetch_script_source: invalid arguments",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(
        global_queue_manager, database);
    if (!db_queue) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: database queue not found for '%s'",
                 LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }
    QueryCacheEntry* cache_entry = lookup_query_cache_entry(
        db_queue, H_SCRIPTING_ORCHESTRATOR_QUERYREF);
    if (!cache_entry) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: QueryRef %d not found in cache for database '%s'",
                 LOG_LEVEL_ERROR, 2, H_SCRIPTING_ORCHESTRATOR_QUERYREF, database);
        return NULL;
    }

    char* params_json = orchestrator_build_params_json(group_name, script_name);
    if (!params_json) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: failed to build parameters for %s.%s",
                 LOG_LEVEL_ERROR, 2, group_name, script_name);
        return NULL;
    }

    char* query_id = generate_query_id();
    if (!query_id) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: failed to generate query ID for %s.%s",
                 LOG_LEVEL_ERROR, 2, group_name, script_name);
        free(params_json);
        return NULL;
    }

    DatabaseQuery db_query = {
        .query_id = query_id,
        .query_template = strdup(cache_entry->sql_template),
        .parameter_json = params_json,
        .queue_type_hint = database_queue_type_from_string(cache_entry->queue_type),
        .submitted_at = time(NULL),
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };
    if (!db_query.query_template) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: failed to duplicate query template for %s.%s",
                 LOG_LEVEL_ERROR, 2, group_name, script_name);
        free(query_id);
        free(params_json);
        return NULL;
    }

    bool submit_success = database_queue_submit_query(db_queue, &db_query);
    if (!submit_success) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: failed to submit query for %s.%s",
                 LOG_LEVEL_ERROR, 2, group_name, script_name);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }

    DatabaseQuery* result_query = database_queue_await_result(
        db_queue, query_id, timeout_seconds);
    if (!result_query) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: query timed out for %s.%s",
                 LOG_LEVEL_ERROR, 2, group_name, script_name);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }

    if (result_query->error_message) {
        log_this(SR_SCRIPTING,
                 "Script source fetch: query failed for %s.%s: %s",
                 LOG_LEVEL_ERROR, 3, group_name, script_name,
                 result_query->error_message);
        free(result_query->query_id);
        free(result_query->query_template);
        free(result_query->error_message);
        free(result_query);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }

    char* code = orchestrator_extract_code_from_result(result_query);
    if (!code) {
        // No row found or malformed result — not necessarily an error.
        log_this(SR_SCRIPTING,
                 "Script source fetch: no script row for %s.%s in database '%s'",
                 LOG_LEVEL_STATE, 3, group_name, script_name, database);
    }

    free(result_query->query_id);
    free(result_query->query_template);
    free(result_query->error_message);
    free(result_query);
    free(query_id);
    free(db_query.query_template);
    free(params_json);

    return code;
}

bool scripting_orchestrator_start_from_db(const char* group_name,
                                          const char* script_name) {
    if (!group_name || !script_name) {
        log_this(SR_SCRIPTING,
                 "scripting_orchestrator_start_from_db: NULL group or script name",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Idempotent: if already running, do nothing.
    pthread_mutex_lock(&orchestrator_mutex);
    bool already_running = orchestrator_thread_valid || (scripting_orchestrator_state != NULL);
    pthread_mutex_unlock(&orchestrator_mutex);
    if (already_running) {
        return true;
    }

    // DefaultDatabase names the DB holding the scripts table. If it is
    // not configured, fall back to the single configured database when
    // exactly one exists: with only one database the choice is
    // unambiguous, so "with" and "without" DefaultDatabase behave the
    // same. Only when two or more databases are configured is an
    // explicit DefaultDatabase required to disambiguate.
    if (!app_config) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: no DefaultDatabase configured; Orchestrator will not start",
                 LOG_LEVEL_STATE, 0);
        return false;
    }
    const char* database = app_config->scripting.DefaultDatabase;
    if (!database || database[0] == '\0') {
        if (app_config->databases.connection_count == 1 &&
            app_config->databases.connections[0].name &&
            app_config->databases.connections[0].name[0] != '\0') {
            database = app_config->databases.connections[0].name;
            log_this(SR_SCRIPTING,
                     "Orchestrator: no DefaultDatabase configured; using the single configured database '%s'",
                     LOG_LEVEL_STATE, 1, database);
        } else {
            log_this(SR_SCRIPTING,
                     "Orchestrator: no DefaultDatabase configured; Orchestrator will not start",
                     LOG_LEVEL_STATE, 0);
            return false;
        }
    }

    // Build the full "group.script" name for the chunk name/log context.
    size_t name_len = strlen(group_name) + 1 + strlen(script_name);
    char* full_name = malloc(name_len + 1);
    if (!full_name) {
        return false;
    }
    snprintf(full_name, name_len + 1, "%s.%s", group_name, script_name);

    char* code = scripting_fetch_script_source(group_name, script_name, database, 5);
    if (!code) {
        log_this(SR_SCRIPTING,
                 "Orchestrator: no script row for %s in database '%s'",
                 LOG_LEVEL_STATE, 2, full_name, database);
        free(full_name);
        return false;
    }

    bool started = scripting_orchestrator_start_with_source(code, full_name);
    free(code);
    free(full_name);
    return started;
}

// ----------------------------------------------------------------------------
// Public: load_configured
// ----------------------------------------------------------------------------

void scripting_orchestrator_load_configured(void) {
    // app_config is the global AppConfig (declared in hydrogen.h).
    // If it is not set (e.g. tests that don't initialize the
    // subsystem), do nothing.
    if (!app_config) {
        return;
    }
    if (!app_config->scripting.Enabled) {
        return;
    }
    const char* name = app_config->scripting.Orchestrator;
    if (!name || name[0] == '\0') {
        log_this(SR_SCRIPTING,
                 "No Orchestrator configured; subsystem will be idle until jobs arrive",
                 LOG_LEVEL_STATE, 0);
        return;
    }
    // Split on the first '.'. The config name is conventionally
    // "group.script" (e.g. "Orchestrators.Orchestrator"), matching
    // the encoding used by the 11g DB-backed require path.
    const char* dot = strchr(name, '.');
    if (!dot || dot == name || !dot[1]) {
        log_this(SR_SCRIPTING,
                 "Invalid Orchestrator name '%s' (expected 'group_name.script_name')",
                 LOG_LEVEL_ERROR, 1, name);
        return;
    }
    size_t group_len = (size_t)(dot - name);
    char* group = malloc(group_len + 1);
    if (!group) {
        return;
    }
    memcpy(group, name, group_len);
    group[group_len] = '\0';
    const char* script = dot + 1;

    bool ok = scripting_orchestrator_start_from_db(group, script);
    if (!ok) {
        log_this(SR_SCRIPTING,
                 "Orchestrator not loaded (no row, or DB unavailable); continuing without one",
                 LOG_LEVEL_STATE, 0);
    }
    free(group);
}

// ----------------------------------------------------------------------------
// Public: destroy / is_running
// ----------------------------------------------------------------------------

void scripting_orchestrator_destroy(void) {
    pthread_mutex_lock(&orchestrator_mutex);
    bool in_flight = orchestrator_thread_valid || scripting_orchestrator_state;
    pthread_mutex_unlock(&orchestrator_mutex);
    if (!in_flight) {
        return;
    }
    orchestrator_set_shutdown_and_join();
    log_this(SR_SCRIPTING, "Orchestrator: destroyed",
             LOG_LEVEL_STATE, 0);
}

bool scripting_orchestrator_is_running(void) {
    pthread_mutex_lock(&orchestrator_mutex);
    bool running = orchestrator_thread_valid || (scripting_orchestrator_state != NULL);
    pthread_mutex_unlock(&orchestrator_mutex);
    return running;
}
