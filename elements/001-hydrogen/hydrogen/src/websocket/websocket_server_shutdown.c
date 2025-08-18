/* Feature test macros must come first */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * WebSocket Server Shutdown Module
 * 
 * Handles complex WebSocket server shutdown including:
 * - Graceful connection termination
 * - Thread synchronization and cleanup
 * - Resource deallocation
 * - Timeout handling
 */

/* System headers */
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

// External libraries
#include <libwebsockets.h>
#include <jansson.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../utils/utils.h"
#include "../threads/threads.h"

/* External variables */
extern AppConfig* app_config;
extern WebSocketServerContext *ws_context;

// Cleanup synchronization data structure
struct CleanupData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool complete;
    bool cancelled;
    WebSocketServerContext* context;
};


// Stop the WebSocket server
void stop_websocket_server()
{
    if (!ws_context) {
        return;
    }

    log_this("WebSocket", "Stopping server on port %d", LOG_LEVEL_STATE, ws_context->port);
    
    // Set shutdown flag
    if (ws_context) {
        log_this("WebSocket", "Setting shutdown flag and cancelling service", LOG_LEVEL_STATE);
        ws_context->shutdown = 1;
        
        // Force close all connections immediately
        pthread_mutex_lock(&ws_context->mutex);
        if (ws_context->active_connections > 0) {
            log_this("WebSocket", "Forcing close of %d connections", LOG_LEVEL_ALERT,
                    true, true, true, ws_context->active_connections);
            ws_context->active_connections = 0;  // Force reset connection counter
        }
        
        // Cancel service multiple times to ensure all threads wake up
        if (ws_context->lws_context) {
            log_this("WebSocket", "Canceling service multiple times to force wakeup", LOG_LEVEL_STATE);
            lws_cancel_service(ws_context->lws_context);
            usleep(10000);  // 10ms sleep
            lws_cancel_service(ws_context->lws_context);
        }
        
        // Signal all waiting threads
        pthread_cond_broadcast(&ws_context->cond);
        pthread_mutex_unlock(&ws_context->mutex);
        
        // During shutdown, be aggressive - don't wait for polite thread exit
        log_this("WebSocket", "Forcing immediate thread termination during shutdown", LOG_LEVEL_STATE);
        
        // Immediately cancel the thread - it's stuck in lws_service() waiting for network activity
        pthread_cancel(ws_context->server_thread);
        pthread_mutex_lock(&ws_context->mutex);
        pthread_cond_broadcast(&ws_context->cond);
        pthread_mutex_unlock(&ws_context->mutex);
        
        // Brief wait for cancellation to take effect (100ms max)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000;  // 100ms in nanoseconds
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
        
        int join_result = pthread_timedjoin_np(ws_context->server_thread, NULL, &ts);
        if (join_result == ETIMEDOUT) {
            log_this("WebSocket", "Thread cancellation timed out, forcing cleanup", LOG_LEVEL_ALERT);
            
            // Force cleanup anyway as a last resort
            extern ServiceThreads websocket_threads;
            pthread_t server_thread = ws_context->server_thread;
            remove_service_thread(&websocket_threads, server_thread);
            
            log_this("WebSocket", "Forced removal of server thread from tracking", LOG_LEVEL_ALERT);
        } else {
            log_this("WebSocket", "Thread terminated after cancellation", LOG_LEVEL_STATE);
        }
        
        // Do NOT destroy context here - leave that to cleanup_websocket_server
        // This prevents race conditions with context access
    }

    log_this("WebSocket", "Server stopped", LOG_LEVEL_STATE);
}

// Clean up server resources
void cleanup_websocket_server()
{
    if (!ws_context) {
        return;
    }
    
    log_this("WebSocket", "Starting WebSocket server cleanup", LOG_LEVEL_STATE);
    
    // Minimal delay - just enough to clear immediate callbacks (reduced from 100ms to 50ms)
    log_this("WebSocket", "Brief pause for callbacks (50ms)", LOG_LEVEL_STATE);
    usleep(50000);  // 50ms delay
    
    // Force close all remaining connections
    pthread_mutex_lock(&ws_context->mutex);
    if (ws_context->active_connections > 0) {
        log_this("WebSocket", "Forcing close of %d connections during cleanup", 
                LOG_LEVEL_ALERT, true, true, true, ws_context->active_connections);
        ws_context->active_connections = 0;
    }
    
    // Broadcast condition so any waiting threads will wake up
    pthread_cond_broadcast(&ws_context->cond);
    pthread_mutex_unlock(&ws_context->mutex);
    
    // Important: Save context locally BEFORE nullifying global pointer
    WebSocketServerContext* ctx_to_destroy = ws_context;
    
    // Nullify the global pointer immediately to prevent further use
    ws_context = NULL;
    
    // Extra cancellation calls on context before destruction
    if (ctx_to_destroy && ctx_to_destroy->lws_context) {
        log_this("WebSocket", "Forcing multiple service cancellations", LOG_LEVEL_STATE);
        lws_cancel_service(ctx_to_destroy->lws_context);
        lws_cancel_service(ctx_to_destroy->lws_context);
        lws_cancel_service(ctx_to_destroy->lws_context);  // Triple cancellation for reliability
    }
    
    // Aggressively terminate any lingering threads BEFORE context destruction
    extern ServiceThreads websocket_threads;
    
    // First log what threads are still active
    log_this("WebSocket", "Checking for remaining threads before cleanup", LOG_LEVEL_STATE);
    update_service_thread_metrics(&websocket_threads);
    
    if (websocket_threads.thread_count > 0) {
        log_this("WebSocket", "Found %d active websocket threads, forcing termination", 
                LOG_LEVEL_ALERT, true, true, true, websocket_threads.thread_count);
        
        // Force cancel all tracked threads
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            pthread_t thread = websocket_threads.thread_ids[i];
            if (thread != 0) {
                log_this("WebSocket", "Cancelling thread %lu", LOG_LEVEL_ALERT, 
                        true, true, true, (unsigned long)thread);
                pthread_cancel(thread);
            }
        }
        
        // Minimal wait for cancellation to take effect (reduced from 100ms to 25ms)
        usleep(25000);  // 25ms delay
        
        // Clear thread tracking completely
        websocket_threads.thread_count = 0;
        log_this("WebSocket", "Forced all thread tracking to clear", LOG_LEVEL_ALERT);
    }
    
    // Direct cleanup without threading to avoid potential issues
    if (ctx_to_destroy) {
        log_this("WebSocket", "Destroying WebSocket context directly", LOG_LEVEL_STATE);
        ws_context_destroy(ctx_to_destroy);
        log_this("WebSocket", "WebSocket context destroyed", LOG_LEVEL_STATE);
    }
    
    // Final check for any remaining threads and force termination
    update_service_thread_metrics(&websocket_threads);
    if (websocket_threads.thread_count > 0) {
        log_this("WebSocket", "CRITICAL: %d threads still remain after full cleanup, forcing exit", 
                LOG_LEVEL_ERROR, true, true, true, websocket_threads.thread_count);
        
        // Force cancel any remaining threads one last time
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            if (websocket_threads.thread_ids[i] != 0) {
                pthread_cancel(websocket_threads.thread_ids[i]);
            }
        }
        
        // Finally just force clear the count
        websocket_threads.thread_count = 0;
    }
    
    log_this("WebSocket", "WebSocket server cleanup completed", LOG_LEVEL_STATE);
}
