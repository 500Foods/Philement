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

#include "chat_storage.h"
#include "chat_lru_cache.h"

static const char* SR_CHAT_STORAGE = "CHAT_STORAGE";

extern DatabaseQueueManager* global_queue_manager;

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
static ChatLRUCache* get_or_create_cache(const char* database) {
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

char* chat_storage_generate_hash(const char* content, size_t length) {
    if (!content || length == 0) {
        return NULL;
    }

    return utils_sha256_hash((const unsigned char*)content, length);
}

char* chat_storage_generate_hash_from_json(const char* json_string) {
    if (!json_string) {
        return NULL;
    }

    return utils_sha256_hash((const unsigned char*)json_string, strlen(json_string));
}

bool chat_storage_compress_message(const char* message, size_t message_len,
                                    uint8_t** compressed_data, size_t* compressed_size) {
    if (!message || message_len == 0 || !compressed_data || !compressed_size) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for compression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = compress_json_result(message, message_len, compressed_data, compressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE, "Brotli compression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Compressed message from %zu to %zu bytes (%.1f%%)",
             LOG_LEVEL_DEBUG, 3, message_len, *compressed_size,
             (double)*compressed_size / (double)message_len * 100.0);

    return true;
}

bool chat_storage_decompress_message(const uint8_t* compressed_data, size_t compressed_size,
                                      char** decompressed_data, size_t* decompressed_size) {
    if (!compressed_data || compressed_size == 0 || !decompressed_data || !decompressed_size) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for decompression", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = decompress_cached_result(compressed_data, compressed_size, decompressed_data, decompressed_size);
    if (!result) {
        log_this(SR_CHAT_STORAGE, "Brotli decompression failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT_STORAGE, "Decompressed message from %zu to %zu bytes",
             LOG_LEVEL_DEBUG, 2, compressed_size, *decompressed_size);

    return true;
}

static bool chat_storage_execute_query(DatabaseQueue* db_queue, int query_ref,
                                        const char* params_json, char** result_json) {
    if (!db_queue || !db_queue->persistent_connection || query_ref <= 0) {
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

    if (result) database_engine_cleanup_result(result);
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
    ChatLRUCache* cache = get_or_create_cache(database);
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
    }

    return hash;
}

char* chat_storage_retrieve_segment(const char* database, const char* segment_hash) {
    if (!database || !segment_hash) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for retrieve_segment", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    /* Phase 9: Check LRU cache first */
    ChatLRUCache* cache = get_or_create_cache(database);
    if (cache) {
        char* cached_content = chat_lru_cache_get(cache, segment_hash);
        if (cached_content) {
            log_this(SR_CHAT_STORAGE, "Retrieved segment from LRU cache: %s",
                     LOG_LEVEL_DEBUG, 1, segment_hash);
            chat_storage_update_access(database, segment_hash);
            return cached_content;
        }
    }

    /* Fall back to database retrieval */
    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }

    char params_json[256];
    snprintf(params_json, sizeof(params_json), "{\"SEGMENT_HASH\": \"%s\"}", segment_hash);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 62, params_json, &result_json);

    if (!query_success || !result_json) {
        log_this(SR_CHAT_STORAGE, "Segment not found or query failed: %s", LOG_LEVEL_ERROR, 1, segment_hash);
        if (result_json) free(result_json);
        return NULL;
    }

    json_error_t error;
    json_t* root = json_loads(result_json, 0, &error);
    free(result_json);

    if (!root || !json_is_array(root) || json_array_size(root) == 0) {
        log_this(SR_CHAT_STORAGE, "No segment data returned for hash: %s", LOG_LEVEL_ERROR, 1, segment_hash);
        if (root) json_decref(root);
        return NULL;
    }

    json_t* row = json_array_get(root, 0);
    json_t* content_obj = json_object_get(row, "segment_content");
    json_t* uncompressed_obj = json_object_get(row, "uncompressed_size");

    if (!content_obj || !json_is_string(content_obj)) {
        log_this(SR_CHAT_STORAGE, "Invalid segment content for hash: %s", LOG_LEVEL_ERROR, 1, segment_hash);
        json_decref(root);
        return NULL;
    }

    const char* content_hex = json_string_value(content_obj);
    (void)uncompressed_obj; // Available if needed for metadata

    size_t hex_len = strlen(content_hex);
    size_t compressed_size = hex_len / 2;
    uint8_t* compressed = calloc(compressed_size + 1, 1);
    if (!compressed) {
        json_decref(root);
        log_this(SR_CHAT_STORAGE, "Failed to allocate compressed buffer", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    for (size_t i = 0; i < compressed_size; i++) {
        unsigned int byte;
        sscanf(content_hex + i * 2, "%02x", &byte);
        compressed[i] = (uint8_t)byte;
    }

    char* decompressed = NULL;
    size_t decompressed_size = 0;
    bool decompress_ok = chat_storage_decompress_message(compressed, compressed_size, &decompressed, &decompressed_size);

    free(compressed);
    json_decref(root);

    if (!decompress_ok || !decompressed) {
        log_this(SR_CHAT_STORAGE, "Failed to decompress segment: %s", LOG_LEVEL_ERROR, 1, segment_hash);
        return NULL;
    }

    /* Phase 9: Store in LRU cache for future access */
    if (cache && decompressed) {
        chat_lru_cache_put(cache, segment_hash, decompressed, decompressed_size, false);
    }

    chat_storage_update_access(database, segment_hash);

    return decompressed;
}

bool chat_storage_update_access(const char* database, const char* segment_hash) {
    if (!database || !segment_hash) {
        return false;
    }

    return true;
}

bool chat_storage_segment_exists(const char* database, const char* segment_hash) {
    if (!database || !segment_hash) {
        return false;
    }

    /* Phase 9: Check LRU cache first */
    ChatLRUCache* cache = get_or_create_cache(database);
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

/* Phase 10: Batch retrieval of multiple segments */
json_t* chat_storage_retrieve_segments_batch(const char* database,
                                              const char** segment_hashes,
                                              size_t hash_count) {
    if (!database || !segment_hashes || hash_count == 0) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for batch retrieval", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ChatLRUCache* cache = get_or_create_cache(database);
    json_t* results = json_array();

    /* Collect hashes that need database fetch */
    char** missing_hashes = calloc(hash_count, sizeof(char*));
    size_t missing_count = 0;

    for (size_t i = 0; i < hash_count; i++) {
        const char* hash = segment_hashes[i];
        if (!hash) continue;

        /* Check cache first */
        if (cache) {
            char* cached_content = chat_lru_cache_get(cache, hash);
            if (cached_content) {
                /* Found in cache - add to results */
                json_t* segment = json_object();
                json_object_set_new(segment, "segment_hash", json_string(hash));
                json_object_set_new(segment, "content", json_string(cached_content));
                json_object_set_new(segment, "source", json_string("cache"));
                json_array_append_new(results, segment);
                free(cached_content);
                continue;
            }
        }

        /* Not in cache - add to missing list */
        missing_hashes[missing_count] = strdup(hash);
        missing_count++;
    }

    /* Batch fetch missing segments from database */
    if (missing_count > 0) {
        /* Build comma-separated hash list */
        size_t hashes_len = 0;
        for (size_t i = 0; i < missing_count; i++) {
            hashes_len += strlen(missing_hashes[i]) + 1;
        }

        char* hashes_csv = calloc(hashes_len + 1, 1);
        if (hashes_csv) {
            for (size_t i = 0; i < missing_count; i++) {
                if (i > 0) strcat(hashes_csv, ",");
                strcat(hashes_csv, missing_hashes[i]);
            }

            DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
            if (db_queue && db_queue->persistent_connection) {
                char params_json[65536];
                snprintf(params_json, sizeof(params_json), "{\"SEGMENT_HASHES\": \"%s\"}", hashes_csv);

                char* result_json = NULL;
                bool query_success = chat_storage_execute_query(db_queue, 70, params_json, &result_json);

                if (query_success && result_json) {
                    json_error_t error;
                    json_t* db_results = json_loads(result_json, 0, &error);

                    if (db_results && json_is_array(db_results)) {
                        size_t db_count = json_array_size(db_results);
                        for (size_t i = 0; i < db_count; i++) {
                            json_t* row = json_array_get(db_results, i);
                            json_t* hash_obj = json_object_get(row, "segment_hash");
                            json_t* content_obj = json_object_get(row, "segment_content");

                            if (hash_obj && content_obj) {
                                const char* db_hash = json_string_value(hash_obj);
                                const char* content_hex = json_string_value(content_obj);

                                /* Decode hex and decompress */
                                size_t hex_len = strlen(content_hex);
                                size_t compressed_size = hex_len / 2;
                                uint8_t* compressed = calloc(compressed_size + 1, 1);

                                if (compressed) {
                                    for (size_t j = 0; j < compressed_size; j++) {
                                        unsigned int byte;
                                        sscanf(content_hex + j * 2, "%02x", &byte);
                                        compressed[j] = (uint8_t)byte;
                                    }

                                    char* decompressed = NULL;
                                    size_t decompressed_size = 0;
                                    if (chat_storage_decompress_message(compressed, compressed_size,
                                                                        &decompressed, &decompressed_size)) {
                                        /* Add to results */
                                        json_t* segment = json_object();
                                        json_object_set_new(segment, "segment_hash", json_string(db_hash));
                                        json_object_set_new(segment, "content", json_string(decompressed));
                                        json_object_set_new(segment, "source", json_string("database"));
                                        json_array_append_new(results, segment);

                                        /* Store in cache for future access */
                                        if (cache) {
                                            chat_lru_cache_put(cache, db_hash, decompressed,
                                                               decompressed_size, false);
                                        }
                                        free(decompressed);
                                    }
                                    free(compressed);
                                }
                            }
                        }
                    }
                    if (db_results) json_decref(db_results);
                    free(result_json);
                }
            }
            free(hashes_csv);
        }
    }

    /* Cleanup missing hashes */
    for (size_t i = 0; i < missing_count; i++) {
        free(missing_hashes[i]);
    }
    free(missing_hashes);

    log_this(SR_CHAT_STORAGE, "Batch retrieval: %zu requested, %zu found in cache, %zu from DB",
             LOG_LEVEL_DEBUG, 3, hash_count, hash_count - missing_count, (size_t)json_array_size(results) - (hash_count - missing_count));

    return results;
}

/* Phase 10: Conservative pre-fetch - pre-fetch next segment when accessing current */
bool chat_storage_prefetch_segment(const char* database, const char* segment_hash) {
    if (!database || !segment_hash) {
        return false;
    }

    /* Conservative approach: just check if segment exists and warm the cache
     * This ensures the segment is in cache for quick access
     * Full conversation-aware pre-fetching would require conversation context */

    ChatLRUCache* cache = get_or_create_cache(database);
    if (!cache) {
        return false;
    }

    /* If already in cache, nothing to do */
    if (chat_lru_cache_contains(cache, segment_hash)) {
        return true;
    }

    /* Attempt to fetch and cache the segment */
    /* This is conservative - only pre-fetches if explicitly requested */
    return chat_storage_segment_exists(database, segment_hash);
}

bool chat_storage_get_stats(const char* database, const char* segment_hash,
                            size_t* uncompressed_size,
                            size_t* compressed_size,
                            double* compression_ratio,
                            int* access_count) {
    if (!database || !segment_hash || !uncompressed_size || !compressed_size ||
        !compression_ratio || !access_count) {
        return false;
    }

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

    json_t* row = json_array_get(root, 0);
    json_t* uncomp_obj = json_object_get(row, "uncompressed_size");
    json_t* comp_obj = json_object_get(row, "compressed_size");
    json_t* ratio_obj = json_object_get(row, "compression_ratio");
    json_t* count_obj = json_object_get(row, "access_count");

    *uncompressed_size = uncomp_obj ? (size_t)json_integer_value(uncomp_obj) : 0;
    *compressed_size = comp_obj ? (size_t)json_integer_value(comp_obj) : 0;
    *compression_ratio = ratio_obj ? json_number_value(ratio_obj) : 0.0;
    *access_count = count_obj ? (int)json_integer_value(count_obj) : 0;

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

json_t* chat_storage_get_chat(const char* database, int convos_id) {
    if (!database || convos_id <= 0) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for get_chat", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }

    char params_json[256];
    snprintf(params_json, sizeof(params_json), "{\"CONVOSID\": %d}", convos_id);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 68, params_json, &result_json);

    if (!query_success || !result_json) {
        log_this(SR_CHAT_STORAGE, "Chat not found or query failed for convos_id: %d", LOG_LEVEL_ERROR, 1, convos_id);
        if (result_json) free(result_json);
        return NULL;
    }

    json_error_t error;
    json_t* root = json_loads(result_json, 0, &error);
    free(result_json);

    if (!root || !json_is_array(root) || json_array_size(root) == 0) {
        log_this(SR_CHAT_STORAGE, "No chat data returned for convos_id: %d", LOG_LEVEL_ERROR, 1, convos_id);
        if (root) json_decref(root);
        return NULL;
    }

    json_t* row = json_array_get(root, 0);
    json_t* segments_obj = json_object_get(row, "segments");

    if (!segments_obj || !json_is_array(segments_obj)) {
        json_decref(root);
        return json_object();
    }

    json_t* messages_array = json_array();
    size_t seg_count = json_array_size(segments_obj);

    for (size_t i = 0; i < seg_count; i++) {
        json_t* seg = json_array_get(segments_obj, i);
        json_t* content_obj = json_object_get(seg, "segment_content");

        if (!content_obj || !json_is_string(content_obj)) continue;

        const char* content_hex = json_string_value(content_obj);
        size_t hex_len = strlen(content_hex);
        size_t comp_len = hex_len / 2;
        uint8_t* comp = calloc(comp_len + 1, 1);
        if (!comp) continue;

        for (size_t j = 0; j < comp_len; j++) {
            unsigned int byte;
            sscanf(content_hex + j * 2, "%02x", &byte);
            comp[j] = (uint8_t)byte;
        }

        char* decomp = NULL;
        size_t decomp_size = 0;
        if (chat_storage_decompress_message(comp, comp_len, &decomp, &decomp_size)) {
            json_array_append_new(messages_array, json_string(decomp));
        }
        free(comp);
        if (decomp) free(decomp);
    }

    json_decref(root);
    return messages_array;
}

void chat_storage_free_compressed(uint8_t* data) {
    if (data) {
        free(data);
    }
}

void chat_storage_free_decompressed(char* data) {
    if (data) {
        free(data);
    }
}

void chat_storage_free_hash(char* hash) {
    if (hash) {
        free(hash);
    }
}

/* Phase 12: Media Assets Storage */

bool chat_storage_store_media(const char* database, const char* media_hash,
                              const unsigned char* media_data, size_t media_size,
                              const char* mime_type) {
    if (!database || !media_hash || !media_data || media_size == 0) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for store_media", LOG_LEVEL_ERROR, 0);
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return false;
    }

    // Convert binary data to hex string
    char* media_data_hex = calloc(media_size * 2 + 1, 1);
    if (!media_data_hex) {
        log_this(SR_CHAT_STORAGE, "Failed to allocate hex buffer for media data", LOG_LEVEL_ERROR, 0);
        return false;
    }
    for (size_t i = 0; i < media_size; i++) {
        sprintf(media_data_hex + i * 2, "%02x", media_data[i]);
    }

    char params_json[4096];
    snprintf(params_json, sizeof(params_json),
        "{"
        "\"MEDIA_HASH\": \"%s\","
        "\"MEDIA_DATA\": \"%s\","
        "\"MEDIA_SIZE\": %zu,"
        "\"MIME_TYPE\": \"%s\""
        "}",
        media_hash,
        media_data_hex,
        media_size,
        mime_type ? mime_type : ""
    );

    free(media_data_hex);

    log_this(SR_CHAT_STORAGE, "Storing media hash: %s (size: %zu)", LOG_LEVEL_DEBUG, 2, media_hash, media_size);

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 71, params_json, &result_json);

    if (result_json) free(result_json);

    if (!query_success) {
        log_this(SR_CHAT_STORAGE, "Failed to execute QueryRef #071 for media hash: %s", LOG_LEVEL_ERROR, 1, media_hash);
    }

    return query_success;
}

bool chat_storage_retrieve_media(const char* database, const char* media_hash,
                                 unsigned char** media_data, size_t* media_size,
                                 char** mime_type) {
    if (!database || !media_hash || !media_data || !media_size) {
        log_this(SR_CHAT_STORAGE, "Invalid parameters for retrieve_media", LOG_LEVEL_ERROR, 0);
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue || !db_queue->persistent_connection) {
        log_this(SR_CHAT_STORAGE, "Database queue not available for: %s", LOG_LEVEL_ERROR, 1, database);
        return false;
    }

    char params_json[1024];
    snprintf(params_json, sizeof(params_json),
        "{\"MEDIA_HASH\": \"%s\"}",
        media_hash
    );

    char* result_json = NULL;
    bool query_success = chat_storage_execute_query(db_queue, 72, params_json, &result_json);

    if (!query_success || !result_json) {
        log_this(SR_CHAT_STORAGE, "Failed to execute QueryRef #072 for media hash: %s", LOG_LEVEL_ERROR, 1, media_hash);
        if (result_json) free(result_json);
        return false;
    }

    // Parse result JSON
    json_error_t error;
    json_t* root = json_loads(result_json, 0, &error);
    free(result_json);

    if (!root || !json_is_array(root) || json_array_size(root) == 0) {
        log_this(SR_CHAT_STORAGE, "No media asset found for hash: %s", LOG_LEVEL_DEBUG, 1, media_hash);
        if (root) json_decref(root);
        return false;
    }

    json_t* row = json_array_get(root, 0);
    if (!json_is_object(row)) {
        json_decref(root);
        return false;
    }

    // Extract media_data hex string
    json_t* media_data_hex_json = json_object_get(row, "media_data");
    if (!media_data_hex_json || !json_is_string(media_data_hex_json)) {
        json_decref(root);
        return false;
    }
    const char* media_data_hex = json_string_value(media_data_hex_json);
    size_t hex_len = strlen(media_data_hex);
    if (hex_len % 2 != 0) {
        json_decref(root);
        return false;
    }
    size_t binary_len = hex_len / 2;
    unsigned char* binary_data = malloc(binary_len);
    if (!binary_data) {
        json_decref(root);
        return false;
    }
    for (size_t i = 0; i < binary_len; i++) {
        sscanf(media_data_hex + i * 2, "%2hhx", &binary_data[i]);
    }

    // Extract mime_type
    json_t* mime_json = json_object_get(row, "mime_type");
    char* mime = NULL;
    if (mime_json && json_is_string(mime_json)) {
        mime = strdup(json_string_value(mime_json));
    }

    // Extract media_size
    json_t* size_json = json_object_get(row, "media_size");
    size_t size = 0;
    if (size_json && json_is_integer(size_json)) {
        size = (size_t)json_integer_value(size_json);
    }

    json_decref(root);

    *media_data = binary_data;
    *media_size = size;
    if (mime_type) {
        *mime_type = mime;
    } else {
        free(mime);
    }

    log_this(SR_CHAT_STORAGE, "Retrieved media hash: %s (size: %zu)", LOG_LEVEL_DEBUG, 2, media_hash, size);
    return true;
}

// Helper to convert base64url to standard base64 with padding
static char* base64url_to_base64(const char* url_base64) {
    if (!url_base64) return NULL;
    
    size_t len = strlen(url_base64);
    // Calculate padding
    size_t padding = 0;
    if (len % 4 == 2) padding = 2;
    else if (len % 4 == 3) padding = 1;
    
    char* standard = malloc(len + padding + 1);
    if (!standard) return NULL;
    
    // Copy and replace characters
    for (size_t i = 0; i < len; i++) {
        char c = url_base64[i];
        if (c == '-') standard[i] = '+';
        else if (c == '_') standard[i] = '/';
        else standard[i] = c;
    }
    // Add padding
    for (size_t i = 0; i < padding; i++) {
        standard[len + i] = '=';
    }
    standard[len + padding] = '\0';
    return standard;
}

bool chat_storage_resolve_media_in_content(const char* database,
                                           const char* content_json,
                                           char** resolved_json,
                                           char** error_message) {
    if (!database || !content_json || !resolved_json) {
        if (error_message) *error_message = strdup("Invalid parameters");
        return false;
    }
    
    json_error_t json_error;
    json_t* content_array = json_loads(content_json, 0, &json_error);
    if (!content_array || !json_is_array(content_array)) {
        if (content_array) json_decref(content_array);
        if (error_message) *error_message = strdup("Invalid JSON array in content");
        return false;
    }
    
    json_t* new_array = json_array();
    if (!new_array) {
        json_decref(content_array);
        if (error_message) *error_message = strdup("Failed to create JSON array");
        return false;
    }
    
    size_t index;
    json_t* item;
    bool success = true;
    
    json_array_foreach(content_array, index, item) {
        if (!json_is_object(item)) {
            json_array_append(new_array, item);
            continue;
        }
        
        json_t* type = json_object_get(item, "type");
        if (!type || !json_is_string(type)) {
            json_array_append(new_array, item);
            continue;
        }
        
        const char* type_str = json_string_value(type);
        if (strcmp(type_str, "image_url") != 0) {
            // Not an image_url, keep as is
            json_array_append(new_array, item);
            continue;
        }
        
        json_t* image_url = json_object_get(item, "image_url");
        if (!image_url || !json_is_object(image_url)) {
            json_array_append(new_array, item);
            continue;
        }
        
        json_t* url = json_object_get(image_url, "url");
        if (!url || !json_is_string(url)) {
            json_array_append(new_array, item);
            continue;
        }
        
        const char* url_str = json_string_value(url);
        if (strncmp(url_str, "media:", 6) != 0) {
            // Not a media reference, keep as is
            json_array_append(new_array, item);
            continue;
        }
        
        // Extract hash after "media:"
        const char* media_hash = url_str + 6;
        
        // Retrieve media from storage
        unsigned char* media_data = NULL;
        size_t media_size = 0;
        char* mime_type = NULL;
        
        if (!chat_storage_retrieve_media(database, media_hash, &media_data, &media_size, &mime_type)) {
            // Media not found - keep original reference? Or error? We'll keep original.
            json_array_append(new_array, item);
            continue;
        }
        
        // Encode to base64
        char* base64url = utils_base64url_encode(media_data, media_size);
        free(media_data);
        if (!base64url) {
            free(mime_type);
            success = false;
            break;
        }
        
        // Convert to standard base64
        char* base64 = base64url_to_base64(base64url);
        free(base64url);
        if (!base64) {
            free(mime_type);
            success = false;
            break;
        }
        
        // Build data URL
        const char* mime = mime_type ? mime_type : "application/octet-stream";
        char* data_url = malloc(strlen(mime) + strlen(base64) + 32);
        if (!data_url) {
            free(base64);
            free(mime_type);
            success = false;
            break;
        }
        sprintf(data_url, "data:%s;base64,%s", mime, base64);
        free(base64);
        free(mime_type);
        
        // Create new image_url object with data URL
        json_t* new_item = json_object();
        json_object_set(new_item, "type", json_string("image_url"));
        json_t* new_image_url = json_object();
        json_object_set(new_image_url, "url", json_string(data_url));
        json_object_set(new_item, "image_url", new_image_url);
        json_array_append(new_array, new_item);
        
        free(data_url);
        json_decref(new_item);
    }
    
    json_decref(content_array);
    
    if (!success) {
        json_decref(new_array);
        if (error_message) *error_message = strdup("Failed to resolve media reference");
        return false;
    }
    
    // Convert new array to string
    *resolved_json = json_dumps(new_array, JSON_COMPACT);
    json_decref(new_array);
    
    if (!*resolved_json) {
        if (error_message) *error_message = strdup("Failed to serialize resolved content");
        return false;
    }
    
    return true;
}

/* Phase 9: LRU Cache Management Functions */

bool chat_storage_cache_init(const char* database, size_t max_size_bytes) {
    if (!database) {
        return false;
    }

    (void)max_size_bytes;  /* Used internally via get_or_create_cache */

    const ChatLRUCache* cache = get_or_create_cache(database);
    return cache != NULL;
}

void chat_storage_cache_shutdown(const char* database) {
    if (!database) {
        return;
    }

    pthread_mutex_lock(&cache_manager_mutex);

    for (int i = 0; i < cache_count; i++) {
        if (cache_entries[i].database && strcmp(cache_entries[i].database, database) == 0) {
            chat_lru_cache_shutdown(cache_entries[i].cache);
            free(cache_entries[i].database);

            /* Shift remaining entries */
            for (int j = i; j < cache_count - 1; j++) {
                cache_entries[j] = cache_entries[j + 1];
            }
            cache_count--;
            break;
        }
    }

    pthread_mutex_unlock(&cache_manager_mutex);
}

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