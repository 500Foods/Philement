#ifndef HYDROGEN_MCP_H
#define HYDROGEN_MCP_H

#include <stdbool.h>

#include <src/mcp/mcp_http.h>

void mcp_init_state(void);
void mcp_shutdown(void);
bool mcp_is_initialized(void);

#endif
