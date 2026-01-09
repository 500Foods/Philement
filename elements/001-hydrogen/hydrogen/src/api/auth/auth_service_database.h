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

// Account management
account_info_t* lookup_account(const char* login_id);
bool verify_password(const char* password, const char* stored_hash);
bool check_username_availability(const char* username);
int create_account_record(const char* username, const char* email,
                          const char* hashed_password, const char* full_name);

// JWT storage
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at);
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                        const char* new_jwt_hash, time_t new_expires);
void delete_jwt_from_storage(const char* jwt_hash);
bool is_token_revoked(const char* token_hash);

// Rate limiting and security
int check_failed_attempts(const char* login_id, const char* client_ip,
                          time_t window_start);
void block_ip_address(const char* client_ip, int duration_minutes);
void log_login_attempt(const char* login_id, const char* client_ip,
                       const char* user_agent, time_t timestamp);

// Memory cleanup
void free_account_info(account_info_t* account);

#endif /* HYDROGEN_AUTH_SERVICE_DATABASE_H */
