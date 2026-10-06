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
    } else if (strcmp(engine_name, "mariadb") == 0) {
        return "mariadb";
    } else if (strcmp(engine_name, "sqlite") == 0) {
        return "sqlite";
    } else if (strcmp(engine_name, "db2") == 0) {
        return "db2";
    } else if (strcmp(engine_name, "firebird") == 0) {
        return "firebird";
    } else if (strcmp(engine_name, "mssql") == 0 || strcmp(engine_name, "sqlserver") == 0) {
        return "mssql";
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
 * Free a design-name list from migration_payload_designs.
 */
void migration_payload_designs_free(char** designs, size_t count) {
    size_t i;

    if (!designs) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(designs[i]);
    }
    free(designs);
}

bool migration_file_design(const char* migration_file, char* design_out, size_t design_len) {
    const char* slash;
    size_t len;

    if (!migration_file || !design_out || design_len < 2) {
        return false;
    }
    slash = strchr(migration_file, '/');
    if (!slash || slash == migration_file) {
        return false;
    }
    len = (size_t)(slash - migration_file);
    if (len == 0 || len > 32 || len >= design_len) {
        return false;
    }
    memcpy(design_out, migration_file, len);
    design_out[len] = '\0';
    return true;
}

bool migration_payload_designs(const char* migrations_config, char*** designs_out,
                              size_t* count_out, const char* dqm_label) {
    const char* spec;
    const char* start;
    char** designs;
    size_t pluses;
    size_t capacity;
    size_t filled;
    bool has_acuranzo;
    bool needs_acuranzo;
    bool ok;
    const char* scan;

    if (designs_out) {
        *designs_out = NULL;
    }
    if (count_out) {
        *count_out = 0;
    }
    if (!migrations_config || !designs_out || !count_out ||
        strncmp(migrations_config, "PAYLOAD:", 8) != 0) {
        log_this(dqm_label, "Invalid PAYLOAD migration format", LOG_LEVEL_ERROR, 0);
        return false;
    }

    spec = migrations_config + 8;
    if (spec[0] == '\0') {
        log_this(dqm_label, "Invalid PAYLOAD migration format", LOG_LEVEL_ERROR, 0);
        return false;
    }

    pluses = 0;
    for (scan = spec; *scan; scan++) {
        if (*scan == '+') {
            pluses++;
        }
    }
    if (pluses > 7) {
        log_this(dqm_label, "Too many designs in Migrations list", LOG_LEVEL_ERROR, 0);
        return false;
    }

    capacity = pluses + 1;
    designs = calloc(capacity, sizeof(char*));
    if (!designs) {
        log_this(dqm_label, "Memory allocation failed for migration designs", LOG_LEVEL_ERROR, 0);
        return false;
    }

    filled = 0;
    has_acuranzo = false;
    needs_acuranzo = false;
    ok = true;
    start = spec;
    while (ok) {
        const char* plus = strchr(start, '+');
        size_t len = plus ? (size_t)(plus - start) : strlen(start);
        char* name;
        size_t i;

        if (len == 0 || len > 32) {
            log_this(dqm_label, "Invalid migration design name", LOG_LEVEL_ERROR, 0);
            ok = false;
            break;
        }
        for (i = 0; i < len; i++) {
            char c = start[i];
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
                log_this(dqm_label, "Invalid migration design name", LOG_LEVEL_ERROR, 0);
                ok = false;
                break;
            }
        }
        if (!ok) {
            break;
        }

        name = malloc(len + 1);
        if (!name) {
            log_this(dqm_label, "Memory allocation failed for migration designs", LOG_LEVEL_ERROR, 0);
            ok = false;
            break;
        }
        memcpy(name, start, len);
        name[len] = '\0';

        for (i = 0; i < filled; i++) {
            if (strcmp(designs[i], name) == 0) {
                log_this(dqm_label, "Migrations list repeats design %s", LOG_LEVEL_ERROR, 1, name);
                free(name);
                ok = false;
                break;
            }
        }
        if (!ok) {
            break;
        }

        if (strcmp(name, "acuranzo") == 0) {
            has_acuranzo = true;
        }
        if (strcmp(name, "argent") == 0 || strcmp(name, "gaius") == 0) {
            needs_acuranzo = true;
        }
        designs[filled++] = name;
        if (!plus) {
            break;
        }
        start = plus + 1;
    }

    if (ok && needs_acuranzo && !has_acuranzo) {
        log_this(dqm_label, "Migrations list names argent or gaius without acuranzo", LOG_LEVEL_ERROR, 0);
        ok = false;
    }
    if (!ok) {
        migration_payload_designs_free(designs, filled);
        return false;
    }

    *designs_out = designs;
    *count_out = filled;
    return true;
}

