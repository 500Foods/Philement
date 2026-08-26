/*
 * MCP Subsystem Launch
 *
 * Registers MCP, validates listen/protocol when enabled, clean-skips when
 * disabled. Binds MHD when enabled.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp.h>
#include <src/registry/registry_integration.h>
#include <src/threads/threads.h>

#include "launch.h"

extern ServiceThreads mcp_threads;
extern volatile sig_atomic_t mcp_system_shutdown;

int mcp_subsystem_id = -1;

LaunchReadiness check_mcp_launch_readiness(void) {
    const char **messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    add_launch_message(&messages, &count, &capacity, strdup(SR_MCP));

    if (mcp_subsystem_id < 0) {
        mcp_subsystem_id = register_subsystem_from_launch(SR_MCP, &mcp_threads, NULL,
                                                         &mcp_system_shutdown,
                                                         launch_mcp_subsystem, mcp_shutdown);
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MCP, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Configuration loaded"));

    if (!app_config->mcp.Enabled) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      MCP disabled, skipping validation"));
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Decide:  Go For Launch of MCP Subsystem"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MCP, .ready = true, .messages = messages };
    }

    if (mcp_subsystem_id >= 0) {
        if (!add_dependency_from_launch(mcp_subsystem_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  No-Go:   Failed to register Network dependency"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      Network dependency registered"));
        }
        if (!add_dependency_from_launch(mcp_subsystem_id, SR_SCRIPTING)) {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  No-Go:   Failed to register Scripting dependency"));
            ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      Scripting dependency registered"));
        }
    }

    if (!app_config->mcp.Interface || app_config->mcp.Interface[0] == '\0') {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   MCP.Interface is required when enabled"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      MCP.Interface set"));
        if (strcmp(app_config->mcp.Interface, "0.0.0.0") == 0 ||
            strcmp(app_config->mcp.Interface, "::") == 0) {
            log_this(SR_MCP, "ALERT: MCP.Interface binds all addresses (%s); spec prefers 127.0.0.1",
                     LOG_LEVEL_ALERT, 1, app_config->mcp.Interface);
            add_launch_message(&messages, &count, &capacity,
                               strdup("  Go:      MCP.Interface is wildcard (logged ALERT)"));
        }
    }

    if (app_config->mcp.Port < 1 || app_config->mcp.Port > 65535) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   MCP.Port must be between 1 and 65535"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      MCP.Port valid"));
    }

    if (!app_config->mcp.Path || app_config->mcp.Path[0] != '/') {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   MCP.Path must start with /"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      MCP.Path valid"));
    }

    if (!app_config->mcp.Protocol || app_config->mcp.Protocol[0] == '\0') {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   MCP.Protocol is required when enabled"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      MCP.Protocol set"));
    }

    if (app_config->scripting.WorkerCount < 2) {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  No-Go:   Scripting.WorkerCount must be >= 2 when MCP is enabled"));
        ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity,
                           strdup("  Go:      Scripting.WorkerCount >= 2"));
    }

    add_launch_message(&messages, &count, &capacity, strdup(ready ?
        "  Decide:  Go For Launch of MCP Subsystem" :
        "  Decide:  No-Go For Launch of MCP Subsystem"));
    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){ .subsystem = SR_MCP, .ready = ready, .messages = messages };
}

int launch_mcp_subsystem(void) {
    log_this(SR_MCP, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MCP, "LAUNCH: " SR_MCP, LOG_LEVEL_STATE, 0);

    if (!app_config) {
        log_this(SR_MCP, "Cannot launch: no application config", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (!app_config->mcp.Enabled) {
        log_this(SR_MCP, "MCP subsystem is disabled, skipping launch", LOG_LEVEL_DEBUG, 0);
        return 1;
    }

    if (!app_config->mcp.RequireJWT) {
        log_this(SR_MCP, "ALERT: MCP.RequireJWT is false; unauthenticated POST is allowed (test-only)",
                 LOG_LEVEL_ALERT, 0);
    }

    mcp_init_state();
    if (!mcp_start_listen(&app_config->mcp)) {
        log_this(SR_MCP, "MCP listen failed", LOG_LEVEL_ERROR, 0);
        mcp_shutdown();
        return 0;
    }
    log_this(SR_MCP, "MCP subsystem launched", LOG_LEVEL_STATE, 0);
    return 1;
}
