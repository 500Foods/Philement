// Core Utilities for High-Precision 3D Printing
// 
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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// For reading /proc files and file descriptors
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
        queue->metrics.virtual_bytes = queue->total_allocation;
        queue->metrics.resident_bytes = queue->total_allocation;
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
            queue->metrics.virtual_bytes = queue->total_allocation;
            queue->metrics.resident_bytes = queue->total_allocation;
            break;
        }
    }
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
            snprintf(msg, sizeof(msg), "Removed thread %lu, new count: %d", 
                     (unsigned long)thread_id, threads->thread_count);
            console_log("ThreadMgmt", 1, msg);
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
            sscanf(line, "VmStk: %zu", &stack_size);
            break;
        }
    }
    
    fclose(status);
    return stack_size;
}

// Get overall process memory from /proc/self/status
static void get_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap) {
    *vmsize = 0;
    *vmrss = 0;
    *vmswap = 0;
    
    FILE *status = fopen("/proc/self/status", "r");
    if (!status) {
        console_log("MemoryMetrics", 3, "Failed to open /proc/self/status");
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %zu", vmsize);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %zu", vmrss);
        } else if (strncmp(line, "VmSwap:", 7) == 0) {
            sscanf(line, "VmSwap: %zu", vmswap);
        }
    }
    
    fclose(status);
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
            threads->thread_count--;
            if (i < threads->thread_count) {
                threads->thread_ids[i] = threads->thread_ids[threads->thread_count];
                threads->thread_tids[i] = threads->thread_tids[threads->thread_count];
                threads->thread_metrics[i] = threads->thread_metrics[threads->thread_count];
            }
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

// Initialize all service thread tracking
static void init_all_service_threads(void) {
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_threads);
    init_service_threads(&print_threads);
}

// Get memory metrics for a specific thread
ThreadMemoryMetrics get_thread_memory_metrics(ServiceThreads *threads, pthread_t thread_id) {
    ThreadMemoryMetrics metrics = {0, 0};
    
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

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;

// Helper function to add thread IDs to a service status object
static void add_thread_ids_to_service(json_t *service_status, ServiceThreads *threads) {
    // Create thread IDs array and add to service status
    json_t *tid_array = json_array();
    for (int i = 0; i < threads->thread_count; i++) {
        json_array_append_new(tid_array, json_integer(threads->thread_tids[i]));
    }
    json_object_set_new(service_status, "threadIds", tid_array);
}

// Helper function to get socket information from /proc/net files
static void get_socket_info(int inode, char *proto, int *port) {
    char path[64];
    const char *net_files[] = {
        "tcp", "tcp6", "udp", "udp6"
    };
    
    *port = 0;
    proto[0] = '\0';
    
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "/proc/net/%s", net_files[i]);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        
        char line[512];
        fgets(line, sizeof(line), f); // Skip header
        
        while (fgets(line, sizeof(line), f)) {
            unsigned local_port;
            unsigned socket_inode;
            if (sscanf(line, "%*x: %*x:%x %*x:%*x %*x %*x:%*x %*x:%*x %*x %*d %*d %u",
                      &local_port, &socket_inode) == 2) {
                if (socket_inode == (unsigned)inode) {
                    *port = local_port;
                    strncpy(proto, net_files[i], 31);
                    proto[31] = '\0';
                    fclose(f);
                    return;
                }
            }
        }
        fclose(f);
    }
}

