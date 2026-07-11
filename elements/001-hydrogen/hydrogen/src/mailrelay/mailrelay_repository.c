/*
 * Mail Relay Repository implementation.
 *
 * Callback-based QueryRef execution for Mail Relay. The default executor
 * resolves the configured Mail Relay database, submits the query to the
 * database queue, waits for the result, and invokes the caller's callback.
 * Unit tests can replace the executor seam to assert on parameters.
 */

// Project includes
#include <src/hydrogen.h>

// Third-party includes
#include <jansson.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_pending.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/api/conduit/query/query.h>

// Mail Relay includes
#include <src/mailrelay/mailrelay_repository.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Default query timeout when no config value is available.
#define MAILRELAY_REPO_DEFAULT_TIMEOUT_SECONDS 30

// Maximum JSON parameter value length we will include in a log line.
#define MAILRELAY_REPO_MAX_LOG_PARAM_LEN 64

// Current executor seam.
static mailrelay_repo_execute_fn g_executor = NULL;

// Forward declaration of the default executor.
static bool mailrelay_repo_default_execute(int query_ref,
                                           const char* params_json,
                                           mailrelay_repo_callback_fn callback,
                                           void* user_data);

/*
 * Resolve the database name for Mail Relay queries.
 * Returns app_config->mail_relay.Database if set. If it is not set and there
 * is exactly one configured database connection, that connection's name is
 * returned. Otherwise returns NULL.
 */
static const char* mailrelay_repo_resolve_database(void) {
    if (!app_config) {
        return NULL;
    }
    const char* database = app_config->mail_relay.Database;
    if (database && database[0] != '\0') {
        return database;
    }
    if (app_config->databases.connection_count == 1 &&
        app_config->databases.connections[0].name &&
        app_config->databases.connections[0].name[0] != '\0') {
        database = app_config->databases.connections[0].name;
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: no explicit database configured; using single database '%s'",
                 LOG_LEVEL_STATE, 1, database);
        return database;
    }
    log_this(SR_MAIL_RELAY,
             "Mail Relay repository: no database configured and no single database to default to",
             LOG_LEVEL_ERROR, 0);
    return NULL;
}

/*
 * Build the result object for a callback invocation.
 */
static void mailrelay_repo_invoke_callback(mailrelay_repo_callback_fn callback,
                                             void* user_data,
                                             MailRelayRepoStatus status,
                                             const char* error_message,
                                             json_t* data,
                                             int affected_rows) {
    MailRelayRepoResult result = {
        .status = status,
        .error_message = error_message,
        .data = data,
        .affected_rows = affected_rows
    };
    if (callback) {
        callback(&result, user_data);
    }
    (void)result; // callback may be NULL in some test paths; result is on stack
}

/*
 * Default executor: resolve database, lookup QTC, submit query, wait, callback.
 */
