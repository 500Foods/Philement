/*
 * WebSocket Server Context Management
 *
 * Handles the creation, initialization, and cleanup of the WebSocket server context:
 * - Memory allocation and initialization
 * - Configuration management
 * - Resource cleanup
 */

/* Feature test macros must come first */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

// System headers
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

// Project headers
#include "websocket_server_internal.h"
#include "../logging/logging.h"
#include "../config/config.h"

// External configuration reference
extern AppConfig *app_config;

WebSocketServerContext* ws_context_create(int port, const char* protocol, const char* key)
{
    WebSocketServerContext *ctx = calloc(1, sizeof(WebSocketServerContext));
    if (!ctx) {
        log_this("WebSocket", "Failed to allocate server context", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Initialize configuration with validation
    ctx->port = port;
    
    // Handle protocol
    if (protocol) {
        strncpy(ctx->protocol, protocol, sizeof(ctx->protocol) - 1);
        ctx->protocol[sizeof(ctx->protocol) - 1] = '\0';
    } else {
        strncpy(ctx->protocol, "hydrogen-protocol", sizeof(ctx->protocol) - 1);
        ctx->protocol[sizeof(ctx->protocol) - 1] = '\0';
    }

    // Handle auth key
    if (key) {
        strncpy(ctx->auth_key, key, sizeof(ctx->auth_key) - 1);
        ctx->auth_key[sizeof(ctx->auth_key) - 1] = '\0';
    } else {
        strncpy(ctx->auth_key, "default_key", sizeof(ctx->auth_key) - 1);
        ctx->auth_key[sizeof(ctx->auth_key) - 1] = '\0';
    }

    // Initialize mutex and condition variable
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        log_this("WebSocket", "Failed to initialize mutex", LOG_LEVEL_ERROR);
        free(ctx);
        return NULL;
    }

    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        log_this("WebSocket", "Failed to initialize condition variable", LOG_LEVEL_ERROR);
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }

    // Initialize message buffer
    ctx->max_message_size = app_config->websocket.max_message_size;
    ctx->message_buffer = malloc(ctx->max_message_size + 1);
    if (!ctx->message_buffer) {
        log_this("WebSocket", "Failed to allocate message buffer", LOG_LEVEL_ERROR);
        pthread_mutex_destroy(&ctx->mutex);
        pthread_cond_destroy(&ctx->cond);
        free(ctx);
        return NULL;
    }

    // Initialize metrics
    ctx->start_time = time(NULL);
    ctx->active_connections = 0;
    ctx->total_connections = 0;
    ctx->total_requests = 0;

    // Initialize state
    ctx->shutdown = 0;
    ctx->vhost_creating = 0;  // Start with vhost creation flag clear
    ctx->message_length = 0;
    ctx->lws_context = NULL;  // Will be set during server initialization

    log_this("WebSocket", "Server context created successfully", LOG_LEVEL_STATE);
    return ctx;
}

void ws_context_destroy(WebSocketServerContext* ctx)
{
    if (!ctx) {
        return;
    }

    log_this("WebSocket", "Starting context destruction", LOG_LEVEL_STATE);
    
    // Ensure we're in shutdown state
    ctx->shutdown = 1;

    // Clean up libwebsockets context if it exists
    if (ctx->lws_context) {
        struct lws_context *lws_ctx = ctx->lws_context;
        
        // Clear the context pointer immediately to prevent further use
        ctx->lws_context = NULL;
        
        // Force clear any remaining connections
        pthread_mutex_lock(&ctx->mutex);
        if (ctx->active_connections > 0) {
            log_this("WebSocket", "Forcing %d connections to close", LOG_LEVEL_ALERT, 
                    ctx->active_connections);
            ctx->active_connections = 0;
        }
        pthread_cond_broadcast(&ctx->cond);
        pthread_mutex_unlock(&ctx->mutex);

        // Cancel any remaining threads before context destruction
        extern ServiceThreads websocket_threads;
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this("WebSocket", "Cancelling %d remaining threads", LOG_LEVEL_ALERT,
                    websocket_threads.thread_count);
            
            for (int i = 0; i < websocket_threads.thread_count; i++) {
                pthread_t thread = websocket_threads.thread_ids[i];
                if (thread != 0) {
                    pthread_cancel(thread);
                }
            }
            
            // Force clear thread count immediately
            websocket_threads.thread_count = 0;
        }

        // Check if we're in a signal-based shutdown (SIGINT/SIGTERM) or restart (SIGHUP) for rapid exit
        extern volatile sig_atomic_t signal_based_shutdown;
        extern volatile sig_atomic_t restart_requested;
        if (signal_based_shutdown || restart_requested) {
            log_this("WebSocket", "Skipping expensive lws_context_destroy during %s", LOG_LEVEL_STATE,
                     signal_based_shutdown ? "signal shutdown" : "restart");
            // OS will clean up resources on process exit/restart, no need for graceful cleanup
        } else {
            log_this("WebSocket", "Destroying libwebsockets context", LOG_LEVEL_STATE);
            
            // Aggressive cleanup sequence - force all pending operations to complete
            lws_cancel_service(lws_ctx);
            
            // Minimal service processing to drain any immediate callbacks
            lws_service(lws_ctx, 0);
            lws_service(lws_ctx, 0);
            
            // Destroy the libwebsockets context immediately - no delays
            log_this("WebSocket", "Calling lws_context_destroy", LOG_LEVEL_STATE);
            lws_context_destroy(lws_ctx);
            log_this("WebSocket", "lws_context_destroy completed", LOG_LEVEL_STATE);
        }
    }

    // Free message buffer
    if (ctx->message_buffer) {
        free(ctx->message_buffer);
        ctx->message_buffer = NULL;
    }

    // Clean up synchronization primitives
    pthread_mutex_destroy(&ctx->mutex);
    pthread_cond_destroy(&ctx->cond);

    log_this("WebSocket", "Freeing context structure", LOG_LEVEL_STATE);
    
    // Free the context itself - this fixes the 680-byte leak
    free(ctx);

    log_this("WebSocket", "Context destruction completed", LOG_LEVEL_STATE);
}