bool migration_design_span(const char* design, char** first_file_out, size_t* first_size_out,
                          long long* highest_out, const char* dqm_label) {
    PayloadFile* files = NULL;
    size_t num_files = 0;
    size_t capacity_files = 0;
    char* first_file = NULL;
    size_t first_size = 0;
    unsigned long lowest_number = 0;
    bool have_lowest = false;
    long long highest = -1;
    char expected_prefix[256];
    size_t i;

    if (first_file_out) {
        *first_file_out = NULL;
    }
    if (first_size_out) {
        *first_size_out = 0;
    }
    if (highest_out) {
        *highest_out = -1;
    }
    if (!design || !first_file_out || !first_size_out || !highest_out) {
        return false;
    }

    if (!get_payload_files_by_prefix(design, &files, &num_files, &capacity_files)) {
        log_this(dqm_label, "Failed to access payload files for migration validation", LOG_LEVEL_ERROR, 0);
        return false;
    }

    snprintf(expected_prefix, sizeof(expected_prefix), "%s/%s_", design, design);
    for (i = 0; i < num_files; i++) {
        const char* number_start;
        const char* lua_ext;
        size_t number_len;
        char number_str[8];
        unsigned long file_number;
        char* endptr;

        if (!files[i].name || strncmp(files[i].name, expected_prefix, strlen(expected_prefix)) != 0) {
            continue;
        }
        number_start = files[i].name + strlen(expected_prefix);
        lua_ext = strstr(number_start, ".lua");
        if (!lua_ext) {
            continue;
        }
        number_len = (size_t)(lua_ext - number_start);
        if (number_len < 1 || number_len > 6) {
            continue;
        }
        memcpy(number_str, number_start, number_len);
        number_str[number_len] = '\0';
        file_number = strtoul(number_str, &endptr, 10);
        if (endptr == number_str || *endptr != '\0') {
            continue;
        }
        if (!have_lowest || file_number < lowest_number) {
            char* copy = strdup(files[i].name);
            if (!copy) {
                free(first_file);
                free_payload_files(files, num_files);
                log_this(dqm_label, "Memory allocation failed for migration file name", LOG_LEVEL_ERROR, 0);
                return false;
            }
            free(first_file);
            first_file = copy;
            first_size = files[i].size;
            lowest_number = file_number;
            have_lowest = true;
        }
        if ((long long)file_number > highest) {
            highest = (long long)file_number;
        }
    }

    free_payload_files(files, num_files);
    *first_file_out = first_file;
    *first_size_out = first_size;
    *highest_out = highest;
    return true;
}

bool payload_files_for_designs(char** designs, size_t design_count, PayloadFile** files_out,
                              size_t* count_out, const char* dqm_label) {
    size_t d;

    if (files_out) {
        *files_out = NULL;
    }
    if (count_out) {
        *count_out = 0;
    }
    if (!designs || design_count == 0 || !files_out || !count_out) {
        return false;
    }

    for (d = 0; d < design_count; d++) {
        PayloadFile* part = NULL;
        size_t part_count = 0;
        size_t part_capacity = 0;
        PayloadFile* grown;

        if (!get_payload_files_by_prefix(designs[d], &part, &part_count, &part_capacity) || part_count == 0) {
            log_this(dqm_label, "No payload files for design %s", LOG_LEVEL_ERROR, 1, designs[d]);
            free_payload_files(part, part_count);
            free_payload_files(*files_out, *count_out);
            *files_out = NULL;
            *count_out = 0;
            return false;
        }

        grown = realloc(*files_out, (*count_out + part_count) * sizeof(PayloadFile));
        if (!grown) {
            log_this(dqm_label, "Memory allocation failed for payload files", LOG_LEVEL_ERROR, 0);
            free_payload_files(part, part_count);
            free_payload_files(*files_out, *count_out);
            *files_out = NULL;
            *count_out = 0;
            return false;
        }
        memcpy(grown + *count_out, part, part_count * sizeof(PayloadFile));
        free(part);
        *files_out = grown;
        *count_out += part_count;
    }

    return true;
}

int migration_ref_band(long long ref) {
    if (ref < 0) {
        return -1;
    }
    return (int)(ref / 1000);
}

