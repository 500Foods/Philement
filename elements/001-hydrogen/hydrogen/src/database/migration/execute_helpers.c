/*
 * Database Migration Execution - Helper Functions
 *
 * Utility functions for migration execution including SQL copying,
 * line counting, and execution finalization.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// System includes
#include <libgen.h>

// Local includes
#include "migration.h"

/*
 * Normalize database engine name to match Lua expectations
 * Returns the normalized engine name or NULL if unsupported
 */
const char* normalize_engine_name(const char* engine_name) {
    if (!engine_name) {
        return NULL;
    }

    if (strcmp(engine_name, "postgresql") == 0 || strcmp(engine_name, "postgres") == 0) {
        return "postgresql";
    } else if (strcmp(engine_name, "mysql") == 0) {
        return "mysql";
    } else if (strcmp(engine_name, "sqlite") == 0) {
        return "sqlite";
    } else if (strcmp(engine_name, "db2") == 0) {
        return "db2";
    }

    return NULL; // Unsupported engine
}

/*
 * Extract migration name from configuration
 * For PAYLOAD: prefix, returns the part after prefix
 * For path-based, returns basename of the path
 * Returns NULL on error
 */
const char* extract_migration_name(const char* migrations_config, char** path_copy_out) {
    if (!migrations_config) {
        return NULL;
    }

    if (strncmp(migrations_config, "PAYLOAD:", 8) == 0) {
        *path_copy_out = NULL;
        return migrations_config + 8;
    } else {
        // For path-based migrations, extract the basename
        *path_copy_out = strdup(migrations_config);
        if (*path_copy_out) {
            const char* migration_name = basename(*path_copy_out);
            return migration_name;
        }
        return NULL;
    }
}

/*
 * Free payload files array
 * CRITICAL: get_payload_files_by_prefix() allocates NEW copies with strdup/malloc
 * These ARE owned allocations that MUST be freed to prevent memory leaks
 */
void free_payload_files(PayloadFile* payload_files, size_t payload_count) {
    if (payload_files) {
        // Free each file's allocated name and data
        for (size_t j = 0; j < payload_count; j++) {
            if (payload_files[j].name) {
                free(payload_files[j].name);
            }
            if (payload_files[j].data) {
                free(payload_files[j].data);
            }
        }
        // Free the array itself
        free(payload_files);
    }
}

/*
 * Helper: Copy SQL result from Lua's internal buffer to C-owned memory
 * Returns allocated string that must be freed by caller, or NULL on failure
 */
char* copy_sql_from_lua(const char* sql_result, size_t sql_length, const char* dqm_label) {
    if (!sql_result || sql_length == 0) {
        return NULL;
    }

    char* sql_copy = malloc(sql_length + 1);
    if (!sql_copy) {
        log_this(dqm_label, "Failed to allocate memory for SQL copy", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    memcpy(sql_copy, sql_result, sql_length);
    sql_copy[sql_length] = '\0';
    return sql_copy;
}

/*
 * Helper: Count lines in SQL string by counting newlines
 * Returns line count (minimum 1 if sql is not NULL)
 */
int count_sql_lines(const char* sql, size_t sql_length) {
    if (!sql || sql_length == 0) {
        return 0;
    }

    int line_count = 1;  // At least 1 line if there's content
    for (size_t i = 0; i < sql_length; i++) {
        if (sql[i] == '\n') {
            line_count++;
        }
    }
    return line_count;
}

/*
 * Helper: Execute SQL transaction against database
 * Returns true if transaction executed successfully
 */
bool execute_migration_sql(DatabaseHandle* connection, const char* sql_copy, size_t sql_length,
                           const char* migration_file, const char* dqm_label) {
    if (!sql_copy || sql_length == 0) {
        log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_file);
        return false;
    }

    return execute_transaction(connection, sql_copy, sql_length,
                             migration_file, connection->engine_type, dqm_label);
}

/*
 * Helper: Execute copied SQL and perform cleanup
 * Takes already-copied SQL and executes it, managing memory appropriately
 * Returns true if SQL was successfully executed
 */
bool execute_copied_sql_and_cleanup(DatabaseHandle* connection, const char* migration_file,
                                    char* sql_copy, size_t sql_length,
                                    const char* dqm_label) {
    // Execute the generated SQL against the database as a transaction
    bool success = execute_migration_sql(connection, sql_copy, sql_length, migration_file, dqm_label);

    // Free the SQL copy
    free(sql_copy);

    return success;
}

/*
 * Helper: Finalize migration execution after SQL generation
 * Handles SQL copying, cleanup, and execution
 * Returns true if SQL was successfully executed
 */
bool finalize_migration_execution(DatabaseHandle* connection, const char* migration_file,
                                  const char* sql_result, size_t sql_length, int query_count,
                                  lua_State* L, PayloadFile* payload_files, size_t payload_count,
                                  const char* dqm_label) {
    // CRITICAL: Copy SQL string from Lua's internal buffer to C-owned memory
    char* sql_copy = copy_sql_from_lua(sql_result, sql_length, dqm_label);
    if (sql_result && sql_length > 0 && !sql_copy) {
        // Memory allocation failed
        free_payload_files(payload_files, payload_count);
        lua_close(L);
        return false;
    }

    // Count lines in SQL and log execution summary
    int line_count = count_sql_lines(sql_copy, sql_length);
    lua_log_execution_summary(migration_file, sql_length, line_count, query_count, dqm_label);

    // CRITICAL: Clean up Lua state BEFORE freeing payload files AND before using SQL
    // Lua holds internal references to payload data (bytecode from luaL_loadbuffer)
    // We've copied the SQL string, so safe to close Lua now
    lua_cleanup(L);

    // Free payload files after Lua cleanup
    free_payload_files(payload_files, payload_count);

    // Execute and cleanup using helper
    return execute_copied_sql_and_cleanup(connection, migration_file, sql_copy, sql_length, dqm_label);
}