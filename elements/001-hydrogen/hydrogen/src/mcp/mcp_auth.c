#include <src/hydrogen.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_stats.h>
#include <src/api/auth/auth_service.h>
#include <src/oidc/oidc_service.h>
#include <src/api/auth/oidc_rp/oidc_rp_discovery.h>
#include <src/api/auth/oidc_rp/oidc_rp_idtoken.h>
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <string.h>

#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif

static McpOidcAcceptFn mcp_oidc_idp_hook = NULL;
static McpOidcAcceptFn mcp_oidc_rp_hook = NULL;

void mcp_auth_set_oidc_idp_hook(McpOidcAcceptFn fn) {
    mcp_oidc_idp_hook = fn;
}

void mcp_auth_set_oidc_rp_hook(McpOidcAcceptFn fn) {
    mcp_oidc_rp_hook = fn;
}

void mcp_auth_result_cleanup(McpAuthResult *result) {
    if (!result) {
        return;
    }
    free(result->sub);
    free(result->iss);
    free(result->roles);
    free(result->database);
    if (result->scopes) {
        size_t i;
        for (i = 0; i < result->scope_count; i++) {
            free(result->scopes[i]);
        }
        free(result->scopes);
    }
    memset(result, 0, sizeof(*result));
}

bool mcp_auth_body_has_hydrogen(const char *body, size_t body_len) {
    json_t *root;
    json_error_t err;
    bool present;

    if (!body || body_len == 0) {
        return false;
    }
    root = json_loadb(body, body_len, JSON_REJECT_DUPLICATES, &err);
    if (!root) {
        return false;
    }
    present = json_object_get(root, "_hydrogen") != NULL;
    if (!present && json_is_object(root)) {
        json_t *params = json_object_get(root, "params");
        if (json_is_object(params) && json_object_get(params, "_hydrogen") != NULL) {
            present = true;
        }
    }
    json_decref(root);
    return present;
}

const char *mcp_auth_resource(const MCPConfig *cfg) {
    static char derived[512];

    if (cfg && cfg->Resource && cfg->Resource[0] != '\0') {
        return cfg->Resource;
    }
    if (!cfg) {
        return "";
    }
    snprintf(derived, sizeof(derived), "http://%s:%d%s",
             cfg->Interface ? cfg->Interface : "127.0.0.1",
             cfg->Port > 0 ? cfg->Port : 3100,
             cfg->Path ? cfg->Path : "/mcp");
    return derived;
}

bool mcp_auth_aud_is_resource_uri(const char *aud) {
    if (!aud || aud[0] == '\0') {
        return false;
    }
    if (strncmp(aud, "http://", 7) == 0) {
        return true;
    }
    if (strncmp(aud, "https://", 8) == 0) {
        return true;
    }
    return false;
}

char *mcp_prm_metadata_url(const MCPConfig *cfg) {
    const char *resource = mcp_auth_resource(cfg);
    const char *path;
    char origin[512];
    char *slash;
    char *out;
    size_t need;

    if (!resource || resource[0] == '\0') {
        return strdup("/.well-known/oauth-protected-resource");
    }
    snprintf(origin, sizeof(origin), "%s", resource);
    path = strstr(origin, "://");
    slash = path ? strchr(path + 3, '/') : NULL;
    if (slash) {
        *slash = '\0';
    }
    need = strlen(origin) + strlen("/.well-known/oauth-protected-resource") +
           (cfg && cfg->Path ? strlen(cfg->Path) : 0) + 1;
    out = malloc(need);
    if (!out) {
        return NULL;
    }
    snprintf(out, need, "%s/.well-known/oauth-protected-resource%s",
             origin, (cfg && cfg->Path) ? cfg->Path : "");
    return out;
}

char *mcp_www_authenticate_value(const MCPConfig *cfg) {
    char *meta;
    char *out;
    size_t need;

    meta = mcp_prm_metadata_url(cfg);
    if (!meta) {
        return NULL;
    }
    need = strlen("Bearer realm=\"hydrogen-mcp\", resource_metadata=\"\"") + strlen(meta) + 1;
    out = malloc(need);
    if (!out) {
        free(meta);
        return NULL;
    }
    snprintf(out, need, "Bearer realm=\"hydrogen-mcp\", resource_metadata=\"%s\"", meta);
    free(meta);
    return out;
}

bool mcp_auth_aud_contains(const OIDCTokenClaims *claims, const char *resource) {
    size_t i;

    if (!claims || !resource || resource[0] == '\0') {
        return true;
    }
    if (claims->aud_count == 0) {
        return true;
    }
    for (i = 0; i < claims->aud_count; i++) {
        if (claims->aud[i] && strcmp(claims->aud[i], resource) == 0) {
            return true;
        }
    }
    return false;
}

