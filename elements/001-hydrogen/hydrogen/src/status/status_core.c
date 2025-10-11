/*
 * Core Status Implementation
 * 
 * Implements the core functionality for system status collection
 * and management. This includes initialization, cleanup, and
 * memory management for the metrics structures.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "status_core.h"

// Function prototypes
pthread_mutex_t* get_status_mutex(void);

// Thread synchronization mutex
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize the metrics collection system
void status_core_init(void) {
    // Currently just logs initialization
    // Will be expanded if needed for more complex initialization
    log_this(SR_STATUS, "Initializing status collection system", LOG_LEVEL_STATE, 0);
}

// Clean up the metrics collection system
void status_core_cleanup(void) {
    log_this(SR_STATUS, "Cleaning up status collection system", LOG_LEVEL_STATE, 0);
    pthread_mutex_destroy(&status_mutex);
}

// Free dynamically allocated memory in CPU metrics
static void free_cpu_metrics(CpuMetrics *cpu) {
    if (cpu) {
        free(cpu->per_core_usage);
        cpu->per_core_usage = NULL;
        cpu->core_count = 0;
    }
}

// Free dynamically allocated memory in network metrics
static void free_network_metrics(NetworkMetrics *network) {
    if (network && network->interfaces) {
        for (int i = 0; i < network->interface_count; i++) {
            NetworkInterfaceMetrics *iface = &network->interfaces[i];
            if (iface->addresses) {
                for (int j = 0; j < iface->address_count; j++) {
                    free(iface->addresses[j]);
                }
                free(iface->addresses);
            }
        }
        free(network->interfaces);
        network->interfaces = NULL;
        network->interface_count = 0;
    }
}

// Free dynamically allocated memory in service thread metrics
static void free_service_thread_metrics(ServiceThreadMetrics *metrics) {
    if (metrics) {
        free(metrics->thread_tids);
        metrics->thread_tids = NULL;
        metrics->thread_count = 0;
    }
}

// Free a SystemMetrics structure and all its dynamically allocated members
void free_system_metrics(SystemMetrics *metrics) {
    if (!metrics) return;

    // Free CPU metrics
    free_cpu_metrics(&metrics->cpu);

    // Free network metrics
    free_network_metrics(&metrics->network);

    // Free filesystem metrics
    if (metrics->filesystems) {
        free(metrics->filesystems);
        metrics->filesystems = NULL;
        metrics->filesystem_count = 0;
    }

    // Free file descriptor info
    if (metrics->file_descriptors) {
        free(metrics->file_descriptors);
        metrics->file_descriptors = NULL;
        metrics->fd_count = 0;
    }

    // Free service thread metrics
    free_service_thread_metrics(&metrics->logging.threads);
    free_service_thread_metrics(&metrics->webserver.threads);
    free_service_thread_metrics(&metrics->websocket.threads);
    free_service_thread_metrics(&metrics->mdns.threads);
    free_service_thread_metrics(&metrics->print.threads);

    // Finally, free the metrics structure itself
    free(metrics);
}

// Get the status mutex for use by other components
pthread_mutex_t* get_status_mutex(void) {
    return &status_mutex;
}

// Allocate and initialize a new SystemMetrics structure
static SystemMetrics* allocate_system_metrics(void) {
    SystemMetrics *metrics = calloc(1, sizeof(SystemMetrics));
    if (!metrics) {
        log_this(SR_STATUS, "Failed to allocate SystemMetrics structure", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    return metrics;
}

// Collect all system metrics into a single structure
SystemMetrics* collect_system_metrics(const WebSocketMetrics *ws_metrics) {
    pthread_mutex_lock(&status_mutex);
    
    SystemMetrics *metrics = allocate_system_metrics();
    if (!metrics) {
        pthread_mutex_unlock(&status_mutex);
        return NULL;
    }

    // Populate server timing metrics from WebSocket context if available
    if (ws_metrics) {
        metrics->server_start_time = ws_metrics->server_start_time;
        metrics->server_uptime = time(NULL) - ws_metrics->server_start_time;
    } else {
        // Default to current time if no WebSocket metrics
        metrics->server_start_time = time(NULL);
        metrics->server_uptime = 0;
    }

    snprintf(metrics->server_version, sizeof(metrics->server_version), "%s", VERSION);
    snprintf(metrics->api_version, sizeof(metrics->api_version), "%s", VERSION);
    snprintf(metrics->release, sizeof(metrics->release), "%s", RELEASE);
    snprintf(metrics->build_type, sizeof(metrics->build_type), "%s", BUILD_TYPE);

    metrics->server_running = server_running;
    metrics->server_stopping = server_stopping;
    metrics->server_starting = server_starting;

    // Note: The actual collection of other metrics will be implemented
    // in separate components (status_system.c, status_process.c, etc.)
    // This function will coordinate those collections.
    
    pthread_mutex_unlock(&status_mutex);
    return metrics;
}
