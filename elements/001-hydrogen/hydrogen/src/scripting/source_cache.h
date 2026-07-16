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
 * This saves the parse/compile step on every require.
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

// Store / lookup source. get() returns a cache-owned pointer valid
// until the entry is replaced or the cache is destroyed. put()
// strdup's all inputs and replaces an existing entry if the key
// matches.
bool        source_cache_put(SourceCache* cache,
                             const char* group_name,
                             const char* script_name,
                             const char* source);
const char* source_cache_get(SourceCache* cache,
                             const char* group_name,
                             const char* script_name);

// Bytecode store / lookup. get_bytecode() returns a cache-owned
// pointer to the bytecode data, valid until the entry is replaced
// or the cache is destroyed. put_bytecode() copies the data with
// malloc. Set bytecode_len to 0 to clear bytecode (keep source).
bool        source_cache_put_bytecode(SourceCache* cache,
                                      const char* group_name,
                                      const char* script_name,
                                      const void* bytecode,
                                      size_t bytecode_len);
const void* source_cache_get_bytecode(SourceCache* cache,
                                      const char* group_name,
                                      const char* script_name,
                                      size_t* out_bytecode_len);

// Count entries.
size_t source_cache_count(SourceCache* cache);

// Exposed for Unity tests (NOT part of the stable public API).
bool source_cache_grow_if_needed(SourceCache* cache);

#endif // SOURCE_CACHE_H