static bool mailrelay_repo_default_execute(int query_ref,
                                           const char* params_json,
                                           mailrelay_repo_callback_fn callback,
                                           void* user_data) {
    (void)params_json;

    const char* database = mailrelay_repo_resolve_database();
    if (!database) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_NO_DATABASE,
                                       "Mail Relay database not configured",
                                       NULL, 0);
        return false;
    }

    DatabaseQueue* db_queue = database_queue_manager_get_database(
        global_queue_manager, database);
    if (!db_queue) {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: database queue not found for '%s'",
                 LOG_LEVEL_ERROR, 1, database);
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_NO_DATABASE,
                                       "Mail Relay database queue not available",
                                       NULL, 0);
        return false;
    }

    QueryCacheEntry* cache_entry = query_cache_lookup(
        db_queue->query_cache, query_ref, SR_MAIL_RELAY);
    if (!cache_entry) {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: QueryRef %d not found in cache for database '%s'",
                 LOG_LEVEL_ERROR, 2, query_ref, database);
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_QUERY_NOT_FOUND,
                                       "Mail Relay QueryRef not found in cache",
                                       NULL, 0);
        return false;
    }

    char* query_id = generate_query_id();
    if (!query_id) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_SUBMIT_FAILED,
                                       "Failed to generate query ID",
                                       NULL, 0);
        return false;
    }

    DatabaseQuery db_query = {
        .query_id = query_id,
        .query_template = strdup(cache_entry->sql_template),
        .parameter_json = params_json ? strdup(params_json) : NULL,
        .queue_type_hint = database_queue_type_from_string(cache_entry->queue_type),
        .submitted_at = time(NULL),
        .submitted_at_ns = 0,
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL,
        .affected_rows = 0
    };
    if (!db_query.query_template) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_SUBMIT_FAILED,
                                       "Failed to duplicate query template",
                                       NULL, 0);
        free(query_id);
        return false;
    }

    PendingResultManager* pending_mgr = get_pending_result_manager();
    if (!pending_mgr) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_SUBMIT_FAILED,
                                       "Pending result manager unavailable",
                                       NULL, 0);
        free(query_id);
        free(db_query.query_template);
        free(db_query.parameter_json);
        return false;
    }

    PendingQueryResult* pending = pending_result_register(
        pending_mgr, query_id, MAILRELAY_REPO_DEFAULT_TIMEOUT_SECONDS, SR_MAIL_RELAY);
    if (!pending) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_SUBMIT_FAILED,
                                       "Failed to register pending result",
                                       NULL, 0);
        free(query_id);
        free(db_query.query_template);
        free(db_query.parameter_json);
        return false;
    }

    bool submit_success = database_queue_submit_query(db_queue, &db_query);
    if (!submit_success) {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: failed to submit query for QueryRef %d",
                 LOG_LEVEL_ERROR, 1, query_ref);
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_SUBMIT_FAILED,
                                       "Failed to submit query",
                                       NULL, 0);
        pending_result_unregister(pending_mgr, pending, SR_MAIL_RELAY);
        free(query_id);
        free(db_query.query_template);
        free(db_query.parameter_json);
        return false;
    }

    int wait_result = pending_result_wait(pending, SR_MAIL_RELAY);
    if (wait_result != 0) {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: query timed out for QueryRef %d",
                 LOG_LEVEL_ERROR, 1, query_ref);
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_TIMEOUT,
                                       "Query execution timed out",
                                       NULL, 0);
        pending_result_unregister(pending_mgr, pending, SR_MAIL_RELAY);
        free(query_id);
        free(db_query.query_template);
        free(db_query.parameter_json);
        return false;
    }

    QueryResult* query_result = pending_result_get(pending);
    json_t* data = NULL;
    bool data_owned = false;
    int affected_rows = 0;
    MailRelayRepoStatus repo_status = MAILRELAY_REPO_OK;
    const char* error_message = NULL;

    if (query_result && query_result->error_message) {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay repository: query failed for QueryRef %d: %s",
                 LOG_LEVEL_ERROR, 2, query_ref, query_result->error_message);
        repo_status = MAILRELAY_REPO_QUERY_ERROR;
        error_message = query_result->error_message;
    } else if (query_result && query_result->data_json && query_result->data_json[0] != '\0') {
        json_error_t err;
        data = json_loads(query_result->data_json, 0, &err);
        if (!data) {
            log_this(SR_MAIL_RELAY,
                     "Mail Relay repository: failed to parse result JSON for QueryRef %d: %s",
                     LOG_LEVEL_ERROR, 2, query_ref, err.text);
            repo_status = MAILRELAY_REPO_PARSE_ERROR;
            error_message = "Failed to parse query result JSON";
        } else {
            data_owned = true;
            affected_rows = query_result->affected_rows;
        }
    }

    mailrelay_repo_invoke_callback(callback, user_data, repo_status, error_message,
                                   data, affected_rows);

    if (data_owned && data) {
        json_decref(data);
    }

    pending_result_unregister(pending_mgr, pending, SR_MAIL_RELAY);
    free(query_id);
    free(db_query.query_template);
    free(db_query.parameter_json);

    return repo_status == MAILRELAY_REPO_OK;
}

