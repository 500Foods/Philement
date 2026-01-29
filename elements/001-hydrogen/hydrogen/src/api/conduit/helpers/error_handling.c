/*
 * Conduit Service Error Handling Helper Functions
 * 
 * Functions for creating error responses and handling error conditions.
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

// Error response creation functions
json_t* create_validation_error_response(const char* error_msg, const char* error_detail) {
    json_t* response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg));
    json_object_set_new(response, "message", json_string(error_detail));
    return response;
}

// Helper function to create error response for lookup failures
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref, const char* message) {
    json_t* response = json_object();
    if (!response) return NULL;

    json_t* success_val = json_false();
    if (!success_val) {
        json_decref(response);
        return NULL;
    }
    json_object_set_new(response, "success", success_val);

    json_t* error_val = json_string(error_msg);
    if (!error_val) {
        json_decref(response);
        return NULL;
    }
    json_object_set_new(response, "error", error_val);

    if (database) {
        json_t* db_val = json_string(database);
        if (!db_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "database", db_val);
    }

    if (include_query_ref) {
        json_t* ref_val = json_integer(query_ref);
        if (!ref_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "query_ref", ref_val);
    }

    if (message) {
        json_t* msg_val = json_string(message);
        if (!msg_val) {
            json_decref(response);
            return NULL;
        }
        json_object_set_new(response, "message", msg_val);
    }

    return response;
}

// Helper function to create error response for processing failures
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref) {
    json_t* response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_msg));
    json_object_set_new(response, "query_ref", json_integer(query_ref));
    json_object_set_new(response, "database", json_string(database ? database : ""));
    return response;
}

// Helper function to create database not found error response
json_t* create_database_not_found_response(const char* database) {
    json_t* response = json_object();
    if (!response) return NULL;
    
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string("Database not found"));
    json_object_set_new(response, "database", json_string(database));
    json_object_set_new(response, "error_code", json_integer(1002));
    
    return response;
}

enum MHD_Result send_database_not_found_response(struct MHD_Connection *connection, const char* database) {
    json_t* error_response = create_database_not_found_response(database);
    
    if (!error_response) {
        return MHD_NO;
    }
    
    char *response_str = json_dumps(error_response, JSON_COMPACT);
    json_decref(error_response);
    
    if (!response_str) {
        return MHD_NO;
    }
    
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(response_str), response_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    
    return MHD_YES;
}