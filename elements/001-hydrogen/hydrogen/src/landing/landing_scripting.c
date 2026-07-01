/*
 * Landing Scripting Subsystem
 *
 * Phase 3b of the LUA_PLAN. Shuts the Scripting subsystem down and
 * confirms it is safe to land (no Orchestrator in flight, no active
 * workers, no Lua state leaks). This is the proof point for the
 * "shutdown is proven early" decision in Phase 3b.
 */

 // Global includes
#include <src/hydrogen.h>

// Local includes
#include "landing.h"
#include <src/landing/landing.h>
#include <src/utils/utils_logging.h>
#include <src/threads/threads.h>
#include <src/config/config.h>
#include <src/registry/registry_integration.h>
#include <src/state/state_types.h>
#include <src/scripting/scripting.h>
#include <src/scripting/lua_context.h>
#include <src/scripting/worker_pool.h>

// External declarations
extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;
extern lua_State* scripting_orchestrator_state;

/*
 * Check if the Scripting subsystem is ready to land.
 *
 * Reports ready=true when:
 *   - the subsystem is not running (a no-op landing is safe), OR
 *   - the subsystem is running, no Orchestrator state is in flight,
 *     and the worker thread count is zero.
 *
 * In Phase 3b the Orchestrator is not yet loaded and the worker pool
 * does not exist, so a running subsystem always reports ready here.
 * Later phases will tighten this (e.g. wait for in-flight jobs).
 */
LaunchReadiness check_scripting_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_SCRIPTING;

    readiness.messages = malloc(8 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;

    readiness.messages[msg_count++] = strdup(SR_SCRIPTING);

    if (!is_subsystem_running_by_name(SR_SCRIPTING)) {
        readiness.messages[msg_count++] = strdup("  Go:      Scripting subsystem not running");
        readiness.messages[msg_count] = NULL;
        readiness.ready = true;
        return readiness;
    }

    readiness.messages[msg_count++] = strdup("  Go:      Scripting subsystem running");

    if (scripting_orchestrator_state) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Orchestrator state still present");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      No Orchestrator state in flight");

    if (scripting_threads.thread_count > 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Active Scripting worker threads");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      No active Scripting worker threads");

    readiness.messages[msg_count++] = strdup("  Decide:  Go For Landing of Scripting Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;

    return readiness;
}

/*
 * Land the Scripting subsystem.
 *
 * Sets the shutdown flag and tears down subsystem state. The
 * Orchestrator state (if any) is destroyed via H_lua_destroy_context
 * through scripting_cleanup_state, ensuring no Lua state outlives
 * the subsystem.
 */
int land_scripting_subsystem(void) {
    log_this(SR_SCRIPTING, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_SCRIPTING, "LANDING: " SR_SCRIPTING, LOG_LEVEL_DEBUG, 0);

    scripting_system_shutdown = 1;

    // Phase 7: stop the worker pool first. This drains the queue,
    // joins worker threads, and frees the pool. The scoreboard must
    // remain alive until the workers have exited (workers update it
    // as they finish each job), so worker pool teardown precedes
    // scripting_cleanup_state.
    scripting_workers_destroy();

    // scripting_cleanup_state destroys any Orchestrator lua_State and
    // the scoreboard, and re-initializes the thread tracking structure.
    // Safe to call even if launch_scripting_subsystem was never invoked.
    scripting_cleanup_state();

    log_this(SR_SCRIPTING, "LANDING: " SR_SCRIPTING " COMPLETE", LOG_LEVEL_DEBUG, 0);

    return 1;
}
