/*
 * MCP subsystem public API (Phase 2 skeleton).
 */

#ifndef HYDROGEN_MCP_H
#define HYDROGEN_MCP_H

#include <stdbool.h>

void mcp_init_state(void);
void mcp_shutdown(void);
bool mcp_is_initialized(void);

#endif /* HYDROGEN_MCP_H */
