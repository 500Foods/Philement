/*
 * OpenID Connect (OIDC) Service — lifecycle
 *
 * Owns the global OIDCContext: initialization, shutdown, and accessors.
 * Protocol handlers live in sibling oidc_service_*.c modules.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "oidc_service.h"
#include "oidc_keys.h"
#include "oidc_tokens.h"
#include "oidc_users.h"
#include "oidc_auth_codes.h"
#include "oidc_refresh_tokens.h"
#include <src/api/oidc/oidc_service.h>

// Global OIDC context (process-wide singleton for the IdP)
static OIDCContext *oidc_context = NULL;

/*
 * Tear down a partially or fully constructed context.
 * Safe with NULL sub-pointers so init failure paths can share one cleanup.
 * Does not touch the global oidc_context pointer.
 */
void oidc_service_release_context(OIDCContext *ctx) {
    if (!ctx) {
        return;
    }

    /* Reverse of init order when subcomponents exist. */
    cleanup_oidc_endpoints();
    if (ctx->client_context) {
        cleanup_oidc_client_registry(ctx->client_context);
        ctx->client_context = NULL;
    }
    if (ctx->user_context) {
        cleanup_oidc_user_management(ctx->user_context);
        ctx->user_context = NULL;
    }
    if (ctx->token_context) {
        cleanup_oidc_token_service(ctx->token_context);
        ctx->token_context = NULL;
    }
    if (ctx->key_context) {
        cleanup_oidc_key_management(ctx->key_context);
        ctx->key_context = NULL;
    }
    if (ctx->refresh_store) {
        oidc_refresh_store_destroy(ctx->refresh_store);
        ctx->refresh_store = NULL;
    }
    if (ctx->auth_code_store) {
        oidc_auth_code_store_destroy(ctx->auth_code_store);
        ctx->auth_code_store = NULL;
    }
    free(ctx->database_name);
    ctx->database_name = NULL;
    free(ctx);
}

/*
 * Initialize the OIDC service and all subordinate components.
 *
 * Order: stores → keys → tokens → users → clients → seed → API endpoints.
 * On any failure, releases what was built and leaves the global pointer NULL.
 *
 * @param config OIDC configuration (must be non-NULL; string fields owned by app_config)
 * @return true on success
 */
bool init_oidc_service(const OIDCConfig *config) {
    if (!config) {
        log_this(SR_OIDC, "Invalid configuration provided", LOG_LEVEL_ERROR, 0);
        return false;
    }

    oidc_context = (OIDCContext *)malloc(sizeof(OIDCContext));
    if (!oidc_context) {
        log_this(SR_OIDC, "Failed to allocate OIDC context", LOG_LEVEL_ERROR, 0);
        return false;
    }

    /* Shallow copy: issuer/endpoint strings remain owned by app_config. */
    memset(oidc_context, 0, sizeof(OIDCContext));
    memcpy(&oidc_context->config, config, sizeof(OIDCConfig));
    oidc_context->initialized = false;
    oidc_context->shutting_down = false;

    if (config->database && config->database[0] != '\0') {
        oidc_context->database_name = strdup(config->database);
    }

    /* Authorization codes (in-memory; DB QueryRefs for multi-process later). */
    oidc_context->auth_code_store = oidc_auth_code_store_create(300);
    if (!oidc_context->auth_code_store) {
        log_this(SR_OIDC, "Failed to create authorization code store", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    int refresh_ttl = config->tokens.refresh_token_lifetime > 0
        ? config->tokens.refresh_token_lifetime
        : OIDC_REFRESH_DEFAULT_TTL_SEC;
    oidc_context->refresh_store = oidc_refresh_store_create(refresh_ttl);
    if (!oidc_context->refresh_store) {
        log_this(SR_OIDC, "Failed to create refresh token store", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    log_this(SR_OIDC, "Initializing key management", LOG_LEVEL_STATE, 0);
    oidc_context->key_context = init_oidc_key_management(
        config->keys.storage_path,
        config->keys.encryption_enabled,
        config->keys.rotation_interval_days
    );
    if (!oidc_context->key_context) {
        log_this(SR_OIDC, "Failed to initialize key management", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    log_this(SR_OIDC, "Initializing token service", LOG_LEVEL_STATE, 0);
    oidc_context->token_context = init_oidc_token_service(
        oidc_context->key_context,
        config->tokens.access_token_lifetime,
        config->tokens.refresh_token_lifetime,
        config->tokens.id_token_lifetime
    );
    if (!oidc_context->token_context) {
        log_this(SR_OIDC, "Failed to initialize token service", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    log_this(SR_OIDC, "Initializing user management", LOG_LEVEL_STATE, 0);
    oidc_context->user_context = init_oidc_user_management(
        5,    /* max_failed_attempts */
        true, /* require_email_verification */
        8     /* password_min_length */
    );
    if (!oidc_context->user_context) {
        log_this(SR_OIDC, "Failed to initialize user management", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    log_this(SR_OIDC, "Initializing client registry", LOG_LEVEL_STATE, 0);
    oidc_context->client_context = init_oidc_client_registry();
    if (!oidc_context->client_context) {
        log_this(SR_OIDC, "Failed to initialize client registry", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    /* Optional config seed (ClientId + RedirectUri) for tests/dev until DB load. */
    if (!oidc_seed_client_from_config((OIDCClientContext*)oidc_context->client_context,
                                     config->client_id, config->client_secret,
                                     config->redirect_uri)) {
        log_this(SR_OIDC, "Failed to seed OIDC client from config", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    log_this(SR_OIDC, "Initializing API endpoints", LOG_LEVEL_STATE, 0);
    if (!init_oidc_endpoints(oidc_context)) {
        log_this(SR_OIDC, "Failed to initialize API endpoints", LOG_LEVEL_ERROR, 0);
        oidc_service_release_context(oidc_context);
        oidc_context = NULL;
        return false;
    }

    oidc_context->initialized = true;
    log_this(SR_OIDC, "OIDC service initialized successfully", LOG_LEVEL_STATE, 0);
    return true;
}

/*
 * Shut down the OIDC service and free the global context.
 * Idempotent when already stopped.
 */
void shutdown_oidc_service(void) {
    if (!oidc_context) {
        return;
    }

    oidc_context->shutting_down = true;
    log_this(SR_OIDC, "Shutting down OIDC service", LOG_LEVEL_STATE, 0);

    oidc_service_release_context(oidc_context);
    oidc_context = NULL;

    log_this(SR_OIDC, "OIDC service shutdown complete", LOG_LEVEL_STATE, 0);
}

/*
 * @return Global OIDC context, or NULL if not initialized / after shutdown
 */
OIDCContext* get_oidc_context(void) {
    return oidc_context;
}
