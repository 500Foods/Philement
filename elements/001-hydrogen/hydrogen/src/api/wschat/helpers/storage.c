/*
 * Chat Storage Module - Content-Addressable Storage Implementation
 *
 * Phase 6: Conversation History with Content-Addressable Storage + Brotli
 * Phase 7: Connected to QueryRefs #063 (store), #062 (retrieve), #067 (store chat)
 * Phase 9: Added LRU disk cache layer for hot segments
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <src/utils/utils_compression.h>
#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

#include "storage.h"
#include "lru_cache.h"
#include "storage_hex.h"
#include "storage_compress.h"
#include "storage_media.h"

static const char* SR_CHAT_STORAGE = "CHAT_STORAGE";

extern DatabaseQueueManager* global_queue_manager;

/* Query execution helper - accessible from chat_storage_media.c */
bool chat_storage_execute_query(DatabaseQueue* db_queue, int query_ref,
                                const char* params_json, char** result_json);

/* LRU Cache storage for databases */
#define MAX_CACHED_DATABASES 32

typedef struct {
    char* database;
    ChatLRUCache* cache;
} DatabaseCacheEntry;

static DatabaseCacheEntry cache_entries[MAX_CACHED_DATABASES] = {0};
static int cache_count = 0;
static pthread_mutex_t cache_manager_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Find or create cache for a database */
 ChatLRUCache* chat_storage_get_or_create_cache(const char* database) {
    if (!database) {
        return NULL;
    }

    pthread_mutex_lock(&cache_manager_mutex);

    /* Look for existing cache */
    for (int i = 0; i < cache_count; i++) {
        if (cache_entries[i].database && strcmp(cache_entries[i].database, database) == 0) {
            ChatLRUCache* cache = cache_entries[i].cache;
            pthread_mutex_unlock(&cache_manager_mutex);
            return cache;
        }
    }

    /* Create new cache if room */
    if (cache_count < MAX_CACHED_DATABASES) {
        ChatLRUCache* cache = chat_lru_cache_init(database, 0);  /* Use default 1GB */
        if (cache) {
            cache_entries[cache_count].database = strdup(database);
            cache_entries[cache_count].cache = cache;
            cache_count++;

            log_this(SR_CHAT_STORAGE, "Created LRU cache for database: %s",
                     LOG_LEVEL_STATE, 1, database);

            pthread_mutex_unlock(&cache_manager_mutex);
            return cache;
        }
    }

    pthread_mutex_unlock(&cache_manager_mutex);
    return NULL;
}

/* Hash and compression functions are now in separate modules:
 * - chat_storage_hash.c: chat_storage_generate_hash
 * - chat_storage_compress.c: chat_storage_compress_message, chat_storage_decompress_message
 */

bool chat_storage_execute_query(DatabaseQueue* db_queue, int query_ref,
                                const char* params_json, char** result_json) {
    if (!db_queue || !db_queue->persistent_connection || query_ref <= 0) {
        log_this(SR_CHAT_STORAGE, "Invalid db_queue/query_ref (queue=%p conn=%p ref=%d)", LOG_LEVEL_ALERT, 0,
                 db_queue, db_queue ? db_queue->persistent_connection : NULL, query_ref);
        return false;
    }

    QueryRequest request = {0};
    request.query_id = NULL;
    request.sql_template = NULL;
    request.parameters_json = params_json ? strdup(params_json) : strdup("{}");
    request.timeout_seconds = 30;
    request.isolation_level = DB_ISOLATION_READ_COMMITTED;
    request.use_prepared_statement = false;

    QueryResult* result = NULL;
    bool success = database_engine_execute(db_queue->persistent_connection, &request, &result);

    if (!success || !result || !result->success) {
        log_this(SR_CHAT_STORAGE, "QueryRef #%d failed: %s", LOG_LEVEL_ERROR, 2, query_ref,
                 result && result->error_message ? result->error_message : "Unknown error");
        if (result) database_engine_cleanup_result(result);
        free(request.parameters_json);
        return false;
    }

    if (result_json && result->data_json) {
        *result_json = strdup(result->data_json);
    }

    database_engine_cleanup_result(result);
    free(request.parameters_json);
    return true;
}

