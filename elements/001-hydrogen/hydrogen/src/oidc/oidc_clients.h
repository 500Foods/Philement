/*
 * OpenID Connect (OIDC) Client Registry Interface
 *
 * Manages OAuth/OIDC client applications for Hydrogen-as-IdP.
 */

#ifndef OIDC_CLIENTS_H
#define OIDC_CLIENTS_H

#include <stdbool.h>
#include <stddef.h>

#define OIDC_CLIENT_ID_MAX 128
#define OIDC_CLIENT_NAME_MAX 500
#define OIDC_CLIENT_SECRET_HASH_MAX 500
#define OIDC_CLIENT_REDIRECTS_MAX 4096
#define OIDC_CLIENT_GRANTS_MAX 500
#define OIDC_CLIENT_RESPONSE_TYPES_MAX 100

/*
 * Single registered client (in-memory or loaded from DB).
 * Never log client_secret_hash contents.
 */
typedef struct OIDCClient {
    char client_id[OIDC_CLIENT_ID_MAX + 1];
    char client_secret_hash[OIDC_CLIENT_SECRET_HASH_MAX + 1];
    char client_name[OIDC_CLIENT_NAME_MAX + 1];
    bool is_confidential;
    bool require_pkce;
    char redirect_uris[OIDC_CLIENT_REDIRECTS_MAX + 1]; /* JSON array text */
    char grant_types[OIDC_CLIENT_GRANTS_MAX + 1];
    char response_types[OIDC_CLIENT_RESPONSE_TYPES_MAX + 1];
    bool active;
} OIDCClient;

/*
 * OIDC Client Context
 */
typedef struct {
    OIDCClient **clients;
    size_t client_count;
    size_t client_capacity;
    bool initialized;
    char *database_name; /* optional DB name for QueryRef path (Phase 5+) */
} OIDCClientContext;

OIDCClientContext* init_oidc_client_registry(void);
void cleanup_oidc_client_registry(OIDCClientContext *client_context);

void oidc_client_free(OIDCClient *client);
OIDCClient* oidc_client_create(const char *client_id,
                               const char *client_secret_hash_or_null,
                               const char *client_name,
                               bool confidential,
                               bool require_pkce,
                               const char *redirect_uris_json,
                               const char *grant_types,
                               const char *response_types);

bool oidc_client_registry_add(OIDCClientContext *ctx, OIDCClient *client);
OIDCClient* oidc_client_registry_find(const OIDCClientContext *ctx, const char *client_id);

/*
 * True if redirect_uri uses an allowed scheme for browser clients.
 * MVP: http and https only (exact scheme match, case-insensitive).
 * Rejects javascript:, data:, file:, empty, and missing "://".
 * Native custom schemes are intentionally not allowed until configured policy exists.
 */
bool oidc_redirect_uri_scheme_allowed(const char *redirect_uri);

bool oidc_redirect_uri_allowed(const char *redirect_uris_json, const char *redirect_uri);
char* oidc_hash_client_secret(const char *client_secret);
bool oidc_client_secrets_equal(const char *stored_hash, const char *presented_secret);

bool oidc_validate_client(const OIDCClientContext *client_context,
                          const char *client_id,
                          const char *redirect_uri);

bool oidc_authenticate_client(const OIDCClientContext *client_context,
                              const char *client_id,
                              const char *client_secret);

/*
 * Seed one client from OIDCConfig ClientId/RedirectUri/ClientSecret when set.
 * Public if secret empty; confidential if secret present. No-op if ClientId empty.
 */
bool oidc_seed_client_from_config(OIDCClientContext *client_context,
                                  const char *client_id,
                                  const char *client_secret_or_null,
                                  const char *redirect_uri);

bool oidc_register_client(OIDCClientContext *client_context,
                          const char *client_name,
                          const char *redirect_uri,
                          bool confidential,
                          char **client_id_out,
                          char **client_secret_out);

#endif /* OIDC_CLIENTS_H */
