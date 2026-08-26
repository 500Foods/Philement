#ifndef HYDROGEN_MCP_AUTH_H
#define HYDROGEN_MCP_AUTH_H

#include <stdbool.h>
#include <stddef.h>
#include <microhttpd.h>

#include <src/config/config.h>
#include <src/config/config_mcp.h>
#include <src/mcp/mcp_stats.h>
#include <src/oidc/oidc_tokens.h>

typedef enum {
    MCP_AUTH_KIND_NONE = 0,
    MCP_AUTH_KIND_HYDROGEN_JWT,
    MCP_AUTH_KIND_OIDC_IDP,
    MCP_AUTH_KIND_OIDC_RP
} McpAuthKind;

typedef struct McpAuthResult {
    bool accepted;
    McpAuthRejectReason reject_reason;
    McpAuthKind kind;
    char *sub;
    char *iss;
    char *roles;
    char *database;
    char **scopes;
    size_t scope_count;
} McpAuthResult;

typedef bool (*McpOidcAcceptFn)(const char *token, const AppConfig *app, OIDCTokenClaims **out);

void mcp_auth_result_cleanup(McpAuthResult *result);
void mcp_auth_set_oidc_idp_hook(McpOidcAcceptFn fn);
void mcp_auth_set_oidc_rp_hook(McpOidcAcceptFn fn);

bool mcp_auth_body_has_hydrogen(const char *body, size_t body_len);
bool mcp_auth_aud_contains(const OIDCTokenClaims *claims, const char *resource);
bool mcp_auth_scopes_satisfied(const char *scope_csv, const MCPConfig *cfg);
const char *mcp_auth_resource(const MCPConfig *cfg);
char *mcp_prm_metadata_url(const MCPConfig *cfg);
char *mcp_www_authenticate_value(const MCPConfig *cfg);

bool mcp_validate_bearer(const char *auth_header,
                         const char *body,
                         size_t body_len,
                         const MCPConfig *cfg,
                         const AppConfig *app,
                         McpAuthResult *out);

enum MHD_Result mcp_send_unauthorized(struct MHD_Connection *connection, const MCPConfig *cfg);

void mcp_auth_fail(McpAuthResult *out, McpAuthRejectReason reason);
bool mcp_auth_copy_oidc(McpAuthResult *out, McpAuthKind kind, const OIDCTokenClaims *claims);
bool mcp_try_hydrogen(const char *auth_header, const MCPConfig *cfg, McpAuthResult *out);
bool mcp_try_oidc_idp(const char *token, const MCPConfig *cfg, const AppConfig *app,
                      McpAuthResult *out, McpAuthRejectReason *detail);
bool mcp_try_one_rp(const char *token, const OIDCRPProviderConfig *provider,
                    const MCPConfig *cfg, McpAuthRejectReason *detail,
                    OIDCTokenClaims **out_claims);
bool mcp_try_oidc_rp(const char *token, const MCPConfig *cfg, const AppConfig *app,
                     McpAuthResult *out, McpAuthRejectReason *detail);

#endif
