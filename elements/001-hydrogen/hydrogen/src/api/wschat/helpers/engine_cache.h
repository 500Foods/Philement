/*
 * Chat Engine Cache (CEC) Implementation
 *
 * Implements in-memory cache for storing AI engine configurations loaded during bootstrap.
 * Provides thread-safe lookup and management of chat engine settings per database.
 */

#ifndef ENGINE_CACHE_H
#define ENGINE_CACHE_H

// Project includes
#include <src/hydrogen.h>

// Maximum lengths for engine configuration fields
#define CEC_MAX_NAME_LEN        64
#define CEC_MAX_PROVIDER_LEN    32
#define CEC_MAX_MODEL_LEN       128
#define CEC_MAX_URL_LEN         512
#define CEC_MAX_KEY_LEN         256

// Chat engine provider types
typedef enum {
    CEC_PROVIDER_OPENAI = 0,     // OpenAI-compatible API (OpenAI, xAI, Gradient)
    CEC_PROVIDER_ANTHROPIC,      // Anthropic Claude native API
    CEC_PROVIDER_OLLAMA,         // Ollama local API
    CEC_PROVIDER_UNKNOWN
} ChatEngineProvider;

// Supported modalities bitmask
typedef enum {
    MODALITY_TEXT = 1 << 0,      // Text generation
    MODALITY_IMAGE = 1 << 1,     // Image understanding/processing
    MODALITY_AUDIO = 1 << 2,     // Audio processing
    MODALITY_PDF = 1 << 3,       // PDF document processing
    MODALITY_DOCUMENT = 1 << 4   // Generic document processing
} ChatModality;

#define MODALITY_COUNT 5
#define MODALITY_ALL (MODALITY_TEXT | MODALITY_IMAGE | MODALITY_AUDIO | MODALITY_PDF | MODALITY_DOCUMENT)
#define MODALITY_DEFAULT MODALITY_TEXT

// Chat engine configuration entry
typedef struct ChatEngineConfig {
    int engine_id;                    // Unique engine identifier (from database)
    char name[CEC_MAX_NAME_LEN];      // Engine name (e.g., "gpt4", "claude")
    ChatEngineProvider provider;      // Provider type
    char model[CEC_MAX_MODEL_LEN];    // Model identifier (e.g., "gpt-4-turbo")
    char api_url[CEC_MAX_URL_LEN];    // API endpoint URL
    char api_key[CEC_MAX_KEY_LEN];    // API key (encrypted in memory, cleared after use)
    int max_tokens;                   // Maximum tokens per request
    double temperature_default;       // Default temperature (0.0 - 2.0)
    bool is_default;                  // Whether this is the default engine
    time_t last_used;                 // LRU tracking
    volatile int usage_count;         // Usage statistics

    // Health tracking (Phase 2.5)
    int liveliness_seconds;           // Health check interval (0 = disabled)
    time_t last_health_check;         // Timestamp of last health check
    bool is_healthy;                  // Current health status
    int consecutive_failures;         // Count of consecutive failed health checks
    time_t last_confirmed_working;    // Timestamp of last successful request
    double avg_response_time_ms;      // Rolling average response time
    unsigned long long conversations_24h;  // Conversation count (24h)
    unsigned long long tokens_24h;         // Token usage count (24h)
    pthread_mutex_t health_mutex;     // Protect health fields

    // Engine limits (Phase 3)
    int max_images_per_message;       // Maximum images allowed per message
    int max_payload_mb;               // Maximum request payload size in MB
    int max_concurrent_requests;      // Maximum concurrent requests per engine

    // Provider-specific settings (Phase 5)
    bool use_native_api;              // Use provider's native API (not OpenAI-compatible)
    
    // Supported modalities (Phase 12)
    int supported_modalities;         // Bitmask of ChatModality values
} ChatEngineConfig;

// Chat Engine Cache structure (per database)
typedef struct ChatEngineCache {
    ChatEngineConfig** engines;       // Array of engine configurations
    size_t engine_count;              // Number of engines
    size_t capacity;                  // Allocated capacity
    pthread_rwlock_t cache_lock;      // Reader-writer lock for concurrent access
    time_t last_refresh;              // Last refresh timestamp
    char* database_name;              // Associated database name
    
    // Health monitoring (Phase 2.5)
    pthread_t health_monitor_thread;  // Health check thread
    volatile bool health_monitor_running;  // Thread control flag
    int health_monitor_interval;      // Seconds between health checks
} ChatEngineCache;

// Function prototypes

// Create and destroy cache
ChatEngineCache* chat_engine_cache_create(const char* database_name);
void chat_engine_cache_destroy(ChatEngineCache* cache);
void chat_engine_cache_clear(ChatEngineCache* cache);

// Entry management
bool chat_engine_cache_add_engine(ChatEngineCache* cache, ChatEngineConfig* engine);
ChatEngineConfig* chat_engine_cache_lookup_by_name(ChatEngineCache* cache, const char* name);
ChatEngineConfig* chat_engine_cache_lookup_by_id(ChatEngineCache* cache, int engine_id);
ChatEngineConfig* chat_engine_cache_get_default(ChatEngineCache* cache);
ChatEngineConfig** chat_engine_cache_get_all(ChatEngineCache* cache, size_t* count);
void chat_engine_cache_update_usage(ChatEngineCache* cache, int engine_id);

// Entry creation and cleanup
ChatEngineConfig* chat_engine_config_create(int engine_id, const char* name, ChatEngineProvider provider,
                                           const char* model, const char* api_url, const char* api_key,
                                           int max_tokens, double temperature_default, bool is_default,
                                           int liveliness_seconds, int max_images_per_message,
                                           int max_payload_mb, int max_concurrent_requests,
                                           int supported_modalities, bool use_native_api);
void chat_engine_config_destroy(ChatEngineConfig* engine);
void chat_engine_config_clear_key(ChatEngineConfig* engine);

// Health tracking functions
void chat_engine_config_update_health(ChatEngineConfig* engine, bool success, double response_time_ms);
void chat_engine_config_mark_health_checked(ChatEngineConfig* engine, bool is_healthy);
const char* chat_engine_config_get_status(ChatEngineConfig* engine);

// Provider type helpers
ChatEngineProvider chat_engine_provider_from_string(const char* provider_str);
const char* chat_engine_provider_to_string(ChatEngineProvider provider);

// Statistics and monitoring
size_t chat_engine_cache_get_engine_count(ChatEngineCache* cache);
void chat_engine_cache_get_stats(ChatEngineCache* cache, char* buffer, size_t buffer_size);
bool chat_engine_cache_needs_refresh(const ChatEngineCache* cache, int refresh_interval_seconds);

// Bootstrap and refresh
bool chat_engine_cache_load_from_database(ChatEngineCache* cache, const char* database_name);

// Convenience function for database bootstrap
// Creates cache if needed and loads from database
bool chat_engine_cache_bootstrap_for_database(const char* database_name);

// Manual refresh - reloads cache from database
bool chat_engine_cache_refresh(ChatEngineCache* cache);

// Check if refresh is needed based on interval
bool chat_engine_cache_should_refresh(const ChatEngineCache* cache, int refresh_interval_seconds);

// Get last refresh timestamp
time_t chat_engine_cache_get_last_refresh(const ChatEngineCache* cache);

#endif // ENGINE_CACHE_H