void migration_ranges_reset(DatabaseQueue* db_queue) {
    if (!db_queue) {
        return;
    }
    memset(db_queue->migration_ranges, 0, sizeof(db_queue->migration_ranges));
    db_queue->migration_range_count = 0;
}

void migration_ranges_clear_progress(DatabaseQueue* db_queue) {
    size_t i;

    if (!db_queue) {
        return;
    }
    for (i = 0; i < db_queue->migration_range_count; i++) {
        db_queue->migration_ranges[i].loaded = 0;
        db_queue->migration_ranges[i].applied = 0;
    }
}

const MigrationRange* migration_range_for_ref(const DatabaseQueue* db_queue, long long query_ref) {
    int band;
    size_t i;

    if (!db_queue) {
        return NULL;
    }
    band = migration_ref_band(query_ref);
    for (i = 0; i < db_queue->migration_range_count; i++) {
        if (db_queue->migration_ranges[i].band == band) {
            return &db_queue->migration_ranges[i];
        }
    }
    return NULL;
}

void migration_ranges_note_query(DatabaseQueue* db_queue, long long query_ref, long long query_type) {
    int band;
    size_t i;

    if (!db_queue || query_ref <= 0) {
        return;
    }
    band = migration_ref_band(query_ref);
    for (i = 0; i < db_queue->migration_range_count; i++) {
        MigrationRange* range = &db_queue->migration_ranges[i];

        if (range->band != band) {
            continue;
        }
        if (query_type == 1000 && query_ref > range->loaded) {
            range->loaded = query_ref;
        } else if (query_type == 1003 && query_ref > range->applied) {
            range->applied = query_ref;
        }
        return;
    }
}

void migration_ranges_sync_globals(DatabaseQueue* db_queue) {
    size_t i;
    long long available = 0;
    long long loaded = 0;
    long long applied = 0;

    if (!db_queue) {
        return;
    }
    for (i = 0; i < db_queue->migration_range_count; i++) {
        const MigrationRange* range = &db_queue->migration_ranges[i];

        if (range->available > available) {
            available = range->available;
        }
        if (range->loaded > loaded) {
            loaded = range->loaded;
        }
        if (range->applied > applied) {
            applied = range->applied;
        }
    }
    db_queue->latest_available_migration = available;
    db_queue->latest_loaded_migration = loaded;
    db_queue->latest_applied_migration = applied;
}

long long migration_range_skip_through(const DatabaseQueue* db_queue, long long query_ref) {
    const MigrationRange* range;
    long long floor;

    range = migration_range_for_ref(db_queue, query_ref);
    if (!range) {
        return -1;
    }
    floor = range->loaded;
    if (floor == 0 && range->applied > 0) {
        floor = range->applied;
    }
    if (range->applied > floor) {
        floor = range->applied;
    }
    return floor;
}

MigrationAction migration_ranges_action(const DatabaseQueue* db_queue) {
    bool need_load = false;
    bool need_apply = false;
    size_t i;

    if (!db_queue) {
        return MIGRATION_ACTION_NONE;
    }
    for (i = 0; i < db_queue->migration_range_count; i++) {
        const MigrationRange* range = &db_queue->migration_ranges[i];
        long long loaded = range->loaded;
        long long applied = range->applied;
        long long available = range->available;

        if (loaded == 0 && applied > 0) {
            loaded = applied;
        }
        if (available == applied) {
            continue;
        }
        if (available > 0 && loaded < available) {
            need_load = true;
        } else if (loaded > applied) {
            need_apply = true;
        }
    }
    if (need_load) {
        return MIGRATION_ACTION_LOAD;
    }
    if (need_apply) {
        return MIGRATION_ACTION_APPLY;
    }
    return MIGRATION_ACTION_NONE;
}

void migration_ranges_log(const DatabaseQueue* db_queue, const char* dqm_label, const char* verb, int level) {
    size_t i;

    if (!db_queue || !verb) {
        return;
    }
    for (i = 0; i < db_queue->migration_range_count; i++) {
        const MigrationRange* range = &db_queue->migration_ranges[i];
        long long loaded = range->loaded;
        long long lo = (long long)range->band * 1000;
        long long hi = lo + 999;
        const char* design = range->design[0] ? range->design : "range";

        if (range->applied > loaded) {
            loaded = range->applied;
        }
        log_this(dqm_label, "Migration %s: %s %lld-%lld AVAIL = %lld, LOAD = %lld, APPLY = %lld",
                 level, 7, verb, design, lo, hi, range->available, loaded, range->applied);
    }
}

