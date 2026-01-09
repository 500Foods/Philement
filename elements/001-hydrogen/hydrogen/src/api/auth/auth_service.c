/*
 * Auth API Service
 *
 * This file serves as an integration point for all auth API endpoints.
 * The actual implementations are split into focused modules:
 * - auth_service_jwt.c: JWT generation, validation, and token management
 * - auth_service_validation.c: Input validation and security checks
 * - auth_service_database.c: Database query wrappers and account management
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "auth_service.h"
#include "auth_service_jwt.h"
#include "auth_service_validation.h"
#include "auth_service_database.h"
