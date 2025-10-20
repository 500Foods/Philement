/*
 * Process Metrics Collection Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "status_process.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

// External service thread structures
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// External queue memory structures
extern QueueMemoryMetrics log_queue_memory;
extern QueueMemoryMetrics webserver_queue_memory;
extern QueueMemoryMetrics websocket_queue_memory;
extern QueueMemoryMetrics mdns_server_queue_memory;
extern QueueMemoryMetrics print_queue_memory;
extern QueueMemoryMetrics database_queue_memory;
extern QueueMemoryMetrics mail_relay_queue_memory;
extern QueueMemoryMetrics notify_queue_memory;

// Forward declarations
static bool collect_process_memory(size_t *vmsize, size_t *vmrss, size_t *vmswap);

// Helper function to safely truncate strings
// Exposed for testing - was previously static
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
        log_this(SR_STATUS, "Failed to open /proc/self/status", LOG_LEVEL_ERROR, 0);
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
void get_socket_info(ino_t inode, char *proto, int *port) {
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
        if (!fgets(line, sizeof(line), f)) { // Skip header
            fclose(f);
            continue;
        }
        
        while (fgets(line, sizeof(line), f)) {
            unsigned local_port;
            ino_t socket_inode;
            if (sscanf(line, "%*x: %*x:%x %*x:%*x %*x %*x:%*x %*x:%*x %*x %*d %*d %lu",
                &local_port, &socket_inode) == 2) {
                if (socket_inode == inode) {
                    *port = (int)local_port;
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
            if (port == app_config->webserver.port) service = SR_WEBSERVER;
            else if (port == app_config->websocket.port) service = SR_WEBSOCKET;
            else if (port == 5353) service = SR_MDNS_SERVER;
            
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
                    const struct sockaddr_un *un = (struct sockaddr_un*)&addr;
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
        log_this(SR_STATUS, "Failed to open /proc/self/fd", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // First pass to count file descriptors
    const struct dirent *ent;
    *count = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] != '.') (*count)++;
    }

    // Allocate array for file descriptors
    *descriptors = calloc((size_t)*count, sizeof(FileDescriptorInfo));
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
// Exposed for testing - was previously static
void convert_thread_metrics(const ServiceThreads *src, ServiceThreadMetrics *dest) {
    if (!src || !dest) return;
    
    dest->thread_count = src->thread_count;
    dest->thread_tids = malloc((size_t)src->thread_count * sizeof(pid_t));
    if (dest->thread_tids) {
        memcpy(dest->thread_tids, src->thread_tids, (size_t)src->thread_count * sizeof(pid_t));
    }
    dest->virtual_memory = src->virtual_memory;
    dest->resident_memory = src->resident_memory;
}

// Collect metrics for all services
bool collect_service_metrics(SystemMetrics *metrics, const WebSocketMetrics *ws_metrics) {
    // Update all thread metrics first
    update_service_thread_metrics(&logging_threads);
    update_service_thread_metrics(&webserver_threads);
    update_service_thread_metrics(&websocket_threads);
    update_service_thread_metrics(&mdns_server_threads);
    update_service_thread_metrics(&print_threads);

    // Logging service
    metrics->logging.enabled = true;
    convert_thread_metrics(&logging_threads, &metrics->logging.threads);
    metrics->logging.specific.logging.message_count = (int)log_queue_memory.entry_count;

    // Web service
    metrics->webserver.enabled = (app_config->webserver.enable_ipv4 || app_config->webserver.enable_ipv6);
    convert_thread_metrics(&webserver_threads, &metrics->webserver.threads);
    metrics->webserver.specific.webserver.active_requests = (int)webserver_queue_memory.entry_count;
    metrics->webserver.specific.webserver.total_requests = (int)webserver_queue_memory.entry_count;

    // WebSocket service
    metrics->websocket.enabled = (app_config->websocket.enable_ipv4 || app_config->websocket.enable_ipv6);
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
    metrics->mdns.enabled = (app_config->mdns_server.enable_ipv4 || app_config->mdns_server.enable_ipv6);
    convert_thread_metrics(&mdns_server_threads, &metrics->mdns.threads);
    metrics->mdns.specific.mdns.discovery_count = (int)mdns_server_queue_memory.entry_count;

    // Print service
    metrics->print.enabled = app_config->print.enabled;
    convert_thread_metrics(&print_threads, &metrics->print.threads);
    metrics->print.specific.print.queued_jobs = (int)print_queue_memory.entry_count;
    metrics->print.specific.print.completed_jobs = 0;

    // Update queue metrics
    metrics->log_queue.entry_count = (int)log_queue_memory.entry_count;
    metrics->log_queue.block_count = (int)log_queue_memory.block_count;
    metrics->log_queue.total_allocation = log_queue_memory.total_allocation;
    metrics->log_queue.virtual_bytes = log_queue_memory.metrics.virtual_bytes;
    metrics->log_queue.resident_bytes = log_queue_memory.metrics.resident_bytes;

    metrics->webserver_queue.entry_count = (int)webserver_queue_memory.entry_count;
    metrics->webserver_queue.block_count = (int)webserver_queue_memory.block_count;
    metrics->webserver_queue.total_allocation = webserver_queue_memory.total_allocation;
    metrics->webserver_queue.virtual_bytes = webserver_queue_memory.metrics.virtual_bytes;
    metrics->webserver_queue.resident_bytes = webserver_queue_memory.metrics.resident_bytes;

    metrics->websocket_queue.entry_count = (int)websocket_queue_memory.entry_count;
    metrics->websocket_queue.block_count = (int)websocket_queue_memory.block_count;
    metrics->websocket_queue.total_allocation = websocket_queue_memory.total_allocation;
    metrics->websocket_queue.virtual_bytes = websocket_queue_memory.metrics.virtual_bytes;
    metrics->websocket_queue.resident_bytes = websocket_queue_memory.metrics.resident_bytes;

    metrics->mdns_server_queue.entry_count = (int)mdns_server_queue_memory.entry_count;
    metrics->mdns_server_queue.block_count = (int)mdns_server_queue_memory.block_count;
    metrics->mdns_server_queue.total_allocation = mdns_server_queue_memory.total_allocation;
    metrics->mdns_server_queue.virtual_bytes = mdns_server_queue_memory.metrics.virtual_bytes;
    metrics->mdns_server_queue.resident_bytes = mdns_server_queue_memory.metrics.resident_bytes;

    metrics->print_queue.entry_count = (int)print_queue_memory.entry_count;
    metrics->print_queue.block_count = (int)print_queue_memory.block_count;
    metrics->print_queue.total_allocation = print_queue_memory.total_allocation;
    metrics->print_queue.virtual_bytes = print_queue_memory.metrics.virtual_bytes;
    metrics->print_queue.resident_bytes = print_queue_memory.metrics.resident_bytes;

    metrics->database_queue.entry_count = (int)database_queue_memory.entry_count;
    metrics->database_queue.block_count = (int)database_queue_memory.block_count;
    metrics->database_queue.total_allocation = database_queue_memory.total_allocation;
    metrics->database_queue.virtual_bytes = database_queue_memory.metrics.virtual_bytes;
    metrics->database_queue.resident_bytes = database_queue_memory.metrics.resident_bytes;

    metrics->mail_relay_queue.entry_count = (int)mail_relay_queue_memory.entry_count;
    metrics->mail_relay_queue.block_count = (int)mail_relay_queue_memory.block_count;
    metrics->mail_relay_queue.total_allocation = mail_relay_queue_memory.total_allocation;
    metrics->mail_relay_queue.virtual_bytes = mail_relay_queue_memory.metrics.virtual_bytes;
    metrics->mail_relay_queue.resident_bytes = mail_relay_queue_memory.metrics.resident_bytes;

    metrics->notify_queue.entry_count = (int)notify_queue_memory.entry_count;
    metrics->notify_queue.block_count = (int)notify_queue_memory.block_count;
    metrics->notify_queue.total_allocation = notify_queue_memory.total_allocation;
    metrics->notify_queue.virtual_bytes = notify_queue_memory.metrics.virtual_bytes;
    metrics->notify_queue.resident_bytes = notify_queue_memory.metrics.resident_bytes;

    return true;
}
