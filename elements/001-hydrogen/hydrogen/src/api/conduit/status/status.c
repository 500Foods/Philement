/*
 * Conduit Status API endpoint implementation for the Hydrogen Project.
 * Provides database readiness status for Conduit service.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes
#include <src/database/dbqueue/dbqueue.h>

// Auth service includes for JWT validation
#include <src/api/auth/auth_service.h>
#include <src/api/auth/auth_service_jwt.h>

// Chat includes for model status (Phase 2.5)
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/health.h>

// Local includes
#include "../conduit_service.h"
#include "status.h"

/**
 * Check if a database is ready for Conduit operations.
 * A database is considered ready if:
 * - DatabaseQueue exists in the manager
 * - Bootstrap has completed
 * - Query cache is populated
 */
static bool check_database_readiness(const DatabaseQueue* db_queue) {
    if (!db_queue) {
        return false;
    }

    // Check if bootstrap completed
    if (!db_queue->bootstrap_completed) {
        return false;
    }

    // Check if query cache exists and has entries
    if (!db_queue->query_cache || db_queue->query_cache->entry_count == 0) {
        return false;
    }

    return true;
}

/**
 * Get migration status string based on database queue state.
 */
static const char* get_migration_status(const DatabaseQueue* db_queue) {
    if (!db_queue) {
        return "not_found";
    }

    if (!db_queue->bootstrap_completed) {
        return "in_progress";
    }

    // Could check latest_applied_migration vs latest_available_migration for more detail
    // For now, just return completed if bootstrap is done
    return "completed";
}

/**
 * Check if a valid JWT token is present in the request.
 * Returns true if a valid JWT is found, false otherwise.
 */
static bool has_valid_jwt(struct MHD_Connection *connection) {
    // Get the Authorization header
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");

    if (!auth_header) {
        log_this(SR_API, "has_valid_jwt: No Authorization header found", LOG_LEVEL_DEBUG, 0);
        return false; // No auth header
    }

    log_this(SR_API, "has_valid_jwt: Found Authorization header: %s", LOG_LEVEL_DEBUG, 1, auth_header);

    // Check for "Bearer " prefix
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        log_this(SR_API, "has_valid_jwt: Authorization header doesn't start with 'Bearer '", LOG_LEVEL_DEBUG, 0);
        return false; // Invalid format
    }

    // Extract token (skip "Bearer " prefix)
    const char *token = auth_header + 7;
    if (strlen(token) == 0) {
        log_this(SR_API, "has_valid_jwt: Empty token after 'Bearer '", LOG_LEVEL_DEBUG, 0);
        return false; // Empty token
    }

    log_this(SR_API, "has_valid_jwt: Extracted token (first 20 chars): %.20s...", LOG_LEVEL_DEBUG, 1, token);

    // Validate JWT token
    jwt_validation_result_t result = validate_jwt(token, NULL);
    bool is_valid = result.valid && result.claims;

    log_this(SR_API, "has_valid_jwt: JWT validation result - valid: %s, has_claims: %s, error: %d",
             LOG_LEVEL_DEBUG, 3,
             result.valid ? "true" : "false",
             result.claims ? "true" : "false",
             result.error);

    free_jwt_validation_result(&result);

    log_this(SR_API, "has_valid_jwt: Returning %s", LOG_LEVEL_DEBUG, 1, is_valid ? "true" : "false");

    return is_valid;
}

/**
 * Handle the /api/conduit/status endpoint.
 * Returns readiness status for all configured databases.
 * Includes additional details when authenticated with a valid JWT.
 */
