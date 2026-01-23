/*
 * Conduit Service Database Operations Helper Functions
 * 
 * Functions for database queue and query cache operations.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Local includes
#include "../conduit_helpers.h"
#include "../conduit_service.h"

// Enable mock database queue functions for unit testing
#ifdef USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#endif

// Enable mock system functions for unit testing
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif

// Lookup database queue from global queue manager
DatabaseQueue* lookup_database_queue(const char* database) {
    return database_queue_manager_get_database(global_queue_manager, database);
}

// Lookup query cache entry from database queue
QueryCacheEntry* lookup_query_cache_entry(DatabaseQueue* db_queue, int query_ref) {
    if (!db_queue || !db_queue->query_cache) {
        return NULL;
    }
    return query_cache_lookup(db_queue->query_cache, query_ref, NULL);
}

// Lookup database queue and query cache entry
#ifndef USE_MOCK_LOOKUP_DATABASE_AND_QUERY
bool lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                const char* database, int query_ref) {
    if (!db_queue || !cache_entry || !database) {
        return false; // NULL pointer parameters not allowed
    }

    *db_queue = lookup_database_queue(database);
    if (!*db_queue) {
        *cache_entry = NULL; // Ensure cache_entry is NULL when database lookup fails
        return false;
    }

    *cache_entry = lookup_query_cache_entry(*db_queue, query_ref);
    if (!*cache_entry) {
        return false;
    }

    return true;
}
#endif

// Lookup database queue and public query cache entry (query_type_a28 = 10)
bool lookup_database_and_public_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                       const char* database, int query_ref) {
    if (!db_queue || !cache_entry || !database) {
        return false; // NULL pointer parameters not allowed
    }

    *db_queue = lookup_database_queue(database);
    if (!*db_queue) {
        *cache_entry = NULL; // Ensure cache_entry is NULL when database lookup fails
        return false;
    }

    if (!(*db_queue)->query_cache) {
        *cache_entry = NULL;
        return false;
    }

    // Use the type-filtered lookup for public queries (type = 10)
    *cache_entry = query_cache_lookup_by_ref_and_type((*db_queue)->query_cache, query_ref, 10, SR_API);
    if (!*cache_entry) {
        return false;
    }

    return true;
}

// Select optimal queue for query execution
DatabaseQueue* select_query_queue(const char* database, const char* queue_type) {
#ifdef USE_MOCK_SELECT_QUERY_QUEUE
    return mock_select_query_queue(database, queue_type);
#else
    return select_optimal_queue(database, queue_type, global_queue_manager);
#endif
}

enum MHD_Result handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                        int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                        bool* query_not_found, bool require_public) {
    bool lookup_success;
    if (require_public) {
        lookup_success = lookup_database_and_public_query(db_queue, cache_entry, database, query_ref);
    } else {
        lookup_success = lookup_database_and_query(db_queue, cache_entry, database, query_ref);
    }

    if (!lookup_success) {
        if (!*db_queue) {
            // Database not found - return 400 with appropriate message
            const char* error_msg = "Invalid database selection";
            unsigned int http_status = MHD_HTTP_BAD_REQUEST;
            const char* message = "The specified database does not exist or is not configured for queries";

            json_t *error_response = create_lookup_error_response(error_msg, database, query_ref, true, message);

            api_send_json_response(connection, error_response, http_status);
            json_decref(error_response);
            return MHD_YES; // Error response sent - main function will check db_queue
        } else {
            // Query not found - continue processing but mark as not found
            *query_not_found = true;
            *cache_entry = NULL;
            return MHD_YES; // Continue processing
        }
    }
    *query_not_found = false;
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                         int query_ref, const QueryCacheEntry* cache_entry,
                                         ParameterList* param_list, char* converted_sql,
                                         TypedParameter** ordered_params,
                                         DatabaseQueue** selected_queue) {
    *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!*selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_t *error_response = create_processing_error_response("No suitable queue available", database, query_ref);

        api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error_response);
        return MHD_NO; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}