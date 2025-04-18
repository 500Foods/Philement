/*
 * System Status Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "status.h"
#include "status_core.h"
#include "status_system.h"
#include "status_process.h"
#include "status_formatters.h"
#include "../logging/logging.h"

// Initialize the status collection system
void status_init(void) {
    log_this("Status", "Initializing status collection system", LOG_LEVEL_STATE);
    status_core_init();
}

// Clean up the status collection system
void status_cleanup(void) {
    log_this("Status", "Cleaning up status collection system", LOG_LEVEL_STATE);
    status_core_cleanup();
}

// Helper function to collect all metrics
static SystemMetrics* collect_all_metrics(const WebSocketMetrics *ws_metrics) {
    SystemMetrics *metrics = collect_system_metrics(ws_metrics);
    if (!metrics) {
        log_this("Status", "Failed to allocate metrics structure", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Collect system metrics
    if (!collect_system_info(metrics)) {
        log_this("Status", "Failed to collect system info", LOG_LEVEL_ERROR);
        goto error;
    }

    if (!collect_cpu_metrics(&metrics->cpu)) {
        log_this("Status", "Failed to collect CPU metrics", LOG_LEVEL_ERROR);
        goto error;
    }

    if (!collect_memory_metrics(&metrics->memory)) {
        log_this("Status", "Failed to collect memory metrics", LOG_LEVEL_ERROR);
        goto error;
    }

    if (!collect_network_metrics(&metrics->network)) {
        log_this("Status", "Failed to collect network metrics", LOG_LEVEL_ERROR);
        goto error;
    }

    if (!collect_filesystem_metrics(&metrics->filesystems, &metrics->filesystem_count)) {
        log_this("Status", "Failed to collect filesystem metrics", LOG_LEVEL_ERROR);
        goto error;
    }

    // Collect process metrics
    if (!collect_file_descriptors(&metrics->file_descriptors, &metrics->fd_count)) {
        log_this("Status", "Failed to collect file descriptors", LOG_LEVEL_ERROR);
        goto error;
    }

    size_t vmsize, vmrss, vmswap;
    if (!get_process_memory(&vmsize, &vmrss, &vmswap)) {
        log_this("Status", "Failed to collect process memory metrics", LOG_LEVEL_ERROR);
        goto error;
    }
    metrics->total_virtual_memory = vmsize;
    metrics->total_resident_memory = vmrss;

    // Collect service metrics
    if (!collect_service_metrics(metrics, ws_metrics)) {
        log_this("Status", "Failed to collect service metrics", LOG_LEVEL_ERROR);
        goto error;
    }

    return metrics;

error:
    free_system_metrics(metrics);
    return NULL;
}

// Get complete system status in JSON format
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics) {
    SystemMetrics *metrics = collect_all_metrics(ws_metrics);
    if (!metrics) {
        return NULL;
    }

    json_t *result = format_system_status_json(metrics);
    free_system_metrics(metrics);
    return result;
}

// Get system status in Prometheus format
char* get_system_status_prometheus(const WebSocketMetrics *ws_metrics) {
    SystemMetrics *metrics = collect_all_metrics(ws_metrics);
    if (!metrics) {
        return NULL;
    }

    char *result = format_system_status_prometheus(metrics);
    free_system_metrics(metrics);
    return result;
}