enum MHD_Result handle_conduit_status_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls
) {
    (void)url;              // Unused parameter
    (void)upload_data;      // Unused parameter
    (void)upload_data_size; // Unused parameter
    (void)con_cls;          // Unused parameter

    log_this(SR_API, "handle_conduit_status_request: Processing status request", LOG_LEVEL_DEBUG, 0);

    // Only GET method is allowed
    if (strcmp(method, "GET") != 0) {
        json_t* error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Method not allowed"));
        json_object_set_new(error_response, "message", json_string("Only GET requests are supported"));

        api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error_response);
        return MHD_NO;
    }

    // Check if queue manager is available
    if (!global_queue_manager) {
        json_t* error_response = json_object();
        json_object_set_new(error_response, "success", json_false());
        json_object_set_new(error_response, "error", json_string("Database manager not available"));
        json_object_set_new(error_response, "details", json_string("Global queue manager is not initialized"));

        api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return MHD_NO;
    }

    // Check for valid JWT authentication
    bool has_jwt = has_valid_jwt(connection);

    // Build response
    json_t* response = json_object();
    json_object_set_new(response, "success", json_true());

    json_t* databases = json_object();
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    // Iterate through all databases in the manager
    for (size_t i = 0; i < global_queue_manager->max_databases; i++) {
        DatabaseQueue* db_queue = global_queue_manager->databases[i];
        if (!db_queue) {
            continue; // Skip empty slots
        }

        const char* db_name = db_queue->database_name;
        bool ready = check_database_readiness(db_queue);

        json_t* db_status = json_object();
        json_object_set_new(db_status, "ready", json_boolean(ready));
        json_object_set_new(db_status, "last_checked", json_string(timestamp));

        // Include additional details only if authenticated
        if (has_jwt) {
            const char* migration_status = get_migration_status(db_queue);
            size_t query_cache_entries = db_queue->query_cache ? db_queue->query_cache->entry_count : 0;

            json_object_set_new(db_status, "migration_status", json_string(migration_status));
            json_object_set_new(db_status, "query_cache_entries", json_integer((json_int_t)query_cache_entries));

            // Include DQM statistics for this database
            json_t* dqm_stats = database_queue_get_stats_json(db_queue);
            if (dqm_stats) {
                json_object_set_new(db_status, "dqm_statistics", dqm_stats);
            }

            // Include chat engine (models) information if CEC exists (Phase 2.5)
            if (db_queue->chat_engine_cache) {
                ChatEngineCache* cec = db_queue->chat_engine_cache;
                json_t* models = json_array();

                pthread_rwlock_rdlock(&cec->cache_lock);

                for (size_t j = 0; j < cec->engine_count; j++) {
                    ChatEngineConfig* engine = cec->engines[j];
                    if (!engine) continue;

                    json_t* model = json_object();

                    // Basic info
                    json_object_set_new(model, "name", json_string(engine->name));
                    json_object_set_new(model, "provider",
                                        json_string(chat_engine_provider_to_string(engine->provider)));
                    json_object_set_new(model, "model", json_string(engine->model));

                    // Health status
                    const char* status = chat_engine_config_get_status(engine);
                    json_object_set_new(model, "status", json_string(status));

                    // Health details (thread-safe access)
                    pthread_mutex_lock(&engine->health_mutex);
                    time_t last_working = engine->last_confirmed_working;
                    unsigned long long conversations = engine->conversations_24h;
                    unsigned long long tokens = engine->tokens_24h;
                    double avg_response = engine->avg_response_time_ms;
                    int failures = engine->consecutive_failures;
                    pthread_mutex_unlock(&engine->health_mutex);

                    // Format last_working as ISO 8601 if set
                    if (last_working > 0) {
                        char time_str[32];
                        strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ",
                                 gmtime(&last_working));
                        json_object_set_new(model, "last_confirmed_working", json_string(time_str));
                    } else {
                        json_object_set_new(model, "last_confirmed_working", json_null());
                    }

                    json_object_set_new(model, "conversations_24h", json_integer((json_int_t)conversations));
                    json_object_set_new(model, "tokens_24h", json_integer((json_int_t)tokens));
                    json_object_set_new(model, "avg_response_time_ms", json_real(avg_response));

                    // Calculate error rate from failures
                    double error_rate = 0.0;
                    if (conversations > 0) {
                        error_rate = (double)failures / (double)(conversations + (unsigned long long)failures);
                    }
                    json_object_set_new(model, "error_rate", json_real(error_rate));

                    json_array_append_new(models, model);
                }

                pthread_rwlock_unlock(&cec->cache_lock);

                if (json_array_size(models) > 0) {
                    json_object_set_new(db_status, "models", models);
                } else {
                    json_decref(models);
                }
            }
        }

        json_object_set_new(databases, db_name, db_status);
    }

    json_object_set_new(response, "databases", databases);

    enum MHD_Result result = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);

    log_this(SR_API, "handle_conduit_status_request: Status request completed (authenticated: %s)", LOG_LEVEL_DEBUG, 1, has_jwt ? "yes" : "no");

    return result;
}