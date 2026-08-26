/*
 * MCP Configuration Implementation
 */

#include <src/hydrogen.h>

#include "config_mcp.h"
#include "config_utils.h"

void mcp_config_apply_defaults(MCPConfig *config) {
    if (!config) {
        return;
    }

    config->Enabled = false;
    config->Interface = strdup("127.0.0.1");
    config->Port = 3100;
    config->Path = strdup("/mcp");
    config->Protocol = NULL;
    config->RequireJWT = true;
    config->AcceptHydrogenJWT = true;
    config->AcceptOidcIdP = false;
    config->AcceptOidcRp = false;
    config->Resource = NULL;
    config->RequiredScopeCount = 0;
    config->Database = NULL;
    config->MaxBodyBytes = 1048576;
    config->MaxResultBytes = 262144;
    config->RequestTimeoutSeconds = 30;
    config->ThreadPoolSize = 4;
    config->AllowedOriginCount = 0;
    config->SessionIdleTimeoutSeconds = 900;
    config->MaxSessions = 256;
}

bool load_mcp_config(json_t *root, AppConfig *config) {
    bool success = true;
    MCPConfig *mcp;

    if (!config) {
        return false;
    }

    mcp = &config->mcp;
    cleanup_mcp_config(mcp);
    mcp_config_apply_defaults(mcp);

    success = PROCESS_SECTION(root, "MCP");
    success = success && PROCESS_BOOL(root, mcp, Enabled, "MCP.Enabled", "MCP");
    success = success && PROCESS_STRING(root, mcp, Interface, "MCP.Interface", "MCP");
    success = success && PROCESS_INT(root, mcp, Port, "MCP.Port", "MCP");
    success = success && PROCESS_STRING(root, mcp, Path, "MCP.Path", "MCP");
    success = success && PROCESS_STRING(root, mcp, Protocol, "MCP.Protocol", "MCP");
    success = success && PROCESS_BOOL(root, mcp, RequireJWT, "MCP.RequireJWT", "MCP");
    success = success && PROCESS_BOOL(root, mcp, AcceptHydrogenJWT, "MCP.AcceptHydrogenJWT", "MCP");
    success = success && PROCESS_BOOL(root, mcp, AcceptOidcIdP, "MCP.AcceptOidcIdP", "MCP");
    success = success && PROCESS_BOOL(root, mcp, AcceptOidcRp, "MCP.AcceptOidcRp", "MCP");
    success = success && PROCESS_STRING(root, mcp, Resource, "MCP.Resource", "MCP");
    success = success && process_string_array_config(root,
        (ConfigStringArray){
            .array = mcp->RequiredScopes,
            .count = &mcp->RequiredScopeCount,
            .capacity = MCP_MAX_REQUIRED_SCOPES
        }, "MCP.RequiredScopes", "MCP");
    success = success && PROCESS_STRING(root, mcp, Database, "MCP.Database", "MCP");
    success = success && PROCESS_INT(root, mcp, MaxBodyBytes, "MCP.MaxBodyBytes", "MCP");
    success = success && PROCESS_INT(root, mcp, MaxResultBytes, "MCP.MaxResultBytes", "MCP");
    success = success && PROCESS_INT(root, mcp, RequestTimeoutSeconds, "MCP.RequestTimeoutSeconds", "MCP");
    success = success && PROCESS_INT(root, mcp, ThreadPoolSize, "MCP.ThreadPoolSize", "MCP");
    success = success && process_string_array_config(root,
        (ConfigStringArray){
            .array = mcp->AllowedOrigins,
            .count = &mcp->AllowedOriginCount,
            .capacity = MCP_MAX_ALLOWED_ORIGINS
        }, "MCP.AllowedOrigins", "MCP");
    success = success && PROCESS_INT(root, mcp, SessionIdleTimeoutSeconds, "MCP.SessionIdleTimeoutSeconds", "MCP");
    success = success && PROCESS_INT(root, mcp, MaxSessions, "MCP.MaxSessions", "MCP");

    if (success && (mcp->Port < 1 || mcp->Port > 65535)) {
        log_this(SR_CONFIG, "MCP.Port must be 1–65535 (got %d)", LOG_LEVEL_ERROR, 1, mcp->Port);
        success = false;
    }

    if (success) {
        log_this(SR_CONFIG, "― MCP configuration loaded successfully", LOG_LEVEL_DEBUG, 0);
    }

    return success;
}

