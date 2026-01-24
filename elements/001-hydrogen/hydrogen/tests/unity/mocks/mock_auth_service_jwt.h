/*
 * Mock Auth Service JWT functions for unit testing
 *
 * This file provides mock implementations of JWT validation functions
 * to enable unit testing of code that depends on auth_service_jwt without requiring
 * the actual auth service during testing.
 */

#ifndef MOCK_AUTH_SERVICE_JWT_H
#define MOCK_AUTH_SERVICE_JWT_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

// Include auth_service.h for type definitions only when mocking is enabled
#ifdef USE_MOCK_AUTH_SERVICE_JWT
#include <src/api/auth/auth_service.h>
#else
// Define types locally when not mocking
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
    char* iss;
    char* sub;
    char* aud;
    time_t exp;
    time_t iat;
    time_t nbf;
    char* jti;
    int user_id;
    int system_id;
    int app_id;
    char* username;
    char* email;
    char* roles;
    char* ip;
    char* tz;
    int tzoffset;
    char* database;
} jwt_claims_t;

typedef struct {
    bool valid;
    jwt_claims_t* claims;
    jwt_error_t error;
} jwt_validation_result_t;
#endif

// Mock function declarations
#ifdef USE_MOCK_AUTH_SERVICE_JWT
#define validate_jwt mock_validate_jwt
#define free_jwt_validation_result mock_free_jwt_validation_result
#endif

// Mock implementations
jwt_validation_result_t mock_validate_jwt(const char* token, const char* database);
void mock_free_jwt_validation_result(jwt_validation_result_t* result);

// Mock control functions
void mock_auth_service_jwt_reset_all(void);
void mock_auth_service_jwt_set_validation_result(jwt_validation_result_t result);

#endif /* MOCK_AUTH_SERVICE_JWT_H */