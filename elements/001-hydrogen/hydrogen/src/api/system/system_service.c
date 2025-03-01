/*
 * System API Service
 * 
 * This file serves as an integration point for all system API endpoints.
 * The actual endpoint implementations are in their specific subdirectories.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "system_service.h"
#include "../../logging/logging.h"

// This file intentionally left minimal as the implementations
// have been moved to their respective subdirectories.