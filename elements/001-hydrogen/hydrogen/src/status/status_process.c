/*
 * Process Metrics Collection Implementation
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "status_process.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../utils/utils_queue.h"
#include "../threads/threads.h"

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;

// External service thread structures
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// External queue memory structures
extern QueueMemoryMetrics log_queue_memory;
extern QueueMemoryMetrics print_queue_memory;

// Forward declarations
static bool collect_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap);

// Helper function to safely truncate strings
char* safe_truncate(char* dest, size_t dest_size, const char* src) {
    size_t src_len = strlen(src);
    size_t copy_len = src_len < dest_size - 1 ? src_len : dest_size - 1;
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return dest;
}

// Collect process memory metrics from /proc/self/status
static bool collect_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap) {
    *vmsize = 0;
    *vmrss = 0;
    *vmswap = 0;
    
    FILE *status = fopen("/proc/self/status", "r");
    if (!status) {
        log_this("ProcessMetrics", "Failed to open /proc/self/status", LOG_LEVEL_ERROR);
        return false;
    }
    
    char line[256];  // Fixed size for /proc/self/status entries
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
    return true;
}

// Get process memory metrics from /proc/self/status
bool get_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap) {
    return collect_process_memory(vmsize, vmrss, vmswap);
}

// Get socket information from /proc/net files
void get_socket_info(int inode, char *proto, int *port) {
    char path[64];  // System path buffer - fixed size for /proc paths
    const char *net_files[] = {
        "tcp", "tcp6", "udp", "udp6"
    };
    
    *port = 0;
    proto[0] = '\0';
    
    for (int i = 0; i < 4; i++) {
        snprintf(path, sizeof(path), "/proc/net/%s", net_files[i]);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        
        char line[256];  // Fixed size for /proc/net entries
        fgets(line, sizeof(line), f); // Skip header
        
        while (fgets(line, sizeof(line), f)) {
            unsigned local_port;
            unsigned socket_inode;
            if (sscanf(line, "%*x: %*x:%x %*x:%*x %*x %*x:%*x %*x:%*x %*x %*d %*d %u",
                      &local_port, &socket_inode) == 2) {
                if (socket_inode == (unsigned)inode) {
                    *port = local_port;
                    snprintf(proto, 32, "%s", net_files[i]);  // 32 = size of proto buffer
                    fclose(f);
                    return;
                }
            }
        }
        fclose(f);
    }
}

// Get detailed information about a specific file descriptor
void get_fd_info(int fd, FileDescriptorInfo *info) {
    char path[64], target[256];  // System path buffers
    struct stat st;
    
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    ssize_t len = readlink(path, target, sizeof(target) - 1);
    if (len < 0) {
        safe_truncate(info->type, sizeof(info->type), "unknown");
        safe_truncate(info->description, sizeof(info->description), "error reading link");
        return;
    }
    target[len] = '\0';
    
    if (fstat(fd, &st) < 0) {
        safe_truncate(info->type, sizeof(info->type), "error");
        safe_truncate(info->description, sizeof(info->description), "fstat failed");
        return;
    }
    
    // Handle standard streams
    if (fd <= 2) {
        safe_truncate(info->type, sizeof(info->type), "stdio");
        const char *stream_name = (fd == 0) ? "stdin" : (fd == 1) ? "stdout" : "stderr";
        char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
        snprintf(temp, sizeof(temp), "%s: terminal", stream_name);
        safe_truncate(info->description, sizeof(info->description), temp);
        return;
    }

    // Handle socket
    if (S_ISSOCK(st.st_mode)) {
        char proto[32];
        int port;
        get_socket_info(st.st_ino, proto, &port);
        
        safe_truncate(info->type, sizeof(info->type), "socket");
        if (port > 0) {
            const char *service = "";
            if (port == app_config->web.port) service = "web server";
            else if (port == app_config->websocket.port) service = "websocket server";
            else if (port == 5353) service = "mDNS server";
            
            char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
            if (service[0]) {
                snprintf(temp, sizeof(temp), "socket (%s port %d - %s)", proto, port, service);
            } else {
                snprintf(temp, sizeof(temp), "socket (%s port %d)", proto, port);
            }
            safe_truncate(info->description, sizeof(info->description), temp);
        } else if (strstr(target, "socket:[")) {
            struct sockaddr_storage addr;
            socklen_t addr_len = sizeof(addr);
            if (getsockname(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
                if (addr.ss_family == AF_UNIX) {
                    struct sockaddr_un *un = (struct sockaddr_un*)&addr;
                    if (un->sun_path[0]) {
                        char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
                        snprintf(temp, sizeof(temp), "Unix domain socket: %s", un->sun_path);
                        safe_truncate(info->description, sizeof(info->description), temp);
                    } else {
                        safe_truncate(info->description, sizeof(info->description), 
                                    "Unix domain socket: *");
                    }
                } else {
                    char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
                    snprintf(temp, sizeof(temp), "socket (inode: %lu)", st.st_ino);
                    safe_truncate(info->description, sizeof(info->description), temp);
                }
            } else {
                char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
                snprintf(temp, sizeof(temp), "socket (inode: %lu)", st.st_ino);
                safe_truncate(info->description, sizeof(info->description), temp);
            }
        }
        return;
    }
    
    // Handle anonymous inodes
    if (strncmp(target, "anon_inode:", 11) == 0) {
        safe_truncate(info->type, sizeof(info->type), "anon_inode");
        const char *anon_type = target + 11;
        if (strcmp(anon_type, "[eventfd]") == 0)
            safe_truncate(info->description, sizeof(info->description), "event notification channel");
        else if (strcmp(anon_type, "[eventpoll]") == 0)
            safe_truncate(info->description, sizeof(info->description), "epoll instance");
        else if (strcmp(anon_type, "[timerfd]") == 0)
            safe_truncate(info->description, sizeof(info->description), "timer notification");
        else {
            char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
            snprintf(temp, sizeof(temp), "anonymous inode: %s", anon_type);
            safe_truncate(info->description, sizeof(info->description), temp);
        }
        return;
    }
    
    // Handle regular files and other types
    if (S_ISREG(st.st_mode)) {
        safe_truncate(info->type, sizeof(info->type), "file");
        char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
        snprintf(temp, sizeof(temp), "file: %s", target);
        safe_truncate(info->description, sizeof(info->description), temp);
    } else if (strncmp(target, "/dev/", 5) == 0) {
        safe_truncate(info->type, sizeof(info->type), "device");
        if (strcmp(target, "/dev/urandom") == 0) {
            safe_truncate(info->description, sizeof(info->description), "random number source");
        } else {
            char temp[MAX_DESC_STRING * 2];  // Larger temp buffer
            snprintf(temp, sizeof(temp), "%s", target);
            safe_truncate(info->description, sizeof(info->description), temp);
        }
    } else {
        safe_truncate(info->type, sizeof(info->type), "other");
        safe_truncate(info->description, sizeof(info->description), target);
    }
}

// Collect file descriptor information
bool collect_file_descriptors(FileDescriptorInfo **descriptors, int *count) {
    DIR *dir = opendir("/proc/self/fd");
    if (!dir) {
        log_this("ProcessMetrics", "Failed to open /proc/self/fd", LOG_LEVEL_ERROR);
        return false;
    }

    // First pass to count file descriptors
    struct dirent *ent;
    *count = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] != '.') (*count)++;
    }

    // Allocate array for file descriptors
    *descriptors = calloc(*count, sizeof(FileDescriptorInfo));
    if (!*descriptors) {
        closedir(dir);
        return false;
    }

    // Reset and second pass to collect information
    rewinddir(dir);
    int current_fd = 0;

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        int fd = atoi(ent->d_name);
        FileDescriptorInfo *info = &(*descriptors)[current_fd];
        info->fd = fd;
        get_fd_info(fd, info);
        current_fd++;
    }

    closedir(dir);
    return true;
}

// Convert ServiceThreads metrics to ServiceThreadMetrics
void convert_thread_metrics(const ServiceThreads *src, ServiceThreadMetrics *dest) {
    if (!src || !dest) return;
    
    dest->thread_count = src->thread_count;
    dest->thread_tids = malloc(src->thread_count * sizeof(pid_t));
    if (dest->thread_tids) {
        memcpy(dest->thread_tids, src->thread_tids, src->thread_count * sizeof(pid_t));
    }
    dest->virtual_memory = src->virtual_memory;
    dest->resident_memory = src->resident_memory;
}

// Collect metrics for all services
bool collect_service_metrics(SystemMetrics *metrics, const WebSocketMetrics *ws_metrics) {
    // Update all thread metrics first
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&web_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_server_threads);
    update_service_thread_metrics(&print_threads);

    // Logging service
    metrics->logging.enabled = true;
    convert_thread_metrics(&logging_threads, &metrics->logging.threads);
    metrics->logging.specific.logging.message_count = 0;  // TODO: Implement message counting

    // Web service
    metrics->web.enabled = (app_config->web.enable_ipv4 || app_config->web.enable_ipv6);
    convert_thread_metrics(&web_threads, &metrics->web.threads);
    metrics->web.specific.web.active_requests = 0;  // TODO: Implement request tracking
    metrics->web.specific.web.total_requests = 0;

    // WebSocket service
    metrics->websocket.enabled = app_config->websocket.enabled;
    convert_thread_metrics(&websocket_threads, &metrics->websocket.threads);
    if (ws_metrics) {
        metrics->websocket.specific.websocket.uptime = 
            time(NULL) - ws_metrics->server_start_time;
        metrics->websocket.specific.websocket.active_connections = 
            ws_metrics->active_connections;
        metrics->websocket.specific.websocket.total_connections = 
            ws_metrics->total_connections;
        metrics->websocket.specific.websocket.total_requests = 
            ws_metrics->total_requests;
    }

    // mDNS service
    metrics->mdns.enabled = app_config->mdns_server.enabled;
    convert_thread_metrics(&mdns_server_threads, &metrics->mdns.threads);
    metrics->mdns.specific.mdns.discovery_count = 0;  // TODO: Implement discovery counting

    // Print service
    metrics->print.enabled = app_config->print_queue.enabled;
    convert_thread_metrics(&print_threads, &metrics->print.threads);
    metrics->print.specific.print.queued_jobs = 0;  // TODO: Implement job counting
    metrics->print.specific.print.completed_jobs = 0;

    // Update queue metrics
    // Copy queue metrics
    metrics->log_queue.entry_count = log_queue_memory.entry_count;
    metrics->log_queue.block_count = log_queue_memory.block_count;
    metrics->log_queue.total_allocation = log_queue_memory.total_allocation;
    metrics->log_queue.virtual_bytes = log_queue_memory.metrics.virtual_bytes;
    metrics->log_queue.resident_bytes = log_queue_memory.metrics.resident_bytes;

    metrics->print_queue.entry_count = print_queue_memory.entry_count;
    metrics->print_queue.block_count = print_queue_memory.block_count;
    metrics->print_queue.total_allocation = print_queue_memory.total_allocation;
    metrics->print_queue.virtual_bytes = print_queue_memory.metrics.virtual_bytes;
    metrics->print_queue.resident_bytes = print_queue_memory.metrics.resident_bytes;

    return true;
}