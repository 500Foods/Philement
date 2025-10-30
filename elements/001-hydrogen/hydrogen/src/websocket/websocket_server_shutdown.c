
/*
 * WebSocket Server Shutdown Module
 * 
 * Handles complex WebSocket server shutdown including:
 * - Graceful connection termination
 * - Thread synchronization and cleanup
 * - Resource deallocation
 * - Timeout handling
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include <netinet/in.h>

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
void stop_websocket_server(void)
{
    if (!ws_context) {
        return;
    }

    log_this(SR_WEBSOCKET, "Stopping server on port %d", LOG_LEVEL_STATE, 1, ws_context->port);
    
    // Set shutdown flag
    if (ws_context) {
        log_this(SR_WEBSOCKET, "Setting shutdown flag and cancelling service", LOG_LEVEL_STATE, 0);
        ws_context->shutdown = 1;
        
        // Force close all connections immediately
        pthread_mutex_lock(&ws_context->mutex);
        if (ws_context->active_connections > 0) {
            log_this(SR_WEBSOCKET, "Forcing close of %d connections", LOG_LEVEL_ALERT, 1, ws_context->active_connections);
            ws_context->active_connections = 0;  // Force reset connection counter
        }
        
        // Cancel service multiple times to ensure all threads wake up
        if (ws_context->lws_context) {
            log_this(SR_WEBSOCKET, "Canceling service multiple times to force wakeup", LOG_LEVEL_STATE, 0);
            lws_cancel_service(ws_context->lws_context);
            usleep(10000);  // 10ms sleep
            lws_cancel_service(ws_context->lws_context);
        }
        
        // Signal all waiting threads
        pthread_cond_broadcast(&ws_context->cond);
        pthread_mutex_unlock(&ws_context->mutex);
        
        // During shutdown, be aggressive - don't wait for polite thread exit
        log_this(SR_WEBSOCKET, "Forcing immediate thread termination during shutdown", LOG_LEVEL_STATE, 0);
        
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
            log_this(SR_WEBSOCKET, "Thread cancellation timed out, forcing cleanup",4,3,2,1, LOG_LEVEL_ALERT,0);
            
            // Force cleanup anyway as a last resort
            pthread_t server_thread = ws_context->server_thread;
            remove_service_thread(&websocket_threads, server_thread);
            
            log_this(SR_WEBSOCKET, "Forced removal of server thread from tracking", LOG_LEVEL_ALERT,4,3,2,1,0);
        } else {
            log_this(SR_WEBSOCKET, "Thread terminated after cancellation", LOG_LEVEL_STATE,4,3,2,1,0);
        }
        
        // Do NOT destroy context here - leave that to cleanup_websocket_server
        // This prevents race conditions with context access
    }

    log_this(SR_WEBSOCKET, "Server stopped", LOG_LEVEL_STATE,4,3,2,1,0);
}

// Clean up server resources
void cleanup_websocket_server(void)
{
    if (!ws_context) {
        return;
    }
    
    log_this(SR_WEBSOCKET, "Starting WebSocket server cleanup", LOG_LEVEL_STATE, 0);
    
    // Minimal delay - just enough to clear immediate callbacks (reduced from 100ms to 50ms)
    log_this(SR_WEBSOCKET, "Brief pause for callbacks (50ms)", LOG_LEVEL_STATE, 0);
    usleep(50000);  // 50ms delay
    
    // Force close all remaining connections
    pthread_mutex_lock(&ws_context->mutex);
    if (ws_context->active_connections > 0) {
        log_this(SR_WEBSOCKET, "Forcing close of %d connections during cleanup", 
                LOG_LEVEL_ALERT, 1, ws_context->active_connections);
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
        log_this(SR_WEBSOCKET, "Forcing multiple service cancellations", LOG_LEVEL_STATE, 0);
        lws_cancel_service(ctx_to_destroy->lws_context);
        lws_cancel_service(ctx_to_destroy->lws_context);
        lws_cancel_service(ctx_to_destroy->lws_context);  // Triple cancellation for reliability
    }
    
    // Aggressively terminate any lingering threads BEFORE context destruction
    
    // First log what threads are still active
    log_this(SR_WEBSOCKET, "Checking for remaining threads before cleanup", LOG_LEVEL_STATE, 0);
    update_service_thread_metrics(&websocket_threads);
    
    if (websocket_threads.thread_count > 0) {
        log_this(SR_WEBSOCKET, "Found %d active websocket threads, forcing termination", 
                LOG_LEVEL_ALERT, 1, websocket_threads.thread_count);
        
        // Force cancel all tracked threads
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            pthread_t thread = websocket_threads.thread_ids[i];
            if (thread != 0) {
                log_this(SR_WEBSOCKET, "Cancelling thread %lu", LOG_LEVEL_ALERT, 1, (unsigned long)thread);
                pthread_cancel(thread);
            }
        }
        
        // Minimal wait for cancellation to take effect (reduced from 100ms to 25ms)
        usleep(25000);  // 25ms delay
        
        // Clear thread tracking completely
        websocket_threads.thread_count = 0;
        log_this(SR_WEBSOCKET, "Forced all thread tracking to clear", LOG_LEVEL_ALERT, 0);
    }
    
    // Direct cleanup without threading to avoid potential issues
    if (ctx_to_destroy) {
        log_this(SR_WEBSOCKET, "Destroying WebSocket context directly", LOG_LEVEL_STATE, 0);
        ws_context_destroy(ctx_to_destroy);
        log_this(SR_WEBSOCKET, "WebSocket context destroyed", LOG_LEVEL_STATE, 0);
    }
    
    // Final check for any remaining threads and force termination
    update_service_thread_metrics(&websocket_threads);
    if (websocket_threads.thread_count > 0) {
        log_this(SR_WEBSOCKET, "CRITICAL: %d threads still remain after full cleanup, forcing exit", 
                LOG_LEVEL_ERROR, 1, websocket_threads.thread_count);
        
        // Force cancel any remaining threads one last time
        for (int i = 0; i < websocket_threads.thread_count; i++) {
            if (websocket_threads.thread_ids[i] != 0) {
                pthread_cancel(websocket_threads.thread_ids[i]);
            }
        }
        
        // Finally just force clear the count
        websocket_threads.thread_count = 0;
    }
    
    log_this(SR_WEBSOCKET, "WebSocket server cleanup completed", LOG_LEVEL_STATE, 0);
}
