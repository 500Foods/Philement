/*
 * Chat LRU Disk Cache Implementation - Phase 9
 *
 * Provides a local disk cache for hot conversation segments to avoid
 * repeated decompression and improve performance for active conversations.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>

#include <jansson.h>

#include "lru_cache.h"
#include <src/logging/logging.h>

static const char* SR_CHAT_LRU = "CHAT_LRU_CACHE";

/* Get cache base directory from env var or default */
static const char* get_cache_base_dir(void) {
    const char* env_dir = getenv(CHAT_CACHE_DIR_ENV_VAR);
    return env_dir ? env_dir : LRU_CACHE_DIR_NAME;
}

/* Hash function for hash table (DJB2) */
static size_t hash_string(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + (size_t)c;
    }
    return hash % LRU_CACHE_HASH_TABLE_SIZE;
}

/* Create cache directory path */
char* chat_lru_cache_get_dir(const char* database) {
    if (!database) {
        return NULL;
    }
    /* database parameter is currently unused due to no topology awareness */
    const char* base_dir = get_cache_base_dir();
    size_t path_len = strlen(base_dir) + 2;

    char* path = calloc(path_len, 1);
    if (!path) {
        return NULL;
    }

    snprintf(path, path_len, "%s", base_dir);

    return path;
}

/* Ensure directory exists */
static bool ensure_directory_exists(const char* path) {
    if (!path) {
        return false;
    }

    struct stat st = {0};
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    /* Create directory recursively */
    char* path_copy = strdup(path);
    if (!path_copy) {
        return false;
    }

    char* dir = path_copy;
    char* p = dir;

    /* Skip leading slash */
    if (*p == '/') p++;

    while (*p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
                free(path_copy);
                return false;
            }
            *p = '/';
        }
        p++;
    }

    /* Create final directory */
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        free(path_copy);
        return false;
    }

    free(path_copy);
    return true;
}

/* Get path to segment file */
static char* get_segment_path(const char* cache_dir, const char* hash) {
    if (!cache_dir || !hash || strlen(hash) < LRU_CACHE_PREFIX_LEN) {
        return NULL;
    }

    /* Use first 2 chars as subdirectory for distribution */
    char prefix[LRU_CACHE_PREFIX_LEN + 1];
    memcpy(prefix, hash, LRU_CACHE_PREFIX_LEN);
    prefix[LRU_CACHE_PREFIX_LEN] = '\0';

    size_t path_len = strlen(cache_dir) + strlen(prefix) + strlen(hash) + 10;
    char* path = calloc(path_len, 1);
    if (!path) {
        return NULL;
    }

    snprintf(path, path_len, "%s/%s/%s.json", cache_dir, prefix, hash);
    return path;
}

/* Get path to metadata file */
static char* get_metadata_path(const char* cache_dir) {
    if (!cache_dir) {
        return NULL;
    }

    size_t path_len = strlen(cache_dir) + strlen("/") + strlen(LRU_CACHE_METADATA_FILE) + 2;
    char* path = calloc(path_len, 1);
    if (!path) {
        return NULL;
    }

    snprintf(path, path_len, "%s/%s", cache_dir, LRU_CACHE_METADATA_FILE);
    return path;
}

/* Free a cache entry */
static void free_cache_entry(ChatLRUCacheEntry* entry) {
    if (entry) {
        free(entry->content);
        free(entry);
    }
}

/* Remove entry from LRU list */
static void lru_remove(ChatLRUCache* cache, ChatLRUCacheEntry* entry) {
    if (!cache || !entry) {
        return;
    }

    if (entry->lru_prev) {
        entry->lru_prev->lru_next = entry->lru_next;
    } else {
        cache->lru_head = entry->lru_next;
    }

    if (entry->lru_next) {
        entry->lru_next->lru_prev = entry->lru_prev;
    } else {
        cache->lru_tail = entry->lru_prev;
    }

    entry->lru_prev = NULL;
    entry->lru_next = NULL;
}

