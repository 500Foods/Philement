// Global includes 
#include "../hydrogen.h"

// Local includes
#include "threads.h"
#include <sys/syscall.h>

// // Public interface declarations
// void init_service_threads(ServiceThreads *threads);
// void add_service_thread(ServiceThreads *threads, pthread_t thread_id);
// void remove_service_thread(ServiceThreads *threads, pthread_t thread_id);
// void update_service_thread_metrics(ServiceThreads *threads);
// ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id);

// // External declarations
// extern ServiceThreads logging_threads;
// extern ServiceThreads webserver_threads;
// extern ServiceThreads websocket_threads;
// extern ServiceThreads mdns_server_threads;
// extern ServiceThreads print_threads;

// Flag to indicate we're in final shutdown mode - no more thread management logging
// This will be set just before logging the final "Shutdown complete" message
volatile sig_atomic_t final_shutdown_mode = 0;

// Internal state
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

// Private function declarations
static size_t get_thread_stack_size(pid_t tid);

// Initialize service thread tracking
void init_service_threads(ServiceThreads *threads, const char* subsystem_name) {
    pthread_mutex_lock(&thread_mutex);
    threads->thread_count = 0;
    memset(threads->thread_ids, 0, sizeof(pthread_t) * MAX_SERVICE_THREADS);
    memset(threads->thread_metrics, 0, sizeof(ThreadMemoryMetrics) * MAX_SERVICE_THREADS);
    memset(threads->thread_tids, 0, sizeof(pid_t) * MAX_SERVICE_THREADS);

    if (subsystem_name) {
        strncpy(threads->subsystem, subsystem_name, 31); // Leave space for null terminator
        threads->subsystem[31] = '\0'; // Ensure null-termination
    } else {
        strncpy(threads->subsystem, "Unknown", 31);
        threads->subsystem[31] = '\0';
    }

    pthread_mutex_unlock(&thread_mutex);
}

// Add a thread to service tracking
void add_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    if (threads->thread_count < MAX_SERVICE_THREADS) {
        pid_t tid = (pid_t)syscall(SYS_gettid);
        threads->thread_ids[threads->thread_count] = thread_id;
        threads->thread_tids[threads->thread_count] = tid;
        threads->thread_count++;
        
        // Only log if not in final shutdown mode
        if (!final_shutdown_mode) {
            char msg[128];
            // Use log group to ensure consistent formatting
            log_group_begin();
            snprintf(msg, sizeof(msg), "%s: Thread %lu (tid: %d) added, count: %d", 
                     threads->subsystem, (unsigned long)thread_id, tid, threads->thread_count);
            log_this("Threads-Manager", msg, LOG_LEVEL_STATE);
            log_group_end();
        }
    } else {
        log_this("Threads-Manager", "Failed to add thread: MAX_SERVICE_THREADS reached", LOG_LEVEL_DEBUG);
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Remove a thread from service tracking
// If skip_logging is true, don't generate log messages (used during shutdown)
static void remove_thread_internal(ServiceThreads *threads, int index, bool skip_logging) {
    pthread_t thread_id = threads->thread_ids[index];

    // Move last thread to this position to maintain array compaction
    if (index < threads->thread_count - 1) {  // Only move if not removing the last element
        threads->thread_ids[index] = threads->thread_ids[threads->thread_count - 1];
        threads->thread_tids[index] = threads->thread_tids[threads->thread_count - 1];
        threads->thread_metrics[index] = threads->thread_metrics[threads->thread_count - 1];
    }

    // Always clear the last element (whether moved or not)
    int last_index = threads->thread_count - 1;
    threads->thread_ids[last_index] = 0;
    threads->thread_tids[last_index] = 0;
    threads->thread_metrics[last_index].virtual_bytes = 0;
    threads->thread_metrics[last_index].resident_bytes = 0;

    threads->thread_count--;

    // Only log if not skipping, not in final shutdown, and early in shutdown (when app_config still exists)
    if (!skip_logging && !final_shutdown_mode) {
        char msg[128];
        log_group_begin();
        snprintf(msg, sizeof(msg), "%s: Thread %lu removed, count: %d", 
                 threads->subsystem, (unsigned long)thread_id, threads->thread_count);
        log_this("Threads-Manager", msg, LOG_LEVEL_STATE);
        log_group_end();
    }
}

void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            remove_thread_internal(threads, i, false);
            break;
        }
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Get thread stack size from /proc/[tid]/status
static size_t get_thread_stack_size(pid_t tid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/self/task/%d/status", tid);
    
    FILE *status = fopen(path, "r");
    if (!status) return 0;
    
    char line[256];
    size_t stack_size = 0;
    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %20zu", &stack_size);
            break;
        }
    }
    
    fclose(status);
    return stack_size;
}

