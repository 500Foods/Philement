// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils_status.h"
#include "utils_threads.h"
#include "utils_queue.h"
#include "utils_time.h"
#include "utils_logging.h"
#include "configuration.h"
#include "logging.h"

// System headers
#include <math.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// Thread synchronization mutex
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;

// Forward declarations of static functions
static void get_socket_info(int inode, char *proto, int *port);
static void get_fd_info(int fd, FileDescriptorInfo *info);
static void get_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap);
static void add_thread_ids_to_service(json_t *service_status, ServiceThreads *threads);

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

// Helper function to add thread IDs to a service status object
static void add_thread_ids_to_service(json_t *service_status, ServiceThreads *threads) {
    json_t *tid_array = json_array();
    for (int i = 0; i < threads->thread_count; i++) {
        json_array_append_new(tid_array, json_integer(threads->thread_tids[i]));
    }
    json_object_set_new(service_status, "threadIds", tid_array);
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
        info.fd = fd;
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

// Generate system status report in JSON format
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
    
    // Status Information
    json_t *status = json_object();
    json_object_set_new(status, "server_running", json_boolean(server_running));
    json_object_set_new(status, "server_stopping", json_boolean(server_stopping));
    json_object_set_new(status, "server_starting", json_boolean(server_starting));

    // Add server start time and runtime if server is ready
    if (is_server_ready_time_set()) {
        time_t ready_time = get_server_ready_time();
        char iso_time[32];
        struct tm *tm_info = gmtime(&ready_time);
        strftime(iso_time, sizeof(iso_time), "%Y-%m-%dT%H:%M:%S.000Z", tm_info);
        json_object_set_new(status, "server_started", json_string(iso_time));

        time_t runtime = time(NULL) - ready_time;
        json_object_set_new(status, "server_runtime", json_integer(runtime));

        char runtime_str[32];
        format_duration(runtime, runtime_str, sizeof(runtime_str));
        json_object_set_new(status, "server_runtime_formatted", json_string(runtime_str));
    }
    
    // Get process memory metrics
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
    
    // Calculate service memory totals
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
    
    // Add process totals to status object
    json_object_set_new(status, "totalThreads", json_integer((int)total_threads + 1));
    json_object_set_new(status, "totalVirtualMemoryBytes", json_integer(process_virtual * 1024));
    json_object_set_new(status, "totalResidentMemoryBytes", json_integer(process_resident * 1024));
    
    // Other memory calculations
    size_t other_virtual = (process_virtual * 1024) > service_virtual_total + queue_virtual_total ? 
                         (process_virtual * 1024) - service_virtual_total - queue_virtual_total : 0;
    size_t other_resident = (process_resident * 1024) > service_resident_total + queue_resident_total ? 
                          (process_resident * 1024) - service_resident_total - queue_resident_total : 0;
    
    // Calculate percentages
    double service_percent = process_resident > 0 ? 
                          round((double)service_resident_total / (process_resident * 1024) * 100000.0) / 1000.0 : 0.0;
    double queue_percent = process_resident > 0 ? 
                         round((double)queue_resident_total / (process_resident * 1024) * 100000.0) / 1000.0 : 0.0;
    double other_percent = round((100.0 - service_percent - queue_percent) * 1000.0) / 1000.0;
    
    // Format percentages
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
    
    // Other resources
    json_t *other_resources = json_object();
    json_object_set_new(other_resources, "threads", json_integer(1));
    json_object_set_new(other_resources, "virtualMemoryBytes", json_integer(other_virtual));
    json_object_set_new(other_resources, "residentMemoryBytes", json_integer(other_resident));
    json_object_set_new(other_resources, "allocationPercent", json_string(other_percent_str));
    json_object_set_new(resources, "otherResources", other_resources);
    
    json_object_set_new(status, "resources", resources);
    
    // Add file descriptors
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
    
    // Logging service
    json_t *logging = json_object();
    json_object_set_new(logging, "enabled", json_true());
    json_object_set_new(logging, "log_file", json_string(app_config->log_file_path));
    
    json_t *logging_status = json_object();
    json_object_set_new(logging_status, "messageCount", json_integer(0));
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
    
    json_t *web_status = json_object();
    json_object_set_new(web_status, "activeRequests", json_integer(0));
    json_object_set_new(web_status, "totalRequests", json_integer(0));
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
    
    json_t *mdns_status = json_object();
    json_object_set_new(mdns_status, "discoveryCount", json_integer(0));
    json_object_set_new(mdns_status, "threads", json_integer(mdns_threads.thread_count));
    json_object_set_new(mdns_status, "virtualMemoryBytes", json_integer(mdns_threads.virtual_memory));
    json_object_set_new(mdns_status, "residentMemoryBytes", json_integer(mdns_threads.resident_memory));
    add_thread_ids_to_service(mdns_status, &mdns_threads);
    json_object_set_new(mdns, "status", mdns_status);
    json_object_set_new(services, "mdns", mdns);
    
    // Print queue configuration
    json_t *print = json_object();
    json_object_set_new(print, "enabled", json_boolean(app_config->print_queue.enabled));
    
    json_t *print_status = json_object();
    json_object_set_new(print_status, "queuedJobs", json_integer(0));
    json_object_set_new(print_status, "completedJobs", json_integer(0));
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