/* Add entry to front of LRU list (most recently used) */
static void lru_add_front(ChatLRUCache* cache, ChatLRUCacheEntry* entry) {
    if (!cache || !entry) {
        return;
    }

    entry->lru_prev = NULL;
    entry->lru_next = cache->lru_head;

    if (cache->lru_head) {
        cache->lru_head->lru_prev = entry;
    }
    cache->lru_head = entry;

    if (!cache->lru_tail) {
        cache->lru_tail = entry;
    }
}

/* Evict least recently used entries until under size limit */
static bool evict_lru_entries(ChatLRUCache* cache, size_t needed_bytes) {
    if (!cache) {
        return false;
    }

    while (cache->current_size_bytes + needed_bytes > cache->max_size_bytes &&
           cache->lru_tail) {
        ChatLRUCacheEntry* victim = cache->lru_tail;

        log_this(SR_CHAT_LRU, "Evicting LRU entry: %s (size: %zu)",
                 LOG_LEVEL_DEBUG, 2, victim->hash, victim->content_size);

        /* Remove from hash table */
        size_t bucket = hash_string(victim->hash);
        ChatLRUCacheEntry** pp = &cache->hash_table[bucket];
        while (*pp) {
            if (*pp == victim) {
                *pp = victim->hash_next;
                break;
            }
            pp = &(*pp)->hash_next;
        }

        /* Remove from LRU list */
        lru_remove(cache, victim);

        /* Update stats */
        cache->current_size_bytes -= victim->content_size;
        cache->stats.evictions++;

        /* Remove file from disk */
        char* path = get_segment_path(cache->cache_dir, victim->hash);
        if (path) {
            unlink(path);
            free(path);
        }

        free_cache_entry(victim);
    }

    return cache->current_size_bytes + needed_bytes <= cache->max_size_bytes;
}

/* Save metadata to disk */
static bool save_metadata(ChatLRUCache* cache) {
    if (!cache || !cache->cache_dir) {
        return false;
    }

    char* meta_path = get_metadata_path(cache->cache_dir);
    if (!meta_path) {
        return false;
    }

    /* Ensure directory exists */
    ensure_directory_exists(cache->cache_dir);

    json_t* root = json_object();
    json_object_set_new(root, "database", json_string(cache->database));
    json_object_set_new(root, "max_size_bytes", json_integer((json_int_t)cache->max_size_bytes));
    json_object_set_new(root, "current_size_bytes", json_integer((json_int_t)cache->current_size_bytes));
    json_object_set_new(root, "total_entries", json_integer((json_int_t)cache->stats.total_entries));
    json_object_set_new(root, "cache_hits", json_integer((json_int_t)cache->stats.cache_hits));
    json_object_set_new(root, "cache_misses", json_integer((json_int_t)cache->stats.cache_misses));
    json_object_set_new(root, "evictions", json_integer((json_int_t)cache->stats.evictions));

    /* Save LRU order (hashes only) */
    json_t* lru_order = json_array();
    ChatLRUCacheEntry* entry = cache->lru_head;
    while (entry) {
        json_array_append_new(lru_order, json_string(entry->hash));
        entry = entry->lru_next;
    }
    json_object_set_new(root, "lru_order", lru_order);

    char* json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);

    if (json_str) {
        FILE* fp = fopen(meta_path, "w");
        if (fp) {
            fputs(json_str, fp);
            fclose(fp);
            free(json_str);
            free(meta_path);
            return true;
        }
        free(json_str);
    }

    free(meta_path);
    return false;
}

