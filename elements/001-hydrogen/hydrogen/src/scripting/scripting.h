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

// The scripting subsystem's scoreboard (Phase 5). Allocated when
// scripting is enabled and destroyed during landing. The Orchestrator
// (Phase 11) and the worker pool (Phase 7) read and write it.
struct Scoreboard;
extern struct Scoreboard* scripting_scoreboard;

// The scripting subsystem's worker pool (Phase 7). Allocated by
// scripting_workers_init() during launch and destroyed during
// landing. Owns the job queue, worker pthread IDs, and the in-memory
// script registry. NULL when the subsystem is not running.
struct ScriptingWorkerPool;
extern struct ScriptingWorkerPool* scripting_workers;

// The scripting subsystem's source cache (Phase 11g). Allocated when
// scripting is enabled and destroyed during landing. Shared across
// all lua_State instances so DB-backed `require` results are cached
// for the process lifetime.
struct SourceCache;
extern struct SourceCache* scripting_source_cache;

// The scripting subsystem's source cache (Phase 11g). Allocated when
// scripting is enabled and destroyed during landing. Shared across
// all lua_State instances so DB-backed `require` results are cached
// for the process lifetime.
struct SourceCache;
extern struct SourceCache* scripting_source_cache;

// Initialize the Scripting subsystem's static state.
// Called once from launch_scripting_subsystem(). Idempotent.
void scripting_init_state(void);

// Tear down the Scripting subsystem's static state.
// Called once from land_scripting_subsystem(). Idempotent.
void scripting_cleanup_state(void);

#endif /* HYDROGEN_SCRIPTING_H */