// Helper function to get file descriptor type and description
static void get_fd_info(int fd, FileDescriptorInfo *info) {
    char path[64], target[256];
    struct stat st;
    
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    ssize_t len = readlink(path, target, sizeof(target) - 1);
    if (len < 0) {
        snprintf(info->type, sizeof(info->type), "unknown");
        snprintf(info->description, sizeof(info->description), "error reading link");
        return;
    }
    target[len] = '\0';
    
    if (fstat(fd, &st) < 0) {
        snprintf(info->type, sizeof(info->type), "error");
        snprintf(info->description, sizeof(info->description), "fstat failed");
        return;
    }
    
    // Handle standard streams
    if (fd <= 2) {
        snprintf(info->type, sizeof(info->type), "stdio");
        const char *stream_name = (fd == 0) ? "stdin" : (fd == 1) ? "stdout" : "stderr";
        snprintf(info->description, sizeof(info->description), "%s: terminal", stream_name);
        return;
    }
    
    // Handle socket
    if (S_ISSOCK(st.st_mode)) {
        char proto[32];
        int port;
        get_socket_info(st.st_ino, proto, &port);
        
        snprintf(info->type, sizeof(info->type), "socket");
        if (port > 0) {
            const char *service = "";
            if (port == 5000) service = "web server";
            else if (port == 5001 || port == 5002) service = "websocket server";
            else if (port == 5353) service = "mDNS";
            
            if (service[0]) {
                snprintf(info->description, sizeof(info->description), 
                        "socket (%s port %d - %s)", proto, port, service);
            } else {
                snprintf(info->description, sizeof(info->description), 
                        "socket (%s port %d)", proto, port);
            }
        } else if (strstr(target, "socket:[")) {
            struct sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            if (getsockname(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
                if (addr.ss_family == AF_UNIX) {
                    struct sockaddr_un *un = (struct sockaddr_un*)&addr;
                    if (un->sun_path[0])
                        snprintf(info->description, sizeof(info->description), 
                                "Unix domain socket: %s", un->sun_path);
                    else
                        snprintf(info->description, sizeof(info->description), 
                                "Unix domain socket: *");
                } else {
                    snprintf(info->description, sizeof(info->description), 
                            "socket (inode: %lu)", st.st_ino);
                }
            } else {
                snprintf(info->description, sizeof(info->description), 
                        "socket (inode: %lu)", st.st_ino);
            }
        }
        return;
    }
    
    // Handle anonymous inodes
    if (strncmp(target, "anon_inode:", 11) == 0) {
        snprintf(info->type, sizeof(info->type), "anon_inode");
        const char *anon_type = target + 11;
        if (strcmp(anon_type, "[eventfd]") == 0)
            snprintf(info->description, sizeof(info->description), "event notification channel");
        else if (strcmp(anon_type, "[eventpoll]") == 0)
            snprintf(info->description, sizeof(info->description), "epoll instance");
        else if (strcmp(anon_type, "[timerfd]") == 0)
            snprintf(info->description, sizeof(info->description), "timer notification");
        else {
            // Ensure we have room for "anonymous inode: " (17 chars) plus null terminator
            char truncated_type[sizeof(info->description) - 17];
            strncpy(truncated_type, anon_type, sizeof(truncated_type) - 1);
            truncated_type[sizeof(truncated_type) - 1] = '\0';
            snprintf(info->description, sizeof(info->description), "anonymous inode: %s", truncated_type);
        }
        return;
    }
    
    // Handle regular files and other types
    if (S_ISREG(st.st_mode)) {
        snprintf(info->type, sizeof(info->type), "file");
        // Ensure we have room for "file: " (6 chars) plus null terminator
        char truncated_path[sizeof(info->description) - 6];
        strncpy(truncated_path, target, sizeof(truncated_path) - 1);
        truncated_path[sizeof(truncated_path) - 1] = '\0';
        snprintf(info->description, sizeof(info->description), "file: %s", truncated_path);
    } else if (strncmp(target, "/dev/", 5) == 0) {
        snprintf(info->type, sizeof(info->type), "device");
        if (strcmp(target, "/dev/urandom") == 0)
            snprintf(info->description, sizeof(info->description), "random number source");
        else {
            strncpy(info->description, target, sizeof(info->description) - 1);
            info->description[sizeof(info->description) - 1] = '\0';
        }
    } else {
        snprintf(info->type, sizeof(info->type), "other");
        strncpy(info->description, target, sizeof(info->description) - 1);
        info->description[sizeof(info->description) - 1] = '\0';
    }
}

// Get file descriptor information as JSON
json_t* get_file_descriptors_json(void) {
    DIR *dir;
    struct dirent *ent;
    json_t *fd_array = json_array();
    
    dir = opendir("/proc/self/fd");
    if (!dir) {
        console_log("Utils", 3, "Failed to open /proc/self/fd");
        return fd_array;
    }
    
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        
        int fd = atoi(ent->d_name);
        FileDescriptorInfo info;
        get_fd_info(fd, &info);
        
        json_t *fd_obj = json_object();
        json_object_set_new(fd_obj, "fd", json_integer(info.fd));
        json_object_set_new(fd_obj, "type", json_string(info.type));
        json_object_set_new(fd_obj, "description", json_string(info.description));
        json_array_append_new(fd_array, fd_obj);
    }
    
    closedir(dir);
    return fd_array;
}

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
    
    // Get process memory metrics using /proc/self/status
    size_t process_virtual = 0, process_resident = 0, process_swap = 0;
    get_process_memory(&process_virtual, &process_resident, &process_swap);
    
    // Count total threads
    size_t total_threads = logging_threads.thread_count + 
                          web_threads.thread_count + 
                          websocket_threads.thread_count + 
                          mdns_threads.thread_count + 
                          print_threads.thread_count;
    
    // Update memory metrics for each service
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_threads);
    update_service_thread_metrics(&print_threads);
    
    // Calculate service memory totals (just stacks)
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
    
    // Calculate queue memory
    size_t queue_virtual_total = log_queue_memory.metrics.virtual_bytes + 
                               print_queue_memory.metrics.virtual_bytes;
    size_t queue_resident_total = log_queue_memory.metrics.resident_bytes + 
                                print_queue_memory.metrics.resident_bytes;
    
    // Add process totals to status object (in bytes)
    json_object_set_new(status, "totalThreads", json_integer((int)total_threads + 1));  // +1 for main thread
    json_object_set_new(status, "totalVirtualMemoryBytes", json_integer(process_virtual * 1024));
    json_object_set_new(status, "totalResidentMemoryBytes", json_integer(process_resident * 1024));
    
    // Other memory is everything not accounted for in services or queues
    size_t other_virtual = (process_virtual * 1024) > service_virtual_total + queue_virtual_total ? 
                         (process_virtual * 1024) - service_virtual_total - queue_virtual_total : 0;
    size_t other_resident = (process_resident * 1024) > service_resident_total + queue_resident_total ? 
                          (process_resident * 1024) - service_resident_total - queue_resident_total : 0;
    
    // Calculate percentages with rounding to 3 decimal places
    double service_percent = process_resident > 0 ? 
                          round((double)service_resident_total / (process_resident * 1024) * 100000.0) / 1000.0 : 0.0;
    double queue_percent = process_resident > 0 ? 
                         round((double)queue_resident_total / (process_resident * 1024) * 100000.0) / 1000.0 : 0.0;
    double other_percent = round((100.0 - service_percent - queue_percent) * 1000.0) / 1000.0;
    
    // Format percentages as strings with exactly 3 decimal places
    char service_percent_str[16], queue_percent_str[16], other_percent_str[16];
    snprintf(service_percent_str, sizeof(service_percent_str), "%.3f", service_percent);
    snprintf(queue_percent_str, sizeof(queue_percent_str), "%.3f", queue_percent);
    snprintf(other_percent_str, sizeof(other_percent_str), "%.3f", other_percent);
    
    // Create resources object
    json_t *resources = json_object();
    
    // Service resources
    json_t *service_resources = json_object();
    json_object_set_new(service_resources, "threads", json_integer((int)total_threads));
    json_object_set_new(service_resources, "virtualMemoryBytes", json_integer(service_virtual_total));
    json_object_set_new(service_resources, "residentMemoryBytes", json_integer(service_resident_total));
    json_object_set_new(service_resources, "allocationPercent", json_string(service_percent_str));
    json_object_set_new(resources, "serviceResources", service_resources);
    
    // Queue resources
    json_t *queue_resources = json_object();
    json_object_set_new(queue_resources, "entries", json_integer(log_queue_memory.entry_count + print_queue_memory.entry_count));
    json_object_set_new(queue_resources, "virtualMemoryBytes", json_integer(queue_virtual_total));
    json_object_set_new(queue_resources, "residentMemoryBytes", json_integer(queue_resident_total));
    json_object_set_new(queue_resources, "allocationPercent", json_string(queue_percent_str));
    json_object_set_new(resources, "queueResources", queue_resources);
    
    // Other resources (main thread and shared libraries)
    json_t *other_resources = json_object();
    json_object_set_new(other_resources, "threads", json_integer(1));  // Main thread
    json_object_set_new(other_resources, "virtualMemoryBytes", json_integer(other_virtual));
    json_object_set_new(other_resources, "residentMemoryBytes", json_integer(other_resident));
    json_object_set_new(other_resources, "allocationPercent", json_string(other_percent_str));
    json_object_set_new(resources, "otherResources", other_resources);
    
    json_object_set_new(status, "resources", resources);
    // Add file descriptors to status
    json_t *files = get_file_descriptors_json();
    json_object_set_new(status, "files", files);
    
    json_object_set_new(root, "status", status);
    
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
    json_object_set_new(logging, "enabled", json_true());
    json_object_set_new(logging, "log_file", json_string(app_config->log_file_path));
    
    // Status with thread and memory metrics
    json_t *logging_status = json_object();
    json_object_set_new(logging_status, "messageCount", json_integer(0));  // Placeholder
    json_object_set_new(logging_status, "threads", json_integer(logging_threads.thread_count));
    json_object_set_new(logging_status, "virtualMemoryBytes", json_integer(logging_threads.virtual_memory));
    json_object_set_new(logging_status, "residentMemoryBytes", json_integer(logging_threads.resident_memory));
    add_thread_ids_to_service(logging_status, &logging_threads);
    json_object_set_new(logging, "status", logging_status);
    json_object_set_new(services, "logging", logging);
    
    // Web server configuration
    json_t *web = json_object();
    json_object_set_new(web, "enabled", json_boolean(app_config->web.enabled));
    json_object_set_new(web, "port", json_integer(app_config->web.port));
    json_object_set_new(web, "upload_path", json_string(app_config->web.upload_path));
    json_object_set_new(web, "max_upload_size", json_integer(app_config->web.max_upload_size));
    json_object_set_new(web, "log_level", json_string(app_config->web.log_level));
    
    json_t *web_status = json_object();
    json_object_set_new(web_status, "activeRequests", json_integer(0));   // Placeholder
    json_object_set_new(web_status, "totalRequests", json_integer(0));    // Placeholder
    json_object_set_new(web_status, "threads", json_integer(web_threads.thread_count));
    json_object_set_new(web_status, "virtualMemoryBytes", json_integer(web_threads.virtual_memory));
    json_object_set_new(web_status, "residentMemoryBytes", json_integer(web_threads.resident_memory));
    add_thread_ids_to_service(web_status, &web_threads);
    json_object_set_new(web, "status", web_status);
    json_object_set_new(services, "web", web);
    
    // WebSocket configuration
    json_t *websocket = json_object();
    json_object_set_new(websocket, "enabled", json_boolean(app_config->websocket.enabled));
    json_object_set_new(websocket, "port", json_integer(app_config->websocket.port));
    json_object_set_new(websocket, "protocol", json_string(app_config->websocket.protocol));
    json_object_set_new(websocket, "max_message_size", json_integer(app_config->websocket.max_message_size));
    json_object_set_new(websocket, "log_level", json_string(app_config->websocket.log_level));
    
    json_t *websocket_status = json_object();
    json_object_set_new(websocket_status, "threads", json_integer(websocket_threads.thread_count));
    
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
    
    json_object_set_new(websocket_status, "virtualMemoryBytes", json_integer(websocket_threads.virtual_memory));
    json_object_set_new(websocket_status, "residentMemoryBytes", json_integer(websocket_threads.resident_memory));
    add_thread_ids_to_service(websocket_status, &websocket_threads);
    json_object_set_new(websocket, "status", websocket_status);
    json_object_set_new(services, "websocket", websocket);
    
    // mDNS configuration
    json_t *mdns = json_object();
    json_object_set_new(mdns, "enabled", json_boolean(app_config->mdns.enabled));
    json_object_set_new(mdns, "device_id", json_string(app_config->mdns.device_id));
    json_object_set_new(mdns, "friendly_name", json_string(app_config->mdns.friendly_name));
    json_object_set_new(mdns, "model", json_string(app_config->mdns.model));
    json_object_set_new(mdns, "manufacturer", json_string(app_config->mdns.manufacturer));
    json_object_set_new(mdns, "log_level", json_string(app_config->mdns.log_level));
    
    json_t *mdns_status = json_object();
    json_object_set_new(mdns_status, "discoveryCount", json_integer(0));  // Placeholder
    json_object_set_new(mdns_status, "threads", json_integer(mdns_threads.thread_count));
    json_object_set_new(mdns_status, "virtualMemoryBytes", json_integer(mdns_threads.virtual_memory));
    json_object_set_new(mdns_status, "residentMemoryBytes", json_integer(mdns_threads.resident_memory));
    add_thread_ids_to_service(mdns_status, &mdns_threads);
    json_object_set_new(mdns, "status", mdns_status);
    json_object_set_new(services, "mdns", mdns);
    
    // Print queue configuration
    json_t *print = json_object();
    json_object_set_new(print, "enabled", json_boolean(app_config->print_queue.enabled));
    json_object_set_new(print, "log_level", json_string(app_config->print_queue.log_level));
    
    json_t *print_status = json_object();
    json_object_set_new(print_status, "queuedJobs", json_integer(0));     // Placeholder
    json_object_set_new(print_status, "completedJobs", json_integer(0));  // Placeholder
    json_object_set_new(print_status, "threads", json_integer(print_threads.thread_count));
    json_object_set_new(print_status, "virtualMemoryBytes", json_integer(print_threads.virtual_memory));
    json_object_set_new(print_status, "residentMemoryBytes", json_integer(print_threads.resident_memory));
    add_thread_ids_to_service(print_status, &print_threads);
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