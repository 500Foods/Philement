/*
 * Query Result Cache Implementation
 *
 * Indefinite, server-lifetime cache for successful database query results.
 * Keys are database_name:sql_template_hash:param_hash, where sql_template_hash
 * is SHA256 of the SQL template and param_hash is SHA256 of normalized
 * parameter JSON.  This layer is transparent to all existing callers.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/dbqueue/query_result_cache.h>
#include <src/utils/utils_crypto.h>

// System includes
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/*
 * Internal cache entry.  Stores the parsed JSON result and metadata from a
 * successful query execution.
 */
typedef struct QueryResultCacheEntry {
    char* key;                 // Composite key: database:template_hash:param_hash
    json_t* data;              // Parsed JSON result data
    size_t row_count;          // QueryResult row_count
    size_t column_count;       // QueryResult column_count
    int affected_rows;         // QueryResult affected_rows
    time_t execution_time_ms;  // QueryResult execution_time_ms
    struct QueryResultCacheEntry* next;
} QueryResultCacheEntry;

/*
 * Cache structure: a simple bucket hash table protected by a mutex.
 */
struct QueryResultCache {
    pthread_mutex_t lock;
    QueryResultCacheEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
    char* label;
};

// Global singleton cache
static QueryResultCache* g_query_result_cache = NULL;
static pthread_mutex_t g_query_result_cache_init_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * djb2 hash of a NUL-terminated string.
 */
 unsigned long query_result_cache_hash_string(const char* str) {
    unsigned long hash = 5381;
    int c;

    while ((c = (unsigned char)*str++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }

    return hash;
}

/*
 * Compare two string pointers for qsort.
 */
 int query_result_cache_compare_string_pointers(const void* a, const void* b) {
    const char* sa = *(const char* const*)a;
    const char* sb = *(const char* const*)b;
    return strcmp(sa, sb);
}

/*
 * Recursively normalize a JSON value so that equivalent values serialize to
 * the same string.  Object keys are sorted alphabetically; array order is
 * preserved.  Primitives are deep-copied.
 */
 json_t* query_result_cache_normalize_json(const json_t* value) {
    if (!value) {
        return NULL;
    }

    if (json_is_object(value)) {
        size_t count = json_object_size(value);
        const char** keys = calloc(count, sizeof(char*));
        if (!keys) {
            return NULL;
        }

        size_t idx = 0;
        const char* key = NULL;
        json_t* v = NULL;
        json_object_foreach((json_t*)value, key, v) {
            keys[idx++] = key;
        }

        qsort(keys, count, sizeof(char*), query_result_cache_compare_string_pointers);

        json_t* out = json_object();
        if (!out) {
            free(keys);
            return NULL;
        }

        for (idx = 0; idx < count; idx++) {
            json_t* normalized_value = query_result_cache_normalize_json(json_object_get(value, keys[idx]));
            if (!normalized_value) {
                json_decref(out);
                free(keys);
                return NULL;
            }
            json_object_set_new(out, keys[idx], normalized_value);
        }

        free(keys);
        return out;
    }

    if (json_is_array(value)) {
        json_t* out = json_array();
        if (!out) {
            return NULL;
        }

        size_t idx = 0;
        json_t* v = NULL;
        json_array_foreach((json_t*)value, idx, v) {
            json_t* normalized_value = query_result_cache_normalize_json(v);
            if (!normalized_value) {
                json_decref(out);
                return NULL;
            }
            json_array_append_new(out, normalized_value);
        }

        return out;
    }

    return json_deep_copy(value);
}

/*
 * Compute the SHA256 base64url hash of a SQL template.
 */
 char* query_result_cache_compute_template_hash(const char* sql_template) {
    const char* input = sql_template ? sql_template : "";
    return utils_sha256_hash((const unsigned char*)input, strlen(input));
}

/*
 * Compute the SHA256 base64url hash of normalized parameter JSON.
 */
 char* query_result_cache_compute_param_hash(const char* params_json) {
    const char* input = params_json ? params_json : "";

    json_error_t error;
    json_t* parsed = json_loads(input, 0, &error);
    if (!parsed) {
        // Fallback: hash the raw string if it is not valid JSON.
        return utils_sha256_hash((const unsigned char*)input, strlen(input));
    }

    json_t* normalized = query_result_cache_normalize_json(parsed);
    json_decref(parsed);
    if (!normalized) {
        return NULL;
    }

    char* compact = json_dumps(normalized, JSON_COMPACT);
    json_decref(normalized);
    if (!compact) {
        return NULL;
    }

    char* hash = utils_sha256_hash((const unsigned char*)compact, strlen(compact));
    free(compact);
    return hash;
}

/*
 * Build the composite cache key from database name, template hash, and
 * parameter hash.  Returns an allocated string or NULL on failure.
 */
 char* query_result_cache_build_key(const char* database_name, const char* template_hash, const char* param_hash) {
    const char* db_safe = database_name ? database_name : "";
    const char* template_hash_safe = template_hash ? template_hash : "";
    const char* param_hash_safe = param_hash ? param_hash : "";

    int len = snprintf(NULL, 0, "%s:%s:%s", db_safe, template_hash_safe, param_hash_safe);
    if (len < 0) {
        return NULL;
    }

    char* key = malloc((size_t)len + 1);
    if (!key) {
        return NULL;
    }

    snprintf(key, (size_t)len + 1, "%s:%s:%s", db_safe, template_hash_safe, param_hash_safe);
    return key;
}

/*
 * Compute the bucket index for a given key string.
 */
 size_t query_result_cache_bucket_index(const QueryResultCache* cache, const char* key) {
    return (size_t)(query_result_cache_hash_string(key) % (unsigned long)cache->bucket_count);
}

QueryResultCache* query_result_cache_create(size_t bucket_count, const char* label) {
    if (bucket_count == 0) {
        bucket_count = 64;
    }

    QueryResultCache* cache = calloc(1, sizeof(QueryResultCache));
    if (!cache) {
        return NULL;
    }

    cache->buckets = calloc(bucket_count, sizeof(QueryResultCacheEntry*));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }

    cache->bucket_count = bucket_count;
    pthread_mutex_init(&cache->lock, NULL);

    if (label && *label) {
        cache->label = strdup(label);
    }

    return cache;
}