/* Load metadata from disk */
static bool load_metadata(ChatLRUCache* cache) {
    if (!cache || !cache->cache_dir) {
        return false;
    }

    char* meta_path = get_metadata_path(cache->cache_dir);
    if (!meta_path) {
        return false;
    }

    FILE* fp = fopen(meta_path, "r");
    if (!fp) {
        free(meta_path);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fp);
        free(meta_path);
        return false;
    }

    char* json_str = calloc((size_t)file_size + 1, 1);
    if (!json_str) {
        fclose(fp);
        free(meta_path);
        return false;
    }

    fread(json_str, 1, (size_t)file_size, fp);
    fclose(fp);
    free(meta_path);

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    free(json_str);

    if (!root) {
        return false;
    }

    /* Load statistics */
    json_t* obj = json_object_get(root, "total_entries");
    if (obj) cache->stats.total_entries = (size_t)json_integer_value(obj);

    obj = json_object_get(root, "cache_hits");
    if (obj) cache->stats.cache_hits = (uint64_t)json_integer_value(obj);

    obj = json_object_get(root, "cache_misses");
    if (obj) cache->stats.cache_misses = (uint64_t)json_integer_value(obj);

    obj = json_object_get(root, "evictions");
    if (obj) cache->stats.evictions = (uint64_t)json_integer_value(obj);

    json_decref(root);
    return true;
}

/* Background sync thread */
static void* sync_thread_func(void* arg) {
    ChatLRUCache* cache = (ChatLRUCache*)arg;
    if (!cache) {
        return NULL;
    }

    log_this(SR_CHAT_LRU, "Background sync thread started for: %s",
             LOG_LEVEL_DEBUG, 1, cache->database);

    while (cache->sync_running) {
        /* Sleep for sync interval */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += LRU_CACHE_SYNC_INTERVAL_SEC;

        pthread_mutex_lock(&cache->cache_mutex);
        bool should_sync = cache->sync_requested;
        cache->sync_requested = false;
        pthread_mutex_unlock(&cache->cache_mutex);

        if (should_sync) {
            log_this(SR_CHAT_LRU, "Background sync executing for: %s",
                     LOG_LEVEL_DEBUG, 1, cache->database);

            int synced = chat_lru_cache_flush(cache);
            if (synced > 0) {
                cache->stats.syncs_completed++;
                log_this(SR_CHAT_LRU, "Background sync completed: %d entries",
                         LOG_LEVEL_DEBUG, 1, synced);
            }
        }

        /* Save metadata periodically */
        save_metadata(cache);

        /* Wait for next sync or shutdown */
        pthread_mutex_lock(&cache->cache_mutex);
        if (cache->sync_running) {
            struct timespec wake_time;
            clock_gettime(CLOCK_REALTIME, &wake_time);
            wake_time.tv_sec += LRU_CACHE_SYNC_INTERVAL_SEC;

            /* pthread_cond_timedwait would be better but keeping it simple */
            pthread_mutex_unlock(&cache->cache_mutex);

            /* Sleep in small intervals to allow quick shutdown */
            for (int i = 0; i < LRU_CACHE_SYNC_INTERVAL_SEC && cache->sync_running; i++) {
                sleep(1);
            }
        } else {
            pthread_mutex_unlock(&cache->cache_mutex);
        }
    }

    log_this(SR_CHAT_LRU, "Background sync thread stopped for: %s",
             LOG_LEVEL_DEBUG, 1, cache->database);

    return NULL;
}