// Update memory metrics for all threads in a service
void update_service_thread_metrics(ServiceThreads *threads) {
    pthread_mutex_lock(&thread_mutex);
    
    // Reset service totals
    threads->virtual_memory = 0;
    threads->resident_memory = 0;
    
    // Update each thread's metrics using stack size only
    for (int i = 0; i < threads->thread_count; i++) {
        pid_t tid = threads->thread_tids[i];
        
        // Check if thread is alive
        if (kill(tid, 0) != 0) {
            // Thread is dead, remove it
            remove_thread_internal(threads, i, true);  // Skip logging during metrics update
            i--; // Reprocess this index
            continue;
        }
        
        // Get thread stack size
        size_t stack_size = get_thread_stack_size(tid);
        
        // Update thread metrics with stack size
        ThreadMemoryMetrics *metrics = &threads->thread_metrics[i];
        metrics->virtual_bytes = stack_size * 1024; // Convert KB to bytes
        metrics->resident_bytes = stack_size * 1024; // Assume stack is resident
        
        // Add to service totals
        threads->virtual_memory += metrics->virtual_bytes;
        threads->resident_memory += metrics->resident_bytes;
    }
    
    pthread_mutex_unlock(&thread_mutex);
}

// Get memory metrics for a specific thread
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id) {
    ThreadMemoryMetrics metrics = {0, 0};

    // Check for NULL threads parameter
    if (!threads) {
        return metrics;
    }

    pthread_mutex_lock(&thread_mutex);

    // Find the thread in our tracking array
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            metrics = threads->thread_metrics[i];
            break;
        }
    }

    pthread_mutex_unlock(&thread_mutex);

    return metrics;
}

// Report status of all service threads
void report_thread_status(void) {
    pthread_mutex_lock(&thread_mutex);
    
    log_this("Threads", "Thread Status Report:", LOG_LEVEL_STATE);
    
    // Report logging threads
    log_this("Threads", "  Logging Threads: %d active", LOG_LEVEL_STATE, 
             logging_threads.thread_count);
    
    // Report web threads
    log_this("Threads", "  Web Threads: %d active", LOG_LEVEL_STATE, 
             webserver_threads.thread_count);
    
    // Report websocket threads
    log_this("Threads", "  WebSocket Threads: %d active", LOG_LEVEL_STATE, 
             websocket_threads.thread_count);
    
    // Report mdns server threads
    log_this("Threads", "  mDNS Server Threads: %d active", LOG_LEVEL_STATE, 
             mdns_server_threads.thread_count);
    
    // Report print threads
    log_this("Threads", "  Print Threads: %d active", LOG_LEVEL_STATE, 
             print_threads.thread_count);
    
    // Calculate total threads
    int total_threads = logging_threads.thread_count +
                       webserver_threads.thread_count +
                       websocket_threads.thread_count +
                       mdns_server_threads.thread_count +
                       print_threads.thread_count;
    
    log_this("Threads", "Total Active Threads: %d", LOG_LEVEL_STATE, total_threads);
    
    pthread_mutex_unlock(&thread_mutex);
}

// Free thread-related resources
void free_threads_resources(void) {
    pthread_mutex_lock(&thread_mutex);
    
    // Set final shutdown mode to prevent excessive logging
    final_shutdown_mode = 1;
    
    // Clean up logging threads
    init_service_threads(&logging_threads, SR_LOGGING);
    
    // Clean up web threads
    init_service_threads(&webserver_threads, SR_WEBSERVER);
    
    // Clean up websocket threads
    init_service_threads(&websocket_threads, SR_WEBSOCKET);
    
    // Clean up mdns server threads
    init_service_threads(&mdns_server_threads, SR_MDNS_SERVER);
    
    // Clean up print threads
    init_service_threads(&print_threads, SR_PRINT);
    
    pthread_mutex_unlock(&thread_mutex);
}
