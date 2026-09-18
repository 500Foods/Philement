/*
 * Database Engine Metrics and Counts
 *
 * Global atomic counters and helpers that report per-engine database
 * counts, the list of supported engines, and the cumulative metrics
 * snapshot. Counter definitions live here; writers in
 * database_engine.c and database_engine_prepared.c declare them
 * extern via database_engine_metrics_internal.h.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "database_engine_metrics_internal.h"

// Global database metrics instance
DatabaseMetrics database_metrics;

// Counter definitions
volatile unsigned long long db_queries_executed_total = 0;
volatile unsigned long long db_queries_successful = 0;
volatile unsigned long long db_queries_failed = 0;
volatile unsigned long long db_queries_prepared_executed = 0;
volatile unsigned long long db_queries_direct_executed = 0;
volatile unsigned long long db_bytes_sent_total = 0;
volatile unsigned long long db_bytes_received_total = 0;
volatile unsigned long long db_prepared_statements_cached = 0;
volatile unsigned long long db_prepared_statements_evicted = 0;
volatile unsigned long long db_prepared_statement_cache_hits = 0;
volatile unsigned long long db_prepared_statement_cache_misses = 0;
volatile unsigned long long db_connections_created = 0;
volatile unsigned long long db_connections_closed = 0;
volatile unsigned long long db_connection_errors = 0;

void database_get_counts_by_type(int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count) {
    if (!app_config) {
        *postgres_count = 0;
        *mysql_count = 0;
        *sqlite_count = 0;
        *db2_count = 0;
        return;
    }

    const DatabaseConfig* db_config = &app_config->databases;

    *postgres_count = 0;
    *mysql_count = 0;
    *sqlite_count = 0;
    *db2_count = 0;

    for (int i = 0; i < db_config->connection_count; i++) {
        const DatabaseConnection* conn = &db_config->connections[i];
        if (conn->enabled && conn->type) {
            if (strcmp(conn->type, "postgresql") == 0 || strcmp(conn->type, "postgres") == 0) {
                (*postgres_count)++;
            } else if (strcmp(conn->type, "mysql") == 0) {
                (*mysql_count)++;
            } else if (strcmp(conn->type, "sqlite") == 0) {
                (*sqlite_count)++;
            } else if (strcmp(conn->type, "db2") == 0) {
                (*db2_count)++;
            }
        }
    }
}

void database_get_supported_engines(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

    if (!database_subsystem) {
        snprintf(buffer, buffer_size, "Database subsystem not initialized");
        return;
    }

    const char* engines = "PostgreSQL, SQLite, MySQL, DB2";
    strncpy(buffer, engines, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

void get_database_metrics(DatabaseMetrics *metrics) {
    if (!metrics) {
        return;
    }

    metrics->queries_executed_total = __sync_fetch_and_add(&db_queries_executed_total, 0);
    metrics->queries_successful = __sync_fetch_and_add(&db_queries_successful, 0);
    metrics->queries_failed = __sync_fetch_and_add(&db_queries_failed, 0);
    metrics->queries_prepared_executed = __sync_fetch_and_add(&db_queries_prepared_executed, 0);
    metrics->queries_direct_executed = __sync_fetch_and_add(&db_queries_direct_executed, 0);
    metrics->bytes_sent_total = __sync_fetch_and_add(&db_bytes_sent_total, 0);
    metrics->bytes_received_total = __sync_fetch_and_add(&db_bytes_received_total, 0);
    metrics->prepared_statements_cached = __sync_fetch_and_add(&db_prepared_statements_cached, 0);
    metrics->prepared_statements_evicted = __sync_fetch_and_add(&db_prepared_statements_evicted, 0);
    metrics->prepared_statement_cache_hits = __sync_fetch_and_add(&db_prepared_statement_cache_hits, 0);
    metrics->prepared_statement_cache_misses = __sync_fetch_and_add(&db_prepared_statement_cache_misses, 0);
    metrics->connections_created = __sync_fetch_and_add(&db_connections_created, 0);
    metrics->connections_closed = __sync_fetch_and_add(&db_connections_closed, 0);
    metrics->connection_errors = __sync_fetch_and_add(&db_connection_errors, 0);
}