/* --------------------------------------------------------------------------
 * Executor seam management
 * -------------------------------------------------------------------------- */

void mailrelay_repo_set_executor(mailrelay_repo_execute_fn executor) {
    g_executor = executor;
}

mailrelay_repo_execute_fn mailrelay_repo_get_executor(void) {
    if (g_executor == NULL) {
        g_executor = mailrelay_repo_default_execute;
    }
    return g_executor;
}

/* --------------------------------------------------------------------------
 * JSON parameter building helpers
 * -------------------------------------------------------------------------- */

static json_t* repo_params_new(void) {
    json_t* root = json_object();
    if (!root) {
        return NULL;
    }
    json_t* string_obj = json_object();
    json_t* integer_obj = json_object();
    json_t* boolean_obj = json_object();
    if (!string_obj || !integer_obj || !boolean_obj) {
        json_decref(root);
        json_decref(string_obj);
        json_decref(integer_obj);
        json_decref(boolean_obj);
        return NULL;
    }
    json_object_set_new(root, "STRING", string_obj);
    json_object_set_new(root, "INTEGER", integer_obj);
    json_object_set_new(root, "BOOLEAN", boolean_obj);
    return root;
}

static bool repo_add_string(json_t* root, const char* name, const char* value) {
    if (!root || !name) {
        return false;
    }
    json_t* string_obj = json_object_get(root, "STRING");
    if (!string_obj) {
        return false;
    }
    if (value) {
        return json_object_set_new(string_obj, name, json_string(value)) == 0;
    }
    return json_object_set_new(string_obj, name, json_null()) == 0;
}

static bool repo_add_int(json_t* root, const char* name, int value) {
    if (!root || !name) {
        return false;
    }
    json_t* integer_obj = json_object_get(root, "INTEGER");
    if (!integer_obj) {
        return false;
    }
    return json_object_set_new(integer_obj, name, json_integer(value)) == 0;
}

static bool repo_add_int64(json_t* root, const char* name, long long value) {
    if (!root || !name) {
        return false;
    }
    json_t* integer_obj = json_object_get(root, "INTEGER");
    if (!integer_obj) {
        return false;
    }
    return json_object_set_new(integer_obj, name, json_integer((json_int_t)value)) == 0;
}

/*
 * Execute a query ref with the given JSON parameter object. The parameter
 * object is consumed (decref'd) regardless of success or failure.
 */
static bool repo_execute_json(int query_ref, json_t* params,
                              mailrelay_repo_callback_fn callback,
                              void* user_data) {
    if (!params) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_INVALID_ARGS,
                                       "Failed to build query parameters",
                                       NULL, 0);
        return false;
    }
    char* params_json = json_dumps(params, JSON_COMPACT);
    json_decref(params);
    if (!params_json) {
        mailrelay_repo_invoke_callback(callback, user_data,
                                       MAILRELAY_REPO_INVALID_ARGS,
                                       "Failed to serialize query parameters",
                                       NULL, 0);
        return false;
    }

    mailrelay_repo_execute_fn executor = mailrelay_repo_get_executor();
    bool result = executor(query_ref, params_json, callback, user_data);
    free(params_json);
    return result;
}

/*
 * Helper for parameter-less queries. Builds an empty JSON parameter object.
 */
