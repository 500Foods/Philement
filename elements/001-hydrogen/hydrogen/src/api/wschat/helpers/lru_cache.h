/*
 * Chat LRU Disk Cache - Phase 9
 *
 * Provides a local disk cache for hot conversation segments to avoid
 * repeated decompression and improve performance for active conversations.
 *
 * Features:
 * - 1GB storage limit with LRU eviction
 * - Uncompressed segment storage for fast access
 * - Organized by hash prefixes for efficient lookup
 * - Background sync to database
 * - Thread-safe operations
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <src/hydrogen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <jansson.h>

/* Cache configuration defaults */
#define LRU_CACHE_MAX_SIZE_BYTES     (1024ULL * 1024ULL * 1024ULL)  /* 1GB */
#define LRU_CACHE_DIR_NAME           "cache"       /* Relative to cwd */
#define LRU_CACHE_METADATA_FILE      "cache_metadata.json"
#define LRU_CACHE_MAX_HASH_LEN       64
#define LRU_CACHE_PREFIX_LEN         2   /* First 2 chars for directory */
#define LRU_CACHE_SYNC_INTERVAL_SEC  60  /* Background sync interval */
#define CHAT_CACHE_DIR_ENV_VAR            "CHAT_CACHE_DIR"  /* Override cache dir */

/* Hash table size - power of 2 for fast modulo */
#define LRU_CACHE_HASH_TABLE_SIZE    4096

/* LRU Cache entry */
typedef struct ChatLRUCacheEntry {
    char hash[LRU_CACHE_MAX_HASH_LEN];     /* Segment hash (key) */
    char* content;                               /* Uncompressed JSON content */
    size_t content_size;                         /* Size of content in bytes */
    time_t last_accessed;                        /* Last access timestamp */
    time_t created_at;                           /* Creation timestamp */
    int access_count;                            /* Number of times accessed */
    bool is_dirty;                               /* Needs sync to database */

    struct ChatLRUCacheEntry* lru_prev;          /* LRU list prev */
    struct ChatLRUCacheEntry* lru_next;          /* LRU list next */
    struct ChatLRUCacheEntry* hash_next;         /* Hash table collision chain */
} ChatLRUCacheEntry;

/* LRU Cache statistics */
typedef struct ChatLRUCacheStats {
    size_t total_entries;            /* Number of entries in cache */
    size_t total_size_bytes;         /* Current cache size in bytes */
    size_t max_size_bytes;           /* Maximum cache size */
    uint64_t cache_hits;             /* Total cache hits */
    uint64_t cache_misses;           /* Total cache misses */
    uint64_t evictions;              /* Total evictions */
    uint64_t syncs_completed;        /* Background syncs completed */
    double hit_ratio;                /* Current hit ratio */
} ChatLRUCacheStats;

/* LRU Cache structure */
typedef struct ChatLRUCache {
    char* cache_dir;                             /* Base cache directory */
    char* database;                              /* Database name */
    size_t max_size_bytes;                       /* Maximum cache size */
    size_t current_size_bytes;                   /* Current cache size */

    /* Hash table for O(1) lookup */
    ChatLRUCacheEntry** hash_table;              /* Hash table buckets */

    /* LRU doubly-linked list */
    ChatLRUCacheEntry* lru_head;                 /* Most recently used */
    ChatLRUCacheEntry* lru_tail;                 /* Least recently used */

    /* Statistics */
    ChatLRUCacheStats stats;

    /* Thread safety */
    pthread_mutex_t cache_mutex;
    bool mutex_initialized;

    /* Background sync */
    pthread_t sync_thread;
    bool sync_running;
    bool sync_requested;                         /* Dirty entries need sync */
} ChatLRUCache;

/**
 * Initialize the LRU cache for a database
 *
 * @param database Database name
 * @param max_size_bytes Maximum cache size (0 for default 1GB)
 * @return Initialized cache, or NULL on error
 */
ChatLRUCache* chat_lru_cache_init(const char* database, size_t max_size_bytes);

/**
 * Shutdown the LRU cache and free resources
 *
 * @param cache Cache to shutdown
 */
void chat_lru_cache_shutdown(ChatLRUCache* cache);

/**
 * Check if a segment exists in the cache
 *
 * @param cache Cache instance
 * @param segment_hash Segment hash to look up
 * @return true if found in cache, false otherwise
 */
bool chat_lru_cache_contains(ChatLRUCache* cache, const char* segment_hash);

/**
 * Retrieve a segment from the cache
 *
 * @param cache Cache instance
 * @param segment_hash Segment hash to retrieve
 * @return Allocated content string, or NULL if not found
 * @note Caller must free the returned string
 */
char* chat_lru_cache_get(ChatLRUCache* cache, const char* segment_hash);

/**
 * Store a segment in the cache
 *
 * @param cache Cache instance
 * @param segment_hash Segment hash
 * @param content Uncompressed content to store
 * @param content_size Size of content
 * @param is_dirty Whether content needs sync to database
 * @return true on success, false on error
 */
bool chat_lru_cache_put(ChatLRUCache* cache, const char* segment_hash,
                        const char* content, size_t content_size, bool is_dirty);

/**
 * Remove a segment from the cache
 *
 * @param cache Cache instance
 * @param segment_hash Segment hash to remove
 * @return true if removed, false if not found
 */
bool chat_lru_cache_remove(ChatLRUCache* cache, const char* segment_hash);

/**
 * Get cache statistics
 *
 * @param cache Cache instance
 * @param stats Output statistics structure
 * @return true on success, false on error
 */
bool chat_lru_cache_get_stats(ChatLRUCache* cache, ChatLRUCacheStats* stats);

/**
 * Request background sync of dirty entries
 *
 * @param cache Cache instance
 */
void chat_lru_cache_request_sync(ChatLRUCache* cache);

/**
 * Flush all dirty entries to database (blocking)
 *
 * @param cache Cache instance
 * @return Number of entries synced, or -1 on error
 */
int chat_lru_cache_flush(ChatLRUCache* cache);

/**
 * Clear all entries from cache
 *
 * @param cache Cache instance
 * @return true on success, false on error
 */
bool chat_lru_cache_clear(ChatLRUCache* cache);

/**
 * Get the cache directory path for a database
 *
 * @param database Database name
 * @return Allocated path string, or NULL on error
 * @note Caller must free the returned string
 */
char* chat_lru_cache_get_dir(const char* database);

#endif /* LRU_CACHE_H */
