/*
 * Scripting Subsystem - State
 *
 * Phase 3b of the LUA_PLAN. Owns the subsystem-level state: shutdown
 * flag, ServiceThreads handle, and the placeholder for the long-lived
 * Orchestrator lua_State (filled in by Phase 11).
 *
 * The subsystem is disabled by default (config->scripting.Enabled).
 */

#ifndef HYDROGEN_SCRIPTING_H
#define HYDROGEN_SCRIPTING_H

// System headers
#include <stdbool.h>

// Third-party headers
#include <lua.h>

// Project headers
#include <src/threads/threads.h>

// Subsystem-level shutdown flag. Set by land_scripting_subsystem.
extern volatile sig_atomic_t scripting_system_shutdown;

// Thread tracking handle for the Scripting subsystem.
extern ServiceThreads scripting_threads;

// Long-lived Orchestrator lua_State, or NULL if not running.
// Phase 3b reserves the storage; Phase 11 actually loads and runs the
// configured orchestrator script.
extern lua_State* scripting_orchestrator_state;

// Initialize the Scripting subsystem's static state.
// Called once from launch_scripting_subsystem(). Idempotent.
void scripting_init_state(void);

// Tear down the Scripting subsystem's static state.
// Called once from land_scripting_subsystem(). Idempotent.
void scripting_cleanup_state(void);

#endif /* HYDROGEN_SCRIPTING_H */
