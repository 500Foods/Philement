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

// Stop health monitoring for a database
// Signals thread to stop and waits for it to complete
void chat_health_monitor_stop(ChatEngineCache* cache);

// Check if health monitoring is running for a cache
bool chat_health_monitor_is_running(const ChatEngineCache* cache);

// Convert ChatHealthStatus to string
const char* chat_health_status_to_string(ChatHealthStatus status);

// Get health status for an engine (thread-safe)
ChatHealthStatus chat_health_get_engine_status(ChatEngineConfig* engine);

// Update engine stats after each request
void chat_health_update_stats(ChatEngineConfig* engine, bool success, double response_time_ms, int tokens_used);

#endif // HEALTH_H
