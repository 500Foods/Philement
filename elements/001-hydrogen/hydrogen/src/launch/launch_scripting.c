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
#include <src/scripting/worker_pool.h>
#include <src/scripting/http_pool.h>

// External declarations
extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;

// Registry ID for the Scripting subsystem. Registered in
// check_scripting_launch_readiness() so the launch loop's
// get_subsystem_id_by_name() can find it. (Phase 11h: the
// registry registration was missing, which caused the launch
// loop to skip Scripting with a "Failed to get subsystem ID"
// debug message — the Orchestrator never started.)
static int scripting_subsystem_id = -1;

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

    // Register with the subsystem registry so the launch loop can find
    // us via get_subsystem_id_by_name(). Phase 11h: this call was
    // missing, which is why the Orchestrator never started in test_43.
    if (scripting_subsystem_id < 0) {
        scripting_subsystem_id = register_subsystem(SR_SCRIPTING, NULL, NULL, NULL, NULL, NULL);
    }

    /*
     * Scripting submits queries and holds pending waiters on the Database
     * subsystem. Land Scripting before Database so orchestrator/workers
     * are stopped before queues and the global pending manager are torn
     * down (otherwise a late waiter blocks cleanup_global_pending_manager).
     */
    if (scripting_subsystem_id >= 0) {
        if (!add_dependency_from_launch(scripting_subsystem_id, SR_DATABASE)) {
            scripting_readiness_messages_add(&messages, &count, &capacity,
                strdup("  No-Go:   Failed to register Database dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){
                .subsystem = SR_SCRIPTING,
                .ready = false,
                .messages = messages
            };
        }
        scripting_readiness_messages_add(&messages, &count, &capacity,
            strdup("  Go:      Database dependency registered"));
    }

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

    // Phase 7: bring up the worker pool. WorkerCount was validated in
    // check_scripting_launch_readiness (1..MAX_CONCURRENT_JOBS).
    int worker_count = app_config ? app_config->scripting.WorkerCount : 2;
    if (!scripting_workers_init(worker_count)) {
        log_this(SR_SCRIPTING, "LAUNCH: " SR_SCRIPTING " FAILED (worker pool init)",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Phase 17: bring up the HTTP worker pool. HTTPWorkerCount was
    // defaulted to 4 in config_defaults; if it's 0 or negative we
    // treat it as a no-op (the host function falls back to the
    // inline path).
    int http_workers = app_config ? app_config->scripting.HTTPWorkerCount : 4;
    if (http_workers > 0) {
        if (!scripting_http_pool_init(http_workers)) {
            log_this(SR_SCRIPTING, "LAUNCH: " SR_SCRIPTING " FAILED (HTTP pool init)",
                     LOG_LEVEL_ERROR, 0);
            scripting_workers_destroy();
            return 0;
        }
    }

    // Phase 11h: mark the subsystem as running in the registry.
    // Without this, get_subsystem_state() returns INACTIVE and
    // dependent subsystems can't see Scripting as available.
    update_subsystem_on_startup(SR_SCRIPTING, true);

    log_this(SR_SCRIPTING, "LAUNCH: " SR_SCRIPTING " COMPLETE", LOG_LEVEL_STATE, 0);

    return 1;
}
