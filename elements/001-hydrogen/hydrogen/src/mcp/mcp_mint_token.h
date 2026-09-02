#ifndef MCP_MINT_TOKEN_H
#define MCP_MINT_TOKEN_H

#include <time.h>

struct MCPConfig;
#define MCP_MINT_DEFAULT_TTL_SECONDS 900

char *mcp_mint_resource_token(const struct MCPConfig *cfg,
                              const char *sub,
                              const char *database,
                              const char *roles,
                              time_t ttl_seconds,
                              const char *correlation_id);

#endif
