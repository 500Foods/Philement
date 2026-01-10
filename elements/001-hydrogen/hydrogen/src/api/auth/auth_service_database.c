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

    // Convert parameters to JSON string
    char* params_json = json_dumps(params, JSON_COMPACT);
    if (!params_json) {
        log_this("AUTH", "Failed to serialize parameters to JSON", LOG_LEVEL_ERROR, 0);
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
        result->data_json = strdup(result_query->parameter_json); // Result data
        result->execution_time_ms = time(NULL) - result_query->submitted_at;
    }

    // Cleanup
    free(query_id);
    free(db_query.query_template);

    return result;
}

/**
 * Lookup account information from database
 */
account_info_t* lookup_account(const char* login_id, const char* database) {
    if (!login_id || !database) return NULL;

    // Create parameters for QueryRef #001: Get Account by Login ID
    json_t* params = json_object();
    json_object_set_new(params, "login_id", json_string(login_id));

    // Execute query
    QueryResult* result = execute_auth_query(1, database, params);
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
    json_t* row = json_array_get(result_json, 0); // First row
    if (row) {
        json_t* id_json = json_object_get(row, "id");
        json_t* username_json = json_object_get(row, "username");
        json_t* email_json = json_object_get(row, "email");
        json_t* enabled_json = json_object_get(row, "enabled");
        json_t* authorized_json = json_object_get(row, "authorized");
        json_t* roles_json = json_object_get(row, "roles");

        if (id_json) account->id = (int)json_integer_value(id_json);
        if (username_json) account->username = strdup(json_string_value(username_json));
        if (email_json) account->email = strdup(json_string_value(email_json));
        if (enabled_json) account->enabled = json_is_true(enabled_json);
        if (authorized_json) account->authorized = json_is_true(authorized_json);
        if (roles_json) account->roles = strdup(json_string_value(roles_json));
    }

    // Cleanup
    json_decref(result_json);
    free(result->data_json);
    free(result);

    return account;
}

/**
 * Get stored password hash for an account
 * Uses QueryRef #001 which returns account and password information
 */
char* get_password_hash(int account_id, const char* database) {
    if (account_id <= 0 || !database) return NULL;

    // Create parameters for QueryRef #009: Get Password Hash
    // For now, we use QueryRef #001 to get all account info including password hash
    json_t* params = json_object();
    json_object_set_new(params, "account_id", json_integer(account_id));

    // Execute query
    QueryResult* result = execute_auth_query(1, database, params);
    json_decref(params);

    if (!result || !result->success) {
        log_this("AUTH", "Failed to get password hash for account_id=%d: %s", LOG_LEVEL_ERROR, 2,
                account_id, result ? result->error_message : "Unknown error");
        if (result) {
            if (result->error_message) free(result->error_message);
            if (result->data_json) free(result->data_json);
            free(result);
        }
        return NULL;
    }

    // Parse result JSON
    json_t* result_json = json_loads(result->data_json, 0, NULL);
    if (!result_json) {
        log_this("AUTH", "Failed to parse password hash result", LOG_LEVEL_ERROR, 0);
        free(result->data_json);
        free(result);
        return NULL;
    }

    // Extract password hash from first row
    char* password_hash = NULL;
    json_t* row = json_array_get(result_json, 0);
    if (row) {
        json_t* hash_json = json_object_get(row, "password_hash");
        if (hash_json && json_is_string(hash_json)) {
            password_hash = strdup(json_string_value(hash_json));
        }
    }

    // Cleanup
    json_decref(result_json);
    free(result->data_json);
    if (result->error_message) free(result->error_message);
    free(result);

    return password_hash;
}

/**
 * Verify password against stored hash
 */
bool verify_password(const char* password, const char* stored_hash, int account_id) {
    if (!password || !stored_hash || account_id <= 0) return false;

    // Hash the provided password using account_id as salt
    char* computed_hash = compute_password_hash(password, account_id);
    if (!computed_hash) {
        log_this("AUTH", "Failed to compute password hash", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Compare hashes
    bool match = strcmp(computed_hash, stored_hash) == 0;

    // Cleanup
    free(computed_hash);

    return match;
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

    // Create parameters for QueryRef #002: Store JWT
    json_t* params = json_object();
    json_object_set_new(params, "account_id", json_integer(account_id));
    json_object_set_new(params, "jwt_hash", json_string(jwt_hash));
    json_object_set_new(params, "expires_at", json_integer(expires_at));

    // Execute query
    QueryResult* result = execute_auth_query(2, database, params);
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

    // Create parameters for QueryRef #004: Delete JWT
    json_t* params = json_object();
    json_object_set_new(params, "jwt_hash", json_string(jwt_hash));

    // Execute query
    QueryResult* result = execute_auth_query(4, database, params);
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

    // Create parameters for QueryRef #006: Check Token Revoked
    json_t* params = json_object();
    json_object_set_new(params, "token_hash", json_string(token_hash));

    // Execute query
    QueryResult* result = execute_auth_query(6, database, params);
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
    if (!login_id || !client_ip || !database) return 0;

    // Create parameters for QueryRef #005: Get Login Attempt Count
    json_t* params = json_object();
    json_object_set_new(params, "login_id", json_string(login_id));
    json_object_set_new(params, "ip", json_string(client_ip));
    json_object_set_new(params, "since", json_integer(window_start));

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

    // Create parameters for QueryRef #007: Block IP Address
    json_t* params = json_object();
    json_object_set_new(params, "ip", json_string(client_ip));
    json_object_set_new(params, "duration_minutes", json_integer(duration_minutes));

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

    // Create parameters for QueryRef #004: Log Login Attempt
    json_t* params = json_object();
    json_object_set_new(params, "login_id", json_string(login_id));
    json_object_set_new(params, "ip", json_string(client_ip));
    if (user_agent) {
        json_object_set_new(params, "user_agent", json_string(user_agent));
    }
    json_object_set_new(params, "timestamp", json_integer(timestamp));

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
    json_t* params = json_object();
    json_object_set_new(params, "api_key", json_string(api_key));

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

    // Extract system_id, app_id, and license_expiry
    json_t* system_id_json = json_object_get(row, "system_id");
    json_t* app_id_json = json_object_get(row, "app_id");
    json_t* license_expiry_json = json_object_get(row, "license_expiry");

    if (system_id_json) {
        sys_info->system_id = (int)json_integer_value(system_id_json);
    } else {
        sys_info->system_id = 0;
    }

    if (app_id_json) {
        sys_info->app_id = (int)json_integer_value(app_id_json);
    } else {
        sys_info->app_id = 0;
    }

    if (license_expiry_json) {
        sys_info->license_expiry = (time_t)json_integer_value(license_expiry_json);
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