static bool repo_execute_empty(int query_ref,
                               mailrelay_repo_callback_fn callback,
                               void* user_data) {
    json_t* params = repo_params_new();
    return repo_execute_json(query_ref, params, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Queue QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_queue_insert(const MailRelayRepoQueueInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "MESSAGE_UUID", params->message_uuid);
    repo_add_int(p, "PRIORITY", params->priority);
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "FROM_ADDR", params->from_addr);
    repo_add_string(p, "REPLY_TO", params->reply_to);
    repo_add_string(p, "RECIPIENTS_JSON", params->recipients_json);
    repo_add_string(p, "SUBJECT", params->subject);
    repo_add_string(p, "BODY_TEXT", params->body_text);
    repo_add_string(p, "BODY_HTML", params->body_html);
    repo_add_string(p, "HEADERS_JSON", params->headers_json);
    repo_add_string(p, "IDEMPOTENCY_KEY", params->idempotency_key);
    repo_add_string(p, "NEXT_ATTEMPT_AT", params->next_attempt_at);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_INSERT, p, callback, user_data);
}

bool mailrelay_repo_queue_get_by_uuid(const MailRelayRepoQueueGetByUuid* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "MESSAGE_UUID", params->message_uuid);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_GET_BY_UUID, p, callback, user_data);
}

bool mailrelay_repo_queue_get_by_idempotency(
    const MailRelayRepoQueueGetByIdempotency* params,
    mailrelay_repo_callback_fn callback,
    void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "IDEMPOTENCY_KEY", params->idempotency_key);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_GET_BY_IDEMPOTENCY, p, callback, user_data);
}

bool mailrelay_repo_queue_select_next_pending(mailrelay_repo_callback_fn callback,
                                              void* user_data) {
    if (!callback) {
        return false;
    }
    return repo_execute_empty(MAILRELAY_QREF_QUEUE_SELECT_NEXT_PENDING, callback, user_data);
}

bool mailrelay_repo_queue_mark_sending(const MailRelayRepoQueueMarkSending* params,
                                       mailrelay_repo_callback_fn callback,
                                       void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    repo_add_string(p, "INSTANCE_ID", params->instance_id);
    repo_add_string(p, "CLAIM_TOKEN", params->claim_token);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_MARK_SENDING, p, callback, user_data);
}

bool mailrelay_repo_queue_mark_sent(const MailRelayRepoQueueMarkSent* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    repo_add_int(p, "SMTP_CODE", params->smtp_code);
    repo_add_string(p, "SMTP_TEXT", params->smtp_text);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_MARK_SENT, p, callback, user_data);
}

bool mailrelay_repo_queue_mark_failed(const MailRelayRepoQueueMarkFailed* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    repo_add_int(p, "SMTP_CODE", params->smtp_code);
    repo_add_string(p, "SMTP_TEXT", params->smtp_text);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_MARK_FAILED, p, callback, user_data);
}

bool mailrelay_repo_queue_reschedule(const MailRelayRepoQueueReschedule* params,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    repo_add_string(p, "NEXT_ATTEMPT_AT", params->next_attempt_at);
    repo_add_int(p, "ATTEMPTS", params->attempts);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_RESCHEDULE, p, callback, user_data);
}

