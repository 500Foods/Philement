/*
 * Conduit Status API endpoint implementation for the Hydrogen Project.
 * Provides database readiness status for Conduit service.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes
#include <src/database/dbqueue/dbqueue.h>

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
static bool check_database_readiness(DatabaseQueue* db_queue) {
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
static const char* get_migration_status(DatabaseQueue* db_queue) {
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
 * Handle the /api/conduit/status endpoint.
 * Returns readiness status for all configured databases.
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
        const char* migration_status = get_migration_status(db_queue);
        size_t query_cache_entries = db_queue->query_cache ? db_queue->query_cache->entry_count : 0;

        json_t* db_status = json_object();
        json_object_set_new(db_status, "ready", json_boolean(ready));
        json_object_set_new(db_status, "migration_status", json_string(migration_status));
        json_object_set_new(db_status, "query_cache_entries", json_integer((json_int_t)query_cache_entries));
        json_object_set_new(db_status, "last_checked", json_string(timestamp));

        json_object_set_new(databases, db_name, db_status);
    }

    json_object_set_new(response, "databases", databases);

    enum MHD_Result result = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);

    log_this(SR_API, "handle_conduit_status_request: Status request completed", LOG_LEVEL_DEBUG, 0);

    return result;
}