/* Initialize the LRU cache */
ChatLRUCache* chat_lru_cache_init(const char* database, size_t max_size_bytes) {
    if (!database) {
        log_this(SR_CHAT_LRU, "Invalid parameters for init", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    ChatLRUCache* cache = calloc(1, sizeof(ChatLRUCache));
    if (!cache) {
        log_this(SR_CHAT_LRU, "Failed to allocate cache", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    cache->database = strdup(database);
    cache->max_size_bytes = max_size_bytes > 0 ? max_size_bytes : LRU_CACHE_MAX_SIZE_BYTES;
    cache->current_size_bytes = 0;

    /* Allocate hash table */
    cache->hash_table = calloc(LRU_CACHE_HASH_TABLE_SIZE, sizeof(ChatLRUCacheEntry*));
    if (!cache->hash_table) {
        free(cache->database);
        free(cache);
        return NULL;
    }

    /* Create cache directory (no database subdirectory - shared cache) */
    cache->cache_dir = chat_lru_cache_get_dir(database);
    if (!cache->cache_dir) {
        free(cache->hash_table);
        free(cache->database);
        free(cache);
        return NULL;
    }

    if (!ensure_directory_exists(cache->cache_dir)) {
        log_this(SR_CHAT_LRU, "Failed to create cache directory: %s",
                 LOG_LEVEL_ERROR, 1, cache->cache_dir);
        free(cache->cache_dir);
        free(cache->hash_table);
        free(cache->database);
        free(cache);
        return NULL;
    }

    /* Create subdirectories for hash prefixes (00-ff) - two-tier structure */
    for (int i = 0; i < 256; i++) {
        char prefix_dir[256];
        snprintf(prefix_dir, sizeof(prefix_dir), "%s/%02x", cache->cache_dir, i);
        ensure_directory_exists(prefix_dir);
    }

    /* Initialize mutex */
    if (pthread_mutex_init(&cache->cache_mutex, NULL) != 0) {
        log_this(SR_CHAT_LRU, "Failed to initialize mutex", LOG_LEVEL_ERROR, 0);
        free(cache->cache_dir);
        free(cache->hash_table);
        free(cache->database);
        free(cache);
        return NULL;
    }
    cache->mutex_initialized = true;

    /* Load existing metadata */
    load_metadata(cache);

    /* Start background sync thread */
    cache->sync_running = true;
    cache->sync_requested = false;
    if (pthread_create(&cache->sync_thread, NULL, sync_thread_func, cache) != 0) {
        log_this(SR_CHAT_LRU, "Failed to start sync thread", LOG_LEVEL_ALERT, 0);
        /* Non-fatal - continue without background sync */
        cache->sync_running = false;
    }

    log_this(SR_CHAT_LRU, "LRU cache initialized for: %s (max: %zu MB)",
             LOG_LEVEL_STATE, 2, database, cache->max_size_bytes / (1024 * 1024));

    return cache;
}

/* Shutdown the LRU cache */
void chat_lru_cache_shutdown(ChatLRUCache* cache) {
    if (!cache) {
        return;
    }

    log_this(SR_CHAT_LRU, "Shutting down LRU cache for: %s",
             LOG_LEVEL_DEBUG, 1, cache->database);

    /* Stop background sync thread */
    if (cache->sync_running) {
        cache->sync_running = false;
        pthread_join(cache->sync_thread, NULL);
    }

    /* Final sync of dirty entries */
    chat_lru_cache_flush(cache);

    /* Save metadata */
    save_metadata(cache);

    /* Free all entries */
    ChatLRUCacheEntry* entry = cache->lru_head;
    while (entry) {
        ChatLRUCacheEntry* next = entry->lru_next;
        free_cache_entry(entry);
        entry = next;
    }

    /* Cleanup */
    if (cache->mutex_initialized) {
        pthread_mutex_destroy(&cache->cache_mutex);
    }

    free(cache->hash_table);
    free(cache->cache_dir);
    free(cache->database);
    free(cache);
}

/* Check if segment exists in cache */
bool chat_lru_cache_contains(ChatLRUCache* cache, const char* segment_hash) {
    if (!cache || !segment_hash) {
        return false;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    size_t bucket = hash_string(segment_hash);
    const ChatLRUCacheEntry* entry = cache->hash_table[bucket];

    bool found = false;
    while (entry) {
        if (strcmp(entry->hash, segment_hash) == 0) {
            found = true;
            break;
        }
        entry = entry->hash_next;
    }

    pthread_mutex_unlock(&cache->cache_mutex);
    return found;
}

/* Retrieve segment from cache */
char* chat_lru_cache_get(ChatLRUCache* cache, const char* segment_hash) {
    if (!cache || !segment_hash) {
        return NULL;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    size_t bucket = hash_string(segment_hash);
    ChatLRUCacheEntry* entry = cache->hash_table[bucket];

    while (entry) {
        if (strcmp(entry->hash, segment_hash) == 0) {
            /* Cache hit - update LRU position */
            lru_remove(cache, entry);
            lru_add_front(cache, entry);

            entry->last_accessed = time(NULL);
            entry->access_count++;
            cache->stats.cache_hits++;

            /* Update hit ratio */
            uint64_t total = cache->stats.cache_hits + cache->stats.cache_misses;
            cache->stats.hit_ratio = total > 0 ?
                (double)cache->stats.cache_hits / (double)total : 0.0;

            /* Return copy of content */
            char* content = strdup(entry->content);

            pthread_mutex_unlock(&cache->cache_mutex);

            log_this(SR_CHAT_LRU, "Cache hit for: %s", LOG_LEVEL_DEBUG, 1, segment_hash);
            return content;
        }
        entry = entry->hash_next;
    }

    /* Cache miss */
    cache->stats.cache_misses++;
    uint64_t total = cache->stats.cache_hits + cache->stats.cache_misses;
    cache->stats.hit_ratio = total > 0 ?
        (double)cache->stats.cache_hits / (double)total : 0.0;

    pthread_mutex_unlock(&cache->cache_mutex);

    log_this(SR_CHAT_LRU, "Cache miss for: %s", LOG_LEVEL_DEBUG, 1, segment_hash);
    return NULL;
}

/* Store segment in cache */
bool chat_lru_cache_put(ChatLRUCache* cache, const char* segment_hash,
                        const char* content, size_t content_size, bool is_dirty) {
    if (!cache || !segment_hash || !content || content_size == 0) {
        return false;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    /* Check if already exists */
    size_t bucket = hash_string(segment_hash);
    ChatLRUCacheEntry* entry = cache->hash_table[bucket];

    while (entry) {
        if (strcmp(entry->hash, segment_hash) == 0) {
            /* Update existing entry */
            if (entry->content_size < content_size) {
                /* Need to grow - check eviction */
                size_t needed = content_size - entry->content_size;
                if (!evict_lru_entries(cache, needed)) {
                    pthread_mutex_unlock(&cache->cache_mutex);
                    return false;
                }
            }

            cache->current_size_bytes -= entry->content_size;
            free(entry->content);
            entry->content = strndup(content, content_size);
            entry->content_size = content_size;
            entry->last_accessed = time(NULL);
            entry->access_count++;
            entry->is_dirty = entry->is_dirty || is_dirty;

            cache->current_size_bytes += content_size;

            /* Move to front of LRU */
            lru_remove(cache, entry);
            lru_add_front(cache, entry);

            pthread_mutex_unlock(&cache->cache_mutex);
            return true;
        }
        entry = entry->hash_next;
    }

    /* Check if we have space (evict if needed) */
    if (!evict_lru_entries(cache, content_size)) {
        pthread_mutex_unlock(&cache->cache_mutex);
        log_this(SR_CHAT_LRU, "Failed to evict enough space for: %s",
                 LOG_LEVEL_ALERT, 1, segment_hash);
        return false;
    }

    /* Create new entry */
    entry = calloc(1, sizeof(ChatLRUCacheEntry));
    if (!entry) {
        pthread_mutex_unlock(&cache->cache_mutex);
        return false;
    }

    strncpy(entry->hash, segment_hash, LRU_CACHE_MAX_HASH_LEN - 1);
    entry->content = strndup(content, content_size);
    entry->content_size = content_size;
    entry->last_accessed = time(NULL);
    entry->created_at = time(NULL);
    entry->access_count = 1;
    entry->is_dirty = is_dirty;

    /* Add to hash table */
    entry->hash_next = cache->hash_table[bucket];
    cache->hash_table[bucket] = entry;

    /* Add to front of LRU */
    lru_add_front(cache, entry);

    /* Update stats */
    cache->current_size_bytes += content_size;
    cache->stats.total_entries++;
    cache->stats.total_size_bytes = cache->current_size_bytes;

    pthread_mutex_unlock(&cache->cache_mutex);

    /* Request background sync if dirty */
    if (is_dirty) {
        chat_lru_cache_request_sync(cache);
    }

    log_this(SR_CHAT_LRU, "Cached segment: %s (size: %zu, total: %zu)",
             LOG_LEVEL_DEBUG, 3, segment_hash, content_size, cache->current_size_bytes);

    return true;
}

/* Remove segment from cache */
bool chat_lru_cache_remove(ChatLRUCache* cache, const char* segment_hash) {
    if (!cache || !segment_hash) {
        return false;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    size_t bucket = hash_string(segment_hash);
    ChatLRUCacheEntry** pp = &cache->hash_table[bucket];

    while (*pp) {
        ChatLRUCacheEntry* entry = *pp;
        if (strcmp(entry->hash, segment_hash) == 0) {
            /* Remove from hash table */
            *pp = entry->hash_next;

            /* Remove from LRU list */
            lru_remove(cache, entry);

            /* Update stats */
            cache->current_size_bytes -= entry->content_size;
            cache->stats.total_entries--;
            cache->stats.total_size_bytes = cache->current_size_bytes;

            pthread_mutex_unlock(&cache->cache_mutex);

            /* Remove file from disk */
            char* path = get_segment_path(cache->cache_dir, segment_hash);
            if (path) {
                unlink(path);
                free(path);
            }

            free_cache_entry(entry);
            return true;
        }
        pp = &(*pp)->hash_next;
    }

    pthread_mutex_unlock(&cache->cache_mutex);
    return false;
}

/* Get cache statistics */
bool chat_lru_cache_get_stats(ChatLRUCache* cache, ChatLRUCacheStats* stats) {
    if (!cache || !stats) {
        return false;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    *stats = cache->stats;
    stats->total_size_bytes = cache->current_size_bytes;
    stats->max_size_bytes = cache->max_size_bytes;

    pthread_mutex_unlock(&cache->cache_mutex);
    return true;
}

/* Request background sync */
void chat_lru_cache_request_sync(ChatLRUCache* cache) {
    if (!cache) {
        return;
    }

    pthread_mutex_lock(&cache->cache_mutex);
    cache->sync_requested = true;
    pthread_mutex_unlock(&cache->cache_mutex);
}

/* Flush dirty entries to database (stub - will integrate with storage layer) */
int chat_lru_cache_flush(ChatLRUCache* cache) {
    if (!cache) {
        return -1;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    int synced = 0;
    ChatLRUCacheEntry* entry = cache->lru_head;

    while (entry) {
        if (entry->is_dirty) {
            /* TODO: Integrate with chat_storage to sync to database */
            /* For now, just mark as clean */
            entry->is_dirty = false;
            synced++;
        }
        entry = entry->lru_next;
    }

    pthread_mutex_unlock(&cache->cache_mutex);

    if (synced > 0) {
        log_this(SR_CHAT_LRU, "Flushed %d dirty entries to database",
                 LOG_LEVEL_STATE, 1, synced);
    }

    return synced;
}

/* Clear all entries from cache */
bool chat_lru_cache_clear(ChatLRUCache* cache) {
    if (!cache) {
        return false;
    }

    pthread_mutex_lock(&cache->cache_mutex);

    /* Free all entries */
    ChatLRUCacheEntry* entry = cache->lru_head;
    while (entry) {
        ChatLRUCacheEntry* next = entry->lru_next;
        free_cache_entry(entry);
        entry = next;
    }

    /* Reset hash table */
    memset(cache->hash_table, 0, LRU_CACHE_HASH_TABLE_SIZE * sizeof(ChatLRUCacheEntry*));

    /* Reset list pointers */
    cache->lru_head = NULL;
    cache->lru_tail = NULL;

    /* Reset stats */
    cache->current_size_bytes = 0;
    cache->stats.total_entries = 0;
    cache->stats.total_size_bytes = 0;

    pthread_mutex_unlock(&cache->cache_mutex);

    /* Remove all files from disk */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s/*", cache->cache_dir);
    system(cmd);

    /* Recreate subdirectories */
    for (int i = 0; i < 256; i++) {
        char prefix_dir[256];
        snprintf(prefix_dir, sizeof(prefix_dir), "%s/%02x", cache->cache_dir, i);
        ensure_directory_exists(prefix_dir);
    }

    log_this(SR_CHAT_LRU, "Cache cleared for: %s", LOG_LEVEL_STATE, 1, cache->database);
    return true;
}
