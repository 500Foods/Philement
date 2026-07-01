/*
 * Launch Scripting Subsystem
 *
 * Phase 3b of the LUA_PLAN. Registers the Scripting subsystem, performs
 * launch readiness checks, and initializes the subsystem on launch.
 *
 * The Scripting subsystem is disabled by default; when disabled, this
 * reports a clean "No-Go / disabled" (mirroring how Print reports
 * "Print queue disabled in configuration"). When enabled, it brings up
 * the subsystem state (thread tracking, shutdown flag) and is ready to
 * load an Orchestrator or run jobs - those land in later phases.
 */

 // Global includes
#include <src/hydrogen.h>

// Local includes
#include "launch.h"
#include <src/scripting/scripting.h>

// External declarations
extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;

// Forward declarations of internal helpers
static void scripting_readiness_messages_add(const char*** messages,
                                              size_t* count,
                                              size_t* capacity,
                                              const char* message);

/*
 * Append a formatted message to a dynamically growing messages array.
 * Mirrors the helper used in launch_print.c. Lives here so the readiness
 * functions can build their list without dragging in launch.c helpers.
 */
static void scripting_readiness_messages_add(const char*** messages,
                                              size_t* count,
                                              size_t* capacity,
                                              const char* message) {
    if (*count >= *capacity) {
        size_t new_capacity = (*capacity == 0) ? 4 : (*capacity * 2);
        *messages = realloc(*messages, new_capacity * sizeof(char*));
        if (!*messages) {
            fprintf(stderr, "Fatal: Memory allocation failed during startup\n");
            exit(1);
        }
        *capacity = new_capacity;
    }
    (*messages)[(*count)++] = message;
}

/*
 * Check if the Scripting subsystem is ready to launch.
 *
 * Phase 3b: when scripting is enabled, the subsystem is ready. When it
 * is disabled, this returns ready=false with a "disabled in
 * configuration" message (not an error). Later phases will validate
 * worker count, default database, and orchestrator script path here.
 */
LaunchReadiness check_scripting_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    scripting_readiness_messages_add(&messages, &count, &capacity, strdup(SR_SCRIPTING));

    if (!app_config) {
        scripting_readiness_messages_add(&messages, &count, &capacity,
            strdup("  No-Go:   app_config not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){
            .subsystem = SR_SCRIPTING,
            .ready = false,
            .messages = messages
        };
    }

    if (!app_config->scripting.Enabled) {
        scripting_readiness_messages_add(&messages, &count, &capacity,
            strdup("  No-Go:   Scripting subsystem disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){
            .subsystem = SR_SCRIPTING,
            .ready = false,
            .messages = messages
        };
    }

    scripting_readiness_messages_add(&messages, &count, &capacity,
        strdup("  Go:      Scripting subsystem enabled in configuration"));

    // Validate worker count bounds so a future Phase 7 (worker pool)
    // cannot start with an out-of-range value. Phase 3b does not act
    // on the value yet, but we surface the validation now so the
    // readiness check is a no-op later.
    int worker_count = app_config->scripting.WorkerCount;
    if (worker_count < 1 || worker_count > MAX_CONCURRENT_JOBS) {
        char msg[160];
        snprintf(msg, sizeof(msg),
                 "  No-Go:   Invalid Scripting.WorkerCount %d (must be between 1 and %d)",
                 worker_count, MAX_CONCURRENT_JOBS);
        scripting_readiness_messages_add(&messages, &count, &capacity, strdup(msg));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){
            .subsystem = SR_SCRIPTING,
            .ready = false,
            .messages = messages
        };
    }
    scripting_readiness_messages_add(&messages, &count, &capacity,
        strdup("  Go:      Scripting.WorkerCount within limits"));

    scripting_readiness_messages_add(&messages, &count, &capacity,
        strdup("  Decide:  Go For Launch of Scripting Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_SCRIPTING,
        .ready = true,
        .messages = messages
    };
}

/*
 * Launch the Scripting subsystem.
 *
 * Phase 3b: initialize the static state (thread tracking, shutdown
 * flag). Later phases will:
 *   - create the worker pool (Phase 7)
 *   - load and run the configured Orchestrator (Phase 11)
 *   - register with the dependency system
 */
int launch_scripting_subsystem(void) {
    log_this(SR_SCRIPTING, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_SCRIPTING, "LAUNCH: " SR_SCRIPTING, LOG_LEVEL_STATE, 0);

    scripting_init_state();

    if (app_config) {
        log_this(SR_SCRIPTING, "WorkerCount=%d, DefaultDatabase=%s, Orchestrator=%s",
                 LOG_LEVEL_STATE, 3,
                 app_config->scripting.WorkerCount,
                 app_config->scripting.DefaultDatabase ? app_config->scripting.DefaultDatabase : "(none)",
                 app_config->scripting.Orchestrator ? app_config->scripting.Orchestrator : "(none)");
    }

    log_this(SR_SCRIPTING, "LAUNCH: " SR_SCRIPTING " COMPLETE", LOG_LEVEL_STATE, 0);

    return 1;
}