void dump_mcp_config(const MCPConfig *config) {
    if (!config) {
        return;
    }

    log_this(SR_CONFIG_CURRENT, "MCP Configuration:", LOG_LEVEL_DEBUG, 0);
    log_this(SR_CONFIG_CURRENT, "  Enabled: %s", LOG_LEVEL_DEBUG, 1, config->Enabled ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  Interface: %s", LOG_LEVEL_DEBUG, 1,
             config->Interface ? config->Interface : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Port: %d", LOG_LEVEL_DEBUG, 1, config->Port);
    log_this(SR_CONFIG_CURRENT, "  Path: %s", LOG_LEVEL_DEBUG, 1, config->Path ? config->Path : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Protocol: %s", LOG_LEVEL_DEBUG, 1,
             config->Protocol ? config->Protocol : "(null)");
    log_this(SR_CONFIG_CURRENT, "  RequireJWT: %s", LOG_LEVEL_DEBUG, 1, config->RequireJWT ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  AcceptHydrogenJWT: %s", LOG_LEVEL_DEBUG, 1,
             config->AcceptHydrogenJWT ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  AcceptOidcIdP: %s", LOG_LEVEL_DEBUG, 1,
             config->AcceptOidcIdP ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  AcceptOidcRp: %s", LOG_LEVEL_DEBUG, 1,
             config->AcceptOidcRp ? "true" : "false");
    log_this(SR_CONFIG_CURRENT, "  Resource: %s", LOG_LEVEL_DEBUG, 1,
             config->Resource ? config->Resource : "(null)");
    log_this(SR_CONFIG_CURRENT, "  Database: %s", LOG_LEVEL_DEBUG, 1,
             config->Database ? config->Database : "(null)");
    log_this(SR_CONFIG_CURRENT, "  MaxBodyBytes: %d", LOG_LEVEL_DEBUG, 1, config->MaxBodyBytes);
    log_this(SR_CONFIG_CURRENT, "  MaxResultBytes: %d", LOG_LEVEL_DEBUG, 1, config->MaxResultBytes);
    log_this(SR_CONFIG_CURRENT, "  RequestTimeoutSeconds: %d", LOG_LEVEL_DEBUG, 1, config->RequestTimeoutSeconds);
    log_this(SR_CONFIG_CURRENT, "  ThreadPoolSize: %d", LOG_LEVEL_DEBUG, 1, config->ThreadPoolSize);
    log_this(SR_CONFIG_CURRENT, "  AllowedOriginCount: %zu", LOG_LEVEL_DEBUG, 1, config->AllowedOriginCount);
    log_this(SR_CONFIG_CURRENT, "  RequiredScopeCount: %zu", LOG_LEVEL_DEBUG, 1, config->RequiredScopeCount);
    log_this(SR_CONFIG_CURRENT, "  SessionIdleTimeoutSeconds: %d", LOG_LEVEL_DEBUG, 1,
             config->SessionIdleTimeoutSeconds);
    log_this(SR_CONFIG_CURRENT, "  MaxSessions: %d", LOG_LEVEL_DEBUG, 1, config->MaxSessions);
}

void cleanup_mcp_config(MCPConfig *config) {
    size_t i;

    if (!config) {
        return;
    }

    free(config->Interface);
    free(config->Path);
    free(config->Protocol);
    free(config->Resource);
    free(config->Database);
    for (i = 0; i < config->AllowedOriginCount && i < MCP_MAX_ALLOWED_ORIGINS; i++) {
        free(config->AllowedOrigins[i]);
        config->AllowedOrigins[i] = NULL;
    }
    for (i = 0; i < config->RequiredScopeCount && i < MCP_MAX_REQUIRED_SCOPES; i++) {
        free(config->RequiredScopes[i]);
        config->RequiredScopes[i] = NULL;
    }

    memset(config, 0, sizeof(MCPConfig));
}
