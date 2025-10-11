/*
 * WebSocket Server Context Management
 *
 * Handles the creation, initialization, and cleanup of the WebSocket server context:
 * - Memory allocation and initialization
 * - Configuration management
 * - Resource cleanup
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_internal.h"

WebSocketServerContext* ws_context_create(int port, const char* protocol, const char* key)
{
    WebSocketServerContext *ctx = calloc(1, sizeof(WebSocketServerContext));
    if (!ctx) {
        log_this(SR_WEBSOCKET, "Failed to allocate server context", LOG_LEVEL_ERROR, 0);
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
        log_this(SR_WEBSOCKET, "Failed to initialize mutex", LOG_LEVEL_ERROR, 0);
        free(ctx);
        return NULL;
    }

    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        log_this(SR_WEBSOCKET, "Failed to initialize condition variable", LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }

    // Initialize message buffer
    ctx->max_message_size = app_config->websocket.max_message_size;
    ctx->message_buffer = malloc(ctx->max_message_size + 1);
    if (!ctx->message_buffer) {
        log_this(SR_WEBSOCKET, "Failed to allocate message buffer", LOG_LEVEL_ERROR, 0);
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

    log_this(SR_WEBSOCKET, "Server context created successfully", LOG_LEVEL_STATE, 0);
    return ctx;
}

void ws_context_destroy(WebSocketServerContext* ctx)
{
    if (!ctx) {
        return;
    }

    log_this(SR_WEBSOCKET, "Starting context destruction", LOG_LEVEL_STATE, 0);
    
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
            log_this(SR_WEBSOCKET, "Forcing %d connections to close", LOG_LEVEL_ALERT, 1, ctx->active_connections);
            ctx->active_connections = 0;
        }
        pthread_cond_broadcast(&ctx->cond);
        pthread_mutex_unlock(&ctx->mutex);

        // Cancel any remaining threads before context destruction
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this(SR_WEBSOCKET, "Cancelling %d remaining threads", LOG_LEVEL_ALERT, 1, websocket_threads.thread_count);
            
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
        if (signal_based_shutdown || restart_requested) {
            log_this(SR_WEBSOCKET, "Skipping expensive lws_context_destroy during %s", LOG_LEVEL_STATE, 1, signal_based_shutdown ? "signal shutdown" : "restart");
            // OS will clean up resources on process exit/restart, no need for graceful cleanup
        } else {
            log_this(SR_WEBSOCKET, "Destroying libwebsockets context", LOG_LEVEL_STATE, 0);
            
            // Aggressive cleanup sequence - force all pending operations to complete
            lws_cancel_service(lws_ctx);
            
            // Minimal service processing to drain any immediate callbacks
            lws_service(lws_ctx, 0);
            lws_service(lws_ctx, 0);
            
            // Destroy the libwebsockets context immediately - no delays
            log_this(SR_WEBSOCKET, "Calling lws_context_destroy", LOG_LEVEL_STATE, 0);
            lws_context_destroy(lws_ctx);
            log_this(SR_WEBSOCKET, "lws_context_destroy completed", LOG_LEVEL_STATE, 0);
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

    log_this(SR_WEBSOCKET, "Freeing context structure", LOG_LEVEL_STATE, 0);
    
    // Free the context itself - this fixes the 680-byte leak
    free(ctx);

    log_this(SR_WEBSOCKET, "Context destruction completed", LOG_LEVEL_STATE, 0);
}
