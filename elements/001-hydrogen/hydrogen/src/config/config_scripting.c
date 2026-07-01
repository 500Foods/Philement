/*
 * Scripting Configuration Implementation
 *
 * Implements the configuration handlers for the scripting subsystem,
 * including JSON parsing and environment variable handling. The subsystem
 * is disabled by default; enabling requires explicit configuration.
 */

 // Global includes
#include <src/hydrogen.h>

// Local includes
#include "config_scripting.h"
#include "config_utils.h"

// Load scripting configuration from JSON
bool load_scripting_config(json_t* root, AppConfig* config) {
    bool success = true;
    ScriptingConfig* scripting = &config->scripting;

    // Zero out the config structure (sets Enabled=false by default)
    memset(scripting, 0, sizeof(ScriptingConfig));

    // Set baseline defaults
    scripting->WorkerCount = 2;
    scripting->DefaultQueryTimeout = 30;
    scripting->DefaultMaxRuntime = 3600;
    scripting->InstructionHookInterval = 5000;
    scripting->MemorySampleEveryNHooks = 20;
    scripting->MemorySoftLimitKB = 32768;
    scripting->MemoryHardLimitKB = 65536;

    // Sandbox defaults: strict - no io, no debug, no loadlib, no os.execute
    scripting->Sandbox.AllowOsTime = true;
    scripting->Sandbox.AllowOsExecute = false;
    scripting->Sandbox.AllowIo = false;
    scripting->Sandbox.AllowDebug = false;
    scripting->Sandbox.AllowLoadlib = false;

    scripting->AllowDBModuleLoad = false;

    // Process main scripting section
    success = PROCESS_SECTION(root, "Scripting");
    success = success && PROCESS_BOOL(root, scripting, Enabled, "Scripting.Enabled", "Scripting");
    success = success && PROCESS_STRING(root, scripting, Orchestrator, "Scripting.Orchestrator", "Scripting");
    success = success && PROCESS_INT(root, scripting, WorkerCount, "Scripting.WorkerCount", "Scripting");
    success = success && PROCESS_STRING(root, scripting, DefaultDatabase, "Scripting.DefaultDatabase", "Scripting");
    success = success && PROCESS_INT(root, scripting, DefaultQueryTimeout, "Scripting.DefaultQueryTimeout", "Scripting");
    success = success && PROCESS_INT(root, scripting, DefaultMaxRuntime, "Scripting.DefaultMaxRuntime", "Scripting");
    success = success && PROCESS_INT(root, scripting, InstructionHookInterval, "Scripting.InstructionHookInterval", "Scripting");
    success = success && PROCESS_INT(root, scripting, MemorySampleEveryNHooks, "Scripting.MemorySampleEveryNHooks", "Scripting");
    success = success && PROCESS_INT(root, scripting, MemorySoftLimitKB, "Scripting.MemorySoftLimitKB", "Scripting");
    success = success && PROCESS_INT(root, scripting, MemoryHardLimitKB, "Scripting.MemoryHardLimitKB", "Scripting");
    success = success && PROCESS_BOOL(root, scripting, AllowDBModuleLoad, "Scripting.AllowDBModuleLoad", "Scripting");

    // Process sandbox subsection
    if (success) {
        success = PROCESS_SECTION(root, "Scripting.Sandbox");
        success = success && PROCESS_BOOL(root, &scripting->Sandbox, AllowOsTime, "Scripting.Sandbox.AllowOsTime", "Scripting");
        success = success && PROCESS_BOOL(root, &scripting->Sandbox, AllowOsExecute, "Scripting.Sandbox.AllowOsExecute", "Scripting");
        success = success && PROCESS_BOOL(root, &scripting->Sandbox, AllowIo, "Scripting.Sandbox.AllowIo", "Scripting");
        success = success && PROCESS_BOOL(root, &scripting->Sandbox, AllowDebug, "Scripting.Sandbox.AllowDebug", "Scripting");
        success = success && PROCESS_BOOL(root, &scripting->Sandbox, AllowLoadlib, "Scripting.Sandbox.AllowLoadlib", "Scripting");
    }

    return success;
}

// Free resources allocated for scripting configuration
void cleanup_scripting_config(ScriptingConfig* config) {
    if (!config) {
        return;
    }

    // Free strings
    if (config->Orchestrator) {
        free(config->Orchestrator);
        config->Orchestrator = NULL;
    }
    if (config->DefaultDatabase) {
        free(config->DefaultDatabase);
        config->DefaultDatabase = NULL;
    }

    // Free source roots
    for (int i = 0; i < config->SourceRootCount; i++) {
        if (config->SourceRoots[i]) {
            free(config->SourceRoots[i]);
            config->SourceRoots[i] = NULL;
        }
    }
    config->SourceRootCount = 0;

    // Zero out the structure
    memset(config, 0, sizeof(ScriptingConfig));
}

// Dump scripting configuration
void dump_scripting_config(const ScriptingConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL scripting config");
        return;
    }

    DUMP_BOOL("Enabled", config->Enabled);
    DUMP_STRING("Orchestrator", config->Orchestrator);
    DUMP_INT("Worker Count", config->WorkerCount);
    DUMP_STRING("Default Database", config->DefaultDatabase);
    DUMP_INT("Default Query Timeout", config->DefaultQueryTimeout);
    DUMP_INT("Default Max Runtime", config->DefaultMaxRuntime);
    DUMP_INT("Instruction Hook Interval", config->InstructionHookInterval);
    DUMP_INT("Memory Sample Every N Hooks", config->MemorySampleEveryNHooks);
    DUMP_INT("Memory Soft Limit (KB)", config->MemorySoftLimitKB);
    DUMP_INT("Memory Hard Limit (KB)", config->MemoryHardLimitKB);
    DUMP_BOOL("Allow DB Module Load", config->AllowDBModuleLoad);

    DUMP_TEXT("――", "Sandbox Configuration");
    DUMP_BOOL("―― Allow Os Time", config->Sandbox.AllowOsTime);
    DUMP_BOOL("―― Allow Os Execute", config->Sandbox.AllowOsExecute);
    DUMP_BOOL("―― Allow Io", config->Sandbox.AllowIo);
    DUMP_BOOL("―― Allow Debug", config->Sandbox.AllowDebug);
    DUMP_BOOL("―― Allow Loadlib", config->Sandbox.AllowLoadlib);

    if (config->SourceRootCount > 0) {
        DUMP_TEXT("――", "Source Roots");
        for (int i = 0; i < config->SourceRootCount; i++) {
            DUMP_STRING("――――", config->SourceRoots[i]);
        }
    }
}