bool migration_ranges_load_from_payload(DatabaseQueue* db_queue, const char* migrations_config,
                                        const char* dqm_label) {
    MigrationRange previous[MIGRATION_RANGE_CAP];
    size_t previous_count;
    char** designs = NULL;
    size_t design_count = 0;
    size_t d;
    size_t a;
    size_t b;
    bool ok = true;

    if (!db_queue || !migrations_config) {
        return false;
    }

    previous_count = db_queue->migration_range_count;
    if (previous_count > MIGRATION_RANGE_CAP) {
        previous_count = MIGRATION_RANGE_CAP;
    }
    memcpy(previous, db_queue->migration_ranges, previous_count * sizeof(MigrationRange));
    migration_ranges_reset(db_queue);

    if (!migration_payload_designs(migrations_config, &designs, &design_count, dqm_label)) {
        return false;
    }

    for (d = 0; ok && d < design_count; d++) {
        PayloadFile* files = NULL;
        size_t num_files = 0;
        size_t capacity_files = 0;
        char expected_prefix[256];
        size_t i;
        bool saw_number = false;

        if (!get_payload_files_by_prefix(designs[d], &files, &num_files, &capacity_files)) {
            log_this(dqm_label, "Failed to access payload files for migration ranges", LOG_LEVEL_ERROR, 0);
            ok = false;
            break;
        }
        snprintf(expected_prefix, sizeof(expected_prefix), "%s/%s_", designs[d], designs[d]);
        for (i = 0; i < num_files; i++) {
            const char* number_start;
            const char* lua_ext;
            size_t number_len;
            char number_str[8];
            char* endptr;
            unsigned long file_number;
            int band;
            size_t r;
            MigrationRange* range;

            if (!files[i].name || strncmp(files[i].name, expected_prefix, strlen(expected_prefix)) != 0) {
                continue;
            }
            number_start = files[i].name + strlen(expected_prefix);
            lua_ext = strstr(number_start, ".lua");
            if (!lua_ext) {
                continue;
            }
            number_len = (size_t)(lua_ext - number_start);
            if (number_len < 1 || number_len > 6) {
                continue;
            }
            memcpy(number_str, number_start, number_len);
            number_str[number_len] = '\0';
            file_number = strtoul(number_str, &endptr, 10);
            if (endptr == number_str || *endptr != '\0') {
                continue;
            }
            band = migration_ref_band((long long)file_number);
            range = NULL;
            for (r = 0; r < db_queue->migration_range_count; r++) {
                if (db_queue->migration_ranges[r].band == band) {
                    range = &db_queue->migration_ranges[r];
                    break;
                }
            }
            if (!range) {
                if (db_queue->migration_range_count >= MIGRATION_RANGE_CAP) {
                    log_this(dqm_label, "Too many migration ranges in payload", LOG_LEVEL_ERROR, 0);
                    ok = false;
                    break;
                }
                range = &db_queue->migration_ranges[db_queue->migration_range_count];
                memset(range, 0, sizeof(*range));
                range->band = band;
                snprintf(range->design, sizeof(range->design), "%s", designs[d]);
                db_queue->migration_range_count++;
            }
            if ((long long)file_number > range->available) {
                range->available = (long long)file_number;
            }
            saw_number = true;
        }
        free_payload_files(files, num_files);
        if (ok && !saw_number) {
            log_this(dqm_label, "No numbered migration files for design %s", LOG_LEVEL_ERROR, 1, designs[d]);
            ok = false;
        }
    }
    migration_payload_designs_free(designs, design_count);

    if (!ok || db_queue->migration_range_count == 0) {
        migration_ranges_reset(db_queue);
        return false;
    }

    for (a = 0; a < db_queue->migration_range_count; a++) {
        size_t p;

        for (p = 0; p < previous_count; p++) {
            if (previous[p].band == db_queue->migration_ranges[a].band) {
                db_queue->migration_ranges[a].loaded = previous[p].loaded;
                db_queue->migration_ranges[a].applied = previous[p].applied;
                break;
            }
        }
    }

    for (a = 0; a + 1 < db_queue->migration_range_count; a++) {
        for (b = 0; b + 1 < db_queue->migration_range_count - a; b++) {
            if (db_queue->migration_ranges[b].band > db_queue->migration_ranges[b + 1].band) {
                MigrationRange tmp = db_queue->migration_ranges[b];
                db_queue->migration_ranges[b] = db_queue->migration_ranges[b + 1];
                db_queue->migration_ranges[b + 1] = tmp;
            }
        }
    }

    migration_ranges_sync_globals(db_queue);
    return true;
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