void query_result_cache_destroy(QueryResultCache* cache) {
    if (!cache) {
        return;
    }

    query_result_cache_clear(cache);
    free(cache->buckets);
    pthread_mutex_destroy(&cache->lock);
    free(cache->label);
    free(cache);
}

bool query_result_cache_get(QueryResultCache* cache,
                            const char* database_name,
                            const char* sql_template,
                            const char* params_json,
                            json_t** out_data,
                            size_t* out_row_count,
                            size_t* out_column_count,
                            int* out_affected_rows,
                            time_t* out_execution_time_ms) {
    if (!cache || !sql_template || !out_data) {
        return false;
    }

    *out_data = NULL;

    char* template_hash = query_result_cache_compute_template_hash(sql_template);
    if (!template_hash) {
        return false;
    }

    char* param_hash = query_result_cache_compute_param_hash(params_json);
    if (!param_hash) {
        free(template_hash);
        return false;
    }

    char* key = query_result_cache_build_key(database_name, template_hash, param_hash);
    free(template_hash);
    free(param_hash);
    if (!key) {
        return false;
    }

    size_t bucket = query_result_cache_bucket_index(cache, key);
    bool found = false;

    pthread_mutex_lock(&cache->lock);

    QueryResultCacheEntry* entry = cache->buckets[bucket];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            *out_data = json_deep_copy(entry->data);
            if (out_row_count) {
                *out_row_count = entry->row_count;
            }
            if (out_column_count) {
                *out_column_count = entry->column_count;
            }
            if (out_affected_rows) {
                *out_affected_rows = entry->affected_rows;
            }
            if (out_execution_time_ms) {
                *out_execution_time_ms = entry->execution_time_ms;
            }
            found = (*out_data != NULL);
            break;
        }
        entry = entry->next;
    }

    pthread_mutex_unlock(&cache->lock);
    free(key);

    return found;
}

