/*
 * Status Output Formatters
 * 
 * Defines functions for formatting system status information into
 * different output formats (JSON and Prometheus).
 */

#ifndef STATUS_FORMATTERS_H
#define STATUS_FORMATTERS_H

#include <jansson.h>
#include "status_core.h"

// Convert system metrics to JSON format
json_t* format_system_status_json(const SystemMetrics *metrics);

// Convert system metrics to Prometheus format
// Returns a newly allocated string containing the metrics in Prometheus format
// Caller is responsible for freeing the returned string
char* format_system_status_prometheus(const SystemMetrics *metrics);

// Helper function to format percentage values consistently
void format_prometheus_percentage(char *buffer, size_t buffer_size, 
                               const char *metric_name, const char *labels, 
                               const char *value);

#endif /* STATUS_FORMATTERS_H */