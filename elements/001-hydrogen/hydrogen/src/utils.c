/*
 * Core Utilities for High-Precision 3D Printing
 * 
 * Why Precision Matters:
 * 1. Print Quality
 *    - Layer alignment accuracy
 *    - Extrusion precision
 *    - Motion smoothness
 *    - Surface finish
 * 
 * 2. Hardware Safety
 *    Why These Checks?
 *    - Motor protection
 *    - Thermal management
 *    - Mechanical limits
 *    - Power monitoring
 * 
 * 3. Real-time Control
 *    Why This Design?
 *    - Immediate response
 *    - Timing accuracy
 *    - Command precision
 *    - State tracking
 * 
 * 4. Resource Management
 *    Why This Matters?
 *    - Memory efficiency
 *    - CPU optimization
 *    - Long-print stability
 *    - System reliability
 * 
 * 5. Error Prevention
 *    Why These Features?
 *    - Print failure avoidance
 *    - Quality assurance
 *    - Material waste reduction
 *    - Time savings
 * 
 * 6. Diagnostic Support
 *    Why These Tools?
 *    - Quality monitoring
 *    - Performance tracking
 *    - Maintenance prediction
 *    - Issue resolution
 * 
 * 7. Data Integrity
 *    Why So Critical?
 *    - Configuration accuracy
 *    - State consistency
 *    - Command validation
 *    - History tracking
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers first (for types and constants)
#include "utils.h"
#include "logging.h"
#include "configuration.h"
#include "state.h"

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// For reading /proc files
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/syscall.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

// Forward declarations of static functions
static void init_all_service_threads(void);
static void update_queue_memory_metrics(QueueMemoryMetrics *queue);

// Thread synchronization mutexes
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize service threads when module loads
static void __attribute__((constructor)) init_utils(void) {
    init_all_service_threads();
    init_queue_memory(&log_queue_memory);
    init_queue_memory(&print_queue_memory);
}

// Global tracking structures
ServiceThreads logging_threads;
ServiceThreads web_threads;
ServiceThreads websocket_threads;
ServiceThreads mdns_threads;
ServiceThreads print_threads;

// Queue memory tracking
QueueMemoryMetrics log_queue_memory;
QueueMemoryMetrics print_queue_memory;

// Initialize queue memory tracking
void init_queue_memory(QueueMemoryMetrics *queue) {
    queue->block_count = 0;
    queue->total_allocation = 0;
    queue->entry_count = 0;
    queue->metrics.virtual_bytes = 0;
    queue->metrics.resident_bytes = 0;
    memset(queue->block_sizes, 0, sizeof(size_t) * MAX_QUEUE_BLOCKS);
}

// Track memory allocation in a queue
void track_queue_allocation(QueueMemoryMetrics *queue, size_t size) {
    if (queue->block_count < MAX_QUEUE_BLOCKS) {
        queue->block_sizes[queue->block_count++] = size;
        queue->total_allocation += size;
        update_queue_memory_metrics(queue);
    }
}

// Track memory deallocation in a queue
void track_queue_deallocation(QueueMemoryMetrics *queue, size_t size) {
    for (size_t i = 0; i < queue->block_count; i++) {
        if (queue->block_sizes[i] == size) {
            // Remove this block by shifting others down
            for (size_t j = i; j < queue->block_count - 1; j++) {
                queue->block_sizes[j] = queue->block_sizes[j + 1];
            }
            queue->block_count--;
            queue->total_allocation -= size;
            update_queue_memory_metrics(queue);
            break;
        }
    }
}

// Update memory metrics for a queue
void update_queue_memory_metrics(QueueMemoryMetrics *queue) {
    // For now, use total_allocation as both virtual and resident
    // In the future, we could add more sophisticated tracking
    queue->metrics.virtual_bytes = queue->total_allocation;
    queue->metrics.resident_bytes = queue->total_allocation;
}

// Initialize service thread tracking
void init_service_threads(ServiceThreads *threads) {
    pthread_mutex_lock(&thread_mutex);
    threads->thread_count = 0;
    memset(threads->thread_metrics, 0, sizeof(ThreadMemoryMetrics) * MAX_SERVICE_THREADS);
    memset(threads->thread_tids, 0, sizeof(pid_t) * MAX_SERVICE_THREADS);
    pthread_mutex_unlock(&thread_mutex);
}

// Add a thread to service tracking
void add_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    if (threads->thread_count < MAX_SERVICE_THREADS) {
        pid_t tid = syscall(SYS_gettid);
        threads->thread_ids[threads->thread_count] = thread_id;
        threads->thread_tids[threads->thread_count] = tid;
        threads->thread_count++;
        char msg[128];
        snprintf(msg, sizeof(msg), "Added thread %lu (tid: %d), new count: %d", 
                 (unsigned long)thread_id, tid, threads->thread_count);
        console_log("ThreadMgmt", 1, msg);
    } else {
        console_log("ThreadMgmt", 3, "Failed to add thread: MAX_SERVICE_THREADS reached");
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Remove a thread from service tracking
void remove_service_thread(ServiceThreads *threads, pthread_t thread_id) {
    pthread_mutex_lock(&thread_mutex);
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            // Move last thread to this position
            threads->thread_count--;
            if (i < threads->thread_count) {
                threads->thread_ids[i] = threads->thread_ids[threads->thread_count];
                threads->thread_tids[i] = threads->thread_tids[threads->thread_count];
                threads->thread_metrics[i] = threads->thread_metrics[threads->thread_count];
            }
            char msg[128];
            snprintf(msg, sizeof(msg), "Removed thread %lu (tid: %d), new count: %d", 
                     (unsigned long)thread_id, threads->thread_tids[i], threads->thread_count);
            console_log("ThreadMgmt", 1, msg);
            break;
        }
    }
    pthread_mutex_unlock(&thread_mutex);
}

// Update memory metrics for all threads in a service
void update_service_thread_metrics(ServiceThreads *threads) {
    pthread_mutex_lock(&thread_mutex);
    
    // Reset service totals
    threads->virtual_memory = 0;
    threads->resident_memory = 0;
    
    // Update each thread's metrics and add to totals
    for (int i = 0; i < threads->thread_count; i++) {
        ThreadMemoryMetrics metrics = get_thread_memory_metrics(threads, threads->thread_ids[i]);
        threads->thread_metrics[i] = metrics;
        threads->virtual_memory += metrics.virtual_bytes;
        threads->resident_memory += metrics.resident_bytes;
    }
    
    pthread_mutex_unlock(&thread_mutex);
}

// Initialize all service thread tracking
static void init_all_service_threads(void) {
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_threads);
    init_service_threads(&print_threads);
}

// Get memory metrics for a specific thread
// Returns memory usage in bytes
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id) {
    ThreadMemoryMetrics metrics = {0, 0};
    char path[256];
    char msg[512];
    
    // Find the thread's TID in our tracking array
    pid_t tid = 0;
    int thread_index = -1;
    for (int i = 0; i < threads->thread_count; i++) {
        if (pthread_equal(threads->thread_ids[i], thread_id)) {
            tid = threads->thread_tids[i];
            thread_index = i;
            break;
        }
    }

    if (tid == 0 || thread_index == -1) {
        snprintf(msg, sizeof(msg), "Thread %lu not found in tracking array", (unsigned long)thread_id);
        console_log("MemoryMetrics", 3, msg);
        return metrics;
    }

    // Check if thread is still alive by sending signal 0
    if (kill(tid, 0) != 0) {
        snprintf(msg, sizeof(msg), "Thread %lu (tid: %d) appears to be terminated, removing from tracking", 
                 (unsigned long)thread_id, tid);
        console_log("MemoryMetrics", 2, msg);
        
        // Remove the terminated thread from tracking
        threads->thread_count--;
        if (thread_index < threads->thread_count) {
            threads->thread_ids[thread_index] = threads->thread_ids[threads->thread_count];
            threads->thread_tids[thread_index] = threads->thread_tids[threads->thread_count];
            threads->thread_metrics[thread_index] = threads->thread_metrics[threads->thread_count];
        }
        return metrics;
    }

    // Try reading from /proc/self/task/[tid]/smaps for accurate per-thread memory
    snprintf(path, sizeof(path), "/proc/self/task/%d/smaps", tid);
    snprintf(msg, sizeof(msg), "Attempting to read memory metrics from: %s for thread %lu (tid: %d)", 
             path, (unsigned long)thread_id, tid);
    console_log("MemoryMetrics", 0, msg);
    
    FILE *maps = fopen(path, "r");
    if (!maps) {
        // Try alternate path with process ID
        pid_t pid = getpid();
        snprintf(path, sizeof(path), "/proc/%d/task/%d/smaps", pid, tid);
        snprintf(msg, sizeof(msg), "Trying alternate path: %s for thread %lu (tid: %d)", 
                 path, (unsigned long)thread_id, tid);
        console_log("MemoryMetrics", 0, msg);
        maps = fopen(path, "r");
    }
    
    if (maps) {
        char line[1024];
        unsigned long start, end;
        char perms[5];
        
        // Track memory totals
        size_t total_private_clean = 0;
        size_t total_private_dirty = 0;
        size_t total_pss = 0;
        size_t total_writable_private = 0;
        bool in_mapping = false;
        
        while (fgets(line, sizeof(line), maps)) {
            if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
                // Only count writable private mappings for virtual memory
                if (perms[1] == 'w' && perms[3] == 'p') {
                    total_writable_private += (end - start);
                }
                in_mapping = true;
                continue;
            }
            
            if (in_mapping) {
                size_t value;
                if (sscanf(line, "Private_Clean: %zu", &value) == 1) {
                    total_private_clean += value;
                }
                else if (sscanf(line, "Private_Dirty: %zu", &value) == 1) {
                    total_private_dirty += value;
                }
                else if (sscanf(line, "Pss: %zu", &value) == 1) {
                    total_pss += value;
                }
                else if (strstr(line, "VmFlags:") != NULL) {
                    in_mapping = false;
                }
            }
        }
        
        // Set metrics based on actual thread memory
        metrics.virtual_bytes = total_writable_private;  // Only count writable private mappings
        metrics.resident_bytes = (total_private_clean + total_private_dirty) * 1024;
        
        char debug_msg[256];
        snprintf(debug_msg, sizeof(debug_msg), 
                "Thread %lu (tid: %d) - Private Clean: %zu kB, Private Dirty: %zu kB, PSS: %zu kB", 
                (unsigned long)thread_id, tid, total_private_clean, total_private_dirty, total_pss);
        console_log("MemoryMetrics", 0, debug_msg);
        
        snprintf(msg, sizeof(msg), 
                 "Thread %lu (tid: %d) metrics - Virtual: %zu bytes, Writable/Private: %zu bytes", 
                 (unsigned long)thread_id, tid, metrics.virtual_bytes, metrics.resident_bytes);
        console_log("MemoryMetrics", 0, msg);
        
        fclose(maps);
    } else {
        snprintf(msg, sizeof(msg), "Failed to open %s (errno: %d: %s) for thread %lu (tid: %d)", 
                 path, errno, strerror(errno), (unsigned long)thread_id, tid);
        console_log("MemoryMetrics", 3, msg);
    }
    
    return metrics;
}

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
// System status reporting with hierarchical organization
//
// Status generation strategy:
// 1. Data Organization
//    - Logical grouping
//    - Clear hierarchy
//    - Consistent structure
//    - Extensible format
//
// 2. Performance Optimization
//    - Memory preallocation
//    - Batched collection
//    - Efficient string handling
//    - Minimal allocations
//
// 3. Error Handling
//    - Partial data recovery
//    - Memory cleanup
//    - Error reporting
//    - Fallback values
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics) {
    pthread_mutex_lock(&status_mutex);
    
    json_t *root = json_object();
    
    // Version Information
    json_t *version = json_object();
    json_object_set_new(version, "server", json_string(VERSION));
    json_object_set_new(version, "api", json_string("1.0"));
    json_object_set_new(root, "version", version);

    // System Information
    struct utsname system_info;
    if (uname(&system_info) == 0) {
        json_t *system = json_object();
        json_object_set_new(system, "sysname", json_string(system_info.sysname));
        json_object_set_new(system, "nodename", json_string(system_info.nodename));
        json_object_set_new(system, "release", json_string(system_info.release));
        json_object_set_new(system, "version", json_string(system_info.version));
        json_object_set_new(system, "machine", json_string(system_info.machine));
        json_object_set_new(root, "system", system);
    }

    // Status Information with resource summary
    json_t *status = json_object();
    json_object_set_new(status, "running", json_boolean(keep_running));
    json_object_set_new(status, "shutting_down", json_boolean(shutting_down));
    char msg[256];
    
    // Get main process memory metrics using maps (like pmap does)
    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/self/maps");
    FILE *proc_maps = fopen(proc_path, "r");
    size_t process_virtual = 0, process_resident = 0;
    
    if (proc_maps) {
        char line[1024];
        unsigned long start, end;
        char perms[5];
        
        while (fgets(line, sizeof(line), proc_maps)) {
            // Parse map entry: address perms offset dev inode pathname
            if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
                size_t size = end - start;
                process_virtual += size;  // All mappings count towards virtual memory
                
                // Count as resident if writable and private (like pmap -d shows)
                if (perms[1] == 'w' && perms[3] == 'p') {
                    process_resident += size;
                }
            }
        }
        
        snprintf(msg, sizeof(msg), "Process memory - Virtual: %zu bytes, Writable/Private: %zu bytes",
                 process_virtual, process_resident);
        console_log("SystemStatus", 0, msg);
        
        fclose(proc_maps);
    }

    // Count total threads
    size_t total_threads = logging_threads.thread_count + 
                          web_threads.thread_count + 
                          websocket_threads.thread_count + 
                          mdns_threads.thread_count + 
                          print_threads.thread_count;

    // Update memory metrics for each service using actual measurements
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);

    // Log thread and memory distribution
    snprintf(msg, sizeof(msg), "Thread distribution - logging: %d, web: %d, websocket: %d, mdns: %d, print: %d, total: %zu",
             logging_threads.thread_count, web_threads.thread_count, websocket_threads.thread_count,
             mdns_threads.thread_count, print_threads.thread_count, total_threads);
    console_log("SystemStatus", 0, msg);

    snprintf(msg, sizeof(msg), "Process memory - Total Virtual: %zu bytes, Total Resident: %zu bytes",
             process_virtual, process_resident);
    console_log("SystemStatus", 0, msg);

    // Calculate queue memory including base structures, state, and allocations
    const size_t QUEUE_BASE_SIZE = sizeof(QueueMemoryMetrics) + 4096;  // Base structure + state overhead
    size_t queue_virtual_total = QUEUE_BASE_SIZE * 2;  // Two queues
    size_t queue_resident_total = QUEUE_BASE_SIZE * 2;
    
    // Add actual queue allocations
    queue_virtual_total += log_queue_memory.total_allocation + 
                          print_queue_memory.total_allocation;
    queue_resident_total += log_queue_memory.total_allocation + 
                           print_queue_memory.total_allocation;

    // Update queue metrics to include base structure and state overhead
    log_queue_memory.metrics.virtual_bytes = QUEUE_BASE_SIZE + log_queue_memory.total_allocation;
    log_queue_memory.metrics.resident_bytes = QUEUE_BASE_SIZE + log_queue_memory.total_allocation;
    print_queue_memory.metrics.virtual_bytes = QUEUE_BASE_SIZE + print_queue_memory.total_allocation;
    print_queue_memory.metrics.resident_bytes = QUEUE_BASE_SIZE + print_queue_memory.total_allocation;

    // Update memory metrics for each service to get latest measurements
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);

    // Use the actual measured values from each service's threads
    size_t service_virtual_total = logging_threads.virtual_memory +
                                  web_threads.virtual_memory +
                                  websocket_threads.virtual_memory +
                                  mdns_threads.virtual_memory +
                                  print_threads.virtual_memory;

    size_t service_resident_total = logging_threads.resident_memory +
                                   web_threads.resident_memory +
                                   websocket_threads.resident_memory +
                                   mdns_threads.resident_memory +
                                   print_threads.resident_memory;

    // Ensure service totals don't exceed process totals
    if (service_virtual_total > process_virtual) {
        service_virtual_total = process_virtual;
    }
    if (service_resident_total > process_resident) {
        service_resident_total = process_resident;
    }

    // Calculate service percentages based on actual measurements
    if (process_resident > 0) {
        logging_threads.memory_percent = round((double)logging_threads.resident_memory / process_resident * 100000.0) / 1000.0;
        web_threads.memory_percent = round((double)web_threads.resident_memory / process_resident * 100000.0) / 1000.0;
        websocket_threads.memory_percent = round((double)websocket_threads.resident_memory / process_resident * 100000.0) / 1000.0;
        mdns_threads.memory_percent = round((double)mdns_threads.resident_memory / process_resident * 100000.0) / 1000.0;
        print_threads.memory_percent = round((double)print_threads.resident_memory / process_resident * 100000.0) / 1000.0;
    }

    // Add process totals to status object
    json_object_set_new(status, "totalThreads", json_integer((int)total_threads + 1));  // +1 for main thread
    json_object_set_new(status, "totalVirtualMemoryBytes", json_integer(process_virtual));
    json_object_set_new(status, "totalResidentMemoryBytes", json_integer(process_resident));

    // Create and populate resources object with memory measurements
    json_t *resources = json_object();
    
    // Calculate total queue entries
    size_t total_queue_entries = log_queue_memory.entry_count + print_queue_memory.entry_count;
    
    // Service resources
    json_t *service_resources = json_object();
    json_object_set_new(service_resources, "threads", json_integer((int)total_threads));
    json_object_set_new(service_resources, "virtualMemoryBytes", json_integer(service_virtual_total));
    json_object_set_new(service_resources, "residentMemoryBytes", json_integer(service_resident_total));
    
    // Queue resources
    json_t *queue_resources = json_object();
    json_object_set_new(queue_resources, "entries", json_integer(total_queue_entries));
    json_object_set_new(queue_resources, "virtualMemoryBytes", json_integer(queue_virtual_total));
    json_object_set_new(queue_resources, "residentMemoryBytes", json_integer(queue_resident_total));
    
    // Calculate percentages ensuring they sum to 100%
    double service_percent = 0.0, queue_percent = 0.0, other_percent = 0.0;
    
    if (process_resident > 0) {
        service_percent = round((double)service_resident_total / process_resident * 100000.0) / 1000.0;
        queue_percent = round((double)queue_resident_total / process_resident * 100000.0) / 1000.0;
        other_percent = 100.0 - service_percent - queue_percent;
        
        // Ensure percentages are non-negative and sum to 100%
        if (other_percent < 0.0) {
            other_percent = 0.0;
            // Adjust service_percent to account for rounding
            service_percent = 100.0 - queue_percent;
        }
    }
    
    // Add percentages to resources objects
    json_object_set_new(service_resources, "allocationPercent", json_real(service_percent));
    json_object_set_new(resources, "serviceResources", service_resources);
    
    json_object_set_new(queue_resources, "allocationPercent", json_real(queue_percent));
    json_object_set_new(resources, "queueResources", queue_resources);
    
    // Other resources (main thread and shared libraries)
    json_t *other_resources = json_object();
    // Calculate other memory as remaining after services and queues
    size_t other_virtual = (process_virtual > service_virtual_total + queue_virtual_total) ?
                          (process_virtual - service_virtual_total - queue_virtual_total) : 0;
    size_t other_resident = (process_resident > service_resident_total + queue_resident_total) ?
                           (process_resident - service_resident_total - queue_resident_total) : 0;
    json_object_set_new(other_resources, "threads", json_integer(1));  // Main thread
    json_object_set_new(other_resources, "virtualMemoryBytes", json_integer(other_virtual));
    json_object_set_new(other_resources, "residentMemoryBytes", json_integer(other_resident));
    
    // Ensure other_percent is not negative
    if (other_percent < 0.0) {
        other_percent = 0.0;
        // Adjust service_percent to account for rounding
        service_percent = 100.0 - queue_percent;
    }
    
    json_object_set_new(other_resources, "allocationPercent", json_real(other_percent));
    json_object_set_new(resources, "otherResources", other_resources);
    
    // Add resources object to root
    json_object_set_new(root, "status", status);
    json_object_set_new(status, "resources", resources);

    // Add queue information
    json_t *queues = json_object();
    
    // Log queue
    json_t *log_queue = json_object();
    json_object_set_new(log_queue, "entryCount", json_integer(log_queue_memory.entry_count));
    json_object_set_new(log_queue, "blockCount", json_integer(log_queue_memory.block_count));
    json_object_set_new(log_queue, "totalAllocation", json_integer(log_queue_memory.total_allocation));
    json_object_set_new(log_queue, "virtualMemoryBytes", json_integer(log_queue_memory.metrics.virtual_bytes));
    json_object_set_new(log_queue, "residentMemoryBytes", json_integer(log_queue_memory.metrics.resident_bytes));
    json_object_set_new(queues, "log", log_queue);
    
    // Print queue
    json_t *print_queue = json_object();
    json_object_set_new(print_queue, "entryCount", json_integer(print_queue_memory.entry_count));
    json_object_set_new(print_queue, "blockCount", json_integer(print_queue_memory.block_count));
    json_object_set_new(print_queue, "totalAllocation", json_integer(print_queue_memory.total_allocation));
    json_object_set_new(print_queue, "virtualMemoryBytes", json_integer(print_queue_memory.metrics.virtual_bytes));
    json_object_set_new(print_queue, "residentMemoryBytes", json_integer(print_queue_memory.metrics.resident_bytes));
    json_object_set_new(queues, "print", print_queue);
    
    json_object_set_new(root, "queues", queues);

    // List of enabled services
    json_t *enabled_services = json_array();
    // Logging is always enabled
    json_array_append_new(enabled_services, json_string("logging"));
    if (app_config->web.enabled)
        json_array_append_new(enabled_services, json_string("web"));
    if (app_config->websocket.enabled)
        json_array_append_new(enabled_services, json_string("websocket"));
    if (app_config->mdns.enabled)
        json_array_append_new(enabled_services, json_string("mdns"));
    if (app_config->print_queue.enabled)
        json_array_append_new(enabled_services, json_string("print"));
    json_object_set_new(root, "enabledServices", enabled_services);

    // All service configurations
    json_t *services = json_object();
    
    // Logging service (always enabled)
    json_t *logging = json_object();
    // Configuration
    json_object_set_new(logging, "enabled", json_true());
    json_object_set_new(logging, "log_file", json_string(app_config->log_file_path));

    // Status with thread and memory metrics
    json_t *logging_status = json_object();
    json_object_set_new(logging_status, "messageCount", json_integer(0));  // Placeholder for message count
    json_object_set_new(logging_status, "threads", json_integer(logging_threads.thread_count));
    
    // Use measured memory values from smaps
    json_object_set_new(logging_status, "virtualMemoryBytes", json_integer(logging_threads.virtual_memory));
    json_object_set_new(logging_status, "residentMemoryBytes", json_integer(logging_threads.resident_memory));
    json_object_set_new(logging, "status", logging_status);
    json_object_set_new(services, "logging", logging);
    
    // Web server configuration
    json_t *web = json_object();
    // Configuration
    json_object_set_new(web, "enabled", json_boolean(app_config->web.enabled));
    json_object_set_new(web, "port", json_integer(app_config->web.port));
    json_object_set_new(web, "upload_path", json_string(app_config->web.upload_path));
    json_object_set_new(web, "max_upload_size", json_integer(app_config->web.max_upload_size));
    json_object_set_new(web, "log_level", json_string(app_config->web.log_level));
    // Status with thread and memory metrics
    json_t *web_status = json_object();
    json_object_set_new(web_status, "activeRequests", json_integer(0));   // Placeholder for active requests
    json_object_set_new(web_status, "totalRequests", json_integer(0));    // Placeholder for total requests
    json_object_set_new(web_status, "threads", json_integer(web_threads.thread_count));
    
    // Use measured memory values from smaps
    json_object_set_new(web_status, "virtualMemoryBytes", json_integer(web_threads.virtual_memory));
    json_object_set_new(web_status, "residentMemoryBytes", json_integer(web_threads.resident_memory));
    json_object_set_new(web, "status", web_status);
    json_object_set_new(services, "web", web);
    
    // WebSocket configuration
    json_t *websocket = json_object();
    // Configuration
    json_object_set_new(websocket, "enabled", json_boolean(app_config->websocket.enabled));
    json_object_set_new(websocket, "port", json_integer(app_config->websocket.port));
    json_object_set_new(websocket, "protocol", json_string(app_config->websocket.protocol));
    json_object_set_new(websocket, "max_message_size", json_integer(app_config->websocket.max_message_size));
    json_object_set_new(websocket, "log_level", json_string(app_config->websocket.log_level));
    
    // WebSocket status with thread and memory info
    json_t *websocket_status = json_object();
    json_object_set_new(websocket_status, "threads", json_integer(websocket_threads.thread_count));
    
    // Add WebSocket metrics directly to status
    if (ws_metrics != NULL) {
        json_object_set_new(websocket_status, "uptime", 
            json_integer(time(NULL) - ws_metrics->server_start_time));
        json_object_set_new(websocket_status, "activeConnections", 
            json_integer(ws_metrics->active_connections));
        json_object_set_new(websocket_status, "totalConnections", 
            json_integer(ws_metrics->total_connections));
        json_object_set_new(websocket_status, "totalRequests", 
            json_integer(ws_metrics->total_requests));
    }
    
    // Use measured memory values from smaps
    json_object_set_new(websocket_status, "virtualMemoryBytes", json_integer(websocket_threads.virtual_memory));
    json_object_set_new(websocket_status, "residentMemoryBytes", json_integer(websocket_threads.resident_memory));
    json_object_set_new(websocket, "status", websocket_status);
    json_object_set_new(services, "websocket", websocket);
    
    // mDNS configuration
    json_t *mdns = json_object();
    // Configuration
    json_object_set_new(mdns, "enabled", json_boolean(app_config->mdns.enabled));
    json_object_set_new(mdns, "device_id", json_string(app_config->mdns.device_id));
    json_object_set_new(mdns, "friendly_name", json_string(app_config->mdns.friendly_name));
    json_object_set_new(mdns, "model", json_string(app_config->mdns.model));
    json_object_set_new(mdns, "manufacturer", json_string(app_config->mdns.manufacturer));
    json_object_set_new(mdns, "log_level", json_string(app_config->mdns.log_level));
    // Status with thread and memory metrics
    json_t *mdns_status = json_object();
    json_object_set_new(mdns_status, "discoveryCount", json_integer(0));  // Placeholder for discovery count
    json_object_set_new(mdns_status, "threads", json_integer(mdns_threads.thread_count));
    
    // Use measured memory values from smaps
    json_object_set_new(mdns_status, "virtualMemoryBytes", json_integer(mdns_threads.virtual_memory));
    json_object_set_new(mdns_status, "residentMemoryBytes", json_integer(mdns_threads.resident_memory));
    json_object_set_new(mdns, "status", mdns_status);
    json_object_set_new(services, "mdns", mdns);
    
    // Print queue configuration
    json_t *print = json_object();
    // Configuration
    json_object_set_new(print, "enabled", json_boolean(app_config->print_queue.enabled));
    json_object_set_new(print, "log_level", json_string(app_config->print_queue.log_level));
    // Status with thread and memory metrics
    json_t *print_status = json_object();
    json_object_set_new(print_status, "queuedJobs", json_integer(0));     // Placeholder for queued jobs
    json_object_set_new(print_status, "completedJobs", json_integer(0));  // Placeholder for completed jobs
    json_object_set_new(print_status, "threads", json_integer(print_threads.thread_count));
    
    // Use measured memory values from smaps
    json_object_set_new(print_status, "virtualMemoryBytes", json_integer(print_threads.virtual_memory));
    json_object_set_new(print_status, "residentMemoryBytes", json_integer(print_threads.resident_memory));
    json_object_set_new(print, "status", print_status);
    json_object_set_new(services, "print", print);
    
    json_object_set_new(root, "services", services);

    pthread_mutex_unlock(&status_mutex);
    return root;
}

// Track when an entry is added to a queue
void track_queue_entry_added(QueueMemoryMetrics *queue) {
    queue->entry_count++;
}

// Track when an entry is removed from a queue
void track_queue_entry_removed(QueueMemoryMetrics *queue) {
    if (queue->entry_count > 0) {
        queue->entry_count--;
    }
}
// Thread-safe identifier generation with collision resistance
//
// ID generation design prioritizes:
// 1. Uniqueness
//    - Time-based seeding
//    - Character space optimization
//    - Length considerations
//    - Entropy maximization
//
// 2. Safety
//    - Buffer overflow prevention
//    - Thread-safe initialization
//    - Input validation
//    - Error reporting
//
// 3. Usability
//    - Human-readable format
//    - Consistent length
//    - Easy validation
//    - Simple comparison
void generate_id(char *buf, size_t len) {
    if (len < ID_LEN + 1) {
        log_this("Utils", "Buffer too small for ID", 3, true, true, true);
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    size_t id_chars_len = strlen(ID_CHARS);
    for (int i = 0; i < ID_LEN; i++) {
        buf[i] = ID_CHARS[rand() % id_chars_len];
    }
    buf[ID_LEN] = '\0';
}

// Format and output a log message directly to console
// Matches the format of the logging queue system for consistency
//
// Format components:
// 1. Timestamp with ms precision
// 2. Priority label with padding
// 3. Subsystem label with padding
// 4. Message content
//
// Example output:
// 2025-02-16 14:24:10.065  [ INFO      ]  [ Shutdown           ]  message
void console_log(const char* subsystem, int priority, const char* message) {
    // Get current time with millisecond precision
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    // Format timestamp
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    char timestamp_ms[36];
    snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

    // Format priority and subsystem labels with proper padding
    char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
    snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

    char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
    snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

    // Output the formatted log entry
    printf("%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, message);
}

