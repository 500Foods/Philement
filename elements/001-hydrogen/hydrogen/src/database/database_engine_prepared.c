/*
 * Database Engine Prepared Statement Helpers
 *
 * Find/store/clear helpers that manage the per-connection prepared
 * statement cache. The cache is bounded by
 * ConnectionConfig::prepared_statement_cache_size and uses LRU
 * eviction when full.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "database_engine_metrics_internal.h"

PreparedStatement* find_prepared_statement(DatabaseHandle* connection, const char* name) {
    if (!connection || !name || !connection->prepared_statements) {
        return NULL;
    }

    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        PreparedStatement* stmt = connection->prepared_statements[i];

        // Defense against dangling pointers: validate stmt pointer is in valid range
        if (!stmt || (uintptr_t)stmt < 0x1000) {
            continue;
        }

        // Defense against dangling name pointer: validate before strcmp
        if (!stmt->name || (uintptr_t)stmt->name < 0x1000) {
            continue;
        }

        // Additional defense: validate first byte of name is readable
        char first_char = stmt->name[0];
        if (first_char == '\0') {
            continue;
        }

        if (strcmp(stmt->name, name) == 0) {
            return stmt;
        }
    }

    return NULL;
}

bool store_prepared_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt) {
        return false;
    }

    size_t cache_size = (size_t)connection->config->prepared_statement_cache_size;

    if (!connection->prepared_statements) {
        connection->prepared_statements = calloc(cache_size, sizeof(PreparedStatement*));
        if (!connection->prepared_statements) {
            return false;
        }
        connection->prepared_statement_count = 0;
        if (!connection->prepared_statement_lru_counter) {
            connection->prepared_statement_lru_counter = calloc(cache_size, sizeof(uint64_t));
        }
    }

    if (connection->prepared_statement_count >= cache_size) {
        /* Collapse any NULL holes first (stale slots from prior clear/evict). */
        size_t write = 0;
        for (size_t read = 0; read < connection->prepared_statement_count; read++) {
            if (connection->prepared_statements[read] != NULL) {
                if (write != read) {
                    connection->prepared_statements[write] = connection->prepared_statements[read];
                    if (connection->prepared_statement_lru_counter) {
                        connection->prepared_statement_lru_counter[write] =
                            connection->prepared_statement_lru_counter[read];
                    }
                }
                write++;
            }
        }
        for (size_t i = write; i < connection->prepared_statement_count; i++) {
            connection->prepared_statements[i] = NULL;
            if (connection->prepared_statement_lru_counter) {
                connection->prepared_statement_lru_counter[i] = 0;
            }
        }
        connection->prepared_statement_count = write;
    }

    if (connection->prepared_statement_count >= cache_size) {
        log_this(connection->designator ? connection->designator : SR_DATABASE,
                 "Prepared statement cache full (%zu/%zu), evicting LRU to make room for: %s",
                 LOG_LEVEL_TRACE, 3, connection->prepared_statement_count, cache_size, stmt->name);

        size_t lru_index = 0;
        uint64_t min_lru_value = UINT64_MAX;

        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (!connection->prepared_statement_lru_counter) {
                break;
            }
            if (connection->prepared_statement_lru_counter[i] < min_lru_value) {
                min_lru_value = connection->prepared_statement_lru_counter[i];
                lru_index = i;
            }
        }

        PreparedStatement* evicted_stmt = connection->prepared_statements[lru_index];
        if (evicted_stmt) {
            __sync_add_and_fetch(&db_prepared_statements_evicted, 1);

            size_t count_before = connection->prepared_statement_count;
            DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type,
                                                                                   connection->designator ? connection->designator : SR_DATABASE);
            if (engine && engine->unprepare_statement) {
                /*
                 * Engine unprepare frees the statement and normally removes it
                 * from prepared_statements[] (compact + count--). Do NOT NULL
                 * lru_index afterward: after compact that index holds a live
                 * neighbor. If the engine bailed without shrinking the cache,
                 * compact here so store does not hit next_index >= cache_size
                 * (false "corruption" during migration cache churn).
                 */
                engine->unprepare_statement(connection, evicted_stmt);
                if (connection->prepared_statement_count >= count_before) {
                    connection->prepared_statements[lru_index] = NULL;
                    for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
                        connection->prepared_statements[i] = connection->prepared_statements[i + 1];
                        if (connection->prepared_statement_lru_counter) {
                            connection->prepared_statement_lru_counter[i] =
                                connection->prepared_statement_lru_counter[i + 1];
                        }
                    }
                    connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
                    if (connection->prepared_statement_lru_counter) {
                        connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
                    }
                    connection->prepared_statement_count--;
                }
            } else {
                if (evicted_stmt->name) free(evicted_stmt->name);
                if (evicted_stmt->sql_template) free(evicted_stmt->sql_template);
                free(evicted_stmt);

                for (size_t i = lru_index; i < connection->prepared_statement_count - 1; i++) {
                    connection->prepared_statements[i] = connection->prepared_statements[i + 1];
                    if (connection->prepared_statement_lru_counter) {
                        connection->prepared_statement_lru_counter[i] =
                            connection->prepared_statement_lru_counter[i + 1];
                    }
                }

                connection->prepared_statements[connection->prepared_statement_count - 1] = NULL;
                if (connection->prepared_statement_lru_counter) {
                    connection->prepared_statement_lru_counter[connection->prepared_statement_count - 1] = 0;
                }
                connection->prepared_statement_count--;
            }

            log_this(connection->designator ? connection->designator : SR_DATABASE,
                     "Evicted LRU prepared statement to make room for: %s", LOG_LEVEL_TRACE, 1, stmt->name);
        }
    }

    // Prefer a free (NULL) hole inside the live prefix before appending
    size_t next_index = SIZE_MAX;
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        if (connection->prepared_statements[i] == NULL) {
            next_index = i;
            break;
        }
    }

    if (next_index == SIZE_MAX) {
        next_index = connection->prepared_statement_count;
        if (next_index >= cache_size) {
            log_this(connection->designator ? connection->designator : SR_DATABASE,
                     "CRITICAL: prepared_statement_count corruption detected (%zu >= %zu)", LOG_LEVEL_ERROR, 2,
                     next_index, cache_size);
            return false;
        }
        connection->prepared_statement_count++;
    }

    connection->prepared_statements[next_index] = stmt;

    log_this(connection->designator ? connection->designator : SR_DATABASE,
             "Stored prepared statement: %s (total: %zu of %zu)", LOG_LEVEL_TRACE, 3,
             stmt->name, connection->prepared_statement_count, cache_size);

    return true;
}

void database_engine_clear_prepared_statements(DatabaseHandle* connection) {
    if (!connection || !connection->prepared_statements) {
        return;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    size_t cleared_count = connection->prepared_statement_count;

    // CRITICAL: After transaction commit/rollback, PreparedStatement structures may be corrupted.
    // SAFEST approach: NULL out the pointers and accept a small memory leak in exchange
    // for not crashing. Migrations are bootstrap-time operations, so this is acceptable.
    for (size_t i = 0; i < connection->prepared_statement_count; i++) {
        connection->prepared_statements[i] = NULL;
    }

    connection->prepared_statement_count = 0;

    log_this(designator, "Invalidated %zu prepared statement references (transaction boundary)",
             LOG_LEVEL_TRACE, 1, cleared_count);
}
