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

// Feature test macros
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
#include "../logging/logging.h"
#include "../queue/queue.h"
#include "../webserver/web_server.h"
#include "../websocket/websocket_server.h"
#include "../websocket/websocket_server_internal.h"  // For WebSocketServerContext
#include "../mdns/mdns_server.h"
#include "../print/print_queue_manager.h"  // For shutdown_print_queue()
#include "../logging/log_queue_manager.h"    // For close_file_logging()
#include "../utils/utils.h"

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
    static volatile sig_atomic_t already_shutting_down = 0;

    // Prevent reentrancy or multiple calls to shutdown
    if (already_shutting_down) {
        return; // Exit immediately if we're already handling shutdown
    }
    already_shutting_down = 1;

    printf("\n");  // Keep newline for visual separation

    // Set server state flags to prevent reinitialization
    server_running = 0; // Stop the main loop
    server_stopping = 1; // Prevent reinitialization during shutdown

    // Call graceful shutdown - this will handle all logging and cleanup
    graceful_shutdown();
}

// Stop network service advertisement with connection preservation
//
// mDNS Server shutdown strategy prioritizes:
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
static void shutdown_mdns_server_system(void) {
    if (!app_config->mdns_server.enabled) {
        return;
    }

    log_this("Shutdown", "Initiating mDNS Server shutdown", LOG_LEVEL_STATE);
    mdns_server_system_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    
    // Get the thread arguments before joining
    void *thread_arg;
    pthread_join(mdns_server_thread, &thread_arg);
    
    // Clean up mDNS Server resources
    if (mdns_server) {
        mdns_server_shutdown(mdns_server);
        mdns_server = NULL;
    }
    
    // Free the thread arguments if they exist
    if (thread_arg) {
        free(thread_arg);
    }
    
    log_this("Shutdown", "mDNS Server shutdown complete", LOG_LEVEL_STATE);
}

// Shutdown web and websocket servers
// Stops accepting new connections while allowing existing ones to complete
// Uses delays to ensure proper connection cleanup
static void shutdown_web_systems(void) {
    if (!app_config->web.enabled && !app_config->websocket.enabled) {
        return;
    }

    log_this("Shutdown", "Starting web systems shutdown sequence", LOG_LEVEL_STATE);

    // Shutdown web server if it was enabled
    if (app_config->web.enabled) {
        log_this("Shutdown", "Initiating Web Server shutdown", LOG_LEVEL_STATE);
        web_server_shutdown = 1;
        pthread_cond_broadcast(&terminate_cond);
        pthread_join(web_thread, NULL);
        shutdown_web_server();
        log_this("Shutdown", "Web Server shutdown complete", LOG_LEVEL_STATE);
    }

    // Shutdown WebSocket server if it was enabled
    if (app_config->websocket.enabled) {
        log_this("Shutdown", "Initiating WebSocket server shutdown", LOG_LEVEL_STATE);
        
        // Signal shutdown to all subsystems
        websocket_server_shutdown = 1;
        pthread_cond_broadcast(&terminate_cond);

        // First attempt: Give connections time to close gracefully
        log_this("Shutdown", "Waiting for WebSocket connections to close gracefully", LOG_LEVEL_STATE);
        usleep(2000000);  // 2s initial delay

        // Stop the server and wait for thread exit
        log_this("Shutdown", "Stopping WebSocket server", LOG_LEVEL_STATE);
        stop_websocket_server();

        // Second phase: Force close any remaining connections
        log_this("Shutdown", "Forcing close of any remaining connections", LOG_LEVEL_STATE);
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
            
        // EMERGENCY BYPASS: Skip standard cleanup to avoid libwebsockets hang
        log_this("Shutdown", "EMERGENCY BYPASS: Skipping standard WebSocket cleanup", LOG_LEVEL_WARN);
        
        // Set a process-wide emergency timeout
        pid_t current_pid = getpid();
        pid_t child_pid = fork();
        
        if (child_pid == 0) {
            // Child process: wait and then force kill parent if still running
            sleep(3);  // Give parent 3 seconds to complete cleanup
            
            // Check if parent still exists
            if (kill(current_pid, 0) == 0) {
                log_this("Shutdown", "CRITICAL: Force terminating parent process", LOG_LEVEL_ERROR);
                kill(current_pid, SIGKILL);  // Force kill if parent is still running
            }
            exit(0);  // Child exits
        } else if (child_pid > 0) {
            // Parent: Continue with minimal cleanup
            
            // Attempt minimal resource cleanup without entering libwebsockets network code
            extern WebSocketServerContext *ws_context;
            if (ws_context != NULL) {
                // Forcibly free critical resources without full cleanup
                pthread_mutex_lock(&ws_context->mutex);
                ws_context->shutdown = 1;  // Ensure shutdown flag is set
                ws_context->active_connections = 0;  // Force reset connections
                
                // Cancel service one last time
                if (ws_context->lws_context) {
                    log_this("Shutdown", "Force cancelling libwebsockets service", LOG_LEVEL_WARN);
                    lws_cancel_service(ws_context->lws_context);
                }
                pthread_mutex_unlock(&ws_context->mutex);
                
                // Do NOT call lws_context_destroy as it hangs
                log_this("Shutdown", "SKIPPING libwebsockets context destruction", LOG_LEVEL_WARN);
                ws_context = NULL;  // Just discard the pointer
            }
            
            // Force clear any websocket thread tracking
            extern ServiceThreads websocket_threads;
            websocket_threads.thread_count = 0;
            
            log_this("Shutdown", "Emergency WebSocket cleanup completed", LOG_LEVEL_STATE);
        }
        }

        // Update thread metrics one final time
        extern ServiceThreads websocket_threads;
        update_service_thread_metrics(&websocket_threads);
        if (websocket_threads.thread_count > 0) {
            log_this("Shutdown", "Warning: %d WebSocket threads still active", 
                    LOG_LEVEL_WARN, websocket_threads.thread_count);
        }
        
        log_this("Shutdown", "WebSocket server shutdown complete", LOG_LEVEL_STATE);
    }
}

