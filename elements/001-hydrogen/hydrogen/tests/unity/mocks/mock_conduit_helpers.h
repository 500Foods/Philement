/*
 * Mock conduit helper functions for unit testing cap_query.c
 *
 * Provides fake implementations of the database-dependent conduit helper
 * functions called by src/api/conduit/cap/cap_query.c. This lets the
 * cap_query request handler be exercised through every error/early-return
 * branch without a live database.
 *
 * Enable with USE_MOCK_CONDUIT_HELPERS (defined in cap_query.c). The
 * function names below are mapped to the real names via macros in
 * cap_query.c so call sites are unchanged.
 */

#ifndef MOCK_CONDUIT_HELPERS_H
#define MOCK_CONDUIT_HELPERS_H

#include <stddef.h>
#include <stdbool.h>
#include <microhttpd.h>
#include <jansson.h>

// Pull in all project type definitions (AppConfig, DatabaseQueue, PayloadData, etc.)
#include <src/hydrogen.h>

// Type definitions for the conduit helpers
#include <src/api/conduit/conduit_helpers.h>
#include <src/api/conduit/helpers/cap_verify.h>

// Forward declarations for the mock fakes
enum MHD_Result mock_handle_method_validation(struct MHD_Connection *connection, const char *method);
enum MHD_Result mock_handle_request_parsing_with_buffer(struct MHD_Connection *connection,
                                                      void *buffer, json_t **request_json);
enum MHD_Result mock_handle_field_extraction(struct MHD_Connection *connection, json_t *request_json,
                                             int *query_ref, const char **database, json_t **params_json);
enum MHD_Result mock_handle_database_lookup(struct MHD_Connection *connection, const char *database,
                                            int query_ref, DatabaseQueue **db_queue, QueryCacheEntry **cache_entry,
                                            bool *query_not_found, int query_type);
enum MHD_Result mock_handle_parameter_processing(struct MHD_Connection *connection, json_t *params_json,
                                                 const DatabaseQueue *db_queue, const QueryCacheEntry *cache_entry,
                                                 const char *database, int query_ref,
                                                 ParameterList **param_list, char **converted_sql,
                                                 TypedParameter ***ordered_params, size_t *param_count,
                                                 char **message);
enum MHD_Result mock_handle_cap_queue_selection(struct MHD_Connection *connection, const char *database,
                                               int query_ref, const QueryCacheEntry *cache_entry,
                                               ParameterList *param_list, char *converted_sql,
                                               TypedParameter **ordered_params,
                                               DatabaseQueue **selected_queue);
enum MHD_Result mock_handle_query_id_generation(struct MHD_Connection *connection, const char *database,
                                                int query_ref, ParameterList *param_list, char *converted_sql,
                                                TypedParameter **ordered_params, char **query_id);
enum MHD_Result mock_handle_pending_registration(struct MHD_Connection *connection, const char *database,
                                                 int query_ref, char *query_id, ParameterList *param_list,
                                                 char *converted_sql, TypedParameter **ordered_params,
                                                 const QueryCacheEntry *cache_entry, PendingQueryResult **pending);
enum MHD_Result mock_handle_query_submission(struct MHD_Connection *connection, const char *database,
                                             int query_ref, DatabaseQueue *selected_queue, char *query_id,
                                             char *converted_sql, ParameterList *param_list,
                                             TypedParameter **ordered_params, size_t param_count,
                                             const QueryCacheEntry *cache_entry);
enum MHD_Result mock_handle_response_building(struct MHD_Connection *connection, int query_ref,
                                              const char *database, const QueryCacheEntry *cache_entry,
                                              const DatabaseQueue *selected_queue, PendingQueryResult *pending,
                                              char *query_id, char *converted_sql, ParameterList *param_list,
                                              TypedParameter **ordered_params, const char *message,
                                              bool cap_fallback, const char *intended_queue_type);
bool mock_query_statement_type_allowed(int query_type, const char *sql_template);
json_t *mock_build_invalid_queryref_response(int query_ref, const char *database, const char *message);
cap_verify_result_t mock_cap_verify_token(const char *token, const char *site_id,
                                          char *error_out, size_t error_sz);
DatabaseQueue *mock_conduit_select_query_queue(const char *database, const char *queue_type);

// Mock control/state functions
void mock_conduit_helpers_reset_all(void);

// Provide a request_json to be returned by mock_handle_request_parsing_with_buffer
// (takes ownership; pass NULL to fall back to an empty object).
void mock_conduit_set_request_json(json_t *request_json);

// Per-call control flags
void mock_conduit_set_method_validation_result(enum MHD_Result result);
void mock_conduit_set_request_parsing_result(enum MHD_Result result);
void mock_conduit_set_field_extraction_result(enum MHD_Result result);
void mock_conduit_set_database_lookup_result(enum MHD_Result result);
void mock_conduit_set_db_queue(DatabaseQueue *queue);
void mock_conduit_set_cache_entry(QueryCacheEntry *entry);
void mock_conduit_set_query_not_found(bool not_found);
void mock_conduit_set_parameter_processing_result(enum MHD_Result result);
void mock_conduit_set_converted_sql(char *sql);
void mock_conduit_set_queue_selection_result(enum MHD_Result result);
void mock_conduit_set_query_id_generation_result(enum MHD_Result result);
void mock_conduit_set_pending_registration_result(enum MHD_Result result);
void mock_conduit_set_query_submission_result(enum MHD_Result result);
void mock_conduit_set_response_building_result(enum MHD_Result result);
void mock_conduit_set_statement_type_allowed(bool allowed);
void mock_conduit_set_invalid_queryref_response(json_t *response);
void mock_conduit_set_cap_verify_result(cap_verify_result_t result);
void mock_conduit_set_cap_verify_error(const char *error);

#endif /* MOCK_CONDUIT_HELPERS_H */
