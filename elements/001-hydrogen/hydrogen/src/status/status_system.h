/*
 * System Metrics Collection
 * 
 * Defines functions for collecting system-level metrics including:
 * - CPU usage and load
 * - Memory utilization
 * - Network interfaces and traffic
 * - Filesystem usage
 */

#ifndef STATUS_SYSTEM_H
#define STATUS_SYSTEM_H

#include "status_core.h"

// Collect CPU metrics (usage per core, load averages)
bool collect_cpu_metrics(CpuMetrics *cpu);

// Collect memory metrics (RAM and swap usage)
bool collect_memory_metrics(SystemMemoryMetrics *memory);

// Collect network interface metrics
bool collect_network_metrics(NetworkMetrics *network);

// Collect filesystem metrics
bool collect_filesystem_metrics(FilesystemMetrics **filesystems, int *count);

// Collect all system information (uname data)
bool collect_system_info(SystemMetrics *metrics);

// Helper function to format percentage with consistent precision
void format_percentage(double value, char *buffer, size_t buffer_size);

#endif /* STATUS_SYSTEM_H */
