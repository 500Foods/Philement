/*
 * Query Result Cache
 *
 * Indefinite, server-lifetime cache for database query results.  A query is
 * cached only when its queue type hint is DB_QUEUE_CACHE (Lookup 58 value 3)
 * and the result is successful.  Cache keys are built from:
 *
 *     database_name:sql_template_hash:param_hash
 *
 * where sql_template_hash is SHA256 of the SQL template and param_hash is
 * SHA256 of the normalized parameter JSON.  This design is transparent to all
 * existing callers and database-agnostic.
 */

#ifndef QUERY_RESULT_CACHE_H
#define QUERY_RESULT_CACHE_H

// System includes
#include <stddef.h>
#include <time.h>

// Third-party includes
#include <jansson.h>

// Forward declaration of opaque cache type
typedef struct QueryResultCache QueryResultCache;

/*
 * Create a new query-result cache.
 *
 * @param bucket_count  Number of hash buckets (a power of two is recommended).
 * @param label         Optional label for logging; may be NULL.
 * @return              New cache instance, or NULL on allocation failure.
 */
QueryResultCache* query_result_cache_create(size_t bucket_count, const char* label);

/*
 * Destroy a cache and free all entries.
 *
 * @param cache  Cache instance to destroy; may be NULL.
 */
void query_result_cache_destroy(QueryResultCache* cache);

/*
 * Retrieve a cached result.
 *
 * On success, *out_data receives a deep copy of the cached JSON data (caller
 * must json_decref it).  Other output parameters are optional and may be NULL.
 *
 * @param cache                  Cache to query.
 * @param database_name          Database name (may be NULL).
 * @param sql_template           SQL template used for the query.
 * @param params_json            Parameter JSON string (may be NULL).
 * @param out_data               Output: deep-copied JSON data.
 * @param out_row_count          Output: row count.
 * @param out_column_count       Output: column count.
 * @param out_affected_rows      Output: affected rows.
 * @param out_execution_time_ms  Output: execution time in milliseconds.
 * @return                       true if a cached entry was found and returned.
 */
bool query_result_cache_get(QueryResultCache* cache,
                            const char* database_name,
                            const char* sql_template,
                            const char* params_json,
                            json_t** out_data,
                            size_t* out_row_count,
                            size_t* out_column_count,
                            int* out_affected_rows,
                            time_t* out_execution_time_ms);

/*
 * Store a successful query result in the cache.
 *
 * The cache parses data_json into a Jansson value and retains it.  If the
 * JSON cannot be parsed, the put fails and the cache is not modified.
 *
 * @param cache                Cache to update.
 * @param database_name        Database name (may be NULL).
 * @param sql_template         SQL template used for the query.
 * @param params_json          Parameter JSON string (may be NULL).
 * @param data_json            JSON result string produced by the database engine.
 * @param row_count            Result row count.
 * @param column_count         Result column count.
 * @param affected_rows        Affected row count.
 * @param execution_time_ms    Execution time in milliseconds.
 * @return                     true if the entry was stored successfully.
 */
bool query_result_cache_put(QueryResultCache* cache,
                            const char* database_name,
                            const char* sql_template,
                            const char* params_json,
                            const char* data_json,
                            size_t row_count,
                            size_t column_count,
                            int affected_rows,
                            time_t execution_time_ms);

/*
 * Remove all entries from a cache.
 *
 * @param cache  Cache to clear; may be NULL.
 */
void query_result_cache_clear(QueryResultCache* cache);

/*
 * Return the number of entries currently in the cache.
 *
 * @param cache  Cache to inspect.
 * @return       Number of entries.
 */
size_t query_result_cache_entry_count(QueryResultCache* cache);

/*
 * Global query-result cache instance.
 *
 * Initialized on first use and destroyed with database_queue_system_destroy().
 */
QueryResultCache* query_result_cache_get_global(void);

/*
 * Initialize the global query-result cache.
 *
 * Safe to call multiple times; subsequent calls are no-ops.
 */
bool query_result_cache_global_init(void);

/*
 * Destroy the global query-result cache.
 */
void query_result_cache_global_destroy(void);

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 * -------------------------------------------------------------------------- */
unsigned long query_result_cache_hash_string(const char* str);
int query_result_cache_compare_string_pointers(const void* a, const void* b);
json_t* query_result_cache_normalize_json(const json_t* value);
char* query_result_cache_compute_template_hash(const char* sql_template);
char* query_result_cache_compute_param_hash(const char* params_json);
char* query_result_cache_build_key(const char* database_name, const char* template_hash, const char* param_hash);
size_t query_result_cache_bucket_index(const QueryResultCache* cache, const char* key);

#endif /* QUERY_RESULT_CACHE_H */
