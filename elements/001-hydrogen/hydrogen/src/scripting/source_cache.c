/*
 * Scripting Subsystem - Source Cache
 *
 * Phase 11g of the LUA_PLAN. A process-wide cache for Lua source
 * strings loaded from the database. Shared across all lua_State
 * instances (Orchestrator and per-job workers) so repeated
 * `require("group.script")` calls do not hit the DB every time.
 *
 * Phase 21: Bytecode Caching. Compiled Lua bytecode is cached
 * alongside the source. Subsequent require calls load bytecode
 * (via luaL_loadbufferx with mode "b") instead of parsing source.
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
     void* bytecode;
     size_t bytecode_len;
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
        free(cache->entries[i].bytecode);
    }
    free(cache->entries);
    pthread_mutex_destroy(&cache->mutex);
    free(cache);
}

/*
 * Grow the entries array on demand, doubling capacity. Returns
 * false on allocation failure; the cache remains valid.
 */
bool source_cache_grow_if_needed(SourceCache* cache) {
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
            free(cache->entries[i].bytecode);
            cache->entries[i].source = new_source;
            cache->entries[i].bytecode = NULL;
            cache->entries[i].bytecode_len = 0;
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
    cache->entries[cache->count].bytecode = NULL;
    cache->entries[cache->count].bytecode_len = 0;
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
 * Store bytecode in the cache under (group_name, script_name).
 * If an entry exists for the key, the old bytecode is freed and
 * replaced. If bytecode_len is 0, the bytecode field is cleared.
 * Returns false on allocation failure, NULL inputs, or NULL bytecode
 * with non-zero length.
 */
bool source_cache_put_bytecode(SourceCache* cache,
                               const char* group_name,
                               const char* script_name,
                               const void* bytecode,
                               size_t bytecode_len) {
    if (!cache || !group_name || !script_name) {
        return false;
    }

    // NULL bytecode with non-zero length is invalid
    if (!bytecode && bytecode_len > 0) {
        return false;
    }

    pthread_mutex_lock(&cache->mutex);

    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].group_name, group_name) == 0 &&
            strcmp(cache->entries[i].script_name, script_name) == 0) {
            free(cache->entries[i].bytecode);
            cache->entries[i].bytecode = NULL;
            cache->entries[i].bytecode_len = 0;

            if (bytecode && bytecode_len > 0) {
                void* new_bytecode = malloc(bytecode_len);
                if (!new_bytecode) {
                    pthread_mutex_unlock(&cache->mutex);
                    return false;
                }
                memcpy(new_bytecode, bytecode, bytecode_len);
                cache->entries[i].bytecode = new_bytecode;
                cache->entries[i].bytecode_len = bytecode_len;
            }
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
    void* dup_bytecode = NULL;

    if (bytecode && bytecode_len > 0) {
        dup_bytecode = malloc(bytecode_len);
        if (!dup_bytecode) {
            free(dup_group);
            free(dup_script);
            pthread_mutex_unlock(&cache->mutex);
            return false;
        }
        memcpy(dup_bytecode, bytecode, bytecode_len);
    }

    if (!dup_group || !dup_script) {
        free(dup_group);
        free(dup_script);
        free(dup_bytecode);
        pthread_mutex_unlock(&cache->mutex);
        return false;
    }

    cache->entries[cache->count].group_name = dup_group;
    cache->entries[cache->count].script_name = dup_script;
    cache->entries[cache->count].source = NULL;
    cache->entries[cache->count].bytecode = dup_bytecode;
    cache->entries[cache->count].bytecode_len = bytecode_len;
    cache->count++;

    pthread_mutex_unlock(&cache->mutex);
    return true;
}

/*
 * Look up bytecode by (group_name, script_name). Returns a
 * cache-owned pointer valid until the entry is replaced or the
 * cache is destroyed. Sets *out_bytecode_len on success.
 * Returns NULL on miss, NULL inputs, or no bytecode cached.
 */
const void* source_cache_get_bytecode(SourceCache* cache,
                                      const char* group_name,
                                      const char* script_name,
                                      size_t* out_bytecode_len) {
    if (!cache || !group_name || !script_name) {
        if (out_bytecode_len) *out_bytecode_len = 0;
        return NULL;
    }

    pthread_mutex_lock(&cache->mutex);
    const void* found = NULL;
    size_t len = 0;
    for (size_t i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].group_name, group_name) == 0 &&
            strcmp(cache->entries[i].script_name, script_name) == 0) {
            if (cache->entries[i].bytecode && cache->entries[i].bytecode_len > 0) {
                found = cache->entries[i].bytecode;
                len = cache->entries[i].bytecode_len;
            }
            break;
        }
    }
    pthread_mutex_unlock(&cache->mutex);

    if (out_bytecode_len) *out_bytecode_len = len;
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