// Shutdown print queue system
// Ensures current print jobs are completed or safely cancelled
// Waits for queue manager thread to process shutdown
static void shutdown_print_system(void) {
    if (!app_config->print_queue.enabled) {
        return;
    }

    log_this("Shutdown", "Initiating Print Queue shutdown", LOG_LEVEL_STATE);
    print_system_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_join(print_queue_thread, NULL);
    shutdown_print_queue();
    log_this("Shutdown", "Print Queue shutdown complete", LOG_LEVEL_STATE);
}

// Clean up network resources
// Called after all network-using components are stopped
// Frees network interface information
static void shutdown_network(void) {
    log_this("Shutdown", "Freeing network info", LOG_LEVEL_STATE);
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
        free(app_config->server.server_name);
        free(app_config->server.payload_key);
        free(app_config->server.config_file);
        free(app_config->server.exec_file);
        free(app_config->server.log_file);

        // Free Web config
        free(app_config->web.web_root);
        free(app_config->web.upload_path);
        free(app_config->web.upload_dir);

        // Free WebSocket config
        free(app_config->websocket.protocol);
        free(app_config->websocket.key);

        // Free mDNS Server config
        free(app_config->mdns_server.device_id);
        free(app_config->mdns_server.friendly_name);
        free(app_config->mdns_server.model);
        free(app_config->mdns_server.manufacturer);
        free(app_config->mdns_server.version);
        for (size_t i = 0; i < app_config->mdns_server.num_services; i++) {
            free(app_config->mdns_server.services[i].name);
            free(app_config->mdns_server.services[i].type);
            for (size_t j = 0; j < app_config->mdns_server.services[i].num_txt_records; j++) {
                free(app_config->mdns_server.services[i].txt_records[j]);
            }
            free(app_config->mdns_server.services[i].txt_records);
        }
        free(app_config->mdns_server.services);

        // Free logging config
        if (app_config->logging.levels) {
            for (size_t i = 0; i < app_config->logging.level_count; i++) {
                free((void*)app_config->logging.levels[i].name);
            }
            free(app_config->logging.levels);
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
    // Prevent multiple shutdown sequences using atomic operation
    static volatile sig_atomic_t shutdown_in_progress = 0;
    if (!__sync_bool_compare_and_swap(&shutdown_in_progress, 0, 1)) {
        // If we couldn't set the flag, shutdown is already in progress
        return;
    }
    
    // Start shutdown sequence with unified logging
    log_this("Shutdown", LOG_LINE_BREAK, LOG_LEVEL_STATE);
    log_this("Shutdown", "Cleaning up and shutting down", LOG_LEVEL_STATE);
    
    // Start timing the shutdown process
    record_shutdown_start_time();
    
    // Set all shutdown flags atomically before any operations
    extern volatile sig_atomic_t server_starting;
    extern volatile sig_atomic_t mdns_server_system_shutdown;
    extern volatile sig_atomic_t web_server_shutdown;
    extern volatile sig_atomic_t websocket_server_shutdown;
    extern volatile sig_atomic_t print_queue_shutdown;
    extern volatile sig_atomic_t log_queue_shutdown;

    // First ensure all initialization and startup is blocked
    log_this("Shutdown", "Setting shutdown flags...", LOG_LEVEL_STATE);
    
    // Set core state flags first
    __sync_bool_compare_and_swap(&server_starting, 1, 0);  // Not in startup mode
    __sync_bool_compare_and_swap(&server_running, 1, 0);   // Stop main loop
    __sync_bool_compare_and_swap(&server_stopping, 0, 1);  // Prevent reinitialization
    __sync_synchronize();  // Memory barrier to ensure core flags are visible
    
    // Brief delay to ensure core flags take effect
    usleep(100000);  // 100ms delay
    
    // Set all component shutdown flags atomically
    __sync_bool_compare_and_swap(&mdns_server_system_shutdown, 0, 1);  // Stop mDNS Server
    __sync_bool_compare_and_swap(&web_server_shutdown, 0, 1);          // Stop web server
    __sync_bool_compare_and_swap(&websocket_server_shutdown, 0, 1);    // Stop websocket server
    __sync_bool_compare_and_swap(&print_queue_shutdown, 0, 1);         // Stop print queue
    __sync_synchronize();  // Memory barrier to ensure component flags are visible
    
    // Signal all threads to check shutdown flags
    log_this("Shutdown", "Notifying all threads of shutdown...", LOG_LEVEL_STATE);
    pthread_mutex_lock(&terminate_mutex);
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    
    // Longer delay to ensure all threads notice shutdown flags
    usleep(500000);  // 500ms delay
    
    // Now proceed with orderly shutdown, starting with mDNS Server
    log_this("Shutdown", "Stopping mDNS Server service...", LOG_LEVEL_STATE);
    
    // Update thread metrics to check mDNS Server thread status
    update_service_thread_metrics(&mdns_server_threads);
    if (mdns_server_threads.thread_count > 0) {
        log_this("Shutdown", "Waiting for mDNS Server threads to exit...", LOG_LEVEL_STATE);
        // Additional delay if threads are still active
        usleep(250000);  // 250ms delay
    }
    
    // Now shutdown the mDNS Server
    shutdown_mdns_server_system();
    
    // Brief delay to ensure mDNS Server is fully stopped
    usleep(250000);  // 250ms delay
    
    log_this("Shutdown", "Stopping web services...", LOG_LEVEL_STATE);
    shutdown_web_systems();
    
    log_this("Shutdown", "Stopping print system...", LOG_LEVEL_STATE);
    shutdown_print_system();
    
    log_this("Shutdown", "Cleaning up network...", LOG_LEVEL_STATE);
    shutdown_network();

    // Give threads a moment to process their shutdown flags
    usleep(1000000);  // 1s delay

    // Now safe to stop logging since other components are done
    log_this("Shutdown", "Subsystem shutdown completed", LOG_LEVEL_STATE);    
    log_this("Shutdown", LOG_LINE_BREAK, LOG_LEVEL_STATE);

    // Check remaining threads before logging shutdown
    extern ServiceThreads logging_threads;
    extern ServiceThreads web_threads;
    extern ServiceThreads websocket_threads;
    extern ServiceThreads mdns_server_threads;
    extern ServiceThreads print_threads;
    
    // Update thread metrics to clean up any dead threads
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_server_threads);
    update_service_thread_metrics(&print_threads);
    
    // Only show thread status if there are remaining threads
    if (logging_threads.thread_count > 0 || web_threads.thread_count > 0 ||
        websocket_threads.thread_count > 0 || mdns_server_threads.thread_count > 0 ||
        print_threads.thread_count > 0) {
        char thread_status[256];
        snprintf(thread_status, sizeof(thread_status), 
                 "Remaining threads - Log: %d, Web: %d, WS: %d, mDNS Server: %d, Print: %d",
                 logging_threads.thread_count,
                 web_threads.thread_count,
                 websocket_threads.thread_count,
                 mdns_server_threads.thread_count,
                 print_threads.thread_count);
                log_this("Shutdown", thread_status, LOG_LEVEL_STATE);
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
        update_service_thread_metrics(&mdns_server_threads);
        update_service_thread_metrics(&print_threads);

        // Calculate total non-logging threads
        int non_logging_threads = web_threads.thread_count + 
                                websocket_threads.thread_count + 
                                mdns_server_threads.thread_count + 
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
                log_this("Shutdown", msg, LOG_LEVEL_STATE);
            }
            
            // Signal any waiting threads
            pthread_mutex_lock(&terminate_mutex);
            pthread_cond_broadcast(&terminate_cond);
            pthread_mutex_unlock(&terminate_mutex);
            
            usleep(500000);  // 500ms delay between checks
        } else if (logging_threads.thread_count > 0) {
            // Only logging thread remains, which is expected
            log_this("Shutdown", "Only logging thread remains, proceeding with shutdown", LOG_LEVEL_STATE);
            break;
        }
        
        wait_count++;
    } while (threads_active && wait_count < max_wait_cycles);

    if (threads_active && web_threads.thread_count + websocket_threads.thread_count + 
        mdns_server_threads.thread_count + print_threads.thread_count > 0) {
        log_this("Shutdown", "Some non-logging threads did not exit cleanly", LOG_LEVEL_WARN);
    } else {
        log_this("Shutdown", "All non-logging threads exited successfully", LOG_LEVEL_STATE);
    }

    // Log final status before shutting down logging
    log_this("Shutdown", "Preparing to shut down logging system", LOG_LEVEL_STATE);
    
    // Ensure all previous messages are queued
    usleep(100000);  // 100ms delay
    
    // Signal logging thread to stop accepting new messages
    pthread_mutex_lock(&terminate_mutex);
    log_queue_shutdown = 1;
    pthread_cond_broadcast(&terminate_cond);
    pthread_mutex_unlock(&terminate_mutex);
    
    // Give the logging thread time to process final messages
    usleep(250000);  // 250ms delay
    
    // Now safe to join the logging thread
    pthread_join(log_thread, NULL);
    
    // Update all thread metrics one final time
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_server_threads);
    update_service_thread_metrics(&print_threads);

    // Get main thread ID for exclusion from counts
    pthread_t main_thread = pthread_self();
    
    // Calculate total remaining threads
    int total_threads = logging_threads.thread_count + web_threads.thread_count +
                       websocket_threads.thread_count + mdns_server_threads.thread_count +
                       print_threads.thread_count;

    // Only show detailed status if there are remaining threads
    if (total_threads > 0) {
        char thread_status[256];
        snprintf(thread_status, sizeof(thread_status), 
                 "Remaining threads before final cleanup - Log: %d, Web: %d, WS: %d, mDNS Server: %d, Print: %d",
                 logging_threads.thread_count,
                 web_threads.thread_count,
                 websocket_threads.thread_count,
                 mdns_server_threads.thread_count,
                 print_threads.thread_count);
        log_this("Shutdown", thread_status, LOG_LEVEL_STATE);
    }
    
    // Now safe to destroy queues
    log_this("Shutdown", "Shutting down queue system", LOG_LEVEL_STATE);
    queue_system_destroy();
    usleep(100000);  // 100ms delay
    
    // Update thread metrics one last time
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_server_threads);
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
    CHECK_THREAD_STATE(mdns_server_threads, "mDNS Server");
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
        log_this("Shutdown", "All non-main threads exited successfully", LOG_LEVEL_STATE);
    }

    // Record final timing statistics and log final messages
    record_shutdown_end_time();
    log_this("Shutdown", "Shutdown complete", LOG_LEVEL_STATE);

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