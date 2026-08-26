/*
 * MCP subsystem lifecycle. Phase 2: state only; no listen.
 */

#include <src/hydrogen.h>
#include <src/mcp/mcp.h>
#include <src/threads/threads.h>

extern ServiceThreads mcp_threads;
extern volatile sig_atomic_t mcp_system_shutdown;

bool mcp_initialized = false;

void mcp_init_state(void) {
    mcp_system_shutdown = 0;
    init_service_threads(&mcp_threads, SR_MCP);
    mcp_initialized = true;
}

void mcp_shutdown(void) {
    mcp_system_shutdown = 1;
    mcp_initialized = false;
}

bool mcp_is_initialized(void) {
    return mcp_initialized;
}