bool mailrelay_repo_queue_recover_stale(const MailRelayRepoQueueRecoverStale* params,
                                        mailrelay_repo_callback_fn callback,
                                        void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "STALE_BEFORE", params->stale_before);
    return repo_execute_json(MAILRELAY_QREF_QUEUE_RECOVER_STALE, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Attempts QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_attempt_insert(const MailRelayRepoAttemptInsert* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    repo_add_int(p, "ATTEMPT_NUMBER", params->attempt_number);
    repo_add_int(p, "SERVER_INDEX", params->server_index);
    repo_add_int(p, "SUCCESS", params->success ? 1 : 0);
    repo_add_int(p, "SMTP_CODE", params->smtp_code);
    repo_add_string(p, "SMTP_TEXT", params->smtp_text);
    repo_add_int64(p, "DURATION_MS", params->duration_ms);
    repo_add_string(p, "ERROR_CLASS", params->error_class);
    return repo_execute_json(MAILRELAY_QREF_ATTEMPT_INSERT, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Template QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_template_get_by_key(const MailRelayRepoTemplateGetByKey* params,
                                        mailrelay_repo_callback_fn callback,
                                        void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    return repo_execute_json(MAILRELAY_QREF_TEMPLATE_GET_BY_KEY, p, callback, user_data);
}

bool mailrelay_repo_template_list_active(mailrelay_repo_callback_fn callback,
                                         void* user_data) {
    if (!callback) {
        return false;
    }
    return repo_execute_empty(MAILRELAY_QREF_TEMPLATE_LIST_ACTIVE, callback, user_data);
}

bool mailrelay_repo_template_insert(const MailRelayRepoTemplateInsert* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "NAME", params->name);
    repo_add_int(p, "STATUS_A64", params->status_a64);
    repo_add_string(p, "SUBJECT_TEMPLATE", params->subject_template);
    repo_add_string(p, "TEXT_TEMPLATE", params->text_template);
    repo_add_string(p, "HTML_TEMPLATE", params->html_template);
    repo_add_string(p, "COLLECTION", params->collection);
    return repo_execute_json(MAILRELAY_QREF_TEMPLATE_INSERT, p, callback, user_data);
}

bool mailrelay_repo_template_update(const MailRelayRepoTemplateUpdate* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "NAME", params->name);
    repo_add_int(p, "STATUS_A64", params->status_a64);
    repo_add_string(p, "SUBJECT_TEMPLATE", params->subject_template);
    repo_add_string(p, "TEXT_TEMPLATE", params->text_template);
    repo_add_string(p, "HTML_TEMPLATE", params->html_template);
    repo_add_string(p, "COLLECTION", params->collection);
    return repo_execute_json(MAILRELAY_QREF_TEMPLATE_UPDATE, p, callback, user_data);
}

bool mailrelay_repo_template_soft_delete(const MailRelayRepoTemplateSoftDelete* params,
                                         mailrelay_repo_callback_fn callback,
                                         void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    return repo_execute_json(MAILRELAY_QREF_TEMPLATE_SOFT_DELETE, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Event QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_event_insert(const MailRelayRepoEventInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "EVENT_KEY", params->event_key);
    repo_add_int(p, "STATUS_A65", params->status_a65);
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "FROM_ADDR", params->from_addr);
    repo_add_string(p, "REPLY_TO", params->reply_to);
    repo_add_string(p, "RECIPIENTS_JSON", params->recipients_json);
    repo_add_string(p, "SUBJECT", params->subject);
    repo_add_string(p, "BODY_TEXT", params->body_text);
    repo_add_string(p, "BODY_HTML", params->body_html);
    repo_add_string(p, "HEADERS_JSON", params->headers_json);
    repo_add_string(p, "PARAMS_JSON", params->params_json);
    repo_add_string(p, "DEBOUNCE_KEY", params->debounce_key);
    repo_add_string(p, "IDEMPOTENCY_KEY", params->idempotency_key);
    repo_add_int(p, "PRIORITY", params->priority);
    return repo_execute_json(MAILRELAY_QREF_EVENT_INSERT, p, callback, user_data);
}

bool mailrelay_repo_event_list_pending(mailrelay_repo_callback_fn callback,
                                       void* user_data) {
    if (!callback) {
        return false;
    }
    return repo_execute_empty(MAILRELAY_QREF_EVENT_LIST_PENDING, callback, user_data);
}

bool mailrelay_repo_event_mark_processed(const MailRelayRepoEventMarkProcessed* params,
                                         mailrelay_repo_callback_fn callback,
                                         void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "EVENT_ID", params->event_id);
    repo_add_int(p, "STATUS_A65", params->status_a65);
    repo_add_int64(p, "QUEUE_ID", params->queue_id);
    return repo_execute_json(MAILRELAY_QREF_EVENT_MARK_PROCESSED, p, callback, user_data);
}

