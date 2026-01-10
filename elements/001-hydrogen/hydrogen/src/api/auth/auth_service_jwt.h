/*
 * Auth Service JWT Functions Header
 *
 * Internal header for JWT-related operations
 */

#ifndef HYDROGEN_AUTH_SERVICE_JWT_H
#define HYDROGEN_AUTH_SERVICE_JWT_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "auth_service.h"

// JWT utility functions (Base64url is now in utils_crypto.h)
char* generate_jti(void);
char* compute_token_hash(const char* token);
char* compute_password_hash(const char* password, int account_id);

// JWT configuration
jwt_config_t* get_jwt_config(void);
void free_jwt_config(jwt_config_t* config);

// JWT generation and validation
char* generate_jwt(account_info_t* account, system_info_t* system,
                   const char* client_ip, const char* database, time_t issued_at);
jwt_validation_result_t validate_jwt(const char* token, const char* database);
char* generate_new_jwt(jwt_claims_t* old_claims);

// JWT validation wrappers
jwt_validation_result_t validate_jwt_token(const char* token, const char* database);
jwt_validation_result_t validate_jwt_for_logout(const char* token, const char* database);

// Memory cleanup
void free_jwt_claims(jwt_claims_t* claims);
void free_jwt_validation_result(jwt_validation_result_t* result);

#endif /* HYDROGEN_AUTH_SERVICE_JWT_H */
