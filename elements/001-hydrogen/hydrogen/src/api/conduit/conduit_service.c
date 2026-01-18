/*
 * Conduit Service Implementation
 *
 * REST API service for database query execution by reference.
 */

#include "conduit_service.h"

// Standard libraries
#include <pthread.h>

// Third-party libraries
#include <microhttpd.h>

// Webserver suspension mechanism for long-running queries
extern pthread_mutex_t webserver_suspend_lock;
extern volatile bool webserver_thread_suspended;

// Initialize conduit service
bool conduit_service_init(void) {
    log_this(conduit_service_name(), "Conduit service initialized", LOG_LEVEL_DEBUG, 0);
    return true;
}

// Cleanup conduit service
void conduit_service_cleanup(void) {
    log_this(conduit_service_name(), "Conduit service cleaned up", LOG_LEVEL_DEBUG, 0);
}

// Get service name for logging
const char* conduit_service_name(void) {
    return "Conduit";
}