bool mcp_auth_scopes_satisfied(const char *scope_csv, const MCPConfig *cfg) {
    size_t i;
    char *copy;
    char *save;
    char *tok;
    bool found;

    if (!cfg || cfg->RequiredScopeCount == 0) {
        return true;
    }
    if (!scope_csv) {
        return false;
    }
    for (i = 0; i < cfg->RequiredScopeCount; i++) {
        const char *need = cfg->RequiredScopes[i];
        if (!need || need[0] == '\0') {
            continue;
        }
        copy = strdup(scope_csv);
        if (!copy) {
            return false;
        }
        found = false;
        tok = strtok_r(copy, " ", &save);
        while (tok) {
            if (strcmp(tok, need) == 0) {
                found = true;
                break;
            }
            tok = strtok_r(NULL, " ", &save);
        }
        free(copy);
        if (!found) {
            return false;
        }
    }
    return true;
}

void mcp_auth_fail(McpAuthResult *out, McpAuthRejectReason reason) {
    if (out) {
        mcp_auth_result_cleanup(out);
        out->accepted = false;
        out->reject_reason = reason;
    }
    mcp_stats_inc_auth_rejected(reason);
}

bool mcp_auth_copy_oidc(McpAuthResult *out, McpAuthKind kind, const OIDCTokenClaims *claims) {
    if (!out || !claims) {
        return false;
    }
    out->kind = kind;
    out->sub = claims->sub ? strdup(claims->sub) : NULL;
    out->iss = claims->iss ? strdup(claims->iss) : NULL;
    out->roles = NULL;
    out->database = NULL;
    out->scopes = NULL;
    out->scope_count = 0;
    if (claims->scope && claims->scope[0] != '\0') {
        char *copy = strdup(claims->scope);
        char *save = NULL;
        char *tok;
        size_t n = 0;
        char **list = NULL;

        if (!copy) {
            return false;
        }
        tok = strtok_r(copy, " ", &save);
        while (tok) {
            char **grown = realloc(list, (n + 1) * sizeof(char *));
            if (!grown) {
                size_t j;
                for (j = 0; j < n; j++) {
                    free(list[j]);
                }
                free(list);
                free(copy);
                return false;
            }
            list = grown;
            list[n] = strdup(tok);
            if (!list[n]) {
                size_t j;
                for (j = 0; j < n; j++) {
                    free(list[j]);
                }
                free(list);
                free(copy);
                return false;
            }
            n++;
            tok = strtok_r(NULL, " ", &save);
        }
        free(copy);
        out->scopes = list;
        out->scope_count = n;
    }
    out->accepted = true;
    return out->sub != NULL;
}

bool mcp_try_hydrogen(const char *auth_header, const MCPConfig *cfg, McpAuthResult *out) {
    jwt_validation_result_t jwt;

    if (!cfg->AcceptHydrogenJWT) {
        return false;
    }
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        return false;
    }
    jwt = validate_jwt(auth_header + 7, NULL);
    if (jwt.error == JWT_ERROR_UNAVAILABLE) {
        if (jwt.claims) {
            free_jwt_claims(jwt.claims);
        }
        mcp_auth_fail(out, MCP_AUTH_REJECT_UNAVAILABLE);
        return false;
    }
    if (!jwt.valid || !jwt.claims || !jwt.claims->database || jwt.claims->database[0] == '\0') {
        if (jwt.claims) {
            free_jwt_claims(jwt.claims);
        }
        return false;
    }
    {
        const char *resource;
        const char *aud = jwt.claims->aud;

        if (aud && aud[0] != '\0' && strcmp(aud, "hydrogen-chat") == 0) {
            mcp_auth_fail(out, MCP_AUTH_REJECT_AUD);
            free_jwt_validation_result(&jwt);
            return false;
        }
        resource = mcp_auth_resource(cfg);
        if (resource && resource[0] != '\0' && mcp_auth_aud_is_resource_uri(aud) &&
            strcmp(aud, resource) != 0) {
            mcp_auth_fail(out, MCP_AUTH_REJECT_AUD);
            free_jwt_validation_result(&jwt);
            return false;
        }
    }
    out->kind = MCP_AUTH_KIND_HYDROGEN_JWT;
    out->sub = jwt.claims->sub ? strdup(jwt.claims->sub) : NULL;
    out->iss = jwt.claims->iss ? strdup(jwt.claims->iss) : NULL;
    out->roles = jwt.claims->roles ? strdup(jwt.claims->roles) : NULL;
    out->database = strdup(jwt.claims->database);
    out->accepted = true;
    free_jwt_validation_result(&jwt);
    return true;
}

