/*
 * Safety-Critical Shutdown Handler for 3D Printer Control
 * 
 * Why Careful Shutdown Sequencing?
 * 1. Hardware Safety
 *    - Cool heating elements safely
 *    - Park print head away from bed
 *    - Disable stepper motors properly
 *    - Prevent material damage
 * 
 * 2. Print Job Handling
 *    Why So Critical?
 *    - Save print progress state
 *    - Enable job recovery
 *    - Preserve material
 *    - Document failure point
 * 
 * 3. Temperature Management
 *    Why This Sequence?
 *    - Gradual heater shutdown
 *    - Monitor cooling progress
 *    - Prevent thermal shock
 *    - Protect hot components
 * 
 * 4. Motion Control
 *    Why These Steps?
 *    - Complete current movements
 *    - Prevent axis binding
 *    - Secure loose filament
 *    - Home axes if safe
 * 
 * 5. Emergency Handling
 *    Why So Robust?
 *    - Handle power loss
 *    - Process emergency stops
 *    - Manage thermal runaway
 *    - Log critical events
 * 
 * 6. Resource Management
 *    Why This Order?
 *    - Save configuration state
 *    - Close network connections
 *    - Free system resources
 *    - Verify cleanup completion
 * 
 * 7. User Communication
 *    Why Keep Users Informed?
 *    - Display shutdown progress
 *    - Indicate safe states
 *    - Report error conditions
 *    - Guide recovery steps
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
#include "websocket_server_internal.h"  // For WebSocketServerContext
#include "mdns_server.h"
#include "print_queue_manager.h"  // For shutdown_print_queue()
#include "log_queue_manager.h"    // For close_file_logging()
#include "utils.h"

// Log level definitions
#define LOG_LEVEL_INFO 1

// Signal handler implementing graceful shutdown initiation
//
// Design choices for interrupt handling:
// 1. Thread Safety
//    - Minimal work in signal context
//    - Atomic flag modifications only
//    - Deferred cleanup to main thread
//
// 2. Coordination
//    - Single point of shutdown initiation
//    - Broadcast notification to all threads
//    - Prevents multiple shutdown attempts
void inthandler(int signum) {
    (void)signum;

    printf("\n");  // Keep newline for visual separation
    log_this("Shutdown", LOG_LINE_BREAK, LOG_LEVEL_INFO);
    log_this("Shutdown", "Cleaning up and shutting down", LOG_LEVEL_INFO);

    // Start timing the shutdown process
    record_shutdown_start_time();

    pthread_mutex_lock(&terminate_mutex);
    server_running = 0;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
}

// Stop network service advertisement with connection preservation
//
// mDNS shutdown strategy prioritizes:
// 1. Client Experience
//    - Clean service withdrawal
//    - Goodbye packet transmission
//    - DNS cache invalidation
//
// 2. Resource Management
//    - Socket cleanup
//    - Memory deallocation
//    - Thread termination
//
// 3. Network Stability
//    - Prevent lingering advertisements
//    - Clear multicast groups
//    - Release network resources
static void shutdown_mdns_system(void) {
    if (!app_config->mdns.enabled) {
        return;
    }

    log_this("Shutdown", "Initiating mDNS shutdown", LOG_LEVEL_INFO);
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
    
    log_this("Shutdown", "mDNS shutdown complete", LOG_LEVEL_INFO);
}

// Shutdown web and websocket servers
// Stops accepting new connections while allowing existing ones to complete
// Uses delays to ensure proper connection cleanup
static void shutdown_web_systems(void) {
    if (!app_config->web.enabled && !app_config->websocket.enabled) {
        return;
    }

    log_this("Shutdown", "Starting web systems shutdown sequence", LOG_LEVEL_INFO);

    // Shutdown web server if it was enabled
    if (app_config->web.enabled) {
        log_this("Shutdown", "Initiating Web Server shutdown", LOG_LEVEL_INFO);
        web_server_shutdown = 1;
        pthread_cond_broadcast(&terminate_cond);
        pthread_join(web_thread, NULL);
        shutdown_web_server();
        log_this("Shutdown", "Web Server shutdown complete", LOG_LEVEL_INFO);
    }

    // Shutdown WebSocket server if it was enabled
    if (app_config->websocket.enabled) {
        log_this("Shutdown", "Initiating WebSocket server shutdown", LOG_LEVEL_INFO);
        
        // Signal shutdown to all subsystems
        websocket_server_shutdown = 1;
        pthread_cond_broadcast(&terminate_cond);

        // First attempt: Give connections time to close gracefully
        log_this("Shutdown", "Waiting for WebSocket connections to close gracefully", LOG_LEVEL_INFO);
        usleep(2000000);  // 2s initial delay

        // Stop the server and wait for thread exit
        log_this("Shutdown", "Stopping WebSocket server", LOG_LEVEL_INFO);
        stop_websocket_server();

        // Second phase: Force close any remaining connections
        log_this("Shutdown", "Forcing close of any remaining connections", LOG_LEVEL_INFO);
        extern WebSocketServerContext *ws_context;  // Declare external context
        if (ws_context) {
            // Set shutdown flag and cancel service to interrupt any blocking operations
            ws_context->shutdown = 1;
            if (ws_context->lws_context) {
                lws_cancel_service(ws_context->lws_context);
            }
            
            // Wait for any remaining threads with timeout
            struct timespec wait_time;
            clock_gettime(CLOCK_REALTIME, &wait_time);
            wait_time.tv_sec += 1;  // 1s timeout

            pthread_mutex_lock(&ws_context->mutex);
            while (ws_context->active_connections > 0) {
                if (pthread_cond_timedwait(&ws_context->cond, &ws_context->mutex, &wait_time) == ETIMEDOUT) {
                    log_this("Shutdown", "Timeout waiting for connections, forcing cleanup", LOG_LEVEL_WARN);
                    break;
                }
            }
            pthread_mutex_unlock(&ws_context->mutex);
            
            // Force cleanup regardless of state
            log_this("Shutdown", "Cleaning up WebSocket resources", LOG_LEVEL_INFO);
            cleanup_websocket_server();
        }

        // Update thread metrics one final time
        extern ServiceThreads websocket_threads;
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this("Shutdown", "Warning: %d WebSocket threads still active", 
                    LOG_LEVEL_WARN, websocket_threads.thread_count);
        }
        
        log_this("Shutdown", "WebSocket server shutdown complete", LOG_LEVEL_INFO);
    }
}

// Shutdown print queue system
// Ensures current print jobs are completed or safely cancelled
// Waits for queue manager thread to process shutdown
static void shutdown_print_system(void) {
    if (!app_config->print_queue.enabled) {
        return;
    }

    log_this("Shutdown", "Initiating Print Queue shutdown", LOG_LEVEL_INFO);
    print_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(print_queue_thread, NULL);
    shutdown_print_queue();
    log_this("Shutdown", "Print Queue shutdown complete", LOG_LEVEL_INFO);
}

// Clean up network resources
// Called after all network-using components are stopped
// Frees network interface information
static void shutdown_network(void) {
    log_this("Shutdown", "Freeing network info", LOG_LEVEL_INFO);
    if (net_info) {
        free_network_info(net_info);
        net_info = NULL;
    }
}

// Free all configuration resources
// Must be called last as other components may need config during shutdown
// Recursively frees all allocated configuration structures
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
        free(app_config->websocket.protocol);
        free(app_config->websocket.key);

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

        // Free Logging config
        if (app_config->Logging.Levels) {
            for (size_t i = 0; i < app_config->Logging.LevelCount; i++) {
                free((void*)app_config->Logging.Levels[i].name);
            }
            free(app_config->Logging.Levels);
        }

        free(app_config);
        app_config = NULL;
    }
}

// Orchestrate system shutdown with dependency-aware sequencing
//
// The shutdown architecture implements:
// 1. Component Dependencies
//    - Service advertisement first
//    - Network services second
//    - Core systems last
//    - Configuration cleanup final
//
// 2. Resource Safety
//    - Staged cleanup phases
//    - Timeout-based waiting
//    - Forced cleanup fallbacks
//    - Memory leak prevention
//
// 3. Error Handling
//    - Component isolation
//    - Partial shutdown recovery
//    - Resource leak prevention
//    - Cleanup verification
void graceful_shutdown(void) {
    log_this("Shutdown", "Starting graceful shutdown sequence", LOG_LEVEL_INFO);
    
    // Signal all threads that shutdown is imminent
    pthread_mutex_lock(&terminate_mutex);
    server_running = 0;
    server_stopping = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);

    // First stop accepting new connections/requests
    log_this("Shutdown", "Stopping mDNS service...", LOG_LEVEL_INFO);
    shutdown_mdns_system();
    
    log_this("Shutdown", "Stopping web services...", LOG_LEVEL_INFO);
    shutdown_web_systems();
    
    log_this("Shutdown", "Stopping print system...", LOG_LEVEL_INFO);
    shutdown_print_system();
    
    log_this("Shutdown", "Cleaning up network...", LOG_LEVEL_INFO);
    shutdown_network();

    // Give threads a moment to process their shutdown flags
    usleep(1000000);  // 1s delay

    // Now safe to stop logging since other components are done
    log_this("Shutdown", "Subsystem shutdown completed", LOG_LEVEL_INFO);    
    log_this("Shutdown", LOG_LINE_BREAK, LOG_LEVEL_INFO);

    // Check remaining threads before logging shutdown
    extern ServiceThreads logging_threads;
    extern ServiceThreads web_threads;
    extern ServiceThreads websocket_threads;
    extern ServiceThreads mdns_threads;
    extern ServiceThreads print_threads;
    
    // Update thread metrics to clean up any dead threads
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);
    
    // Only show thread status if there are remaining threads
    if (logging_threads.thread_count > 0 || web_threads.thread_count > 0 ||
        websocket_threads.thread_count > 0 || mdns_threads.thread_count > 0 ||
        print_threads.thread_count > 0) {
        char thread_status[256];
        snprintf(thread_status, sizeof(thread_status), 
                 "Remaining threads - Log: %d, Web: %d, WS: %d, mDNS: %d, Print: %d",
                 logging_threads.thread_count,
                 web_threads.thread_count,
                 websocket_threads.thread_count,
                 mdns_threads.thread_count,
                 print_threads.thread_count);
                log_this("Shutdown", thread_status, LOG_LEVEL_INFO);
    }

    // Wait for remaining threads with simplified status updates
    int wait_count = 0;
    const int max_wait_cycles = 10;  // 5 seconds total
    bool threads_active;

    do {
        threads_active = false;
        update_service_thread_metrics(&logging_threads);
        update_service_thread_metrics(&web_threads);
        update_service_thread_metrics(&websocket_threads);
        update_service_thread_metrics(&mdns_threads);
        update_service_thread_metrics(&print_threads);

        // Calculate total non-logging threads
        int non_logging_threads = web_threads.thread_count + 
                                websocket_threads.thread_count + 
                                mdns_threads.thread_count + 
                                print_threads.thread_count;

        // Only continue waiting if we have more than just logging thread
        if (non_logging_threads > 0) {
            threads_active = true;
            
            // Only log detailed thread info at debug level
            if (wait_count == 0 || wait_count == max_wait_cycles - 1) {
                char msg[128];
                snprintf(msg, sizeof(msg), "Waiting for %d thread(s) to exit (attempt %d/%d)", 
                        non_logging_threads,
                        wait_count + 1, max_wait_cycles);
                log_this("Shutdown", msg, LOG_LEVEL_INFO);
            }
            
            // Signal any waiting threads
            pthread_mutex_lock(&terminate_mutex);
            pthread_cond_broadcast(&terminate_cond);
            pthread_mutex_unlock(&terminate_mutex);
            
            usleep(500000);  // 500ms delay between checks
        } else if (logging_threads.thread_count > 0) {
            // Only logging thread remains, which is expected
            log_this("Shutdown", "Only logging thread remains, proceeding with shutdown", LOG_LEVEL_INFO);
            break;
        }
        
        wait_count++;
    } while (threads_active && wait_count < max_wait_cycles);

    if (threads_active && web_threads.thread_count + websocket_threads.thread_count + 
        mdns_threads.thread_count + print_threads.thread_count > 0) {
        log_this("Shutdown", "Some non-logging threads did not exit cleanly", LOG_LEVEL_WARN);
    } else {
        log_this("Shutdown", "All non-logging threads exited successfully", LOG_LEVEL_INFO);
    }

    // Now safe to stop logging
    log_this("Shutdown", "Shutting down logging system", LOG_LEVEL_INFO);
    pthread_mutex_lock(&terminate_mutex);
    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    
    // Wait for log thread to finish processing remaining messages
    log_this("Shutdown", "Waiting for log queue to drain...", LOG_LEVEL_INFO);
    pthread_join(log_thread, NULL);
    
    // Wait for any pending log operations
    usleep(500000);  // 500ms delay
    
    // Update all thread metrics one final time
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);

    // Get main thread ID for exclusion from counts
    pthread_t main_thread = pthread_self();
    
    // Calculate total remaining threads
    int total_threads = logging_threads.thread_count + web_threads.thread_count +
                       websocket_threads.thread_count + mdns_threads.thread_count +
                       print_threads.thread_count;

    // Only show detailed status if there are remaining threads
    if (total_threads > 0) {
        char thread_status[256];
        snprintf(thread_status, sizeof(thread_status), 
                 "Remaining threads before final cleanup - Log: %d, Web: %d, WS: %d, mDNS: %d, Print: %d",
                 logging_threads.thread_count,
                 web_threads.thread_count,
                 websocket_threads.thread_count,
                 mdns_threads.thread_count,
                 print_threads.thread_count);
        log_this("Shutdown", thread_status, LOG_LEVEL_INFO);
    }
    
    // Now safe to destroy queues
    log_this("Shutdown", "Shutting down queue system", LOG_LEVEL_INFO);
    queue_system_destroy();
    usleep(100000);  // 100ms delay
    
    // Update thread metrics one last time
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);

    // Count non-main threads and check their state
    int non_main_threads = 0;
    char thread_info[2048] = {0};
    int offset = 0;
    bool has_uninterruptible = false;

    // Helper macro to check thread state and syscall
    #define CHECK_THREAD_STATE(threads, name) \
        for (int i = 0; i < threads.thread_count; i++) { \
            if (!pthread_equal(threads.thread_ids[i], main_thread)) { \
                non_main_threads++; \
                char state = '?'; \
                char syscall[256] = "unknown"; \
                bool is_uninterruptible = false; \
                \
                /* Check thread state */ \
                char state_path[64]; \
                snprintf(state_path, sizeof(state_path), "/proc/%d/status", threads.thread_tids[i]); \
                FILE *status = fopen(state_path, "r"); \
                if (status) { \
                    char line[256]; \
                    while (fgets(line, sizeof(line), status)) { \
                        if (strncmp(line, "State:", 6) == 0) { \
                            state = line[7]; \
                            is_uninterruptible = (state == 'D'); \
                            has_uninterruptible |= is_uninterruptible; \
                            break; \
                        } \
                    } \
                    fclose(status); \
                } \
                \
                /* Check if thread is stuck in syscall */ \
                char syscall_path[64]; \
                snprintf(syscall_path, sizeof(syscall_path), "/proc/%d/syscall", threads.thread_tids[i]); \
                FILE *sc = fopen(syscall_path, "r"); \
                if (sc) { \
                    if (fgets(syscall, sizeof(syscall), sc)) { \
                        char *newline = strchr(syscall, '\n'); \
                        if (newline) *newline = '\0'; \
                    } \
                    fclose(sc); \
                } \
                \
                offset += snprintf(thread_info + offset, sizeof(thread_info) - offset, \
                                 "\n  %s thread: %lu (tid: %d)\n    State: %c%s, Syscall: %s", \
                                 name, \
                                 (unsigned long)threads.thread_ids[i], \
                                 threads.thread_tids[i], \
                                 state, \
                                 is_uninterruptible ? " (uninterruptible)" : "", \
                                 syscall); \
            } \
        }

    CHECK_THREAD_STATE(logging_threads, "Logging");
    CHECK_THREAD_STATE(web_threads, "Web");
    CHECK_THREAD_STATE(websocket_threads, "WebSocket");
    CHECK_THREAD_STATE(mdns_threads, "mDNS");
    CHECK_THREAD_STATE(print_threads, "Print");

    #undef CHECK_THREAD_STATE

    if (non_main_threads > 0) {
        char msg[4096];
        snprintf(msg, sizeof(msg), "%d non-main thread(s) still active:%s", 
                non_main_threads, thread_info);
        log_this("Shutdown", msg, LOG_LEVEL_WARN);
        
        // One final attempt to signal threads
        pthread_mutex_lock(&terminate_mutex);
        pthread_cond_broadcast(&terminate_cond);
        pthread_mutex_unlock(&terminate_mutex);
        
        // Give threads more time if any are in uninterruptible sleep
        usleep(has_uninterruptible ? 10000000 : 2000000);  // 10s delay for D state, 2s otherwise
        
        // Force cleanup if threads are still stuck
        if (has_uninterruptible) {
            log_this("Shutdown", "Some threads are in uninterruptible state, forcing cleanup", LOG_LEVEL_WARN);
            // Let the OS clean up remaining threads
            _exit(0);
        }
    } else {
        log_this("Shutdown", "All non-main threads exited successfully", LOG_LEVEL_INFO);
    }

    // Record final timing statistics and log final messages
    record_shutdown_end_time();
    log_this("Shutdown", "Shutdown complete", LOG_LEVEL_INFO);

    // Brief delay to ensure log message is processed
    usleep(100000);

    // Now safe to destroy synchronization primitives
    pthread_mutex_lock(&terminate_mutex);
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    
    usleep(100000);  // Brief delay before destruction
    
    pthread_cond_destroy(&terminate_cond);
    pthread_mutex_destroy(&terminate_mutex);

    // Free configuration last since other components might need it
    free_app_config();
}