bool mailrelay_repo_event_mark_suppressed(const MailRelayRepoEventMarkSuppressed* params,
                                          mailrelay_repo_callback_fn callback,
                                          void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "EVENT_ID", params->event_id);
    return repo_execute_json(MAILRELAY_QREF_EVENT_MARK_SUPPRESSED, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * OTP QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_otp_insert(const MailRelayRepoOtpInsert* params,
                               mailrelay_repo_callback_fn callback,
                               void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "CODE_HASH", params->code_hash);
    repo_add_string(p, "EMAIL", params->email);
    repo_add_int64(p, "ACCOUNT_ID", params->account_id);
    repo_add_int(p, "PURPOSE_A66", params->purpose_a66);
    repo_add_int(p, "STATUS_A67", params->status_a67);
    repo_add_string(p, "EXPIRY_AT", params->expiry_at);
    repo_add_int(p, "MAX_ATTEMPTS", params->max_attempts);
    return repo_execute_json(MAILRELAY_QREF_OTP_INSERT, p, callback, user_data);
}

bool mailrelay_repo_otp_get_active(const MailRelayRepoOtpGetActive* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "EMAIL", params->email);
    repo_add_int(p, "PURPOSE_A66", params->purpose_a66);
    return repo_execute_json(MAILRELAY_QREF_OTP_GET_ACTIVE, p, callback, user_data);
}

bool mailrelay_repo_otp_consume(const MailRelayRepoOtpConsume* params,
                                mailrelay_repo_callback_fn callback,
                                void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "OTP_ID", params->otp_id);
    return repo_execute_json(MAILRELAY_QREF_OTP_CONSUME, p, callback, user_data);
}

bool mailrelay_repo_otp_increment_attempts(const MailRelayRepoOtpIncrementAttempts* params,
                                           mailrelay_repo_callback_fn callback,
                                           void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "OTP_ID", params->otp_id);
    return repo_execute_json(MAILRELAY_QREF_OTP_INCREMENT_ATTEMPTS, p, callback, user_data);
}

bool mailrelay_repo_otp_expire_old(const MailRelayRepoOtpExpireOld* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "EXPIRY_CUTOFF_AT", params->expiry_cutoff_at);
    return repo_execute_json(MAILRELAY_QREF_OTP_EXPIRE_OLD, p, callback, user_data);
}

bool mailrelay_repo_otp_get_by_id(const MailRelayRepoOtpGetById* params,
                                  mailrelay_repo_callback_fn callback,
                                  void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "OTP_ID", params->otp_id);
    return repo_execute_json(MAILRELAY_QREF_OTP_GET_BY_ID, p, callback, user_data);
}

bool mailrelay_repo_otp_mark_max_attempts(const MailRelayRepoOtpMarkMaxAttempts* params,
                                          mailrelay_repo_callback_fn callback,
                                          void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "OTP_ID", params->otp_id);
    return repo_execute_json(MAILRELAY_QREF_OTP_MARK_MAX_ATTEMPTS, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Route QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_route_get_by_sender_domain(
    const MailRelayRepoRouteGetBySenderDomain* params,
    mailrelay_repo_callback_fn callback,
    void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "SENDER_DOMAIN", params->sender_domain);
    return repo_execute_json(MAILRELAY_QREF_ROUTE_GET_BY_SENDER_DOMAIN, p, callback, user_data);
}

bool mailrelay_repo_route_list_active(mailrelay_repo_callback_fn callback,
                                      void* user_data) {
    if (!callback) {
        return false;
    }
    return repo_execute_empty(MAILRELAY_QREF_ROUTE_LIST_ACTIVE, callback, user_data);
}

