#include <src/hydrogen.h>
#include <src/mcp/mcp_prm.h>
#include <src/mcp/mcp_auth.h>

#include <jansson.h>
#include <string.h>

char *mcp_prm_build(const MCPConfig *cfg, const AppConfig *app) {
    json_t *root;
    json_t *servers;
    json_t *algs;
    json_t *methods;
    char *out;
    const char *resource;

    root = json_object();
    servers = json_array();
    algs = json_array();
    methods = json_array();
    if (!root || !servers || !algs || !methods) {
        if (root) {
            json_decref(root);
        }
        if (servers) {
            json_decref(servers);
        }
        if (algs) {
            json_decref(algs);
        }
        if (methods) {
            json_decref(methods);
        }
        return NULL;
    }

    resource = mcp_auth_resource(cfg);
    json_object_set_new(root, "resource", json_string(resource ? resource : ""));
    json_array_append_new(methods, json_string("header"));
    json_object_set_new(root, "bearer_methods_supported", methods);

    if (!cfg || cfg->AcceptHydrogenJWT) {
        json_array_append_new(algs, json_string("HS256"));
    }
    if (cfg && (cfg->AcceptOidcIdP || cfg->AcceptOidcRp)) {
        json_array_append_new(algs, json_string("RS256"));
    }
    json_object_set_new(root, "resource_signing_alg_values_supported", algs);

    if (cfg && cfg->AcceptOidcIdP && app && app->oidc.enabled &&
        app->oidc.issuer && app->oidc.issuer[0] != '\0') {
        json_array_append_new(servers, json_string(app->oidc.issuer));
    }
    if (cfg && cfg->AcceptOidcRp && app) {
        size_t i;
        for (i = 0; i < app->oidc_rp.provider_count; i++) {
            const char *iss = app->oidc_rp.providers[i].issuer;
            if (iss && iss[0] != '\0') {
                json_array_append_new(servers, json_string(iss));
            }
        }
    }
    json_object_set_new(root, "authorization_servers", servers);

    out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}
