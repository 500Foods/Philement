/*
 * Mock conduit helper functions implementation for unit testing cap_query.c
 */

#include "mock_conduit_helpers.h"
#include <string.h>
#include <stdlib.h>

// Static mock state
static enum MHD_Result mock_method_validation_result = MHD_YES;
static enum MHD_Result mock_request_parsing_result = MHD_YES;
static enum MHD_Result mock_field_extraction_result = MHD_YES;
static enum MHD_Result mock_database_lookup_result = MHD_YES;
static DatabaseQueue *mock_db_queue = NULL;
static QueryCacheEntry *mock_cache_entry = NULL;
static bool mock_query_not_found = false;
static enum MHD_Result mock_parameter_processing_result = MHD_YES;
static char *mock_converted_sql = NULL;
static enum MHD_Result mock_queue_selection_result = MHD_YES;
static enum MHD_Result mock_query_id_generation_result = MHD_YES;
static enum MHD_Result mock_pending_registration_result = MHD_YES;
static enum MHD_Result mock_query_submission_result = MHD_YES;
static enum MHD_Result mock_response_building_result = MHD_YES;
static bool mock_statement_type_allowed = true;
static json_t *mock_invalid_queryref_response = NULL;
static cap_verify_result_t mock_cap_verify_result = CAP_VERIFY_OK;
static char mock_cap_verify_error[256] = {0};
static json_t *mock_request_json = NULL;

enum MHD_Result mock_handle_method_validation(struct MHD_Connection *connection, const char *method) {
    (void)connection;
    (void)method;
    return mock_method_validation_result;
}

enum MHD_Result mock_handle_request_parsing_with_buffer(struct MHD_Connection *connection,
                                                      void *buffer, json_t **request_json) {
    (void)connection;
    (void)buffer;
    if (mock_request_parsing_result == MHD_YES && request_json) {
        if (mock_request_json) {
            *request_json = mock_request_json;
        } else {
            *request_json = json_object();
        }
    }
    return mock_request_parsing_result;
}

enum MHD_Result mock_handle_field_extraction(struct MHD_Connection *connection, json_t *request_json,
                                             int *query_ref, const char **database, json_t **params_json) {
    (void)connection;
    (void)request_json;
    if (mock_field_extraction_result == MHD_YES) {
        if (query_ref) *query_ref = 1;
        if (database) *database = "test_db";
        if (params_json) *params_json = json_object();
    }
    return mock_field_extraction_result;
}

enum MHD_Result mock_handle_database_lookup(struct MHD_Connection *connection, const char *database,
                                            int query_ref, DatabaseQueue **db_queue, QueryCacheEntry **cache_entry,
                                            bool *query_not_found, int query_type) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)query_type;
    if (db_queue) *db_queue = mock_db_queue;
    if (cache_entry) *cache_entry = mock_cache_entry;
    if (query_not_found) *query_not_found = mock_query_not_found;
    return mock_database_lookup_result;
}

enum MHD_Result mock_handle_parameter_processing(struct MHD_Connection *connection, json_t *params_json,
                                                 const DatabaseQueue *db_queue, const QueryCacheEntry *cache_entry,
                                                 const char *database, int query_ref,
                                                 ParameterList **param_list, char **converted_sql,
                                                 TypedParameter ***ordered_params, size_t *param_count,
                                                 char **message) {
    (void)connection;
    (void)params_json;
    (void)db_queue;
    (void)cache_entry;
    (void)database;
    (void)query_ref;
    if (mock_parameter_processing_result == MHD_YES) {
        if (param_list) *param_list = (ParameterList *)0x1;
        if (converted_sql) *converted_sql = mock_converted_sql;
        if (ordered_params) *ordered_params = NULL;
        if (param_count) *param_count = 0;
        if (message) *message = NULL;
    }
    return mock_parameter_processing_result;
}

enum MHD_Result mock_handle_cap_queue_selection(struct MHD_Connection *connection, const char *database,
                                               int query_ref, const QueryCacheEntry *cache_entry,
                                               ParameterList *param_list, char *converted_sql,
                                               TypedParameter **ordered_params,
                                               DatabaseQueue **selected_queue) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)cache_entry;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    if (selected_queue) *selected_queue = mock_db_queue;
    return mock_queue_selection_result;
}

enum MHD_Result mock_handle_query_id_generation(struct MHD_Connection *connection, const char *database,
                                                int query_ref, ParameterList *param_list, char *converted_sql,
                                                TypedParameter **ordered_params, char **query_id) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    if (mock_query_id_generation_result == MHD_YES && query_id) {
        *query_id = strdup("conduit_test_id");
    }
    return mock_query_id_generation_result;
}

enum MHD_Result mock_handle_pending_registration(struct MHD_Connection *connection, const char *database,
                                                 int query_ref, char *query_id, ParameterList *param_list,
                                                 char *converted_sql, TypedParameter **ordered_params,
                                                 const QueryCacheEntry *cache_entry, PendingQueryResult **pending) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)query_id;
    (void)param_list;
    (void)converted_sql;
    (void)ordered_params;
    (void)cache_entry;
    if (mock_pending_registration_result == MHD_YES && pending) {
        *pending = (PendingQueryResult *)0x2;
    }
    return mock_pending_registration_result;
}

enum MHD_Result mock_handle_query_submission(struct MHD_Connection *connection, const char *database,
                                             int query_ref, DatabaseQueue *selected_queue, char *query_id,
                                             char *converted_sql, ParameterList *param_list,
                                             TypedParameter **ordered_params, size_t param_count,
                                             const QueryCacheEntry *cache_entry) {
    (void)connection;
    (void)database;
    (void)query_ref;
    (void)selected_queue;
    (void)query_id;
    (void)converted_sql;
    (void)param_list;
    (void)ordered_params;
    (void)param_count;
    (void)cache_entry;
    return mock_query_submission_result;
}

