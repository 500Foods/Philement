/**
 * @file queries_response_helpers.c
 * @brief Helper functions for multi-query response building
 *
 * Common response building helpers shared by queries, auth_queries,
 * and alt_queries endpoints. Extracted from handler functions to improve
 * testability and reduce duplication.
 *
 * @author Hydrogen Framework
 * @date 2026-02-19
 */

#include <src/hydrogen.h>

// Standard includes
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>
#include <microhttpd.h>

// Project headers
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/queries_response_helpers.h>
#include <src/logging/logging.h>

json_t *build_dedup_error_json(DeduplicationResult dedup_code, const char *database, int max_queries) {
    json_t *error_response = json_object();
    if (!error_response) {
        return NULL;
    }

    json_object_set_new(error_response, "success", json_false());

    if (dedup_code == DEDUP_RATE_LIMIT) {
        char limit_msg[100];
        snprintf(limit_msg, sizeof(limit_msg), "Query limit of %d unique queries per request exceeded", max_queries);
        json_object_set_new(error_response, "error", json_string("Rate limit exceeded"));
        json_object_set_new(error_response, "message", json_string(limit_msg));
    } else if (dedup_code == DEDUP_DATABASE_NOT_FOUND) {
        json_object_set_new(error_response, "error", json_string("Invalid database"));
    } else {
        json_object_set_new(error_response, "error", json_string("Validation failed"));
    }

    (void)database;  // Reserved for future use in error messages
    return error_response;
}

unsigned int get_dedup_http_status(DeduplicationResult dedup_code) {
    if (dedup_code == DEDUP_RATE_LIMIT) {
        return MHD_HTTP_TOO_MANY_REQUESTS;
    }
    return MHD_HTTP_BAD_REQUEST;
}

unsigned int determine_queries_http_status(json_t *results_array, size_t result_count) {
    if (!results_array) {
        return MHD_HTTP_INTERNAL_SERVER_ERROR;
    }

    bool rate_limit = false;
    bool has_parameter_errors = false;
    bool has_database_errors = false;
    bool has_duplicate_errors = false;

    for (size_t i = 0; i < result_count; i++) {
        json_t *single_result = json_array_get(results_array, i);
        if (!single_result) continue;

        json_t *error = json_object_get(single_result, "error");
        if (error && json_is_string(error)) {
            const char *error_str = json_string_value(error);

            if (strstr(error_str, "Rate limit")) {
                rate_limit = true;
            } else if (strstr(error_str, "Duplicate")) {
                has_duplicate_errors = true;
            } else if (strstr(error_str, "Parameter") ||
                       strstr(error_str, "Missing") ||
                       strstr(error_str, "Invalid")) {
                has_parameter_errors = true;
            } else if (strstr(error_str, "Auth") ||
                       strstr(error_str, "Permission") ||
                       strstr(error_str, "Unauthorized")) {
                return MHD_HTTP_UNAUTHORIZED;
            } else if (strstr(error_str, "Not found")) {
                return MHD_HTTP_NOT_FOUND;
            } else {
                has_database_errors = true;
            }
        }
    }

    // Apply highest return code strategy
    if (rate_limit) {
        return MHD_HTTP_TOO_MANY_REQUESTS;
    } else if (has_parameter_errors) {
        return MHD_HTTP_BAD_REQUEST;
    } else if (has_database_errors) {
        return MHD_HTTP_UNPROCESSABLE_ENTITY;
    } else if (has_duplicate_errors) {
        return MHD_HTTP_OK;
    }

    return MHD_HTTP_OK;
}

json_t *build_rate_limit_result_entry(int max_queries) {
    json_t *rate_limit_error = json_object();
    if (!rate_limit_error) {
        return NULL;
    }

    json_object_set_new(rate_limit_error, "success", json_false());
    json_object_set_new(rate_limit_error, "error", json_string("Rate limit exceeded"));

    char limit_msg[100];
    snprintf(limit_msg, sizeof(limit_msg), "Query limit of %d unique queries per request exceeded", max_queries);
    json_object_set_new(rate_limit_error, "message", json_string(limit_msg));

    return rate_limit_error;
}

json_t *build_duplicate_result_entry(void) {
    json_t *duplicate_error = json_object();
    if (!duplicate_error) {
        return NULL;
    }

    json_object_set_new(duplicate_error, "success", json_false());
    json_object_set_new(duplicate_error, "error", json_string("Duplicate query"));

    return duplicate_error;
}

json_t *build_invalid_mapping_result_entry(void) {
    json_t *error_response = json_object();
    if (!error_response) {
        return NULL;
    }

    json_object_set_new(error_response, "success", json_false());
    json_object_set_new(error_response, "error", json_string("Internal error: invalid query mapping"));

    return error_response;
}

enum MHD_Result send_conduit_error_response(
    struct MHD_Connection *connection, const char *error_msg, unsigned int http_status) {
    json_t *error_response = json_object();
    if (!error_response) {
        return MHD_NO;
    }
    json_object_set_new(error_response, "success", json_false());
    json_object_set_new(error_response, "error", json_string(error_msg ? error_msg : "Unknown error"));
    return api_send_json_response(connection, error_response, http_status);
}