char* chat_storage_store_segment(const char* database, const char* message, size_t message_len) {
    if (!database || !message || message_len == 0) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for store_segment", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }

    char* hash = chat_storage_generate_hash(message, message_len);
    if (!hash) {
        log_this(SR_CHAT_STORAGE, "Failed to generate hash for message", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    /* Phase 9: Store in LRU cache immediately (marked as dirty) */
    ChatLRUCache* cache = chat_storage_get_or_create_cache(database);
    if (cache) {
        chat_lru_cache_put(cache, hash, message, message_len, true);
    }

    uint8_t* compressed = NULL;
    size_t compressed_size = 0;

    bool compress_result = chat_storage_compress_message(message, message_len, &compressed, &compressed_size);
    if (!compress_result) {
        free(hash);
        log_this(SR_CHAT_STORAGE, "Failed to compress message", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    double compression_ratio = 0.0;
    if (message_len > 0) {
        compression_ratio = (double)compressed_size / (double)message_len;
    }

    char* compressed_hex = NULL;
    if (compressed && compressed_size > 0) {
        compressed_hex = calloc(compressed_size * 2 + 1, 1);
        if (compressed_hex) {
            for (size_t i = 0; i < compressed_size; i++) {
                sprintf(compressed_hex + i * 2, "%02x", compressed[i]);
            }
        }
    }

    char params_json[4096];
    snprintf(params_json, sizeof(params_json),
        "{"
        "\"SEGMENT_HASH\": \"%s\","
        "\"SEGMENT_CONTENT\": \"%s\","
        "\"UNCOMPRESSED_SIZE\": %zu,"
        "\"COMPRESSED_SIZE\": %zu,"
        "\"COMPRESSION_RATIO\": %.4f"
        "}",
        hash,
        compressed_hex ? compressed_hex : "",
        message_len,
        compressed_size,
        compression_ratio
    );

    (void)params_json; // Used in query execution

    if (compressed_hex) free(compressed_hex);
    if (compressed) free(compressed);

    log_this(SR_CHAT_STORAGE, "Storing segment hash: %s (original: %zu, compressed: %zu)",
             LOG_LEVEL_DEBUG, 3, hash, message_len, compressed_size);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 63, params_json, &result_json);

    if (result_json) free(result_json);

    if (!query_success) {
        log_this(SR_CHAT_STORAGE, "Failed to execute QueryRef #063 for hash: %s", LOG_LEVEL_ERROR, 1, hash);
    } else if (cache) {
        /* Write-through complete: keep cache entry, clear dirty */
        chat_lru_cache_put(cache, hash, message, message_len, false);
    }

    return hash;
}

bool chat_storage_segment_exists(const char* database, const char* segment_hash) {
    if (!database || !segment_hash) {
        return false;
    }

    /* Phase 9: Check LRU cache first */
    ChatLRUCache* cache = chat_storage_get_or_create_cache(database);
    if (cache && chat_lru_cache_contains(cache, segment_hash)) {
        log_this(SR_CHAT_STORAGE, "Segment exists (cache hit): %s",
                 LOG_LEVEL_DEBUG, 1, segment_hash);
        return true;
    }

    /* Fall back to database check */
    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        return false;
    }

    char params_json[256];
    snprintf(params_json, sizeof(params_json), "{\"SEGMENT_HASH\": \"%s\"}", segment_hash);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 62, params_json, &result_json);

    if (!query_success || !result_json) {
        return false;
    }

    json_error_t error;
    json_t* root = json_loads(result_json, 0, &error);
    free(result_json);

    if (!root || !json_is_array(root) || json_array_size(root) == 0) {
        if (root) json_decref(root);
        return false;
    }

    /* Segment exists in DB - populate cache */
    if (cache) {
        /* Get the content and cache it */
        json_t* row = json_array_get(root, 0);
        json_t* content_obj = json_object_get(row, "segment_content");

        if (content_obj && json_is_string(content_obj)) {
            const char* content_hex = json_string_value(content_obj);
            size_t hex_len = strlen(content_hex);
            size_t compressed_size = hex_len / 2;

            uint8_t* compressed = calloc(compressed_size + 1, 1);
            if (compressed) {
                for (size_t i = 0; i < compressed_size; i++) {
                    unsigned int byte;
                    sscanf(content_hex + i * 2, "%02x", &byte);
                    compressed[i] = (uint8_t)byte;
                }

                char* decompressed = NULL;
                size_t decompressed_size = 0;
                if (chat_storage_decompress_message(compressed, compressed_size,
                                                    &decompressed, &decompressed_size)) {
                    /* Store uncompressed content in cache */
                    chat_lru_cache_put(cache, segment_hash, decompressed,
                                       decompressed_size, false);
                    free(decompressed);
                }
                free(compressed);
            }
        }
    }

    json_decref(root);
    return true;
}

char* chat_storage_store_chat(const char* database, const char* convos_ref,
                               const char** segment_hashes, size_t hash_count,
                               const char* engine_name, const char* model,
                               int tokens_prompt, int tokens_completion,
                               double cost_total, int user_id, int duration_ms,
                               const char* session_id) {
    if (!database || !convos_ref || !segment_hashes || hash_count == 0) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for store_chat", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }

    json_t* segment_array = json_array();
    for (size_t i = 0; i < hash_count; i++) {
        json_array_append_new(segment_array, json_string(segment_hashes[i]));
    }
    char* segment_refs_json = json_dumps(segment_array, JSON_COMPACT);
    json_decref(segment_array);

    char params_json[8192];
    int params_len = snprintf(params_json, sizeof(params_json),
        "{"
        "\"CONVOREF\": \"%s\","
        "\"SEGMENT_REFS\": %s,"
        "\"ENGINE_NAME\": \"%s\","
        "\"MODEL\": \"%s\","
        "\"TOKENS_PROMPT\": %d,"
        "\"TOKENS_COMPLETION\": %d,"
        "\"COST_TOTAL\": %.4f,"
        "\"USER_ID\": %d,"
        "\"DURATION_MS\": %d,"
        "\"SESSION_ID\": \"%s\","
        "\"ACCOUNTID\": %d"
        "}",
        convos_ref,
        segment_refs_json ? segment_refs_json : "[]",
        engine_name ? engine_name : "",
        model ? model : "",
        tokens_prompt,
        tokens_completion,
        cost_total,
        user_id,
        duration_ms,
        session_id ? session_id : "",
        user_id
    );

    if (segment_refs_json) free(segment_refs_json);

    log_this(SR_CHAT_STORAGE, "Storing chat convos_ref: %s with %zu segments",
             LOG_LEVEL_DEBUG, 2, convos_ref, hash_count);

    (void)params_len;

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 67, params_json, &result_json);

    if (result_json) free(result_json);

    if (!query_success) {
        log_this(SR_CHAT_STORAGE, "Failed to execute QueryRef #067 for convos_ref: %s", LOG_LEVEL_ERROR, 1, convos_ref);
        return NULL;
    }

    return strdup(convos_ref);
}

