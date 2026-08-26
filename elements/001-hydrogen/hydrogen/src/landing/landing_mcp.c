/*
 * MCP Subsystem Landing
 *
 * Ready only when the subsystem is running. Land sets the shutdown flag
 * and clears skeleton state.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp.h>
#include <src/registry/registry.h>
#include <src/threads/threads.h>

#include "landing.h"

extern ServiceThreads mcp_threads;

LaunchReadiness check_mcp_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_MCP;

    readiness.messages = malloc(5 * sizeof(char *));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }

    readiness.messages[0] = strdup(SR_MCP);

    bool is_running = is_subsystem_running_by_name(SR_MCP);
    readiness.ready = is_running;

    if (is_running) {
        readiness.messages[1] = strdup("  Go:      MCP subsystem is running");
        readiness.messages[2] = strdup("  Decide:  Go For Landing of MCP");
        readiness.messages[3] = NULL;
    } else {
        readiness.messages[1] = strdup("  No-Go:   MCP not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of MCP");
        readiness.messages[3] = NULL;
    }

    return readiness;
}

int land_mcp_subsystem(void) {
    if (!is_subsystem_running_by_name(SR_MCP) && !mcp_is_initialized()) {
        log_this(SR_MCP, "MCP subsystem is not running, skipping landing", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    log_this(SR_MCP, "Landing MCP subsystem", LOG_LEVEL_STATE, 0);
    mcp_stop_listen();
    init_service_threads(&mcp_threads, SR_MCP);
    mcp_shutdown();
    update_subsystem_after_shutdown(SR_MCP);
    log_this(SR_MCP, "MCP subsystem landed", LOG_LEVEL_STATE, 0);
    return 1;
}
