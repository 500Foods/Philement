/*
 * WebSocket Server Context Management
 *
 * Handles the creation, initialization, and cleanup of the WebSocket server context:
 * - Memory allocation and initialization
 * - Configuration management
 * - Resource cleanup
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    log_this("WebSocket", "Server context created successfully", LOG_LEVEL_INFO);
    return ctx;
}

void ws_context_destroy(WebSocketServerContext* ctx)
{
    if (!ctx) {
        return;
    }

    // Ensure we're in shutdown state
    ctx->shutdown = 1;

    // Clean up libwebsockets context if it exists
    if (ctx->lws_context) {
        struct lws_context *lws_ctx = ctx->lws_context;
        
        // Cancel any pending service to trigger final callbacks
        lws_cancel_service(lws_ctx);
        
        // Multiple service loops to ensure all callbacks are processed
        for (int i = 0; i < 5; i++) {
            lws_service(lws_ctx, 0);
            usleep(100000);  // 100ms delay between loops
        }

        // Now safe to null the context as callbacks are processed
        ctx->lws_context = NULL;
        
        // Wait for protocol destroy callback and connection cleanup
        struct timespec wait_time;
        clock_gettime(CLOCK_REALTIME, &wait_time);
        wait_time.tv_sec += 3;  // 3 second timeout

        pthread_mutex_lock(&ctx->mutex);
        while (ctx->active_connections > 0) {
            if (pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &wait_time) == ETIMEDOUT) {
                log_this("WebSocket", "Timeout waiting for cleanup, forcing shutdown", LOG_LEVEL_WARN);
                ctx->active_connections = 0;  // Force clear any remaining connections
                break;
            }
        }
        pthread_mutex_unlock(&ctx->mutex);

        // Final check for any remaining threads
        extern ServiceThreads websocket_threads;
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this("WebSocket", "Waiting for %d threads to exit", LOG_LEVEL_INFO,
                    websocket_threads.thread_count);
            
            // Give threads a chance to exit cleanly
            for (int i = 0; i < 10 && websocket_threads.thread_count > 0; i++) {
                usleep(100000);  // 100ms delay
                update_service_thread_metrics(&websocket_threads);
            }
            
            // Force any remaining threads to exit
            if (websocket_threads.thread_count > 0) {
                log_this("WebSocket", "Forcing %d threads to exit", LOG_LEVEL_WARN,
                        websocket_threads.thread_count);
                for (int i = 0; i < websocket_threads.thread_count; i++) {
                    pthread_t thread = websocket_threads.thread_ids[i];
                    pthread_cancel(thread);  // Force thread termination
                }
            }
        }

        // Final service loop to process any remaining callbacks
        lws_service(lws_ctx, 0);
        
        // Now safe to destroy the context
        lws_context_destroy(lws_ctx);
        
        // Final thread cleanup
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this("WebSocket", "Warning: %d threads remain after context destruction",
                    LOG_LEVEL_WARN, websocket_threads.thread_count);
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

    // Free the context itself
    free(ctx);

    log_this("WebSocket", "Server context destroyed", LOG_LEVEL_INFO);
    
    // Final delay to allow any pending logs to be processed
    usleep(100000);  // 100ms delay
}