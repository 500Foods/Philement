/*
 * MCP Configuration
 *
 * JSON section "MCP" (AppConfig letter T). Disabled by default. No tool
 * names live here — only listen surface and the protocol script name.
 */

#ifndef HYDROGEN_CONFIG_MCP_H
#define HYDROGEN_CONFIG_MCP_H

#include <src/globals.h>

#include <stddef.h>
#include <stdbool.h>

#include <jansson.h>

#include "config_forward.h"

#define MCP_MAX_ALLOWED_ORIGINS 32
#define MCP_MAX_REQUIRED_SCOPES 16

typedef struct MCPConfig {
    bool Enabled;
    char *Interface;
    int Port;
    char *Path;
    char *Protocol;
    bool RequireJWT;
    bool AcceptHydrogenJWT;
    bool AcceptOidcIdP;
    bool AcceptOidcRp;
    char *Resource;
    char *RequiredScopes[MCP_MAX_REQUIRED_SCOPES];
    size_t RequiredScopeCount;
    char *Database;
    int MaxBodyBytes;
    int MaxResultBytes;
    int RequestTimeoutSeconds;
    int ThreadPoolSize;
    char *AllowedOrigins[MCP_MAX_ALLOWED_ORIGINS];
    size_t AllowedOriginCount;
    int SessionIdleTimeoutSeconds;
    int MaxSessions;
} MCPConfig;

bool load_mcp_config(json_t *root, AppConfig *config);
void dump_mcp_config(const MCPConfig *config);
void cleanup_mcp_config(MCPConfig *config);
void mcp_config_apply_defaults(MCPConfig *config);

#endif /* HYDROGEN_CONFIG_MCP_H */
