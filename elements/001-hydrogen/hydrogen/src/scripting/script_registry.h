/*
 * Scripting Subsystem - Script Registry
 *
 * Phase 7 of the LUA_PLAN. Minimal in-memory name -> source map for
 * Lua scripts. Workers use this to look up the source code for a
 * given scoreboard entry's script_name.
 *
 * v1 is intentionally tiny:
 *   - register(name, source)    - add or replace
 *   - lookup(name) -> source    - find a registered source
 *   - count()                   - how many registered
 *   - destroy()                 - free the registry
 *
 * Phase 20 (DB-backed scripts) replaces this module's contents with a
 * DB-driven loader; the worker-side API (lookup) stays the same so
 * workers do not change.
 *
 * Concurrency: a single pthread_mutex_t protects the entries array.
 * Lookup is a short critical section; v1 is a map of (strdup'd name,
 * strdup'd source) pairs in a growable array. A hash table would be
 * premature optimization at v1 sizes.
 */

#ifndef HYDROGEN_SCRIPTING_SCRIPT_REGISTRY_H
#define HYDROGEN_SCRIPTING_SCRIPT_REGISTRY_H

// System headers
#include <stdbool.h>
#include <stddef.h>

// Forward declaration; defined in this header below.
struct ScriptRegistry;

typedef struct ScriptRegistry ScriptRegistry;

/*
 * Create an empty script registry. Returns NULL on allocation failure.
 * Destroy with script_registry_destroy().
 */
ScriptRegistry* script_registry_create(void);

/*
 * Free a script registry and all of its entries. Safe to call with
 * NULL. Idempotent (second call is a no-op).
 */
void script_registry_destroy(ScriptRegistry* reg);

/*
 * Register a script under a name, replacing any existing entry with
 * the same name.
 *
 *   reg    - the registry (non-NULL)
 *   name   - non-NULL script identifier (e.g. "orchestrator.submit",
 *            "Reporter.run"). The registry strdup's the name.
 *   source - non-NULL Lua source code. The registry strdup's the source.
 *
 * Returns true on success, false on allocation failure.
 */
bool script_registry_register(ScriptRegistry* reg,
                              const char* name,
                              const char* source);

/*
 * Look up the source for a registered name.
 *
 * Returns a pointer to the registry-owned source string, or NULL if
 * the name is not registered. The pointer is valid until the entry is
 * replaced by another register() with the same name, or until the
 * registry is destroyed; callers must not free it.
 */
const char* script_registry_lookup(ScriptRegistry* reg, const char* name);

/*
 * Number of entries currently in the registry. Thread-safe (briefly
 * takes the mutex). Returns 0 for a NULL registry.
 */
size_t script_registry_count(ScriptRegistry* reg);

#endif /* HYDROGEN_SCRIPTING_SCRIPT_REGISTRY_H */
