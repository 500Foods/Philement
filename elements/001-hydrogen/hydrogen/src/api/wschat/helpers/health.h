/*
 * Chat Health Monitoring
 *
 * Health check implementation for chat engines. Provides background monitoring
 * thread, per-engine health checks, and status tracking.
 */

#ifndef HEALTH_H
#define HEALTH_H

// Project includes
#include <src/hydrogen.h>

// Chat includes
#include "engine_cache.h"

// Health check configuration
#define HEALTH_TIMEOUT_SECONDS     10   // Timeout for health check requests
#define HEALTH_MAX_FAILURES        3    // Failures before marking unhealthy
#define HEALTH_MIN_INTERVAL        10   // Minimum seconds between checks
#define HEALTH_MAX_INTERVAL        3600 // Maximum seconds between checks
#define HEALTH_DEFAULT_INTERVAL    300  // Default 5 minutes

// Health check result
typedef enum {
    HEALTH_UNKNOWN = 0,
    HEALTH_HEALTHY,
    HEALTH_DEGRADED,
    HEALTH_UNAVAILABLE
} ChatHealthStatus;

// Perform health check on a single engine
// Returns true if engine is responding, false otherwise
bool chat_health_check_engine(ChatEngineConfig* engine);

// Background health check thread function
// arg is a pointer to the ChatEngineCache
void* chat_health_monitor_thread(void* arg);

// Start health monitoring for a database
// Creates and starts the health monitoring thread
bool chat_health_monitor_start(ChatEngineCache* cache);



/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 * -------------------------------------------------------------------------- */
bool chat_health_check_openai(const ChatEngineConfig* engine);
bool chat_health_check_anthropic(const ChatEngineConfig* engine);
bool chat_health_check_ollama(const ChatEngineConfig* engine);

#endif // HEALTH_H
