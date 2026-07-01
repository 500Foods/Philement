/*
 * Scripting Subsystem - Script Registry
 *
 * Phase 7 of the LUA_PLAN. See script_registry.h for the v1 surface
 * and the rationale for keeping this small (replaced by a DB loader
 * in Phase 20).
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Local includes
#include "script_registry.h"

#define SCRIPT_REGISTRY_INITIAL_CAPACITY 16

struct ScriptRegistryEntry {
    char* name;
    char* source;
};

struct ScriptRegistry {
    struct ScriptRegistryEntry* entries;
    size_t                       count;
    size_t                       capacity;
    pthread_mutex_t              mutex;
};

/*
 * Create an empty registry.
 */
ScriptRegistry* script_registry_create(void) {
    ScriptRegistry* reg = calloc(1, sizeof(ScriptRegistry));
    if (!reg) {
        return NULL;
    }
    if (pthread_mutex_init(&reg->mutex, NULL) != 0) {
        free(reg);
        return NULL;
    }
    return reg;
}

/*
 * Free a registry and all owned strings. Safe with NULL and idempotent.
 */
void script_registry_destroy(ScriptRegistry* reg) {
    if (!reg) {
        return;
    }
    for (size_t i = 0; i < reg->count; i++) {
        free(reg->entries[i].name);
        free(reg->entries[i].source);
    }
    free(reg->entries);
    pthread_mutex_destroy(&reg->mutex);
    free(reg);
}

/*
 * Grow the entries array on demand, doubling capacity. Returns
 * false on allocation failure; the registry remains valid.
 */
static bool registry_grow_if_needed(ScriptRegistry* reg) {
    if (reg->count < reg->capacity) {
        return true;
    }
    size_t new_capacity = reg->capacity
        ? reg->capacity * 2
        : SCRIPT_REGISTRY_INITIAL_CAPACITY;
    struct ScriptRegistryEntry* new_entries = realloc(
        reg->entries, new_capacity * sizeof(struct ScriptRegistryEntry));
    if (!new_entries) {
        return false;
    }
    memset(new_entries + reg->capacity, 0,
           (new_capacity - reg->capacity) * sizeof(struct ScriptRegistryEntry));
    reg->entries = new_entries;
    reg->capacity = new_capacity;
    return true;
}

/*
 * Register a script by name, replacing any prior entry. Both name and
 * source are strdup'd so the caller can free its own copies after
 * the call.
 */
bool script_registry_register(ScriptRegistry* reg,
                              const char* name,
                              const char* source) {
    if (!reg || !name || !source) {
        return false;
    }

    pthread_mutex_lock(&reg->mutex);

    // Replace if present.
    for (size_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->entries[i].name, name) == 0) {
            char* new_source = strdup(source);
            if (!new_source) {
                pthread_mutex_unlock(&reg->mutex);
                return false;
            }
            free(reg->entries[i].source);
            reg->entries[i].source = new_source;
            pthread_mutex_unlock(&reg->mutex);
            return true;
        }
    }

    if (!registry_grow_if_needed(reg)) {
        pthread_mutex_unlock(&reg->mutex);
        return false;
    }

    char* dup_name = strdup(name);
    char* dup_source = strdup(source);
    if (!dup_name || !dup_source) {
        free(dup_name);
        free(dup_source);
        pthread_mutex_unlock(&reg->mutex);
        return false;
    }

    reg->entries[reg->count].name = dup_name;
    reg->entries[reg->count].source = dup_source;
    reg->count++;

    pthread_mutex_unlock(&reg->mutex);
    return true;
}

/*
 * Look up a registered source. Returns a registry-owned pointer
 * valid until the entry is replaced or the registry is destroyed.
 */
const char* script_registry_lookup(ScriptRegistry* reg, const char* name) {
    if (!reg || !name) {
        return NULL;
    }

    pthread_mutex_lock(&reg->mutex);
    const char* found = NULL;
    for (size_t i = 0; i < reg->count; i++) {
        if (strcmp(reg->entries[i].name, name) == 0) {
            found = reg->entries[i].source;
            break;
        }
    }
    pthread_mutex_unlock(&reg->mutex);
    return found;
}

/*
 * Count entries. Thread-safe.
 */
size_t script_registry_count(ScriptRegistry* reg) {
    if (!reg) {
        return 0;
    }
    pthread_mutex_lock(&reg->mutex);
    size_t n = reg->count;
    pthread_mutex_unlock(&reg->mutex);
    return n;
}