enum MHD_Result mock_handle_response_building(struct MHD_Connection *connection, int query_ref,
                                              const char *database, const QueryCacheEntry *cache_entry,
                                              const DatabaseQueue *selected_queue, PendingQueryResult *pending,
                                              char *query_id, char *converted_sql, ParameterList *param_list,
                                              TypedParameter **ordered_params, const char *message,
                                              bool cap_fallback, const char *intended_queue_type) {
    (void)connection;
    (void)query_ref;
    (void)database;
    (void)cache_entry;
    (void)selected_queue;
    (void)pending;
    (void)query_id;
    (void)converted_sql;
    (void)param_list;
    (void)ordered_params;
    (void)message;
    (void)cap_fallback;
    (void)intended_queue_type;
    return mock_response_building_result;
}

bool mock_query_statement_type_allowed(int query_type, const char *sql_template) {
    (void)query_type;
    (void)sql_template;
    return mock_statement_type_allowed;
}

json_t *mock_build_invalid_queryref_response(int query_ref, const char *database, const char *message) {
    (void)query_ref;
    (void)database;
    (void)message;
    if (mock_invalid_queryref_response) {
        return json_incref(mock_invalid_queryref_response);
    }
    return json_object();
}

cap_verify_result_t mock_cap_verify_token(const char *token, const char *site_id,
                                          char *error_out, size_t error_sz) {
    (void)token;
    (void)site_id;
    if (error_out && error_sz > 0) {
        strncpy(error_out, mock_cap_verify_error, error_sz - 1);
        error_out[error_sz - 1] = '\0';
    }
    return mock_cap_verify_result;
}

DatabaseQueue *mock_conduit_select_query_queue(const char *database, const char *queue_type) {
    (void)database;
    (void)queue_type;
    return mock_db_queue;
}

void mock_conduit_helpers_reset_all(void) {
    mock_method_validation_result = MHD_YES;
    mock_request_parsing_result = MHD_YES;
    mock_field_extraction_result = MHD_YES;
    mock_database_lookup_result = MHD_YES;
    mock_db_queue = NULL;
    mock_cache_entry = NULL;
    mock_query_not_found = false;
    mock_parameter_processing_result = MHD_YES;
    mock_converted_sql = NULL;
    mock_queue_selection_result = MHD_YES;
    mock_query_id_generation_result = MHD_YES;
    mock_pending_registration_result = MHD_YES;
    mock_query_submission_result = MHD_YES;
    mock_response_building_result = MHD_YES;
    mock_statement_type_allowed = true;
    if (mock_invalid_queryref_response) {
        json_decref(mock_invalid_queryref_response);
        mock_invalid_queryref_response = NULL;
    }
    mock_cap_verify_result = CAP_VERIFY_OK;
    mock_cap_verify_error[0] = '\0';
    if (mock_request_json) {
        json_decref(mock_request_json);
        mock_request_json = NULL;
    }
}

void mock_conduit_set_request_json(json_t *request_json) {
    if (mock_request_json) {
        json_decref(mock_request_json);
    }
    mock_request_json = request_json ? json_incref(request_json) : NULL;
}

void mock_conduit_set_method_validation_result(enum MHD_Result result) {
    mock_method_validation_result = result;
}

void mock_conduit_set_request_parsing_result(enum MHD_Result result) {
    mock_request_parsing_result = result;
}

void mock_conduit_set_field_extraction_result(enum MHD_Result result) {
    mock_field_extraction_result = result;
}

void mock_conduit_set_database_lookup_result(enum MHD_Result result) {
    mock_database_lookup_result = result;
}

void mock_conduit_set_db_queue(DatabaseQueue *queue) {
    mock_db_queue = queue;
}

void mock_conduit_set_cache_entry(QueryCacheEntry *entry) {
    mock_cache_entry = entry;
}

void mock_conduit_set_query_not_found(bool not_found) {
    mock_query_not_found = not_found;
}

void mock_conduit_set_parameter_processing_result(enum MHD_Result result) {
    mock_parameter_processing_result = result;
}

void mock_conduit_set_converted_sql(char *sql) {
    mock_converted_sql = sql;
}

void mock_conduit_set_queue_selection_result(enum MHD_Result result) {
    mock_queue_selection_result = result;
}

void mock_conduit_set_query_id_generation_result(enum MHD_Result result) {
    mock_query_id_generation_result = result;
}

void mock_conduit_set_pending_registration_result(enum MHD_Result result) {
    mock_pending_registration_result = result;
}

void mock_conduit_set_query_submission_result(enum MHD_Result result) {
    mock_query_submission_result = result;
}

void mock_conduit_set_response_building_result(enum MHD_Result result) {
    mock_response_building_result = result;
}

void mock_conduit_set_statement_type_allowed(bool allowed) {
    mock_statement_type_allowed = allowed;
}

void mock_conduit_set_invalid_queryref_response(json_t *response) {
    if (mock_invalid_queryref_response) {
        json_decref(mock_invalid_queryref_response);
    }
    mock_invalid_queryref_response = response ? json_incref(response) : NULL;
}

void mock_conduit_set_cap_verify_result(cap_verify_result_t result) {
    mock_cap_verify_result = result;
}

void mock_conduit_set_cap_verify_error(const char *error) {
    if (error) {
        strncpy(mock_cap_verify_error, error, sizeof(mock_cap_verify_error) - 1);
        mock_cap_verify_error[sizeof(mock_cap_verify_error) - 1] = '\0';
    } else {
        mock_cap_verify_error[0] = '\0';
    }
}
