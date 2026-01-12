/*
 * Auth Service Database Functions
 *
 * This file contains all database-related operations including:
 * - Query execution wrappers
 * - Account lookup and management
 * - Password verification
 * - JWT storage and revocation
 * - Login attempt logging
 * - IP blocking
 */

// Global includes
#include <src/hydrogen.h>
#include <string.h>
#include <src/logging/logging.h>

// Local includes
#include "auth_service.h"
#include "auth_service_database.h"
#include "auth_service_jwt.h"
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/api/conduit/query/query.h>
#include <jansson.h>
#include <src/config/config_databases.h>

/**
 * Execute a database query using the conduit system
 * Returns QueryResult or NULL on error
 */
QueryResult* execute_auth_query(int query_ref, const char* database, json_t* params) {
    if (!database || query_ref <= 0) {
        log_this(SR_AUTH, "Invalid parameters for database query", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Lookup the database queue
    DatabaseQueue* db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        log_this(SR_AUTH, "Database queue not found: %s", LOG_LEVEL_ERROR, 1, database);
        return NULL;
    }

    // Lookup query cache entry
    const QueryCacheEntry* cache_entry = lookup_query_cache_entry(db_queue, query_ref);
    if (!cache_entry) {
        log_this("AUTH", "QueryRef %d not found in cache for database %s", LOG_LEVEL_ERROR, 2, query_ref, database);
        return NULL;
    }

    // Merge database connection parameters with query parameters
    json_t* merged_params = params; // Default to original params
    if (app_config) {
        const DatabaseConnection* conn = find_database_connection(&app_config->databases, database);
        if (conn && conn->parameters) {
            merged_params = merge_database_parameters(conn, params);
            log_this("AUTH", "Merged database connection parameters for database: %s", LOG_LEVEL_DEBUG, 1, database);
        }
    }

    // Convert parameters to JSON string
    char* params_json = json_dumps(merged_params, JSON_COMPACT);
    if (!params_json) {
        log_this("AUTH", "Failed to serialize parameters to JSON", LOG_LEVEL_ERROR, 0);
        if (merged_params != params) {
            json_decref(merged_params);
        }
        return NULL;
    }

    // Generate query ID
    char* query_id = generate_query_id();
    if (!query_id) {
        free(params_json);
        log_this("AUTH", "Failed to generate query ID", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Create and submit database query
    DatabaseQuery db_query = {
        .query_id = query_id,
        .query_template = strdup(cache_entry->sql_template),
        .parameter_json = params_json,
        .queue_type_hint = database_queue_type_from_string(cache_entry->queue_type),
        .submitted_at = time(NULL),
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };

    bool submit_success = database_queue_submit_query(db_queue, &db_query);
    if (!submit_success) {
        log_this("AUTH", "Failed to submit query to database queue", LOG_LEVEL_ERROR, 0);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }

    // Wait for query result
    const DatabaseQuery* result_query = database_queue_await_result(db_queue, query_id, 30); // 30 second timeout
    if (!result_query) {
        log_this("AUTH", "Query execution timed out or failed: %s", LOG_LEVEL_ERROR, 1, query_id);
        free(query_id);
        return NULL;
    }

    // Parse the result
    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) {
        log_this("AUTH", "Failed to allocate memory for query result", LOG_LEVEL_ERROR, 0);
        free(query_id);
        return NULL;
    }

    if (result_query->error_message) {
        result->success = false;
        result->error_message = strdup(result_query->error_message);
        log_this("AUTH", "Query execution error: %s", LOG_LEVEL_ERROR, 1, result->error_message);
    } else {
        result->success = true;
        // Note: await_result stores data_json in query_template (see submit.c line 298)
        result->data_json = result_query->query_template ? strdup(result_query->query_template) : NULL;
        result->execution_time_ms = time(NULL) - result_query->submitted_at;
    }

    // Cleanup
    free(query_id);
    free(db_query.query_template);
    if (merged_params != params) {
        json_decref(merged_params);
    }

    return result;
}

/**
 * Lookup account information from database
 * Note: Actual authorization (status check) happens during password verification in QueryRef #012
 * which requires both correct password AND status_a16=1 (Active)
 */
account_info_t* lookup_account(const char* login_id, const char* database) {
    if (!login_id || !database) return NULL;

    // Create parameters for QueryRef #008: Get Account ID
    // Use typed parameter format: {"STRING": {"LOGINID": "value"}}
    // Parameter name must match SQL placeholder :LOGINID
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_object_set_new(string_params, "LOGINID", json_string(login_id));
    json_object_set_new(params, "STRING", string_params);

    // Execute query
    QueryResult* result = execute_auth_query(8, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to lookup account: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
        if (result) {
            free(result->error_message);
            free(result);
        }
        return NULL;
    }

    // Parse result JSON
    json_t* result_json = json_loads(result->data_json, 0, NULL);
    if (!result_json) {
        log_this("AUTH", "Failed to parse account lookup result", LOG_LEVEL_ERROR, 0);
        free(result->data_json);
        free(result);
        return NULL;
    }

    // Create account info structure
    account_info_t* account = calloc(1, sizeof(account_info_t));
    if (!account) {
        log_this("AUTH", "Failed to allocate memory for account info", LOG_LEVEL_ERROR, 0);
        json_decref(result_json);
        free(result->data_json);
        free(result);
        return NULL;
    }

    // Extract account data from JSON
    // QueryRef #008 returns only account_id from account_contacts table
    json_t* row = json_array_get(result_json, 0); // First row
    if (row) {
        json_t* account_id_json = json_object_get(row, "account_id");
        
        if (account_id_json) {
            account->id = (int)json_integer_value(account_id_json);
        } else {
            // Fallback: try uppercase column name (DB2)
            account_id_json = json_object_get(row, "ACCOUNT_ID");
            if (account_id_json) {
                account->id = (int)json_integer_value(account_id_json);
            }
        }
        
        // Set enabled/authorized to true here
        // The actual status check happens in QueryRef #012 during password verification
        // which requires both password_hash AND status_a16=1
        // This prevents revealing whether accounts exist but are disabled
        account->enabled = true;
        account->authorized = true;
        
        // Username, email, and roles will be populated during password verification
        account->username = NULL;
        account->email = NULL;
        account->roles = NULL;
    }

    // Cleanup
    json_decref(result_json);
    free(result->data_json);
    free(result);

    return account;
}

/**
 * Verify password AND account status in one secure database query
 * Uses QueryRef #012 which checks password_hash AND status_a16=1
 * Returns true only if BOTH password correct AND account active
 * More secure: never exposes hash, doesn't reveal if account exists but disabled
 */
bool verify_password_and_status(const char* password, int account_id, const char* database, account_info_t* account) {
    if (!password || account_id <= 0 || !database || !account) return false;

    // Compute password hash
    char* computed_hash = compute_password_hash(password, account_id);
    if (!computed_hash) {
        log_this("AUTH", "Failed to compute password hash", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create parameters for QueryRef #012: Check Password (with status)
    // Uses typed parameter format for DB2 compatibility
    json_t* params = json_object();
    json_t* integer_params = json_object();
    json_t* string_params = json_object();
   
    json_object_set_new(integer_params, "ACCOUNTID", json_integer(account_id));
    json_object_set_new(string_params, "PASSWORDHASH", json_string(computed_hash));
    
    json_object_set_new(params, "INTEGER", integer_params);
    json_object_set_new(params, "STRING", string_params);

    // Execute query - returns row ONLY if password correct AND status_a16=1
    QueryResult* result = execute_auth_query(12, database, params);
    json_decref(params);
    free(computed_hash); // Clean up sensitive data immediately

    if (!result || !result->success) {
        log_this("AUTH", "Password verification query failed for account_id=%d: %s", LOG_LEVEL_ERROR, 2,
                account_id, result ? result->error_message : "Unknown error");
        if (result) {
            if (result->error_message) free(result->error_message);
            if (result->data_json) free(result->data_json);
            free(result);
        }
        return false;
    }

    // Parse result JSON
    json_t* result_json = json_loads(result->data_json, 0, NULL);
    if (!result_json) {
        log_this("AUTH", "Failed to parse password verification result", LOG_LEVEL_ERROR, 0);
        free(result->data_json);
        free(result);
        return false;
    }

    // Check if we got a row back - if yes, password correct AND account active
    json_t* row = json_array_get(result_json, 0);
    bool verified = (row != NULL);

    if (verified && account) {
        // Populate account info from returned row
        json_t* name_json = json_object_get(row, "name");
        if (!name_json) name_json = json_object_get(row, "NAME"); // DB2 uppercase
        if (name_json && json_is_string(name_json)) {
            if (account->username) free(account->username);
            account->username = strdup(json_string_value(name_json));
        }
    }

    // Cleanup
    json_decref(result_json);
    free(result->data_json);
    if (result->error_message) free(result->error_message);
    free(result);

    return verified;
}

/**
 * DEPRECATED: Use verify_password_and_status() instead
 * This function is kept for compatibility but should not be used
 * The new approach verifies password AND status in one secure database query
 */
char* get_password_hash(int account_id, const char* database) {
    (void)account_id;
    (void)database;
    log_this("AUTH", "get_password_hash() is deprecated - use verify_password_and_status() instead", LOG_LEVEL_ERROR, 0);
    return NULL;
}

/**
 * DEPRECATED: Use verify_password_and_status() instead
 * This function is kept for compatibility but should not be used
 * The new approach verifies password AND status in one secure database query
 */
bool verify_password(const char* password, const char* stored_hash, int account_id) {
    (void)password;
    (void)stored_hash;
    (void)account_id;
    log_this("AUTH", "verify_password() is deprecated - use verify_password_and_status() instead", LOG_LEVEL_ERROR, 0);
    return false;
}

/**
 * Check if username is available
 */
bool check_username_availability(const char* username, const char* database) {
    if (!username || !database) return false;

    // Create parameters for QueryRef #050: Check Username Availability
    json_t* params = json_object();
    json_object_set_new(params, "username", json_string(username));

    // Execute query
    QueryResult* result = execute_auth_query(50, database, params);
    json_decref(params);

    if (!result) {
        log_this("AUTH", "Failed to check username availability", LOG_LEVEL_ERROR, 0);
        return false;
    }

    bool available = result->success && result->row_count == 0;

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    return available;
}

/**
 * Create new account record
 */
int create_account_record(const char* username, const char* email,
                          const char* hashed_password, const char* full_name, const char* database) {
    if (!username || !email || !hashed_password || !database) return -1;

    // Create parameters for QueryRef #051: Create Account
    json_t* params = json_object();
    json_object_set_new(params, "username", json_string(username));
    json_object_set_new(params, "email", json_string(email));
    json_object_set_new(params, "password_hash", json_string(hashed_password));
    if (full_name) {
        json_object_set_new(params, "full_name", json_string(full_name));
    }

    // Execute query
    QueryResult* result = execute_auth_query(51, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to create account: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
        if (result) {
            if (result->error_message) free(result->error_message);
            if (result->data_json) free(result->data_json);
            free(result);
        }
        return -1;
    }

    // Parse result to get new account ID
    int account_id = -1;
    if (result->data_json) {
        json_t* result_json = json_loads(result->data_json, 0, NULL);
        if (result_json) {
            json_t* id_json = json_object_get(result_json, "id");
            if (id_json) {
                account_id = (int)json_integer_value(id_json);
            }
            json_decref(result_json);
        }
    }

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    return account_id;
}

/**
 * Store JWT in database
 */
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, const char* database) {
    if (!jwt_hash || account_id <= 0 || !database) return;

    // Create parameters for QueryRef #013: Store JWT
    json_t* params = json_object();
    json_object_set_new(params, "account_id", json_integer(account_id));
    json_object_set_new(params, "jwt_hash", json_string(jwt_hash));
    json_object_set_new(params, "expires_at", json_integer(expires_at));

    // Execute query
    QueryResult* result = execute_auth_query(13, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to store JWT: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
    }

    // Cleanup
    if (result) {
        if (result->error_message) free(result->error_message);
        if (result->data_json) free(result->data_json);
        free(result);
    }
}

/**
 * Update JWT storage (for renewal)
 */
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                        const char* new_jwt_hash, time_t new_expires, const char* database) {
    if (!old_jwt_hash || !new_jwt_hash || account_id <= 0 || !database) return;

    // Create parameters for QueryRef #003: Update JWT
    json_t* params = json_object();
    json_object_set_new(params, "account_id", json_integer(account_id));
    json_object_set_new(params, "old_jwt_hash", json_string(old_jwt_hash));
    json_object_set_new(params, "new_jwt_hash", json_string(new_jwt_hash));
    json_object_set_new(params, "new_expires", json_integer(new_expires));

    // Execute query
    QueryResult* result = execute_auth_query(3, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to update JWT storage: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
    }

    // Cleanup
    if (result) {
        if (result->error_message) free(result->error_message);
        if (result->data_json) free(result->data_json);
        free(result);
    }
}

/**
 * Delete JWT from storage
 */
void delete_jwt_from_storage(const char* jwt_hash, const char* database) {
    if (!jwt_hash || !database) return;

    // Create parameters for QueryRef #019: Delete JWT
    json_t* params = json_object();
    json_object_set_new(params, "jwt_hash", json_string(jwt_hash));

    // Execute query
    QueryResult* result = execute_auth_query(19, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to delete JWT: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
    }

    // Cleanup
    if (result) {
        if (result->error_message) free(result->error_message);
        if (result->data_json) free(result->data_json);
        free(result);
    }
}

/**
 * Check if token is revoked (database lookup)
 */
bool is_token_revoked(const char* token_hash, const char* database) {
    if (!token_hash || !database) return true; // Assume revoked if invalid

    // Create parameters for QueryRef #018: Validate JWT
    json_t* params = json_object();
    json_object_set_new(params, "token_hash", json_string(token_hash));

    // Execute query
    QueryResult* result = execute_auth_query(18, database, params);
    json_decref(params);

    if (!result) {
        log_this("AUTH", "Failed to check token revocation status", LOG_LEVEL_ERROR, 0);
        return true; // Fail-safe: assume revoked
    }

    bool revoked = result->success && result->row_count > 0;

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    return revoked;
}

/**
 * Check failed login attempts for rate limiting
 */
int check_failed_attempts(const char* login_id, const char* client_ip,
                           time_t window_start, const char* database) {
    (void)window_start; // Unused - LOGINRETRYWINDOW comes from database config Parameters
    if (!login_id || !client_ip || !database) return 0;

    // Create parameters for QueryRef #005: Get Login Attempt Count
    // Use typed parameter format: {"STRING": {...}, "INTEGER": {...}}
    // Parameter names must match SQL placeholders :LOGINID, :IPADDRESS, :LOGINRETRYWINDOW
    // Note: LOGINRETRYWINDOW is provided by database config Parameters section (e.g., 15 minutes)
    // We don't override it here - the config value will be used from merge_database_parameters
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_t* integer_params = json_object();
    json_object_set_new(string_params, "LOGINID", json_string(login_id));
    json_object_set_new(string_params, "IPADDRESS", json_string(client_ip));
    // Don't set LOGINRETRYWINDOW here - it will come from database config
    json_object_set_new(params, "STRING", string_params);
    json_object_set_new(params, "INTEGER", integer_params);

    // Execute query
    QueryResult* result = execute_auth_query(5, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to check failed attempts: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
        if (result) {
            if (result->error_message) free(result->error_message);
            if (result->data_json) free(result->data_json);
            free(result);
        }
        return 0;
    }

    int count = 0;
    if (result->data_json) {
        json_t* result_json = json_loads(result->data_json, 0, NULL);
        if (result_json) {
            json_t* count_json = json_object_get(result_json, "count");
            if (count_json) {
                count = (int)json_integer_value(count_json);
            }
            json_decref(result_json);
        }
    }

    // Cleanup
    if (result->error_message) free(result->error_message);
    if (result->data_json) free(result->data_json);
    free(result);

    return count;
}

/**
 * Block IP address temporarily
 */
void block_ip_address(const char* client_ip, int duration_minutes, const char* database) {
    if (!client_ip || !database) return;

    // Create parameters for QueryRef #007: Block IP Address Temporarily
    // Use typed parameter format: {"STRING": {...}, "INTEGER": {...}}
    // Parameter names must match SQL placeholders:
    // :IPADDRESS, :LOGINID, :REASON (STRING)
    // :LOGINBLOCKDURATION, :LOGINLOGID (INTEGER)
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_t* integer_params = json_object();
    json_object_set_new(string_params, "IPADDRESS", json_string(client_ip));
    json_object_set_new(string_params, "LOGINID", json_string("")); // Not available, use empty
    json_object_set_new(string_params, "REASON", json_string("Rate limit exceeded"));
    json_object_set_new(integer_params, "LOGINBLOCKDURATION", json_integer(duration_minutes));
    json_object_set_new(integer_params, "LOGINLOGID", json_integer(0)); // Not available
    json_object_set_new(params, "STRING", string_params);
    json_object_set_new(params, "INTEGER", integer_params);

    // Execute query
    QueryResult* result = execute_auth_query(7, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to block IP address: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
    }

    // Cleanup
    if (result) {
        if (result->error_message) free(result->error_message);
        if (result->data_json) free(result->data_json);
        free(result);
    }
}

/**
 * Log login attempt to database
 */
void log_login_attempt(const char* login_id, const char* client_ip,
                       const char* user_agent, time_t timestamp, const char* database) {
    if (!login_id || !client_ip || !database) return;
    (void)user_agent; // Unused - query doesn't have a placeholder for user_agent
    (void)timestamp;  // Unused - query uses ${NOW} instead

    // Create parameters for QueryRef #004: Log Login Attempt
    // Use typed parameter format: {"STRING": {...}, "INTEGER": {...}}
    // Parameter names must match SQL placeholders:
    // :APPVERSION, :LOGINID, :IPADDRESS (STRING)
    // :LOGINTIMER, :LOGINLOGID (INTEGER)
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_t* integer_params = json_object();
    json_object_set_new(string_params, "APPVERSION", json_string("1.0.0"));
    json_object_set_new(string_params, "LOGINID", json_string(login_id));
    json_object_set_new(string_params, "IPADDRESS", json_string(client_ip));
    json_object_set_new(integer_params, "LOGINTIMER", json_integer(0)); // Time taken, not available
    json_object_set_new(integer_params, "LOGINLOGID", json_integer(0)); // Log ID, not available
    json_object_set_new(params, "STRING", string_params);
    json_object_set_new(params, "INTEGER", integer_params);

    // Execute query
    QueryResult* result = execute_auth_query(4, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to log login attempt: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
    }

    // Cleanup
    if (result) {
        if (result->error_message) free(result->error_message);
        if (result->data_json) free(result->data_json);
        free(result);
    }
}

/**
 * Verify API key and retrieve system information
 */
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info) {
    if (!api_key || !database || !sys_info) {
        log_this(SR_AUTH, "Invalid parameters for API key verification", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Create parameters for QueryRef #001: Verify API Key
    // Use typed parameter format: {"STRING": {"APIKEY": "value"}}
    // Parameter name must match SQL placeholder :APIKEY
    json_t* params = json_object();
    json_t* string_params = json_object();
    json_object_set_new(string_params, "APIKEY", json_string(api_key));
    json_object_set_new(params, "STRING", string_params);

    // Execute query against specified database
    QueryResult* result = execute_auth_query(1, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this(SR_AUTH, "Failed to verify API key: %s", LOG_LEVEL_ERROR, 1,
                result ? result->error_message : "Unknown error");
        if (result) {
            if (result->error_message) free(result->error_message);
            if (result->data_json) free(result->data_json);
            free(result);
        }
        return false;
    }

    // Check if API key was found
    if (!result->data_json) {
        log_this(SR_AUTH, "Invalid API key attempted: %s", LOG_LEVEL_ALERT, 1, api_key);
        if (result->error_message) free(result->error_message);
        free(result);
        return false;
    }

    // Parse result JSON to extract system information
    json_t* result_json = json_loads(result->data_json, 0, NULL);
    if (!result_json) {
        log_this(SR_AUTH, "Failed to parse API key verification result", LOG_LEVEL_ERROR, 0);
        if (result->error_message) free(result->error_message);
        free(result->data_json);
        free(result);
        return false;
    }

    // Extract system information from first row
    json_t* row = json_array_get(result_json, 0);
    if (!row) {
        log_this(SR_AUTH, "Invalid API key: not found in database", LOG_LEVEL_ALERT, 0);
        json_decref(result_json);
        if (result->error_message) free(result->error_message);
        free(result->data_json);
        free(result);
        return false;
    }

    // Extract system_id, license_id (as app_id), and valid_until (as license_expiry)
    // All database engines now return lowercase column names for consistency
    // Query returns: name, valid_until, license_id, system_id
    json_t* system_id_json = json_object_get(row, "system_id");
    json_t* license_id_json = json_object_get(row, "license_id");
    json_t* valid_until_json = json_object_get(row, "valid_until");

    if (system_id_json) {
        sys_info->system_id = (int)json_integer_value(system_id_json);
    } else {
        sys_info->system_id = 0;
    }

    // Use license_id as app_id
    if (license_id_json) {
        sys_info->app_id = (int)json_integer_value(license_id_json);
    } else {
        sys_info->app_id = 0;
    }

    // Parse valid_until timestamp string to time_t
    // DB2 TIMESTAMP format: "YYYY-MM-DD-HH.MM.SS.FFFFFF"
    if (valid_until_json && json_is_string(valid_until_json)) {
        const char* ts_str = json_string_value(valid_until_json);
        struct tm tm_time = {0};
        // Parse DB2 timestamp format: "2035-01-01-00.00.00.000000"
        if (sscanf(ts_str, "%d-%d-%d-%d.%d.%d",
                   &tm_time.tm_year, &tm_time.tm_mon, &tm_time.tm_mday,
                   &tm_time.tm_hour, &tm_time.tm_min, &tm_time.tm_sec) >= 3) {
            tm_time.tm_year -= 1900;  // struct tm year is years since 1900
            tm_time.tm_mon -= 1;      // struct tm month is 0-11
            sys_info->license_expiry = mktime(&tm_time);
        } else {
            sys_info->license_expiry = 0;
        }
    } else if (valid_until_json && json_is_integer(valid_until_json)) {
        sys_info->license_expiry = (time_t)json_integer_value(valid_until_json);
    } else {
        sys_info->license_expiry = 0;
    }

    log_this(SR_AUTH, "API key verified successfully: system_id=%d, app_id=%d",
             LOG_LEVEL_DEBUG, 2, sys_info->system_id, sys_info->app_id);

    // Cleanup
    json_decref(result_json);
    if (result->error_message) free(result->error_message);
    free(result->data_json);
    free(result);

    return true;
}

/**
 * Check if license has expired
 * Returns true if license is valid (not expired)
 * Returns false if license has expired or if expiry is 0 (invalid)
 */
bool check_license_expiry(time_t license_expiry) {
    // If expiry is 0, license is invalid
    if (license_expiry == 0) {
        log_this(SR_AUTH, "Invalid license expiry: timestamp is 0", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // Get current time
    time_t current_time = time(NULL);
    
    // Check if license has expired
    if (current_time > license_expiry) {
        log_this(SR_AUTH, "License has expired: current=%ld, expiry=%ld",
                 LOG_LEVEL_ALERT, 2, current_time, license_expiry);
        return false;
    }
    
    // License is still valid
    log_this(SR_AUTH, "License is valid: expiry=%ld, remaining=%ld seconds",
             LOG_LEVEL_DEBUG, 2, license_expiry, license_expiry - current_time);
    return true;
}

/**
 * Free account info
 */
void free_account_info(account_info_t* account) {
    if (account) {
        free(account->username);
        free(account->email);
        free(account->roles);
        free(account);
    }
}
