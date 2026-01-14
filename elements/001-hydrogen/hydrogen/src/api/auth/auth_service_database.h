/*
 * Auth Service Database Functions Header
 *
 * Internal header for database-related operations
 */

#ifndef HYDROGEN_AUTH_SERVICE_DATABASE_H
#define HYDROGEN_AUTH_SERVICE_DATABASE_H

#include <stdbool.h>
#include <time.h>
#include <jansson.h>
#include "auth_service.h"

// Database query execution
QueryResult* execute_auth_query(int query_ref, const char* database, json_t* params);
void free_query_result(QueryResult* result);

// Account management
account_info_t* lookup_account(const char* login_id, const char* database);
char* get_password_hash(int account_id, const char* database);
bool verify_password(const char* password, const char* stored_hash, int account_id);
bool check_username_availability(const char* username, const char* database);
int create_account_record(const char* username, const char* email,
                          const char* hashed_password, const char* full_name, const char* database);

// JWT storage
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, int system_id, int app_id, const char* database);
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                        const char* new_jwt_hash, time_t new_expires, int system_id, int app_id, const char* database);
void delete_jwt_from_storage(const char* jwt_hash, const char* database);
bool is_token_revoked(const char* token_hash, const char* database);

// Rate limiting and security
int check_failed_attempts(const char* login_id, const char* client_ip,
                          time_t window_start, const char* database);
void block_ip_address(const char* client_ip, int duration_minutes, const char* database);
void log_login_attempt(const char* login_id, const char* client_ip,
                       const char* user_agent, time_t timestamp, const char* database);

// Memory cleanup
void free_account_info(account_info_t* account);

#endif /* HYDROGEN_AUTH_SERVICE_DATABASE_H */