bool query_result_cache_put(QueryResultCache* cache,
                            const char* database_name,
                            const char* sql_template,
                            const char* params_json,
                            const char* data_json,
                            size_t row_count,
                            size_t column_count,
                            int affected_rows,
                            time_t execution_time_ms) {
    if (!cache || !sql_template || !data_json) {
        return false;
    }

    char* template_hash = query_result_cache_compute_template_hash(sql_template);
    if (!template_hash) {
        return false;
    }

    char* param_hash = query_result_cache_compute_param_hash(params_json);
    if (!param_hash) {
        free(template_hash);
        return false;
    }

    char* key = query_result_cache_build_key(database_name, template_hash, param_hash);
    free(template_hash);
    free(param_hash);
    if (!key) {
        return false;
    }

    json_error_t error;
    json_t* data = json_loads(data_json, 0, &error);
    if (!data) {
        free(key);
        return false;
    }

    size_t bucket = query_result_cache_bucket_index(cache, key);

    pthread_mutex_lock(&cache->lock);

    QueryResultCacheEntry* entry = cache->buckets[bucket];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            json_decref(entry->data);
            entry->data = data;
            entry->row_count = row_count;
            entry->column_count = column_count;
            entry->affected_rows = affected_rows;
            entry->execution_time_ms = execution_time_ms;
            free(key);
            pthread_mutex_unlock(&cache->lock);
            return true;
        }
        entry = entry->next;
    }

    QueryResultCacheEntry* new_entry = calloc(1, sizeof(QueryResultCacheEntry));
    if (!new_entry) {
        json_decref(data);
        free(key);
        pthread_mutex_unlock(&cache->lock);
        return false;
    }

    new_entry->key = key;
    new_entry->data = data;
    new_entry->row_count = row_count;
    new_entry->column_count = column_count;
    new_entry->affected_rows = affected_rows;
    new_entry->execution_time_ms = execution_time_ms;
    new_entry->next = cache->buckets[bucket];
    cache->buckets[bucket] = new_entry;
    cache->entry_count++;

    pthread_mutex_unlock(&cache->lock);
    return true;
}

void query_result_cache_clear(QueryResultCache* cache) {
    if (!cache) {
        return;
    }

    pthread_mutex_lock(&cache->lock);

    for (size_t i = 0; i < cache->bucket_count; i++) {
        QueryResultCacheEntry* entry = cache->buckets[i];
        while (entry) {
            QueryResultCacheEntry* next = entry->next;
            free(entry->key);
            json_decref(entry->data);
            free(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }
    cache->entry_count = 0;

    pthread_mutex_unlock(&cache->lock);
}

// cppcheck-suppress constParameterPointer - pthread_mutex_lock requires non-const mutex even for read-only access
size_t query_result_cache_entry_count(QueryResultCache* cache) {
    if (!cache) {
        return 0;
    }

    size_t count = 0;
    pthread_mutex_lock(&cache->lock);
    count = cache->entry_count;
    pthread_mutex_unlock(&cache->lock);
    return count;
}

QueryResultCache* query_result_cache_get_global(void) {
    query_result_cache_global_init();
    return g_query_result_cache;
}

bool query_result_cache_global_init(void) {
    pthread_mutex_lock(&g_query_result_cache_init_lock);
    if (!g_query_result_cache) {
        g_query_result_cache = query_result_cache_create(256, "global");
    }
    pthread_mutex_unlock(&g_query_result_cache_init_lock);
    return g_query_result_cache != NULL;
}

void query_result_cache_global_destroy(void) {
    pthread_mutex_lock(&g_query_result_cache_init_lock);
    query_result_cache_destroy(g_query_result_cache);
    g_query_result_cache = NULL;
    pthread_mutex_unlock(&g_query_result_cache_init_lock);
}