bool mcp_try_oidc_idp(const char *token, const MCPConfig *cfg, const AppConfig *app,
                             McpAuthResult *out, McpAuthRejectReason *detail) {
    OIDCTokenClaims *claims = NULL;
    OIDCContext *oidc;
    bool ok;

    if (!cfg->AcceptOidcIdP) {
        return false;
    }
    if (mcp_oidc_idp_hook) {
        ok = mcp_oidc_idp_hook(token, app, &claims);
    } else {
        oidc = get_oidc_context();
        if (!oidc || !oidc->token_context) {
            *detail = MCP_AUTH_REJECT_OIDC_IDP;
            return false;
        }
        ok = oidc_validate_access_token((OIDCTokenContext *)oidc->token_context, token, &claims);
    }
    if (!ok || !claims) {
        *detail = MCP_AUTH_REJECT_OIDC_IDP;
        if (claims) {
            oidc_free_token_claims(claims);
        }
        return false;
    }
    if (app && app->oidc.issuer && claims->iss && strcmp(claims->iss, app->oidc.issuer) != 0) {
        *detail = MCP_AUTH_REJECT_OIDC_IDP;
        oidc_free_token_claims(claims);
        return false;
    }
    if (!mcp_auth_aud_contains(claims, mcp_auth_resource(cfg))) {
        *detail = MCP_AUTH_REJECT_AUD;
        oidc_free_token_claims(claims);
        return false;
    }
    if (!mcp_auth_scopes_satisfied(claims->scope, cfg)) {
        *detail = MCP_AUTH_REJECT_SCOPE;
        oidc_free_token_claims(claims);
        return false;
    }
    ok = mcp_auth_copy_oidc(out, MCP_AUTH_KIND_OIDC_IDP, claims);
    oidc_free_token_claims(claims);
    if (!ok) {
        *detail = MCP_AUTH_REJECT_OIDC_IDP;
        return false;
    }
    return true;
}

