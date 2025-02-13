/*
 * Shutdown sequence handler for the Hydrogen 3D printer server.
 * 
 * This file manages the graceful shutdown of all server components including
 * mDNS, web server, WebSocket server, print queue, and logging system. It ensures
 * proper cleanup order and handles resource deallocation to prevent memory leaks.
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Project headers
#include "state.h"
#include "logging.h"
#include "queue.h"
#include "web_server.h"
#include "websocket_server.h"
#include "mdns_server.h"
#include "print_queue_manager.h"  // For shutdown_print_queue()
#include "log_queue_manager.h"    // For close_file_logging()

// Ctrl+C handler
void inthandler(int signum) {
    (void)signum;

    printf("\n");
    log_this("Shutdown", "Cleaning up and shutting down", 0, true, true, true);

    pthread_mutex_lock(&terminate_mutex);
    keep_running = 0;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

static void shutdown_mdns_system(void) {
    log_this("Shutdown", "Initiating mDNS shutdown", 0, true, true, true);
    mdns_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    
    // Get the thread arguments before joining
    void *thread_arg;
    pthread_join(mdns_thread, &thread_arg);
    
    // Clean up mDNS resources
    if (mdns) {
        mdns_shutdown(mdns);
        mdns = NULL;
    }
    
    // Free the thread arguments if they exist
    if (thread_arg) {
        free(thread_arg);
    }
    
    log_this("Shutdown", "mDNS shutdown complete", 0, true, true, true);
}

static void shutdown_web_systems(void) {
    log_this("Shutdown", "Starting web systems shutdown sequence", 0, true, true, true);

    // First shutdown web server
    log_this("Shutdown", "Initiating Web Server shutdown", 0, true, true, true);
    web_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(web_thread, NULL);
    shutdown_web_server();
    log_this("Shutdown", "Web Server shutdown complete", 0, true, true, true);

    // Then handle WebSocket server
    log_this("Shutdown", "Initiating WebSocket server shutdown", 0, true, true, true);
    websocket_server_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);

    // Give WebSocket server time to process shutdown flag and close connections
    usleep(2000000);  // 2s delay for connections to close

    // Stop the server with a timeout
    log_this("Shutdown", "Stopping WebSocket server", 0, true, true, true);
    stop_websocket_server();

    // Wait for server thread to fully exit
    usleep(1000000);  // 1s delay

    // Force cleanup regardless of connection state
    log_this("Shutdown", "Cleaning up WebSocket resources", 0, true, true, true);
    cleanup_websocket_server();

    // Brief final delay
    usleep(100000);  // 100ms delay
}

static void shutdown_print_system(void) {
    log_this("Shutdown", "Initiating Print Queue shutdown", 0, true, true, true);
    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(print_queue_thread, NULL);
    shutdown_print_queue();
    log_this("Shutdown", "Print Queue shutdown complete", 0, true, true, true);
}

static void shutdown_network(void) {
    log_this("Shutdown", "Freeing network info", 0, true, true, true);
    if (net_info) {
        free_network_info(net_info);
        net_info = NULL;
    }
}

static void free_app_config(void) {
    if (app_config) {
        free(app_config->server_name);
        free(app_config->executable_path);
        free(app_config->log_file_path);

        // Free Web config
        free(app_config->web.web_root);
        free(app_config->web.upload_path);
        free(app_config->web.upload_dir);

        // Free WebSocket config
        free(app_config->websocket.key);
        free(app_config->websocket.protocol);

        // Free mDNS config
        free(app_config->mdns.device_id);
        free(app_config->mdns.friendly_name);
        free(app_config->mdns.model);
        free(app_config->mdns.manufacturer);
        free(app_config->mdns.version);
        for (size_t i = 0; i < app_config->mdns.num_services; i++) {
            free(app_config->mdns.services[i].name);
            free(app_config->mdns.services[i].type);
            for (size_t j = 0; j < app_config->mdns.services[i].num_txt_records; j++) {
                free(app_config->mdns.services[i].txt_records[j]);
            }
            free(app_config->mdns.services[i].txt_records);
        }
        free(app_config->mdns.services);

        free(app_config);
        app_config = NULL;
    }
}

void graceful_shutdown(void) {
    printf("\n");  // Keep this printf for visual separation
    log_this("Shutdown", "Initiating graceful shutdown", 0, true, true, true);
    
    // Signal all threads that shutdown is imminent
    pthread_mutex_lock(&terminate_mutex);
    keep_running = 0;
    shutting_down = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);

    // First stop accepting new connections/requests
    log_this("Shutdown", "Initiating mDNS system shutdown", 0, true, true, true);
    shutdown_mdns_system();
    
    log_this("Shutdown", "Initiating web systems shutdown", 0, true, true, true);
    shutdown_web_systems();
    
    log_this("Shutdown", "Initiating print system shutdown", 0, true, true, true);
    shutdown_print_system();
    
    log_this("Shutdown", "Initiating network shutdown", 0, true, true, true);
    shutdown_network();

    // Give threads a moment to process their shutdown flags
    usleep(1000000);  // 1s delay

    // Now safe to stop logging since other components are done
    printf("Shutting down logging system...\n");
    pthread_mutex_lock(&terminate_mutex);
    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    
    // Wait for log thread to finish processing remaining messages
    printf("Waiting for log queue to drain...\n");
    pthread_join(log_thread, NULL);
    
    // Wait for any pending log operations
    usleep(500000);  // 500ms delay
    
    // Now safe to destroy queues since all threads are stopped
    printf("Shutting down queue system...\n");
    queue_system_destroy();

    // Wait for any pending operations
    usleep(500000);  // 500ms delay

    // Release synchronization primitives
    pthread_mutex_lock(&terminate_mutex);
    pthread_cond_broadcast(&terminate_cond);  // Final broadcast
    usleep(100000);  // 100ms delay to let any waiting threads process
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    pthread_mutex_destroy(&terminate_mutex);

    // Wait for any pending operations
    usleep(500000);  // 500ms delay

    // Free configuration last since other components might need it
    free_app_config();

    printf("Shutdown complete\n");
}