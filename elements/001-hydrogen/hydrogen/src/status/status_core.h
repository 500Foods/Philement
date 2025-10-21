/*
 * Core Status Definitions
 * 
 * Defines the core structures and interfaces for system status collection
 * and reporting. This serves as the foundation for both JSON and Prometheus
 * formatted status outputs.
 */

#ifndef STATUS_CORE_H
#define STATUS_CORE_H

#include <globals.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

// WebSocket metrics structure
typedef struct {
    time_t server_start_time;
    int active_connections;
    int total_connections;
    int total_requests;
} WebSocketMetrics;

// File descriptor information
typedef struct {
    int fd;
    char type[MAX_TYPE_STRING];
    char description[MAX_DESC_STRING];
} FileDescriptorInfo;

// CPU metrics
typedef struct {
    char total_usage[MAX_PERCENTAGE_STRING];
    char **per_core_usage;  // Dynamic array of percentage string pointers
    int core_count;
    char load_1min[MAX_PERCENTAGE_STRING];
    char load_5min[MAX_PERCENTAGE_STRING];
    char load_15min[MAX_PERCENTAGE_STRING];
} CpuMetrics;

// System memory metrics
typedef struct {
    unsigned long long total_ram;
    unsigned long long used_ram;
    unsigned long long free_ram;
    char ram_used_percent[MAX_PERCENTAGE_STRING];
    unsigned long long total_swap;
    unsigned long long used_swap;
    unsigned long long free_swap;
    char swap_used_percent[MAX_PERCENTAGE_STRING];
} SystemMemoryMetrics;

// Network interface metrics
typedef struct {
    char name[MAX_TYPE_STRING];
    char **addresses;  // Dynamic array of address strings
    int address_count;
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetworkInterfaceMetrics;

// Network metrics
typedef struct {
    NetworkInterfaceMetrics *interfaces;
    int interface_count;
} NetworkMetrics;

// Filesystem metrics
typedef struct {
    char device[MAX_PATH_STRING];
    char mount_point[MAX_PATH_STRING];
    char type[MAX_TYPE_STRING];
    unsigned long long total_space;
    unsigned long long used_space;
    unsigned long long available_space;
    char used_percent[MAX_PERCENTAGE_STRING];
} FilesystemMetrics;

// Service thread metrics
typedef struct {
    int thread_count;
    pid_t *thread_tids;
    size_t virtual_memory;
    size_t resident_memory;
} ServiceThreadMetrics;

// Queue metrics
typedef struct {
    int entry_count;
    int block_count;
    size_t total_allocation;
    size_t virtual_bytes;
    size_t resident_bytes;
} QueueMetrics;

// Service metrics
typedef struct {
    bool enabled;
    ServiceThreadMetrics threads;
    union {
        struct {
            int message_count;
        } logging;
        struct {
            int active_requests;
            int total_requests;
        } webserver;
        struct {
            time_t uptime;
            int active_connections;
            int total_connections;
            int total_requests;
        } websocket;
        struct {
            int discovery_count;
        } mdns;
        struct {
            int queued_jobs;
            int completed_jobs;
        } print;
    } specific;
} ServiceMetrics;

// Complete system metrics structure
typedef struct {
    // Version information
    char server_version[MAX_VERSION_STRING];
    char api_version[MAX_VERSION_STRING];
    char release[MAX_VERSION_STRING];
    char build_type[MAX_VERSION_STRING];
    
    // System information
    char sysname[MAX_SYSINFO_STRING];
    char nodename[MAX_SYSINFO_STRING];
    char release_info[MAX_SYSINFO_STRING];
    char version_info[MAX_SYSINFO_STRING];
    char machine[MAX_SYSINFO_STRING];
    
    // Core metrics
    CpuMetrics cpu;
    SystemMemoryMetrics memory;
    NetworkMetrics network;
    FilesystemMetrics *filesystems;
    int filesystem_count;
    
    // Process metrics
    FileDescriptorInfo *file_descriptors;
    int fd_count;
    size_t total_threads;
    size_t total_virtual_memory;
    size_t total_resident_memory;
    
    // Service metrics
    ServiceMetrics logging;
    ServiceMetrics webserver;
    ServiceMetrics websocket;
    ServiceMetrics mdns;
    ServiceMetrics print;
    
    // Queue metrics
    QueueMetrics log_queue;
    QueueMetrics webserver_queue;
    QueueMetrics websocket_queue;
    QueueMetrics mdns_server_queue;
    QueueMetrics print_queue;
    QueueMetrics database_queue;
    QueueMetrics mail_relay_queue;
    QueueMetrics notify_queue;
    
    // Server status
    bool server_running;
    bool server_stopping;
    bool server_starting;
    time_t server_start_time;
    time_t server_uptime;
    
    // Resource allocation
    double service_allocation_percent;
    double queue_allocation_percent;
    double other_allocation_percent;
} SystemMetrics;

// Initialize the metrics collection system
void status_core_init(void);

// Clean up the metrics collection system
void status_core_cleanup(void);

// Collect all system metrics into a single structure
SystemMetrics* collect_system_metrics(const WebSocketMetrics *ws_metrics);

// Free a SystemMetrics structure and all its dynamically allocated members
void free_system_metrics(SystemMetrics *metrics);

#endif /* STATUS_CORE_H */
