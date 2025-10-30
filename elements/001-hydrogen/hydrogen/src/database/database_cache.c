/*
 * Query Table Cache (QTC) Implementation
 *
 * Thread-safe in-memory cache for storing query templates loaded during bootstrap.
 */

#include "database_cache.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Initial capacity for cache entries array
#define INITIAL_CACHE_CAPACITY 64

// Create a new query cache
QueryTableCache* query_cache_create(void) {
    QueryTableCache* cache = (QueryTableCache*)malloc(sizeof(QueryTableCache));
    if (!cache) {
        log_this("DATABASE", "Failed to allocate memory for query cache", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize structure
    cache->entries = NULL;
    cache->entry_count = 0;
    cache->capacity = 0;

    // Initialize reader-writer lock
    if (pthread_rwlock_init(&cache->cache_lock, NULL) != 0) {
        log_this("DATABASE", "Failed to initialize cache lock", LOG_LEVEL_ERROR, 0);
        free(cache);
        return NULL;
    }

    // Allocate initial entries array
    cache->entries = (QueryCacheEntry**)malloc(INITIAL_CACHE_CAPACITY * sizeof(QueryCacheEntry*));
    if (!cache->entries) {
        log_this("DATABASE", "Failed to allocate memory for cache entries", LOG_LEVEL_ERROR, 0);
        pthread_rwlock_destroy(&cache->cache_lock);
        free(cache);
        return NULL;
    }

    cache->capacity = INITIAL_CACHE_CAPACITY;

    // Initialize all entries to NULL
    memset(cache->entries, 0, INITIAL_CACHE_CAPACITY * sizeof(QueryCacheEntry*));

    log_this("DATABASE", "Query cache created successfully", LOG_LEVEL_DEBUG, 0);
    return cache;
}

// Destroy query cache and all entries
void query_cache_destroy(QueryTableCache* cache) {
    if (!cache) return;

    // Acquire write lock for destruction
    pthread_rwlock_wrlock(&cache->cache_lock);

    // Free all entries
    for (size_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i]) {
            query_cache_entry_destroy(cache->entries[i]);
        }
    }

    // Free entries array
    free(cache->entries);

    // Unlock before destroying
    pthread_rwlock_unlock(&cache->cache_lock);

    // Destroy lock
    pthread_rwlock_destroy(&cache->cache_lock);

    // Free cache structure
    free(cache);

    log_this("DATABASE", "Query cache destroyed", LOG_LEVEL_DEBUG, 0);
}

// Create a new cache entry
QueryCacheEntry* query_cache_entry_create(int query_ref, const char* sql_template,
                                         const char* description, const char* queue_type,
                                         int timeout_seconds) {
    QueryCacheEntry* entry = (QueryCacheEntry*)malloc(sizeof(QueryCacheEntry));
    if (!entry) {
        log_this("DATABASE", "Failed to allocate memory for cache entry", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize structure
    entry->query_ref = query_ref;
    entry->timeout_seconds = timeout_seconds;
    entry->last_used = time(NULL);
    entry->usage_count = 0;

    // Duplicate strings
    entry->sql_template = sql_template ? strdup(sql_template) : NULL;
    entry->description = description ? strdup(description) : NULL;
    entry->queue_type = queue_type ? strdup(queue_type) : NULL;

    // Check for allocation failures
    if ((sql_template && !entry->sql_template) ||
        (description && !entry->description) ||
        (queue_type && !entry->queue_type)) {
        log_this("DATABASE", "Failed to allocate memory for cache entry strings", LOG_LEVEL_ERROR, 0);
        query_cache_entry_destroy(entry);
        return NULL;
    }

    return entry;
}

// Destroy a cache entry
void query_cache_entry_destroy(QueryCacheEntry* entry) {
    if (!entry) return;

    free(entry->sql_template);
    free(entry->description);
    free(entry->queue_type);
    free(entry);
}

// Add entry to cache (assumes write lock is held)
static bool query_cache_add_entry_locked(QueryTableCache* cache, QueryCacheEntry* entry) {
    // Check if we need to resize
    if (cache->entry_count >= cache->capacity) {
        size_t new_capacity = cache->capacity * 2;
        QueryCacheEntry** new_entries = (QueryCacheEntry**)realloc(cache->entries,
                                                                   new_capacity * sizeof(QueryCacheEntry*));
        if (!new_entries) {
            log_this("DATABASE", "Failed to resize cache entries array", LOG_LEVEL_ERROR, 0);
            return false;
        }

        cache->entries = new_entries;
        cache->capacity = new_capacity;

        // Initialize new entries to NULL
        memset(cache->entries + cache->entry_count, 0,
               (new_capacity - cache->entry_count) * sizeof(QueryCacheEntry*));
    }

    // Add entry to array
    cache->entries[cache->entry_count++] = entry;

    return true;
}

// Add entry to cache with proper locking
bool query_cache_add_entry(QueryTableCache* cache, QueryCacheEntry* entry) {
    if (!cache || !entry) {
        log_this("DATABASE", "Invalid parameters for cache add entry", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Acquire write lock
    if (pthread_rwlock_wrlock(&cache->cache_lock) != 0) {
        log_this("DATABASE", "Failed to acquire write lock for cache", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool result = query_cache_add_entry_locked(cache, entry);

    // Release write lock
    pthread_rwlock_unlock(&cache->cache_lock);

    if (result) {
        log_this("DATABASE", "Added query entry to cache", LOG_LEVEL_DEBUG, 0);
    }

    return result;
}

// Lookup entry by query_ref (read lock)
QueryCacheEntry* query_cache_lookup(QueryTableCache* cache, int query_ref) {
    if (!cache) return NULL;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        log_this("DATABASE", "Failed to acquire read lock for cache lookup", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    QueryCacheEntry* result = NULL;

    // Linear search (could be optimized with hash map if needed)
    for (size_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i] && cache->entries[i]->query_ref == query_ref) {
            result = cache->entries[i];
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

// Update usage statistics for a query
void query_cache_update_usage(QueryTableCache* cache, int query_ref) {
    QueryCacheEntry* entry = query_cache_lookup(cache, query_ref);
    if (entry) {
        entry->last_used = time(NULL);
        __sync_fetch_and_add(&entry->usage_count, 1);
    }
}

// Get entry count
size_t query_cache_get_entry_count(QueryTableCache* cache) {
    if (!cache) return 0;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        return 0;
    }

    size_t count = cache->entry_count;

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    return count;
}

// Get cache statistics
void query_cache_get_stats(QueryTableCache* cache, char* buffer, size_t buffer_size) {
    if (!cache || !buffer || buffer_size == 0) return;

    // Acquire read lock
    if (pthread_rwlock_rdlock(&cache->cache_lock) != 0) {
        snprintf(buffer, buffer_size, "Failed to acquire cache lock");
        return;
    }

    size_t total_usage = 0;
    time_t oldest = time(NULL);
    time_t newest = 0;

    for (size_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i]) {
            total_usage += (size_t)cache->entries[i]->usage_count;
            if (cache->entries[i]->last_used < oldest) oldest = cache->entries[i]->last_used;
            if (cache->entries[i]->last_used > newest) newest = cache->entries[i]->last_used;
        }
    }

    // Release read lock
    pthread_rwlock_unlock(&cache->cache_lock);

    snprintf(buffer, buffer_size,
             "Cache entries: %zu, Total usage: %zu, Oldest: %ld, Newest: %ld",
             cache->entry_count, total_usage, oldest, newest);
}