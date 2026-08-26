/*
 * MCP subsystem lifecycle.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp.h>
#include <src/mcp/mcp_session.h>
#include <src/mcp/mcp_stats.h>
#include <src/threads/threads.h>

extern ServiceThreads mcp_threads;
extern volatile sig_atomic_t mcp_system_shutdown;

bool mcp_initialized = false;

void mcp_init_state(void) {
    mcp_system_shutdown = 0;
    init_service_threads(&mcp_threads, SR_MCP);
    mcp_stats_reset();
    mcp_session_init();
    mcp_initialized = true;
}

void mcp_shutdown(void) {
    mcp_system_shutdown = 1;
    mcp_stop_listen();
    mcp_session_shutdown();
    mcp_initialized = false;
}

bool mcp_is_initialized(void) {
    return mcp_initialized;
}
