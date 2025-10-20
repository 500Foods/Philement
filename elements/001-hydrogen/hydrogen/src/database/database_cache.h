/*
 * Query Table Cache (QTC) Implementation
 *
 * Implements in-memory cache for storing query templates loaded during bootstrap.
 * Provides thread-safe lookup and management of pre-defined SQL queries.
 */

#ifndef DATABASE_CACHE_H
#define DATABASE_CACHE_H

// Project includes
#include <src/hydrogen.h>

// Query cache entry structure
typedef struct QueryCacheEntry {
    int query_ref;                 // Unique query identifier
    char* sql_template;            // SQL with named parameters (e.g., :userId)
    char* description;             // Human-readable description for logging
    char* queue_type;              // Recommended queue: "slow", "medium", "fast", "cache"
    int timeout_seconds;           // Query-specific timeout
    time_t last_used;              // LRU tracking for future optimization
    volatile int usage_count;      // Usage statistics
} QueryCacheEntry;

// Query table cache structure
typedef struct QueryTableCache {
    QueryCacheEntry** entries;     // Array of cache entries
    size_t entry_count;            // Number of entries
    size_t capacity;               // Allocated capacity
    pthread_rwlock_t cache_lock;   // Reader-writer lock for concurrent access
} QueryTableCache;

// Function prototypes

// Create and destroy cache
QueryTableCache* query_cache_create(void);
void query_cache_destroy(QueryTableCache* cache);

// Entry management
bool query_cache_add_entry(QueryTableCache* cache, QueryCacheEntry* entry);
QueryCacheEntry* query_cache_lookup(QueryTableCache* cache, int query_ref);
void query_cache_update_usage(QueryTableCache* cache, int query_ref);

// Entry creation and cleanup
QueryCacheEntry* query_cache_entry_create(int query_ref, const char* sql_template,
                                         const char* description, const char* queue_type,
                                         int timeout_seconds);
void query_cache_entry_destroy(QueryCacheEntry* entry);

// Statistics and monitoring
size_t query_cache_get_entry_count(QueryTableCache* cache);
void query_cache_get_stats(QueryTableCache* cache, char* buffer, size_t buffer_size);

#endif // DATABASE_CACHE_H