void chat_storage_free_hash(char* hash) {
    if (hash) {
        free(hash);
    }
}

/* Phase 12: Media Assets Storage
 * Now in chat_storage_media.c:
 * - chat_storage_store_media
 * - chat_storage_retrieve_media
 * - chat_storage_resolve_media_in_content
 */

/* Phase 9: LRU Cache Management Functions */

ChatLRUCache* chat_storage_get_cache(const char* database) {
    if (!database) {
        return NULL;
    }

    pthread_mutex_lock(&cache_manager_mutex);

    for (int i = 0; i < cache_count; i++) {
        if (cache_entries[i].database && strcmp(cache_entries[i].database, database) == 0) {
            ChatLRUCache* cache = cache_entries[i].cache;
            pthread_mutex_unlock(&cache_manager_mutex);
            return cache;
        }
    }

    pthread_mutex_unlock(&cache_manager_mutex);
    return NULL;
}

bool chat_storage_cache_get_stats(const char* database,
                                  uint64_t* hits, uint64_t* misses, double* hit_ratio) {
    if (!database) {
        return false;
    }

    ChatLRUCache* cache = chat_storage_get_cache(database);
    if (!cache) {
        return false;
    }

    ChatLRUCacheStats stats;
    if (!chat_lru_cache_get_stats(cache, &stats)) {
        return false;
    }

    if (hits) *hits = stats.cache_hits;
    if (misses) *misses = stats.cache_misses;
    if (hit_ratio) *hit_ratio = stats.hit_ratio;

    return true;
}