/*
 * Scripting Subsystem - Source Cache
 *
 * Phase 11g of the LUA_PLAN. A process-wide cache for Lua source
 * strings loaded from the database. Shared across all lua_State
 * instances (Orchestrator and per-job workers) so repeated
 * `require("group.script")` calls do not hit the DB every time.
 */

#ifndef SOURCE_CACHE_H
#define SOURCE_CACHE_H

#include <stddef.h>
#include <stdbool.h>

// Opaque source cache handle.
typedef struct SourceCache SourceCache;

// Create / destroy.
SourceCache* source_cache_create(void);
void         source_cache_destroy(SourceCache* cache);

// Store / lookup. get() returns a cache-owned pointer valid until the
// entry is replaced or the cache is destroyed. put() strdup's all
// inputs and replaces an existing entry if the key matches.
bool        source_cache_put(SourceCache* cache,
                             const char* group_name,
                             const char* script_name,
                             const char* source);
const char* source_cache_get(SourceCache* cache,
                             const char* group_name,
                             const char* script_name);

// Count entries.
size_t source_cache_count(SourceCache* cache);

#endif // SOURCE_CACHE_H
