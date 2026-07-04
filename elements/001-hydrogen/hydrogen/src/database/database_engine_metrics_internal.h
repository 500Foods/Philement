/*
 * Database Engine Metrics - Internal Counters
 *
 * Shared atomic counters used by database_engine.c (writes during
 * execution) and database_engine_metrics.c (reads via get_database_metrics
 * and friends). Counters are defined in database_engine_metrics.c.
 */

#ifndef DATABASE_ENGINE_METRICS_INTERNAL_H
#define DATABASE_ENGINE_METRICS_INTERNAL_H

extern volatile unsigned long long db_queries_executed_total;
extern volatile unsigned long long db_queries_successful;
extern volatile unsigned long long db_queries_failed;
extern volatile unsigned long long db_queries_prepared_executed;
extern volatile unsigned long long db_queries_direct_executed;
extern volatile unsigned long long db_bytes_sent_total;
extern volatile unsigned long long db_bytes_received_total;
extern volatile unsigned long long db_prepared_statements_cached;
extern volatile unsigned long long db_prepared_statements_evicted;
extern volatile unsigned long long db_prepared_statement_cache_hits;
extern volatile unsigned long long db_prepared_statement_cache_misses;
extern volatile unsigned long long db_connections_created;
extern volatile unsigned long long db_connections_closed;
extern volatile unsigned long long db_connection_errors;

#endif // DATABASE_ENGINE_METRICS_INTERNAL_H
