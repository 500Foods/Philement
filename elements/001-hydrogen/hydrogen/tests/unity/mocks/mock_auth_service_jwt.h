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

// Include auth_service.h to get type definitions
// Use header guard to prevent multiple inclusions
#include <src/api/auth/auth_service.h>

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