bool mcp_try_one_rp(const char *token, const OIDCRPProviderConfig *provider,
                           const MCPConfig *cfg, McpAuthRejectReason *detail,
                           OIDCTokenClaims **out_claims) {
    const OidcRpDiscoveryDoc *doc;
    char *buf = NULL;
    char *header_b64 = NULL;
    char *payload_b64 = NULL;
    char *sig_b64 = NULL;
    size_t signing_len = 0;
    char *alg = NULL;
    char *kid = NULL;
    unsigned char *sig = NULL;
    size_t sig_len = 0;
    unsigned char *praw = NULL;
    size_t plen = 0;
    char *pstr = NULL;
    OIDCTokenClaims *claims;
    OidcRpIdTokenError verr;

    doc = oidc_rp_discovery_get(provider->name, provider->issuer, provider->verify_ssl,
                                provider->discovery_cache_seconds);
    if (!doc || !doc->jwks_uri) {
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    if (!split_jws(token, &buf, &header_b64, &payload_b64, &sig_b64, &signing_len)) {
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    if (parse_header(header_b64, &alg, &kid) != OIDC_RP_IDTOKEN_OK) {
        free(buf);
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    sig = utils_base64url_decode(sig_b64, &sig_len);
    verr = verify_signature(provider, doc->jwks_uri, kid,
                            (const unsigned char *)buf, signing_len, sig, sig_len);
    free(sig);
    free(alg);
    free(kid);
    if (verr != OIDC_RP_IDTOKEN_OK) {
        free(buf);
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    praw = utils_base64url_decode(payload_b64, &plen);
    free(buf);
    if (!praw || plen == 0) {
        free(praw);
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    pstr = malloc(plen + 1);
    if (!pstr) {
        free(praw);
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    memcpy(pstr, praw, plen);
    pstr[plen] = '\0';
    free(praw);
    claims = oidc_claims_from_payload_json(pstr);
    free(pstr);
    if (!claims) {
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    if (provider->issuer && claims->iss && strcmp(claims->iss, provider->issuer) != 0) {
        oidc_free_token_claims(claims);
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    if (!mcp_auth_aud_contains(claims, mcp_auth_resource(cfg))) {
        oidc_free_token_claims(claims);
        *detail = MCP_AUTH_REJECT_AUD;
        return false;
    }
    if (!mcp_auth_scopes_satisfied(claims->scope, cfg)) {
        oidc_free_token_claims(claims);
        *detail = MCP_AUTH_REJECT_SCOPE;
        return false;
    }
    *out_claims = claims;
    return true;
}

bool mcp_try_oidc_rp(const char *token, const MCPConfig *cfg, const AppConfig *app,
                            McpAuthResult *out, McpAuthRejectReason *detail) {
    OIDCTokenClaims *claims = NULL;
    size_t i;
    bool ok;

    if (!cfg->AcceptOidcRp) {
        return false;
    }
    if (mcp_oidc_rp_hook) {
        ok = mcp_oidc_rp_hook(token, app, &claims);
        if (!ok || !claims) {
            *detail = MCP_AUTH_REJECT_OIDC_RP;
            if (claims) {
                oidc_free_token_claims(claims);
            }
            return false;
        }
        if (!mcp_auth_aud_contains(claims, mcp_auth_resource(cfg))) {
            *detail = MCP_AUTH_REJECT_AUD;
            oidc_free_token_claims(claims);
            return false;
        }
        if (!mcp_auth_scopes_satisfied(claims->scope, cfg)) {
            *detail = MCP_AUTH_REJECT_SCOPE;
            oidc_free_token_claims(claims);
            return false;
        }
        ok = mcp_auth_copy_oidc(out, MCP_AUTH_KIND_OIDC_RP, claims);
        oidc_free_token_claims(claims);
        return ok;
    }
    if (!app) {
        *detail = MCP_AUTH_REJECT_OIDC_RP;
        return false;
    }
    for (i = 0; i < app->oidc_rp.provider_count; i++) {
        if (mcp_try_one_rp(token, &app->oidc_rp.providers[i], cfg, detail, &claims)) {
            ok = mcp_auth_copy_oidc(out, MCP_AUTH_KIND_OIDC_RP, claims);
            oidc_free_token_claims(claims);
            return ok;
        }
        if (*detail == MCP_AUTH_REJECT_AUD || *detail == MCP_AUTH_REJECT_SCOPE) {
            return false;
        }
    }
    *detail = MCP_AUTH_REJECT_OIDC_RP;
    return false;
}

bool mcp_validate_bearer(const char *auth_header,
                         const char *body,
                         size_t body_len,
                         const MCPConfig *cfg,
                         const AppConfig *app,
                         McpAuthResult *out) {
    const char *token;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    if (out) {
        memset(out, 0, sizeof(*out));
    }
    if (!cfg) {
        mcp_auth_fail(out, MCP_AUTH_REJECT_MALFORMED);
        return false;
    }
    if (mcp_auth_body_has_hydrogen(body, body_len)) {
        mcp_auth_fail(out, MCP_AUTH_REJECT_MALFORMED);
        return false;
    }
    if (!cfg->RequireJWT) {
        if (out) {
            out->accepted = true;
            out->kind = MCP_AUTH_KIND_NONE;
        }
        return true;
    }
    if (!auth_header || auth_header[0] == '\0') {
        mcp_auth_fail(out, MCP_AUTH_REJECT_MISSING);
        return false;
    }
    if (strncmp(auth_header, "Bearer ", 7) != 0 || auth_header[7] == '\0') {
        mcp_auth_fail(out, MCP_AUTH_REJECT_MALFORMED);
        return false;
    }
    token = auth_header + 7;
    if (mcp_try_hydrogen(auth_header, cfg, out)) {
        return true;
    }
    if (out && out->reject_reason == MCP_AUTH_REJECT_UNAVAILABLE) {
        return false;
    }
    if (mcp_try_oidc_idp(token, cfg, app, out, &detail)) {
        return true;
    }
    if (detail == MCP_AUTH_REJECT_AUD || detail == MCP_AUTH_REJECT_SCOPE) {
        mcp_auth_fail(out, detail);
        return false;
    }
    if (mcp_try_oidc_rp(token, cfg, app, out, &detail)) {
        return true;
    }
    if (detail == MCP_AUTH_REJECT_AUD || detail == MCP_AUTH_REJECT_SCOPE) {
        mcp_auth_fail(out, detail);
        return false;
    }
    if (cfg->AcceptHydrogenJWT) {
        detail = MCP_AUTH_REJECT_HYDROGEN_JWT;
    } else if (cfg->AcceptOidcIdP) {
        detail = MCP_AUTH_REJECT_OIDC_IDP;
    } else if (cfg->AcceptOidcRp) {
        detail = MCP_AUTH_REJECT_OIDC_RP;
    }
    mcp_auth_fail(out, detail);
    return false;
}

enum MHD_Result mcp_send_unauthorized(struct MHD_Connection *connection, const MCPConfig *cfg) {
    struct MHD_Response *response;
    enum MHD_Result queued;
    char *www;
    const char *body = "{\"error\":\"unauthorized\"}";

    response = MHD_create_response_from_buffer(strlen(body), (void *)body, MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", "application/json");
    www = mcp_www_authenticate_value(cfg);
    if (www) {
        MHD_add_response_header(response, "WWW-Authenticate", www);
        free(www);
    }
    queued = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
    MHD_destroy_response(response);
    return queued;
}
