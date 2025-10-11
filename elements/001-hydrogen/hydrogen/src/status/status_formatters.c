/*
 * Status Output Formatters Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "status_formatters.h"

// Convert system metrics to JSON format
// Exposed for testing - was previously static
json_t* format_system_status_json(const SystemMetrics *metrics) {
    // Check for NULL metrics parameter
    if (!metrics) {
        return NULL;
    }

    json_t *root = json_object();
    
    // Version Information
    json_t *version = json_object();
    json_object_set_new(version, "server", json_string(metrics->server_version));
    json_object_set_new(version, "api", json_string(metrics->api_version));
    json_object_set_new(version, "release", json_string(metrics->release));
    json_object_set_new(version, "build_type", json_string(metrics->build_type));
    json_object_set_new(root, "version", version);

    // System Information
    json_t *system = json_object();
    json_object_set_new(system, "sysname", json_string(metrics->sysname));
    json_object_set_new(system, "nodename", json_string(metrics->nodename));
    json_object_set_new(system, "release", json_string(metrics->release_info));
    json_object_set_new(system, "version", json_string(metrics->version_info));
    json_object_set_new(system, "machine", json_string(metrics->machine));

    // CPU Information
    json_t *cpu_usage = json_object();
    json_object_set_new(cpu_usage, "total", json_string(metrics->cpu.total_usage));
    
    json_t *cpu_usage_per_core = json_object();
    for (int i = 0; i < metrics->cpu.core_count; i++) {
        char core_name[32];
        snprintf(core_name, sizeof(core_name), "cpu%d", i);
        json_object_set_new(cpu_usage_per_core, core_name, 
                          json_string(metrics->cpu.per_core_usage[i]));
    }
    
    json_object_set_new(system, "cpu_usage", cpu_usage);
    json_object_set_new(system, "cpu_usage_per_core", cpu_usage_per_core);
    json_object_set_new(system, "load_1min", json_string(metrics->cpu.load_1min));
    json_object_set_new(system, "load_5min", json_string(metrics->cpu.load_5min));
    json_object_set_new(system, "load_15min", json_string(metrics->cpu.load_15min));

    // Memory Information
    json_t *memory = json_object();
    json_object_set_new(memory, "total", json_integer((json_int_t)metrics->memory.total_ram));
    json_object_set_new(memory, "used", json_integer((json_int_t)metrics->memory.used_ram));
    json_object_set_new(memory, "free", json_integer((json_int_t)metrics->memory.free_ram));
    json_object_set_new(memory, "used_percent", json_string(metrics->memory.ram_used_percent));

    if (metrics->memory.total_swap > 0) {
        json_object_set_new(memory, "swap_total", json_integer((json_int_t)metrics->memory.total_swap));
        json_object_set_new(memory, "swap_used", json_integer((json_int_t)metrics->memory.used_swap));
        json_object_set_new(memory, "swap_free", json_integer((json_int_t)metrics->memory.free_swap));
        json_object_set_new(memory, "swap_used_percent", 
                          json_string(metrics->memory.swap_used_percent));
    }
    json_object_set_new(system, "memory", memory);

    // Network Interfaces
    json_t *interfaces = json_object();
    for (int i = 0; i < metrics->network.interface_count; i++) {
        const NetworkInterfaceMetrics *iface = &metrics->network.interfaces[i];
        json_t *interface = json_object();
        json_object_set_new(interface, "name", json_string(iface->name));
        
        json_t *addresses = json_array();
        for (int j = 0; j < iface->address_count; j++) {
            json_array_append_new(addresses, json_string(iface->addresses[j]));
        }
        json_object_set_new(interface, "addresses", addresses);
        json_object_set_new(interface, "rx_bytes", json_integer((json_int_t)iface->rx_bytes));
        json_object_set_new(interface, "tx_bytes", json_integer((json_int_t)iface->tx_bytes));
        
        json_object_set_new(interfaces, iface->name, interface);
    }
    json_object_set_new(system, "network", interfaces);

    // Filesystem Information
    json_t *filesystems = json_object();
    for (int i = 0; i < metrics->filesystem_count; i++) {
        const FilesystemMetrics *fs = &metrics->filesystems[i];
        json_t *filesystem = json_object();
        json_object_set_new(filesystem, "device", json_string(fs->device));
        json_object_set_new(filesystem, "mount_point", json_string(fs->mount_point));
        json_object_set_new(filesystem, "type", json_string(fs->type));
        json_object_set_new(filesystem, "total_space", json_integer((json_int_t)fs->total_space));
        json_object_set_new(filesystem, "used_space", json_integer((json_int_t)fs->used_space));
        json_object_set_new(filesystem, "available_space", json_integer((json_int_t)fs->available_space));
        json_object_set_new(filesystem, "used_percent", json_string(fs->used_percent));
        
        json_object_set_new(filesystems, fs->mount_point, filesystem);
    }
    json_object_set_new(system, "filesystems", filesystems);
    json_object_set_new(root, "system", system);

    // Status Information
    json_t *status = json_object();
    json_object_set_new(status, "server_running", json_boolean(metrics->server_running));
    json_object_set_new(status, "server_stopping", json_boolean(metrics->server_stopping));
    json_object_set_new(status, "server_starting", json_boolean(metrics->server_starting));
    json_object_set_new(status, "server_uptime", json_integer(metrics->server_uptime));

    // Format server start time as ISO string
    char iso_time[32];
    const struct tm *tm_info = gmtime(&metrics->server_start_time);
    strftime(iso_time, sizeof(iso_time), "%Y-%m-%dT%H:%M:%S.000Z", tm_info);
    json_object_set_new(status, "server_started", json_string(iso_time));

    // Format uptime as human-readable string
    char uptime_str[32];
    time_t uptime = metrics->server_uptime;
    int days = (int)(uptime / 86400);
    uptime %= 86400;
    int hours = (int)(uptime / 3600);
    uptime %= 3600;
    int minutes = (int)(uptime / 60);
    int seconds = (int)(uptime % 60);
    
    if (days > 0) {
        snprintf(uptime_str, sizeof(uptime_str), "%dd %02dh %02dm %02ds", 
                days, hours, minutes, seconds);
    } else {
        snprintf(uptime_str, sizeof(uptime_str), "%02d:%02d:%02d", 
                hours, minutes, seconds);
    }
    json_object_set_new(status, "server_runtime_formatted", json_string(uptime_str));

    // Add file descriptors
    json_t *files = json_array();
    for (int i = 0; i < metrics->fd_count; i++) {
        const FileDescriptorInfo *fd = &metrics->file_descriptors[i];
        json_t *fd_obj = json_object();
        json_object_set_new(fd_obj, "fd", json_integer(fd->fd));
        json_object_set_new(fd_obj, "type", json_string(fd->type));
        json_object_set_new(fd_obj, "description", json_string(fd->description));
        json_array_append_new(files, fd_obj);
    }
    json_object_set_new(status, "files", files);
    json_object_set_new(root, "status", status);

    // Service Information
    json_t *services = json_object();
    
    // Logging service
    if (metrics->logging.enabled) {
        json_t *logging = json_object();
        json_object_set_new(logging, "enabled", json_true());
        json_t *logging_status = json_object();
        json_object_set_new(logging_status, "messageCount", 
                          json_integer((json_int_t)metrics->logging.specific.logging.message_count));
        json_object_set_new(logging_status, "threads", 
                          json_integer((json_int_t)metrics->logging.threads.thread_count));
        json_object_set_new(logging_status, "virtualMemoryBytes", 
                          json_integer((json_int_t)metrics->logging.threads.virtual_memory));
        json_object_set_new(logging_status, "residentMemoryBytes", 
                          json_integer((json_int_t)metrics->logging.threads.resident_memory));
        json_object_set_new(logging, "status", logging_status);
        json_object_set_new(services, "logging", logging);
    }

    // Webserver service
    if (metrics->webserver.enabled) {
        json_t *webserver = json_object();
        json_object_set_new(webserver, "enabled", json_true());
        json_t *webserver_status = json_object();
        json_object_set_new(webserver_status, "activeRequests", 
                          json_integer((json_int_t)metrics->webserver.specific.webserver.active_requests));
        json_object_set_new(webserver_status, "totalRequests", 
                          json_integer((json_int_t)metrics->webserver.specific.webserver.total_requests));
        json_object_set_new(webserver_status, "threads", 
                          json_integer(metrics->webserver.threads.thread_count));
        json_object_set_new(webserver_status, "virtualMemoryBytes", 
                          json_integer((json_int_t)metrics->webserver.threads.virtual_memory));
        json_object_set_new(webserver_status, "residentMemoryBytes", 
                          json_integer((json_int_t)metrics->webserver.threads.resident_memory));
        json_object_set_new(webserver, "status", webserver_status);
        json_object_set_new(services, "webserver", webserver);
    }

    // WebSocket service
    if (metrics->websocket.enabled) {
        json_t *websocket = json_object();
        json_object_set_new(websocket, "enabled", json_true());
        json_t *websocket_status = json_object();
        json_object_set_new(websocket_status, "uptime", 
                          json_integer((json_int_t)metrics->websocket.specific.websocket.uptime));
        json_object_set_new(websocket_status, "activeConnections", 
                          json_integer((json_int_t)metrics->websocket.specific.websocket.active_connections));
        json_object_set_new(websocket_status, "totalConnections", 
                          json_integer((json_int_t)metrics->websocket.specific.websocket.total_connections));
        json_object_set_new(websocket_status, "totalRequests", 
                          json_integer((json_int_t)metrics->websocket.specific.websocket.total_requests));
        json_object_set_new(websocket_status, "threads", 
                          json_integer(metrics->websocket.threads.thread_count));
        json_object_set_new(websocket_status, "virtualMemoryBytes", 
                          json_integer((json_int_t)metrics->websocket.threads.virtual_memory));
        json_object_set_new(websocket_status, "residentMemoryBytes", 
                          json_integer((json_int_t)metrics->websocket.threads.resident_memory));
        json_object_set_new(websocket, "status", websocket_status);
        json_object_set_new(services, "websocket", websocket);
    }

    json_object_set_new(root, "services", services);
    
    return root;
}

// Helper function to format percentage values consistently for Prometheus
void format_prometheus_percentage(char *buffer, size_t buffer_size, 
                               const char *metric_name, const char *labels, 
                               const char *value) {
    double val;
    sscanf(value, "%lf", &val);  // Convert string percentage to double
    snprintf(buffer, buffer_size,
             "# HELP %s Percentage value\n"
             "# TYPE %s gauge\n"
             "%s{%s} %f\n",
             metric_name, metric_name, metric_name, 
             labels ? labels : "", val / 100.0);  // Convert percentage to ratio
}

// Convert system metrics to Prometheus format
// Exposed for testing - was previously static
char* format_system_status_prometheus(const SystemMetrics *metrics) {
    // Check for NULL metrics parameter
    if (!metrics) {
        return NULL;
    }

    // Initial buffer size - will be expanded if needed
    size_t buffer_size = 16384;
    char *output = malloc(buffer_size);
    size_t offset = 0;

    // Helper macro to safely append to buffer
    #define APPEND(...) do { \
        int written = snprintf(output + offset, buffer_size - offset, __VA_ARGS__); \
        if (written > 0) offset += (size_t)written; \
        if (offset >= buffer_size - 1024) { \
            size_t new_size = buffer_size * 2; \
            char *new_buffer = realloc(output, new_size); \
            if (new_buffer == NULL) { \
                free(output); \
                return NULL; \
            } \
            output = new_buffer; \
            buffer_size = new_size; \
        } \
    } while(0)

    // System Info
    APPEND("# HELP system_info System information\n"
           "# TYPE system_info gauge\n"
           "system_info{version=\"%s\",release=\"%s\",build=\"%s\"} 1\n\n",
           metrics->server_version, metrics->release, metrics->build_type);

    // CPU Metrics
    char cpu_buffer[256];
    format_prometheus_percentage(cpu_buffer, sizeof(cpu_buffer),
                               "cpu_usage_total", "", metrics->cpu.total_usage);
    APPEND("%s\n", cpu_buffer);

    for (int i = 0; i < metrics->cpu.core_count; i++) {
        format_prometheus_percentage(cpu_buffer, sizeof(cpu_buffer),
                                  "cpu_usage_core",
                                  "core", metrics->cpu.per_core_usage[i]);
        APPEND("%s\n", cpu_buffer);
    }

    // Memory Metrics
    APPEND("# HELP memory_total_bytes Total system memory in bytes\n"
           "# TYPE memory_total_bytes gauge\n"
           "memory_total_bytes %llu\n"
           "# HELP memory_used_bytes Used system memory in bytes\n"
           "# TYPE memory_used_bytes gauge\n"
           "memory_used_bytes %llu\n"
           "# HELP memory_free_bytes Free system memory in bytes\n"
           "# TYPE memory_free_bytes gauge\n"
           "memory_free_bytes %llu\n",
           metrics->memory.total_ram,
           metrics->memory.used_ram,
           metrics->memory.free_ram);

    format_prometheus_percentage(cpu_buffer, sizeof(cpu_buffer),
                              "memory_used_ratio", "", 
                              metrics->memory.ram_used_percent);
    APPEND("%s\n", cpu_buffer);

    if (metrics->memory.total_swap > 0) {
        APPEND("# HELP swap_total_bytes Total swap space in bytes\n"
               "# TYPE swap_total_bytes gauge\n"
               "swap_total_bytes %llu\n"
               "# HELP swap_used_bytes Used swap space in bytes\n"
               "# TYPE swap_used_bytes gauge\n"
               "swap_used_bytes %llu\n"
               "# HELP swap_free_bytes Free swap space in bytes\n"
               "# TYPE swap_free_bytes gauge\n"
               "swap_free_bytes %llu\n",
               metrics->memory.total_swap,
               metrics->memory.used_swap,
               metrics->memory.free_swap);

        format_prometheus_percentage(cpu_buffer, sizeof(cpu_buffer),
                                  "swap_used_ratio", "",
                                  metrics->memory.swap_used_percent);
        APPEND("%s\n", cpu_buffer);
    }

    // Network Metrics
    for (int i = 0; i < metrics->network.interface_count; i++) {
        const NetworkInterfaceMetrics *iface = &metrics->network.interfaces[i];
        APPEND("# HELP network_receive_bytes_total Total bytes received per interface\n"
               "# TYPE network_receive_bytes_total counter\n"
               "network_receive_bytes_total{interface=\"%s\"} %llu\n"
               "# HELP network_transmit_bytes_total Total bytes transmitted per interface\n"
               "# TYPE network_transmit_bytes_total counter\n"
               "network_transmit_bytes_total{interface=\"%s\"} %llu\n",
               iface->name, iface->rx_bytes,
               iface->name, iface->tx_bytes);
    }

    // Service Metrics
    if (metrics->logging.enabled) {
        APPEND("# HELP service_threads Number of threads per service\n"
               "# TYPE service_threads gauge\n"
               "service_threads{service=\"logging\"} %d\n"
               "# HELP service_virtual_memory_bytes Virtual memory usage per service\n"
               "# TYPE service_virtual_memory_bytes gauge\n"
               "service_virtual_memory_bytes{service=\"logging\"} %zu\n"
               "# HELP service_resident_memory_bytes Resident memory usage per service\n"
               "# TYPE service_resident_memory_bytes gauge\n"
               "service_resident_memory_bytes{service=\"logging\"} %zu\n",
               metrics->logging.threads.thread_count,
               metrics->logging.threads.virtual_memory,
               metrics->logging.threads.resident_memory);
    }

    if (metrics->webserver.enabled) {
        APPEND("service_threads{service=\"webserver\"} %d\n"
               "service_virtual_memory_bytes{service=\"webserver\"} %zu\n"
               "service_resident_memory_bytes{service=\"webserver\"} %zu\n"
               "# HELP webserver_active_requests Current number of active webserver requests\n"
               "# TYPE webserver_active_requests gauge\n"
               "webserver_active_requests %d\n"
               "# HELP webserver_requests_total Total number of webserver requests\n"
               "# TYPE webserver_requests_total counter\n"
               "webserver_requests_total %d\n",
               metrics->webserver.threads.thread_count,
               metrics->webserver.threads.virtual_memory,
               metrics->webserver.threads.resident_memory,
               metrics->webserver.specific.webserver.active_requests,
               metrics->webserver.specific.webserver.total_requests);
    }

    if (metrics->websocket.enabled) {
        APPEND("service_threads{service=\"websocket\"} %d\n"
               "service_virtual_memory_bytes{service=\"websocket\"} %zu\n"
               "service_resident_memory_bytes{service=\"websocket\"} %zu\n"
               "# HELP websocket_uptime_seconds WebSocket server uptime\n"
               "# TYPE websocket_uptime_seconds counter\n"
               "websocket_uptime_seconds %ld\n"
               "# HELP websocket_active_connections Current number of active WebSocket connections\n"
               "# TYPE websocket_active_connections gauge\n"
               "websocket_active_connections %d\n"
               "# HELP websocket_connections_total Total number of WebSocket connections\n"
               "# TYPE websocket_connections_total counter\n"
               "websocket_connections_total %d\n"
               "# HELP websocket_requests_total Total number of WebSocket requests\n"
               "# TYPE websocket_requests_total counter\n"
               "websocket_requests_total %d\n",
               metrics->websocket.threads.thread_count,
               metrics->websocket.threads.virtual_memory,
               metrics->websocket.threads.resident_memory,
               metrics->websocket.specific.websocket.uptime,
               metrics->websocket.specific.websocket.active_connections,
               metrics->websocket.specific.websocket.total_connections,
               metrics->websocket.specific.websocket.total_requests);
    }

    // Queue Metrics
    APPEND("# HELP queue_entries Current number of entries in queue\n"
           "# TYPE queue_entries gauge\n"
           "queue_entries{queue=\"log\"} %d\n"
           "queue_entries{queue=\"print\"} %d\n"
           "# HELP queue_blocks Current number of memory blocks in queue\n"
           "# TYPE queue_blocks gauge\n"
           "queue_blocks{queue=\"log\"} %d\n"
           "queue_blocks{queue=\"print\"} %d\n"
           "# HELP queue_memory_bytes Memory usage per queue\n"
           "# TYPE queue_memory_bytes gauge\n"
           "queue_memory_bytes{queue=\"log\",type=\"virtual\"} %zu\n"
           "queue_memory_bytes{queue=\"log\",type=\"resident\"} %zu\n"
           "queue_memory_bytes{queue=\"print\",type=\"virtual\"} %zu\n"
           "queue_memory_bytes{queue=\"print\",type=\"resident\"} %zu\n",
           metrics->log_queue.entry_count,
           metrics->print_queue.entry_count,
           metrics->log_queue.block_count,
           metrics->print_queue.block_count,
           metrics->log_queue.virtual_bytes,
           metrics->log_queue.resident_bytes,
           metrics->print_queue.virtual_bytes,
           metrics->print_queue.resident_bytes);

    #undef APPEND
    return output;
}
