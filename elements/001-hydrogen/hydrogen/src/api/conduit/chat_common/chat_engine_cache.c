/*
 * Chat Engine Cache (CEC) Implementation
 *
 * Thread-safe in-memory cache for storing AI engine configurations loaded during bootstrap.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "chat_engine_cache.h"

// Database includes for bootstrap query
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config_databases.h>

// External reference to global config
extern AppConfig* app_config;

// Initial capacity for cache entries array
#define INITIAL_CEC_CAPACITY 16

// Default refresh interval (1 hour)
#define CEC_DEFAULT_REFRESH_INTERVAL 3600

// Create a new chat engine cache
ChatEngineCache* chat_engine_cache_create(const char* database_name) {
    ChatEngineCache* cache = (ChatEngineCache*)malloc(sizeof(ChatEngineCache));
    if (!cache) {
        log_this(SR_CHAT, "Failed to allocate memory for chat engine cache", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize structure
    cache->engines = NULL;
    cache->engine_count = 0;
    cache->capacity = 0;
    cache->last_refresh = 0;
    cache->database_name = database_name ? strdup(database_name) : NULL;

    // Initialize reader-writer lock
    if (pthread_rwlock_init(&cache->cache_lock, NULL) != 0) {
        log_this(SR_CHAT, "Failed to initialize chat engine cache lock", LOG_LEVEL_ERROR, 0);
        free(cache->database_name);
        free(cache);
        return NULL;
    }

    // Allocate initial entries array
    cache->engines = (ChatEngineConfig**)malloc(INITIAL_CEC_CAPACITY * sizeof(ChatEngineConfig*));
    if (!cache->engines) {
        log_this(SR_CHAT, "Failed to allocate memory for chat engine cache entries", LOG_LEVEL_ERROR, 0);
        pthread_rwlock_destroy(&cache->cache_lock);
        free(cache->database_name);
        free(cache);
        return NULL;
    }

    cache->capacity = INITIAL_CEC_CAPACITY;

    // Initialize all entries to NULL
    memset(cache->engines, 0, INITIAL_CEC_CAPACITY * sizeof(ChatEngineConfig*));

    log_this(SR_CHAT, "Chat engine cache created for database: %s", LOG_LEVEL_DEBUG, 1, database_name ? database_name : "(null)");
    return cache;
}

// Destroy chat engine cache and all entries
void chat_engine_cache_destroy(ChatEngineCache* cache) {
    if (!cache) return;

    // Acquire write lock for destruction
    pthread_rwlock_wrlock(&cache->cache_lock);

    // Clear all entries (this also securely wipes API keys)
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i]) {
            chat_engine_config_destroy(cache->engines[i]);
        }
    }

    // Free entries array
    free(cache->engines);

    // Free database name
    free(cache->database_name);

    // Unlock before destroying
    pthread_rwlock_unlock(&cache->cache_lock);

    // Destroy lock
    pthread_rwlock_destroy(&cache->cache_lock);

    // Free cache structure
    free(cache);

    log_this(SR_CHAT, "Chat engine cache destroyed", LOG_LEVEL_DEBUG, 0);
}

// Clear all entries from cache (but keep cache structure)
void chat_engine_cache_clear(ChatEngineCache* cache) {
    if (!cache) return;

    // Acquire write lock
    if (pthread_rwlock_wrlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire write lock for chat engine cache clear", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Free all entries (securely wipes API keys)
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i]) {
            chat_engine_config_destroy(cache->engines[i]);
            cache->engines[i] = NULL;
        }
    }

    // Reset count but keep capacity
    cache->engine_count = 0;
    cache->last_refresh = 0;

    // Release write lock
    pthread_rwlock_unlock(&cache->cache_lock);

    log_this(SR_CHAT, "Chat engine cache cleared", LOG_LEVEL_DEBUG, 0);
}

// Create a new engine configuration
ChatEngineConfig* chat_engine_config_create(int engine_id, const char* name, ChatEngineProvider provider,
                                           const char* model, const char* api_url, const char* api_key,
                                           int max_tokens, double temperature_default, bool is_default) {
    ChatEngineConfig* engine = (ChatEngineConfig*)malloc(sizeof(ChatEngineConfig));
    if (!engine) {
        log_this(SR_CHAT, "Failed to allocate memory for chat engine config", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize structure
    engine->engine_id = engine_id;
    engine->provider = provider;
    engine->max_tokens = max_tokens;
    engine->temperature_default = temperature_default;
    engine->is_default = is_default;
    engine->last_used = 0;
    engine->usage_count = 0;

    // Copy strings with bounds checking
    if (name) {
        strncpy(engine->name, name, CEC_MAX_NAME_LEN - 1);
        engine->name[CEC_MAX_NAME_LEN - 1] = '\0';
    } else {
        engine->name[0] = '\0';
    }

    if (model) {
        strncpy(engine->model, model, CEC_MAX_MODEL_LEN - 1);
        engine->model[CEC_MAX_MODEL_LEN - 1] = '\0';
    } else {
        engine->model[0] = '\0';
    }

    if (api_url) {
        strncpy(engine->api_url, api_url, CEC_MAX_URL_LEN - 1);
        engine->api_url[CEC_MAX_URL_LEN - 1] = '\0';
    } else {
        engine->api_url[0] = '\0';
    }

    if (api_key) {
        strncpy(engine->api_key, api_key, CEC_MAX_KEY_LEN - 1);
        engine->api_key[CEC_MAX_KEY_LEN - 1] = '\0';
    } else {
        engine->api_key[0] = '\0';
    }

    return engine;
}

// Destroy an engine configuration (securely wipes API key)
void chat_engine_config_destroy(ChatEngineConfig* engine) {
    if (!engine) return;

    // Securely clear API key from memory before freeing
    if (engine->api_key[0] != '\0') {
        explicit_bzero(engine->api_key, sizeof(engine->api_key));
    }

    free(engine);
}

// Securely clear API key from an engine config
void chat_engine_config_clear_key(ChatEngineConfig* engine) {
    if (!engine) return;
    explicit_bzero(engine->api_key, sizeof(engine->api_key));
}

// Add entry to cache (assumes write lock is held)
static bool chat_engine_cache_add_engine_locked(ChatEngineCache* cache, ChatEngineConfig* engine) {
    // Check if we need to resize
    if (cache->engine_count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        ChatEngineConfig** new_engines = (ChatEngineConfig**)realloc(cache->engines,
                                                                      new_capacity * sizeof(ChatEngineConfig*));
        if (!new_engines) {
            log_this(SR_CHAT, "Failed to resize chat engine cache entries array", LOG_LEVEL_ERROR, 0);
            return false;
        }

        cache->engines = new_engines;
        cache->capacity = new_capacity;

        // Initialize new entries to NULL
        memset(cache->engines + cache->engine_count, 0,
               (new_capacity - cache->engine_count) * sizeof(ChatEngineConfig*));
    }

    // Add entry to array
    cache->engines[cache->engine_count++] = engine;

    return true;
}

// Add engine to cache with proper locking
bool chat_engine_cache_add_engine(ChatEngineCache* cache, ChatEngineConfig* engine) {
    if (!cache || !engine) {
        log_this(SR_CHAT, "Invalid parameters for chat engine cache add", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Acquire write lock
    if (pthread_rwlock_wrlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire write lock for chat engine cache", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = chat_engine_cache_add_engine_locked(cache, engine);

    // Release write lock
    pthread_rwlock_unlock(&cache->cache_lock);

    return result;
}

// Lookup engine by name (read lock)
ChatEngineConfig* chat_engine_cache_lookup_by_name(ChatEngineCache* cache, const char* name) {
    if (!cache || !name) return NULL;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire read lock for chat engine cache lookup", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ChatEngineConfig* result = NULL;

    // Linear search by name
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i] && strcmp(cache->engines[i]->name, name) == 0) {
            result = cache->engines[i];
            break;
        }
    }

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    if (result) {
        // Update usage statistics (this is a race condition but acceptable for stats)
        result->last_used = time(NULL);
        __sync_fetch_and_add(&result->usage_count, 1);
    }

    return result;
}

// Lookup engine by ID (read lock)
ChatEngineConfig* chat_engine_cache_lookup_by_id(ChatEngineCache* cache, int engine_id) {
    if (!cache) return NULL;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire read lock for chat engine cache lookup", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ChatEngineConfig* result = NULL;

    // Linear search by ID
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i] && cache->engines[i]->engine_id == engine_id) {
            result = cache->engines[i];
            break;
        }
    }

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    if (result) {
        // Update usage statistics
        result->last_used = time(NULL);
        __sync_fetch_and_add(&result->usage_count, 1);
    }

    return result;
}

// Get default engine (read lock)
ChatEngineConfig* chat_engine_cache_get_default(ChatEngineCache* cache) {
    if (!cache) return NULL;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire read lock for chat engine cache lookup", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ChatEngineConfig* result = NULL;

    // Find first engine marked as default, or first engine if no default
    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i]) {
            if (cache->engines[i]->is_default) {
                result = cache->engines[i];
                break;
            }
            // Keep first engine as fallback
            if (!result) {
                result = cache->engines[i];
            }
        }
    }

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    if (result) {
        // Update usage statistics
        result->last_used = time(NULL);
        __sync_fetch_and_add(&result->usage_count, 1);
    }

    return result;
}

// Get all engines (returns a copy of the array - caller must free array but NOT entries)
ChatEngineConfig** chat_engine_cache_get_all(ChatEngineCache* cache, size_t* count) {
    if (!cache || !count) return NULL;

    *count = 0;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        log_this(SR_CHAT, "Failed to acquire read lock for chat engine cache", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Allocate array for results
    ChatEngineConfig** result = (ChatEngineConfig**)malloc(cache->engine_count * sizeof(ChatEngineConfig*));
    if (!result) {
        pthread_rwlock_unlock(&cache->cache_lock);
        return NULL;
    }

    // Copy engine pointers (shallow copy - entries still owned by cache)
    for (size_t i = 0; i < cache->engine_count; i++) {
        result[i] = cache->engines[i];
    }

    *count = cache->engine_count;

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    return result;
}

// Update usage statistics for an engine
void chat_engine_cache_update_usage(ChatEngineCache* cache, int engine_id) {
    ChatEngineConfig* engine = chat_engine_cache_lookup_by_id(cache, engine_id);
    if (engine) {
        engine->last_used = time(NULL);
        __sync_fetch_and_add(&engine->usage_count, 1);
    }
}

// Convert provider string to enum
ChatEngineProvider chat_engine_provider_from_string(const char* provider_str) {
    if (!provider_str) return CEC_PROVIDER_UNKNOWN;

    if (strcasecmp(provider_str, "openai") == 0) return CEC_PROVIDER_OPENAI;
    if (strcasecmp(provider_str, "anthropic") == 0) return CEC_PROVIDER_ANTHROPIC;
    if (strcasecmp(provider_str, "ollama") == 0) return CEC_PROVIDER_OLLAMA;

    // Check for OpenAI-compatible providers
    if (strcasecmp(provider_str, "xai") == 0) return CEC_PROVIDER_OPENAI;
    if (strcasecmp(provider_str, "gradient") == 0) return CEC_PROVIDER_OPENAI;

    return CEC_PROVIDER_UNKNOWN;
}

// Convert provider enum to string
const char* chat_engine_provider_to_string(ChatEngineProvider provider) {
    switch (provider) {
        case CEC_PROVIDER_OPENAI: return "openai";
        case CEC_PROVIDER_ANTHROPIC: return "anthropic";
        case CEC_PROVIDER_OLLAMA: return "ollama";
        case CEC_PROVIDER_UNKNOWN: return "unknown";
    }
    return "unknown";
}

// Get engine count
size_t chat_engine_cache_get_engine_count(ChatEngineCache* cache) {
    if (!cache) return 0;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        return 0;
    }

    size_t count = cache->engine_count;

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    return count;
}

// Get cache statistics
void chat_engine_cache_get_stats(ChatEngineCache* cache, char* buffer, size_t buffer_size) {
    if (!cache || !buffer || buffer_size == 0) return;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        snprintf(buffer, buffer_size, "Failed to acquire chat engine cache lock");
        return;
    }

    size_t total_usage = 0;
    time_t oldest = time(NULL);
    time_t newest = 0;
    size_t default_count = 0;

    for (size_t i = 0; i < cache->engine_count; i++) {
        if (cache->engines[i]) {
            total_usage += (size_t)cache->engines[i]->usage_count;
            if (cache->engines[i]->last_used < oldest) oldest = cache->engines[i]->last_used;
            if (cache->engines[i]->last_used > newest) newest = cache->engines[i]->last_used;
            if (cache->engines[i]->is_default) default_count++;
        }
    }

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    snprintf(buffer, buffer_size,
             "Chat engines: %zu (default: %zu), Total usage: %zu, Last refresh: %ld",
             cache->engine_count, default_count, total_usage, cache->last_refresh);
}

// Check if cache needs refresh
bool chat_engine_cache_needs_refresh(ChatEngineCache* cache, int refresh_interval_seconds) {
    if (!cache) return false;

    time_t now = time(NULL);
    int interval = refresh_interval_seconds > 0 ? refresh_interval_seconds : CEC_DEFAULT_REFRESH_INTERVAL;

    return (now - cache->last_refresh) > interval;
}

// Load chat engine configurations from database using internal QueryRef #061
// This is called during database bootstrap for databases with Chat enabled
bool chat_engine_cache_load_from_database(ChatEngineCache* cache, const char* database_name) {
    if (!cache || !database_name) {
        log_this(SR_CHAT, "Invalid parameters for chat engine cache load", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Check if chat is enabled for this database
    const DatabaseConnection* db_conn = find_database_connection(&app_config->databases, database_name);
    if (!db_conn || !db_conn->chat) {
        log_this(SR_CHAT, "Chat not enabled for database: %s", LOG_LEVEL_DEBUG, 1, database_name);
        return false;
    }

    // Get database queue
    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database_name);
    if (!db_queue) {
        log_this(SR_CHAT, "Database queue not found for: %s", LOG_LEVEL_ERROR, 1, database_name);
        return false;
    }

    // QueryRef #061 is the internal query for AI engine configurations
    // This query is type=0 (internal_sql) and can only be accessed internally
    QueryCacheEntry* cache_entry = query_cache_lookup(db_queue->query_cache, 61, NULL);
    if (!cache_entry) {
        log_this(SR_CHAT, "QueryRef #061 not found for database: %s", LOG_LEVEL_ALERT, 1, database_name);
        return false;
    }

    log_this(SR_CHAT, "Loading chat engine configurations from database: %s", LOG_LEVEL_DEBUG, 1, database_name);

    // Execute QueryRef #061 using the persistent connection
    if (!db_queue->persistent_connection) {
        log_this(SR_CHAT, "No persistent connection available for database: %s", LOG_LEVEL_ERROR, 1, database_name);
        return false;
    }

    QueryRequest* request = calloc(1, sizeof(QueryRequest));
    if (!request) {
        log_this(SR_CHAT, "Failed to allocate query request", LOG_LEVEL_ERROR, 0);
        return false;
    }

    request->query_id = strdup("chat_engine_bootstrap");
    request->sql_template = strdup(cache_entry->sql_template);
    request->parameters_json = strdup("{}");
    request->timeout_seconds = cache_entry->timeout_seconds > 0 ? cache_entry->timeout_seconds : 30;
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = false;

    QueryResult* result = NULL;
    bool query_success = database_engine_execute(db_queue->persistent_connection, request, &result);

    if (!query_success || !result || !result->success) {
        log_this(SR_CHAT, "Failed to execute QueryRef #061: %s", LOG_LEVEL_ERROR, 1,
                 result && result->error_message ? result->error_message : "Unknown error");
        free(request->query_id);
        free(request->sql_template);
        free(request->parameters_json);
        free(request);
        if (result) database_engine_cleanup_result(result);
        return false;
    }

    log_this(SR_CHAT, "QueryRef #061 executed successfully: %zu rows", LOG_LEVEL_DEBUG, 1, result->row_count);

    // Parse results and populate cache
    if (result->row_count > 0 && result->data_json) {
        json_error_t error;
        json_t* root = json_loads(result->data_json, 0, &error);

        if (!root || !json_is_array(root)) {
            log_this(SR_CHAT, "Failed to parse engine configuration JSON", LOG_LEVEL_ERROR, 0);
            if (root) json_decref(root);
        } else {
            // Clear existing cache before repopulating
            chat_engine_cache_clear(cache);

            size_t row_count = json_array_size(root);
            for (size_t i = 0; i < row_count; i++) {
                json_t* row = json_array_get(root, i);
                if (!json_is_object(row)) continue;

                // Extract engine configuration fields
                json_t* id_obj = json_object_get(row, "id");
                json_t* name_obj = json_object_get(row, "name");
                json_t* provider_obj = json_object_get(row, "provider");
                json_t* model_obj = json_object_get(row, "model");
                json_t* url_obj = json_object_get(row, "api_url");
                json_t* key_obj = json_object_get(row, "api_key");
                json_t* max_tokens_obj = json_object_get(row, "max_tokens");
                json_t* temp_obj = json_object_get(row, "temperature");
                json_t* is_default_obj = json_object_get(row, "is_default");

                if (!id_obj || !json_is_integer(id_obj) ||
                    !name_obj || !json_is_string(name_obj)) {
                    continue;
                }

                int engine_id = (int)json_integer_value(id_obj);
                const char* name = json_string_value(name_obj);
                const char* provider_str = provider_obj && json_is_string(provider_obj) ?
                                           json_string_value(provider_obj) : "openai";
                const char* model = model_obj && json_is_string(model_obj) ?
                                    json_string_value(model_obj) : name;
                const char* api_url = url_obj && json_is_string(url_obj) ?
                                      json_string_value(url_obj) : "";
                const char* api_key = key_obj && json_is_string(key_obj) ?
                                      json_string_value(key_obj) : "";
                int max_tokens = max_tokens_obj && json_is_integer(max_tokens_obj) ?
                                 (int)json_integer_value(max_tokens_obj) : 4096;
                double temperature = temp_obj && json_is_real(temp_obj) ?
                                     json_real_value(temp_obj) : 0.7;
                bool is_default = is_default_obj && json_is_boolean(is_default_obj) ?
                                  json_boolean_value(is_default_obj) : false;

                ChatEngineProvider provider = chat_engine_provider_from_string(provider_str);

                ChatEngineConfig* engine = chat_engine_config_create(
                    engine_id, name, provider, model, api_url, api_key,
                    max_tokens, temperature, is_default
                );

                if (engine) {
                    if (chat_engine_cache_add_engine(cache, engine)) {
                        log_this(SR_CHAT, "Added engine to cache: %s (provider: %s)",
                                 LOG_LEVEL_DEBUG, 2, name, provider_str);
                    } else {
                        chat_engine_config_destroy(engine);
                    }
                }
            }
            json_decref(root);
        }
    }

    // Cleanup
    free(request->query_id);
    free(request->sql_template);
    free(request->parameters_json);
    free(request);
    database_engine_cleanup_result(result);

    cache->last_refresh = time(NULL);
    size_t engine_count = chat_engine_cache_get_engine_count(cache);
    log_this(SR_CHAT, "Chat engine cache loaded for database %s: %zu engines",
             LOG_LEVEL_STATE, 2, database_name, engine_count);

    return engine_count > 0;
}

// Convenience function for database bootstrap
// Creates cache if needed and loads from database
bool chat_engine_cache_bootstrap_for_database(const char* database_name) {
    if (!database_name) return false;

    // Get database queue
    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database_name);
    if (!db_queue) {
        log_this(SR_CHAT, "Database queue not found for bootstrap: %s", LOG_LEVEL_ERROR, 1, database_name);
        return false;
    }

    // Check if chat is enabled for this database
    const DatabaseConnection* db_conn = find_database_connection(&app_config->databases, database_name);
    if (!db_conn || !db_conn->chat) {
        log_this(SR_CHAT, "Chat not enabled for database: %s", LOG_LEVEL_DEBUG, 1, database_name);
        return false;
    }

    // Create CEC if not exists
    if (!db_queue->chat_engine_cache) {
        db_queue->chat_engine_cache = chat_engine_cache_create(database_name);
        if (!db_queue->chat_engine_cache) {
            log_this(SR_CHAT, "Failed to create chat engine cache for: %s", LOG_LEVEL_ERROR, 1, database_name);
            return false;
        }
    } else {
        // Clear existing cache before repopulating
        chat_engine_cache_clear(db_queue->chat_engine_cache);
    }

    // Load from database
    return chat_engine_cache_load_from_database(db_queue->chat_engine_cache, database_name);
}

// Manual refresh - reloads cache from database
bool chat_engine_cache_refresh(ChatEngineCache* cache) {
    if (!cache || !cache->database_name) {
        log_this(SR_CHAT, "Cannot refresh: invalid cache or no database name", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(SR_CHAT, "Manually refreshing chat engine cache for: %s", LOG_LEVEL_STATE, 1, cache->database_name);
    return chat_engine_cache_load_from_database(cache, cache->database_name);
}

// Check if refresh is needed based on interval
bool chat_engine_cache_should_refresh(ChatEngineCache* cache, int refresh_interval_seconds) {
    if (!cache) return false;

    time_t now = time(NULL);
    int interval = refresh_interval_seconds > 0 ? refresh_interval_seconds : 3600; // Default 1 hour

    return (now - cache->last_refresh) >= interval;
}

// Get last refresh timestamp
time_t chat_engine_cache_get_last_refresh(ChatEngineCache* cache) {
    if (!cache) return 0;
    return cache->last_refresh;
}
