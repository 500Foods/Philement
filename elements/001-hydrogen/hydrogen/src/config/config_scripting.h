/*
 * Scripting Configuration
 *
 * Defines the configuration structure and defaults for the scripting subsystem.
 * The scripting subsystem embeds a Lua runtime and exposes a curated host API
 * to scripts. It is disabled by default; enabling requires explicit configuration.
 */

#ifndef HYDROGEN_CONFIG_SCRIPTING_H
#define HYDROGEN_CONFIG_SCRIPTING_H

#include <src/globals.h>

// System headers
#include <stddef.h>
#include <stdbool.h>

// Third-party headers
#include <jansson.h>

// Project headers
#include "config_forward.h"  // For AppConfig forward declaration

// Maximum number of script source root directories
#define MAX_SCRIPTING_SOURCE_ROOTS 16

// Sandbox configuration for the Lua runtime
typedef struct ScriptingSandboxConfig {
    bool AllowOsTime;         // os.time and os.date only
    bool AllowOsExecute;      // os.execute, os.exit, os.remove
    bool AllowIo;             // io.* library (file I/O)
    bool AllowDebug;          // debug.* library
    bool AllowLoadlib;        // package.loadlib / require of native modules
} ScriptingSandboxConfig;

// Main scripting configuration structure
typedef struct ScriptingConfig {
    bool Enabled;                                          // Whether the scripting subsystem is enabled
    char* Orchestrator;                                    // Optional path/name of the long-running Orchestrator script (NULL = none)
    int WorkerCount;                                       // Number of worker threads (bounded by Resources)
    char* DefaultDatabase;                                 // Optional default database name for H.query
    int DefaultQueryTimeout;                               // Default timeout in seconds for query/HTTP/LLM handles and service tokens
    int DefaultMaxRuntime;                                 // Default per-job max runtime in seconds
    int InstructionHookInterval;                            // Frequency of scoreboard updates in VM instructions
    int MemorySoftLimitKB;                                 // Soft memory limit in KB; warning + GC hint
    int MemoryHardLimitKB;                                 // Hard memory limit in KB; kills the job
    ScriptingSandboxConfig Sandbox;                        // Sandbox policy

    // Script source roots (filesystem paths)
    char* SourceRoots[MAX_SCRIPTING_SOURCE_ROOTS];
    int SourceRootCount;

    bool AllowDBModuleLoad;                                // Whether require() may load modules from the database (Phase 20+)
} ScriptingConfig;

/*
 * Load scripting configuration from JSON
 *
 * Loads and processes scripting configuration from JSON, setting defaults and
 * handling environment variable overrides. The scripting subsystem is disabled
 * by default; enabling requires an explicit "Enabled": true in the JSON.
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on error
 */
bool load_scripting_config(json_t* root, AppConfig* config);

/*
 * Dump scripting configuration for debugging
 *
 * Outputs the current scripting configuration state in a structured format.
 *
 * @param config The scripting configuration to dump
 */
void dump_scripting_config(const ScriptingConfig* config);

/*
 * Clean up scripting configuration
 *
 * Frees all resources allocated for the scripting configuration. After cleanup,
 * the structure is zeroed to prevent use-after-free.
 *
 * @param config The scripting configuration to clean up
 */
void cleanup_scripting_config(ScriptingConfig* config);

#endif /* HYDROGEN_CONFIG_SCRIPTING_H */