bool mailrelay_repo_route_insert(const MailRelayRepoRouteInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int(p, "STATUS_A68", params->status_a68);
    repo_add_string(p, "SOURCE_NETWORK", params->source_network);
    repo_add_string(p, "SENDER_DOMAIN", params->sender_domain);
    repo_add_string(p, "SENDER_PATTERN", params->sender_pattern);
    repo_add_string(p, "RECIPIENT_DOMAIN", params->recipient_domain);
    repo_add_string(p, "RECIPIENT_PATTERN", params->recipient_pattern);
    repo_add_int(p, "AUTH_REQUIRED", params->auth_required ? 1 : 0);
    repo_add_int(p, "REQUIRE_TLS", params->require_tls ? 1 : 0);
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "REWRITE_FROM", params->rewrite_from);
    repo_add_string(p, "REWRITE_TO", params->rewrite_to);
    repo_add_string(p, "ADD_RECIPIENTS_JSON", params->add_recipients_json);
    repo_add_int(p, "PRIORITY", params->priority);
    repo_add_int(p, "SORT_SEQ", params->sort_seq);
    return repo_execute_json(MAILRELAY_QREF_ROUTE_INSERT, p, callback, user_data);
}

bool mailrelay_repo_route_update(const MailRelayRepoRouteUpdate* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "ROUTE_ID", params->route_id);
    repo_add_int(p, "STATUS_A68", params->status_a68);
    repo_add_string(p, "SOURCE_NETWORK", params->source_network);
    repo_add_string(p, "SENDER_DOMAIN", params->sender_domain);
    repo_add_string(p, "SENDER_PATTERN", params->sender_pattern);
    repo_add_string(p, "RECIPIENT_DOMAIN", params->recipient_domain);
    repo_add_string(p, "RECIPIENT_PATTERN", params->recipient_pattern);
    repo_add_int(p, "AUTH_REQUIRED", params->auth_required ? 1 : 0);
    repo_add_int(p, "REQUIRE_TLS", params->require_tls ? 1 : 0);
    repo_add_string(p, "TEMPLATE_KEY", params->template_key);
    repo_add_string(p, "REWRITE_FROM", params->rewrite_from);
    repo_add_string(p, "REWRITE_TO", params->rewrite_to);
    repo_add_string(p, "ADD_RECIPIENTS_JSON", params->add_recipients_json);
    repo_add_int(p, "PRIORITY", params->priority);
    repo_add_int(p, "SORT_SEQ", params->sort_seq);
    return repo_execute_json(MAILRELAY_QREF_ROUTE_UPDATE, p, callback, user_data);
}

bool mailrelay_repo_route_soft_delete(const MailRelayRepoRouteSoftDelete* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_int64(p, "ROUTE_ID", params->route_id);
    return repo_execute_json(MAILRELAY_QREF_ROUTE_SOFT_DELETE, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Cleanup QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_cleanup_queue(const MailRelayRepoCleanupQueue* params,
                                  mailrelay_repo_callback_fn callback,
                                  void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "CUTOFF_AT", params->cutoff_at);
    return repo_execute_json(MAILRELAY_QREF_CLEANUP_QUEUE, p, callback, user_data);
}

bool mailrelay_repo_cleanup_events(const MailRelayRepoCleanupEvents* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "CUTOFF_AT", params->cutoff_at);
    return repo_execute_json(MAILRELAY_QREF_CLEANUP_EVENTS, p, callback, user_data);
}

bool mailrelay_repo_cleanup_attempts(const MailRelayRepoCleanupAttempts* params,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "CUTOFF_AT", params->cutoff_at);
    return repo_execute_json(MAILRELAY_QREF_CLEANUP_ATTEMPTS, p, callback, user_data);
}

bool mailrelay_repo_cleanup_otp(const MailRelayRepoCleanupOtp* params,
                                  mailrelay_repo_callback_fn callback,
                                  void* user_data) {
    if (!params || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "CUTOFF_AT", params->cutoff_at);
    return repo_execute_json(MAILRELAY_QREF_CLEANUP_OTP, p, callback, user_data);
}

/* --------------------------------------------------------------------------
 * Role QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_role_get_by_name(const char* name,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data) {
    if (!name || !callback) {
        return false;
    }
    json_t* p = repo_params_new();
    if (!p) {
        return false;
    }
    repo_add_string(p, "ROLENAME", name);
    return repo_execute_json(MAILRELAY_QREF_ROLE_GET_BY_NAME, p, callback, user_data);
}
