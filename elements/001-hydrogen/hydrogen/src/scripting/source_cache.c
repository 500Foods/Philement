/*
 * Scripting Subsystem - Source Cache
 *
 * Phase 11g of the LUA_PLAN. A process-wide cache for Lua source
 * strings loaded from the database. Shared across all lua_State
 * instances (Orchestrator and per-job workers) so repeated
 * `require("group.script")` calls do not hit the DB every time.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdlib.h>
#include <string.h>

// Local includes
#include "source_cache.h"

#define SOURCE_CACHE_INITIAL_CAPACITY 16

struct SourceCacheEntry {
    char* group_name;
    char* script_name;
    char* source;
};

struct SourceCache {
    struct SourceCacheEntry* entries;
    size_t                   count;
    size_t                   capacity;
    pthread_mutex_t          mutex;
};

/*
 * Create an empty source cache.
 */
SourceCache* source_cache_create(void) {
    SourceCache* cache = calloc(1, sizeof(SourceCache));
    if (!cache) {
        return NULL;
    }
    if (pthread_mutex_init(&cache->mutex, NULL) != 0) {
        free(cache);
        return NULL;
    }
    return cache;
}

/*
 * Destroy a source cache and all owned strings. Safe with NULL and
 * idempotent.
 */
void source_cache_destroy(SourceCache* cache) {
    if (!cache) {
        return;
    }
    for (size_t i = 0; i < cache->count; i++) {
        free(cache->entries[i].group_name);
        free(cache->entries[i].script_name);
        free(cache->entries[i].source);
    }
    free(cache->entries);
    pthread_mutex_destroy(&cache->mutex);
    free(cache);
}

/*
 * Grow the entries array on demand, doubling capacity. Returns
 * false on allocation failure; the cache remains valid.
 */
static bool source_cache_grow_if_needed(SourceCache* cache) {
    if (cache->count < cache->capacity) {
        return true;
    }
    size_t new_capacity = cache->capacity
        ? cache->capacity * 2
        : SOURCE_CACHE_INITIAL_CAPACITY;
    struct SourceCacheEntry* new_entries = realloc(
        cache->entries, new_capacity * sizeof(struct SourceCacheEntry));
    if (!new_entries) {
        return false;
    }
    memset(new_entries + cache->capacity, 0,
           (new_capacity - cache->capacity) * sizeof(struct SourceCacheEntry));
    cache->entries = new_entries;
    cache->capacity = new_capacity;
    return true;
}

/*
 * Store a source string in the cache under (group_name, script_name).
 * Both names and the source are strdup'd. Replacing an existing entry
 * is allowed and frees the old source. Returns false on allocation
 * failure or NULL inputs.
 */
bool source_cache_put(SourceCache* cache,
                      const char* group_name,
                      const char* script_name,
                      const char* source) {
    if (!cache || !group_name || !script_name || !source) {
        return false;
    }

    pthread_mutex_lock(&cache->mutex);

    // Replace if present.
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].group_name, group_name) == 0 &&
            strcmp(cache->entries[i].script_name, script_name) == 0) {
            char* new_source = strdup(source);
            if (!new_source) {
                pthread_mutex_unlock(&cache->mutex);
                return false;
            }
            free(cache->entries[i].source);
            cache->entries[i].source = new_source;
            pthread_mutex_unlock(&cache->mutex);
            return true;
        }
    }

    if (!source_cache_grow_if_needed(cache)) {
        pthread_mutex_unlock(&cache->mutex);
        return false;
    }

    char* dup_group = strdup(group_name);
    char* dup_script = strdup(script_name);
    char* dup_source = strdup(source);
    if (!dup_group || !dup_script || !dup_source) {
        free(dup_group);
        free(dup_script);
        free(dup_source);
        pthread_mutex_unlock(&cache->mutex);
        return false;
    }

    cache->entries[cache->count].group_name = dup_group;
    cache->entries[cache->count].script_name = dup_script;
    cache->entries[cache->count].source = dup_source;
    cache->count++;

    pthread_mutex_unlock(&cache->mutex);
    return true;
}

/*
 * Look up a source string by (group_name, script_name). Returns a
 * cache-owned pointer valid until the entry is replaced or the cache
 * is destroyed. Returns NULL on miss or NULL inputs.
 */
const char* source_cache_get(SourceCache* cache,
                             const char* group_name,
                             const char* script_name) {
    if (!cache || !group_name || !script_name) {
        return NULL;
    }

    pthread_mutex_lock(&cache->mutex);
    const char* found = NULL;
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].group_name, group_name) == 0 &&
            strcmp(cache->entries[i].script_name, script_name) == 0) {
            found = cache->entries[i].source;
            break;
        }
    }
    pthread_mutex_unlock(&cache->mutex);
    return found;
}

/*
 * Count entries. Thread-safe.
 */
size_t source_cache_count(SourceCache* cache) {
    if (!cache) {
        return 0;
    }
    pthread_mutex_lock(&cache->mutex);
    size_t n = cache->count;
    pthread_mutex_unlock(&cache->mutex);
    return n;
}
