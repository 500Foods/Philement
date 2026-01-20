/*
 * Auth Service API module for the Hydrogen Project.
 * Provides login, logout, renew and related JWT operations.
 *
 * IMPLEMENTATION STRUCTURE:
 * This module is split into focused implementation files for maintainability:
 * - auth_service.c: Main integration point (lightweight, includes all modules)
 * - auth_service_jwt.c/.h: JWT generation, validation, token management (~500 lines)
 * - auth_service_validation.c/.h: Input validation, security checks (~180 lines)
 * - auth_service_database.c/.h: Database queries, account management (~500 lines)
 *
 * This public header declares all functions that are part of the auth service API.
 * Internal implementation headers (.h files) define module-specific internal functions.
 */

#ifndef HYDROGEN_AUTH_SERVICE_H
#define HYDROGEN_AUTH_SERVICE_H

//@ swagger:service Auth Service
//@ swagger:description Provides login, logout, renew and related JWT operations.
//@ swagger:version 1.0.0
//@ swagger:tag "Auth Service" Provides login, logout, renew and related JWT operations.

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Project Libraries
#include <src/config/config.h>
#include <src/database/database.h>

// Forward declarations for database structures
typedef struct QueryResult QueryResult;

/*
 * Account Information Structure
 * Contains user account details retrieved from database
 */
typedef struct {
    int id;                    // Account ID
    char* username;            // Username
    char* email;               // Email address
    bool enabled;              // Account enabled flag
    bool authorized;           // Account authorized flag
    char* roles;               // JSON string of user roles
} account_info_t;

/*
 * System Information Structure
 * Contains system/license details from API key validation
 */
typedef struct {
    int system_id;             // System ID
    int app_id;                // Application ID
    time_t license_expiry;     // License expiry timestamp
} system_info_t;

/*
 * JWT Claims Structure
 * Standard JWT claims for authentication tokens
 */
typedef struct {
    char* iss;                 // Issuer
    char* sub;                 // Subject (user ID)
    char* aud;                 // Audience (app ID)
    time_t exp;                // Expiration time
    time_t iat;                // Issued at time
    time_t nbf;                // Not before time
    char* jti;                 // JWT ID
    int user_id;               // User ID
    int system_id;             // System ID
    int app_id;                // Application ID
    char* username;            // Username
    char* email;               // Email address
    char* roles;               // User roles
    char* ip;                  // Client IP address
    char* tz;                  // Client timezone
    int tzoffset;              // Timezone offset from UTC in minutes (e.g., -480 for PST, +60 for CET)
    char* database;            // Database name (for routing authenticated queries)
} jwt_claims_t;

/*
 * JWT Validation Result
 * Result of JWT token validation
 */
typedef enum {
    JWT_ERROR_NONE = 0,
    JWT_ERROR_EXPIRED,
    JWT_ERROR_NOT_YET_VALID,
    JWT_ERROR_INVALID_SIGNATURE,
    JWT_ERROR_UNSUPPORTED_ALGORITHM,
    JWT_ERROR_INVALID_FORMAT,
    JWT_ERROR_REVOKED
} jwt_error_t;

typedef struct {
    bool valid;                // Token is valid
    jwt_claims_t* claims;      // Parsed claims (NULL if invalid)
    jwt_error_t error;         // Error code if invalid
} jwt_validation_result_t;

/*
 * JWT Configuration Structure
 * Configuration for JWT signing and validation
 */
typedef struct {
    char* hmac_secret;         // HMAC secret for signing
    char* rsa_private_key;     // RSA private key (PEM)
    char* rsa_public_key;      // RSA public key (PEM)
    int rotation_interval_days;// Key rotation interval
    bool use_rsa;              // Use RSA instead of HMAC
    time_t last_rotation;      // Last rotation timestamp
    char rotation_salt[32];    // Salt for key derivation
} jwt_config_t;

/*
 * Auth Service Function Prototypes
 */

// Input validation functions
bool validate_login_input(const char* login_id, const char* password,
                         const char* api_key, const char* tz);
bool validate_registration_input(const char* username, const char* password,
                                const char* email, const char* full_name);
bool validate_timezone(const char* tz);

// API key and system validation
bool verify_api_key(const char* api_key, const char* database, system_info_t* sys_info);
bool check_license_expiry(time_t license_expiry);

// IP filtering functions
bool check_ip_whitelist(const char* client_ip, const char* database);
bool check_ip_blacklist(const char* client_ip, const char* database);

// Audit logging functions
void log_login_attempt(const char* login_id, const char* client_ip,
                      const char* user_agent, time_t timestamp, const char* database);

// Rate limiting functions
int check_failed_attempts(const char* login_id, const char* client_ip,
                         time_t window_start, const char* database);
bool handle_rate_limiting(const char* client_ip, int failed_count,
                         bool is_whitelisted, const char* database);

// Account management functions
account_info_t* lookup_account(const char* login_id, const char* database);

// New secure verification: password + status in one database query
bool verify_password_and_status(const char* password, int account_id, const char* database, account_info_t* account);

// Deprecated - kept for compatibility
char* get_password_hash(int account_id, const char* database);
bool verify_password(const char* password, const char* stored_hash, int account_id);

// JWT functions
char* generate_jwt(account_info_t* account, system_info_t* system,
                  const char* client_ip, const char* tz, const char* database, time_t issued_at);
void store_jwt(int account_id, const char* jwt_hash, time_t expires_at, int system_id, int app_id, const char* database);
jwt_validation_result_t validate_jwt(const char* token, const char* database);
char* generate_new_jwt(jwt_claims_t* old_claims);
void update_jwt_storage(int account_id, const char* old_jwt_hash,
                        const char* new_jwt_hash, time_t new_expires, int system_id, int app_id, const char* database);
void delete_jwt_from_storage(const char* jwt_hash, const char* database);

// Password functions
char* hash_password(const char* password, int account_id);

// Registration functions
bool check_username_availability(const char* username, const char* database);
int create_account_record(const char* username, const char* email,
                         const char* hashed_password, const char* full_name, const char* database);

// Request parsing functions
char* parse_renew_request(const char* request_body);
char* parse_logout_request(const char* request_body);

// JWT validation for specific operations
jwt_validation_result_t validate_jwt_token(const char* token, const char* database);
jwt_validation_result_t validate_jwt_for_logout(const char* token, const char* database);

// Database query wrapper functions
QueryResult* execute_auth_query(int query_ref, const char* database, json_t* params);

// JWT storage functions
bool is_token_revoked(const char* token_hash, const char* ip_address, const char* database);

// Rate limiting and security functions
int check_failed_attempts(const char* login_id, const char* client_ip,
                          time_t window_start, const char* database);
void block_ip_address(const char* client_ip, int duration_minutes, const char* database);
void log_login_attempt(const char* login_id, const char* client_ip,
                       const char* user_agent, time_t timestamp, const char* database);

// Utility functions
char* generate_jti(void);
char* compute_password_hash(const char* password, int account_id);
char* compute_token_hash(const char* token);
bool is_valid_timezone(const char* tz);
int calculate_timezone_offset(const char* tz);
bool is_valid_email(const char* email);
bool is_alphanumeric_underscore_hyphen(const char* str);

// Configuration functions
jwt_config_t* get_jwt_config(void);
void free_jwt_config(jwt_config_t* config);

// Memory cleanup functions
void free_account_info(account_info_t* account);
void free_jwt_claims(jwt_claims_t* claims);
void free_jwt_validation_result(jwt_validation_result_t* result);

#endif /* HYDROGEN_AUTH_SERVICE_H */
