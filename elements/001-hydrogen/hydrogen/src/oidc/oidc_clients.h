/*
 * OpenID Connect (OIDC) Client Registry Interface
 *
 * Defines the interface for managing OIDC client applications including
 * registration, authentication, and authorization.
 */

#ifndef OIDC_CLIENTS_H
#define OIDC_CLIENTS_H

// Standard Libraries
#include <stdbool.h>

/*
 * OIDC Client Context
 * Main context structure for the client registry
 */
typedef struct {
    void *client_db;           // Client database context
    bool initialized;          // Whether the registry is initialized
    int client_count;          // Number of registered clients
} OIDCClientContext;

/*
 * Client Registry Management Functions
 * Handle OIDC client registration and lifecycle
 */

// Initialize the client registry
OIDCClientContext* init_oidc_client_registry(void);

// Clean up the client registry and release resources
void cleanup_oidc_client_registry(OIDCClientContext *client_context);

// Validate a client ID and redirect URI
bool oidc_validate_client(OIDCClientContext *client_context, const char *client_id, const char *redirect_uri);

// Authenticate a client using client ID and secret
bool oidc_authenticate_client(OIDCClientContext *client_context, const char *client_id, const char *client_secret);

// Register a new client
bool oidc_register_client(OIDCClientContext *client_context, const char *client_name, const char *redirect_uri, 
                          bool confidential, char **client_id_out, char **client_secret_out);

#endif // OIDC_CLIENTS_H
