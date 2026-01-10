/*
 * Conduit Service Header
 *
 * REST API service for database query execution by reference.
 */

#ifndef CONDUIT_SERVICE_H
#define CONDUIT_SERVICE_H

//@ swagger:title Conduit Service API
//@ swagger:description REST API for executing pre-defined database queries by reference with typed parameters.
//@ swagger:version 1.0.0
//@ swagger:tag "Conduit Service" Provides database query execution by reference with support for multiple database engines.

// Project includes
#include <src/hydrogen.h>

// Function prototypes

// Initialize conduit service
bool conduit_service_init(void);

// Cleanup conduit service
void conduit_service_cleanup(void);

// Get service name for logging
const char* conduit_service_name(void);

#endif // CONDUIT_SERVICE_H