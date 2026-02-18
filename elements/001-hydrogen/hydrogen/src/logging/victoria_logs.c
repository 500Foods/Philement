/*
 * VictoriaLogs Integration Implementation
 *
 * Provides threaded HTTP logging to VictoriaLogs server.
 * Uses a dedicated worker thread with intelligent dual-timer batching:
 * - Short timer (1s): Sends logs when idle
 * - Long timer (10s): Periodic flush during heavy load
 * - First log sent immediately to verify connectivity
 *
 * No dependencies on other subsystems - initializes at startup from env vars.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "victoria_logs.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// Global configuration instance
VictoriaLogsConfig victoria_logs_config = {0};

// Global thread state
VLThreadState victoria_logs_thread = {0};

// Priority level names (must match globals.h)
static const char* priority_labels[] = {
    "TRACE", "DEBUG", "STATE", "ALERT", "ERROR", "FATAL", "QUIET"
};

// Forward declarations
static void* victoria_logs_worker(void* arg);
static bool vl_queue_enqueue(const char* message);
static char* vl_queue_dequeue(void);
static bool vl_queue_init(void);
static void vl_queue_cleanup(void);
static bool send_http_post(const char* host, int port, const char* path, const char* body, size_t body_len, bool use_ssl);
static bool flush_batch_internal(void);
static bool add_to_batch(const char* message);
static void reset_long_timer(void);

/**
 * Parse log level string to numeric value
 */
int victoria_logs_parse_level(const char* level_str, int default_level) {
    if (!level_str || !level_str[0]) {
        return default_level;
    }

    // Convert to uppercase for comparison
    char upper[16];
    size_t len = strlen(level_str);
    if (len >= sizeof(upper)) {
        return default_level;
    }

    for (size_t i = 0; i < len; i++) {
        upper[i] = (char)toupper((unsigned char)level_str[i]);
    }
    upper[len] = '\0';

    if (strcmp(upper, "TRACE") == 0) return LOG_LEVEL_TRACE;
    if (strcmp(upper, "DEBUG") == 0) return LOG_LEVEL_DEBUG;
    if (strcmp(upper, "STATE") == 0) return LOG_LEVEL_STATE;
    if (strcmp(upper, "ALERT") == 0) return LOG_LEVEL_ALERT;
    if (strcmp(upper, "ERROR") == 0) return LOG_LEVEL_ERROR;
    if (strcmp(upper, "FATAL") == 0) return LOG_LEVEL_FATAL;
    if (strcmp(upper, "QUIET") == 0) return LOG_LEVEL_QUIET;

    return default_level;
}

/**
 * Get priority label for a log level (internal to victoria_logs)
 */
const char* victoria_logs_get_priority_label(int priority) {
    if (priority >= 0 && priority <= LOG_LEVEL_QUIET) {
        return priority_labels[priority];
    }
    return "STATE";
}

/**
 * Escape a string for JSON output
 */
int victoria_logs_escape_json(const char* input, char* output, size_t output_size) {
    if (!input || !output || output_size == 0) {
        return -1;
    }
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) {
        if (j >= output_size - 1) {
            return -1;  // Buffer too small
        }

        switch (input[i]) {
            case '"':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = '"';
                break;
            case '\\':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = '\\';
                break;
            case '\b':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = 'b';
                break;
            case '\f':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = 'f';
                break;
            case '\n':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = 'n';
                break;
            case '\r':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = 'r';
                break;
            case '\t':
                if (j + 2 >= output_size) return -1;
                output[j++] = '\\';
                output[j++] = 't';
                break;
            default:
                if ((unsigned char)input[i] < 0x20) {
                    if (j + 6 >= output_size) return -1;
                    snprintf(output + j, 7, "\\u%04x", (unsigned char)input[i]);
                    j += 6;
                } else {
                    output[j++] = input[i];
                }
                break;
        }
    }
    output[j] = '\0';
    return (int)j;
}

/**
 * Parse URL into components
 * Supports http://host:port/path format
 */
static bool parse_url(const char* url, char* host, int* port, char* path, bool* use_ssl) {
    if (!url || !host || !port || !path || !use_ssl) {
        return false;
    }

    *use_ssl = false;
    *port = 80;

    const char* ptr = url;

    // Check for protocol
    if (strncmp(ptr, "https://", 8) == 0) {
        *use_ssl = true;
        *port = 443;
        ptr += 8;
    } else if (strncmp(ptr, "http://", 7) == 0) {
        ptr += 7;
    }

    // Find path
    const char* path_start = strchr(ptr, '/');
    if (path_start) {
        strncpy(path, path_start, 1023);
        path[1023] = '\0';
    } else {
        strcpy(path, "/");
    }

    // Extract host and port
    size_t host_len = path_start ? (size_t)(path_start - ptr) : strlen(ptr);
    if (host_len >= 256) {
        return false;
    }

    char host_port[256];
    strncpy(host_port, ptr, host_len);
    host_port[host_len] = '\0';

    // Check for port in host:port format
    char* colon = strchr(host_port, ':');
    if (colon) {
        *colon = '\0';
        *port = atoi(colon + 1);
    }

    snprintf(host, 256, "%s", host_port);

    return true;
}

/**
 * Send HTTP POST request to VictoriaLogs
 *
 * Sends headers and body separately to avoid buffer size limitations.
 * The body can be up to VICTORIA_LOGS_MAX_BATCH_BUFFER (1MB).
 */
static bool send_http_post(const char* host, int port, const char* path, const char* body, size_t body_len, bool use_ssl) {
    (void)use_ssl;  // SSL not implemented yet, would need OpenSSL

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = VICTORIA_LOGS_TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Resolve hostname
    const struct hostent* server = gethostbyname(host);
    if (!server) {
        close(sock);
        return false;
    }

    // Connect
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, (size_t)server->h_length);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return false;
    }

    // Build HTTP headers (separate from body to avoid buffer size issues)
    char headers[1024];
    int hdr_len = snprintf(headers, sizeof(headers),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/stream+json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host, port, body_len);

    if (hdr_len < 0 || (size_t)hdr_len >= sizeof(headers)) {
        close(sock);
        return false;
    }

    // Send headers
    ssize_t sent = send(sock, headers, (size_t)hdr_len, MSG_NOSIGNAL);
    if (sent < 0) {
        close(sock);
        return false;
    }

    // Send body in chunks if needed
    size_t total_sent = 0;
    while (total_sent < body_len) {
        sent = send(sock, body + total_sent, body_len - total_sent, MSG_NOSIGNAL);
        if (sent < 0) {
            close(sock);
            return false;
        }
        total_sent += (size_t)sent;
    }

    // Read response (we don't really care about the body, just the status)
    char response[1024];
    ssize_t received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    if (received > 0) {
        response[received] = '\0';
        // Check for 204 No Content (success) or 200 OK
        if (strstr(response, "204") != NULL || strstr(response, "200") != NULL) {
            return true;
        }
    }

    return false;
}

/**
 * Initialize the VictoriaLogs message queue
 */
static bool vl_queue_init(void) {
    victoria_logs_thread.queue.head = NULL;
    victoria_logs_thread.queue.tail = NULL;
    victoria_logs_thread.queue.size = 0;
    victoria_logs_thread.queue.max_size = VICTORIA_LOGS_MAX_QUEUE_SIZE;
    
    if (pthread_mutex_init(&victoria_logs_thread.queue.mutex, NULL) != 0) {
        return false;
    }
    
    if (pthread_cond_init(&victoria_logs_thread.queue.cond, NULL) != 0) {
        pthread_mutex_destroy(&victoria_logs_thread.queue.mutex);
        return false;
    }
    
    return true;
}

/**
 * Cleanup the VictoriaLogs message queue
 */
static void vl_queue_cleanup(void) {
    pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
    
    // Free all queued messages
    VLQueueNode* node = victoria_logs_thread.queue.head;
    while (node) {
        VLQueueNode* next = node->next;
        free(node->message);
        free(node);
        node = next;
    }
    
    victoria_logs_thread.queue.head = NULL;
    victoria_logs_thread.queue.tail = NULL;
    victoria_logs_thread.queue.size = 0;
    
    pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
    
    pthread_cond_destroy(&victoria_logs_thread.queue.cond);
    pthread_mutex_destroy(&victoria_logs_thread.queue.mutex);
}

/**
 * Enqueue a message (called from log_this - must be fast and non-blocking)
 */
static bool vl_queue_enqueue(const char* message) {
    if (!message || !victoria_logs_thread.running) {
        return false;
    }
    
    // Allocate node
    VLQueueNode* node = malloc(sizeof(VLQueueNode));
    if (!node) {
        return false;
    }
    
    // Allocate and copy message
    node->message = strdup(message);
    if (!node->message) {
        free(node);
        return false;
    }
    node->next = NULL;
    
    pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
    
    // Check if queue is full
    if (victoria_logs_thread.queue.size >= victoria_logs_thread.queue.max_size) {
        pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
        free(node->message);
        free(node);
        return false;  // Queue full, drop message
    }
    
    // Add to queue
    if (victoria_logs_thread.queue.tail) {
        victoria_logs_thread.queue.tail->next = node;
    } else {
        victoria_logs_thread.queue.head = node;
    }
    victoria_logs_thread.queue.tail = node;
    victoria_logs_thread.queue.size++;
    
    // Signal the worker thread
    pthread_cond_signal(&victoria_logs_thread.queue.cond);
    
    pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
    
    return true;
}

/**
 * Dequeue a message (called from worker thread)
 * Returns NULL if queue is empty
 * Caller must free the returned message
 */
static char* vl_queue_dequeue(void) {
    pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
    
    VLQueueNode* node = victoria_logs_thread.queue.head;
    if (!node) {
        pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
        return NULL;
    }
    
    // Remove from queue
    victoria_logs_thread.queue.head = node->next;
    if (!victoria_logs_thread.queue.head) {
        victoria_logs_thread.queue.tail = NULL;
    }
    victoria_logs_thread.queue.size--;
    
    pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
    
    char* message = node->message;
    free(node);
    
    return message;
}

/**
 * Add a message to the current batch using memcpy at known offset
 *
 * Uses tracked batch_buffer_size for O(1) appends instead of strcat's O(n) scan.
 *
 * @param message The JSON message to add
 * @return true if message was added, false if buffer full
 */
static bool add_to_batch(const char* message) {
    size_t msg_len = strlen(message);

    // Track when first message was added (for retry age limit)
    if (victoria_logs_thread.batch_count == 0) {
        victoria_logs_thread.first_message_time = time(NULL);
    }

    // Add newline separator if not first message in batch
    if (victoria_logs_thread.batch_buffer_size > 0) {
        if (victoria_logs_thread.batch_buffer_size + 1 >= VICTORIA_LOGS_MAX_BATCH_BUFFER) {
            return false;
        }
        victoria_logs_thread.batch_buffer[victoria_logs_thread.batch_buffer_size] = '\n';
        victoria_logs_thread.batch_buffer_size++;
    }

    if (victoria_logs_thread.batch_buffer_size + msg_len >= VICTORIA_LOGS_MAX_BATCH_BUFFER) {
        return false;
    }

    memcpy(victoria_logs_thread.batch_buffer + victoria_logs_thread.batch_buffer_size,
           message, msg_len);
    victoria_logs_thread.batch_buffer_size += msg_len;
    victoria_logs_thread.batch_buffer[victoria_logs_thread.batch_buffer_size] = '\0';
    victoria_logs_thread.batch_count++;

    return true;
}

/**
 * Reset the long timer to fire VICTORIA_LOGS_LONG_TIMER_SEC from now
 */
static void reset_long_timer(void) {
    clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
    victoria_logs_thread.long_timer.tv_sec += VICTORIA_LOGS_LONG_TIMER_SEC;
}

/**
 * Clear the batch buffer and reset counters
 */
static void clear_batch(void) {
    victoria_logs_thread.batch_buffer[0] = '\0';
    victoria_logs_thread.batch_buffer_size = 0;
    victoria_logs_thread.batch_count = 0;
    victoria_logs_thread.first_message_time = 0;
}

/**
 * Public wrapper for clear_batch (exposed for testing)
 */
void victoria_logs_clear_batch(void) {
    clear_batch();
}

/**
 * Flush the current batch to VictoriaLogs
 *
 * On success: clears the batch.
 * On failure: keeps the batch for retry, unless the batch has been
 * failing for longer than VICTORIA_LOGS_MAX_RETRY_SEC, in which case
 * the stale batch is dropped to prevent unbounded memory use.
 *
 * This retry behavior ensures logs survive brief VL outages (restarts,
 * network blips) while not accumulating forever if VL is truly gone.
 */
static bool flush_batch_internal(void) {
    if (victoria_logs_thread.batch_count == 0 || 
        victoria_logs_thread.batch_buffer_size == 0) {
        return true;
    }
    
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;
    
    if (!parse_url(victoria_logs_config.url, host, &port, path, &use_ssl)) {
        // URL parse failure is permanent - drop the batch
        clear_batch();
        return false;
    }
    
    bool result = send_http_post(host, port, path,
        victoria_logs_thread.batch_buffer,
        victoria_logs_thread.batch_buffer_size,
        use_ssl);
    
    if (result) {
        // Success - clear batch
        clear_batch();
    } else {
        // Failed - check if batch is too old to keep retrying
        time_t now = time(NULL);
        time_t batch_age = now - victoria_logs_thread.first_message_time;
        if (batch_age > VICTORIA_LOGS_MAX_RETRY_SEC) {
            // Batch too old, drop it to prevent memory stagnation
            clear_batch();
        }
        // Otherwise keep batch intact for retry on next timer cycle
    }
    
    return result;
}

/**
 * Worker thread for VictoriaLogs
 * Implements dual-timer batching strategy:
 * 1. First log is sent immediately
 * 2. Short timer (1s): Sends when idle (resets on each new log)
 * 3. Long timer (10s): Periodic flush during heavy load
 *
 * IMPORTANT: Both timers are always reset when they expire, even if
 * there is nothing to flush. This prevents a busy-loop when idle.
 */
static void* victoria_logs_worker(void* arg) {
    (void)arg;
    
    // Initialize timers
    reset_long_timer();
    
    victoria_logs_thread.short_timer_active = false;
    victoria_logs_thread.first_log_sent = false;
    
    while (!victoria_logs_thread.shutdown) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        
        // Calculate next timeout (shortest of short_timer or long_timer)
        struct timespec timeout;
        bool have_timeout = false;
        
        if (victoria_logs_thread.short_timer_active) {
            timeout = victoria_logs_thread.short_timer;
            have_timeout = true;
        }
        
        // Use the earlier of the two timers
        if (have_timeout) {
            if (victoria_logs_thread.long_timer.tv_sec < timeout.tv_sec ||
                (victoria_logs_thread.long_timer.tv_sec == timeout.tv_sec && 
                 victoria_logs_thread.long_timer.tv_nsec < timeout.tv_nsec)) {
                timeout = victoria_logs_thread.long_timer;
            }
        } else {
            timeout = victoria_logs_thread.long_timer;
            have_timeout = true;
        }
        
        // Check if we already passed the timeout
        bool timeout_expired = false;
        if (have_timeout) {
            if (now.tv_sec > timeout.tv_sec ||
                (now.tv_sec == timeout.tv_sec && now.tv_nsec >= timeout.tv_nsec)) {
                timeout_expired = true;
            }
        }
        
        // Try to get a message (with timeout if we have one and it hasn't expired)
        char* message = NULL;
        
        if (!timeout_expired && have_timeout) {
            // Wait for message or timeout
            pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
            
            while (victoria_logs_thread.queue.head == NULL && !victoria_logs_thread.shutdown) {
                int wait_result = pthread_cond_timedwait(
                    &victoria_logs_thread.queue.cond,
                    &victoria_logs_thread.queue.mutex,
                    &timeout
                );
                
                if (wait_result == ETIMEDOUT) {
                    break;
                }
            }
            
            // Check for message
            if (victoria_logs_thread.queue.head) {
                VLQueueNode* node = victoria_logs_thread.queue.head;
                victoria_logs_thread.queue.head = node->next;
                if (!victoria_logs_thread.queue.head) {
                    victoria_logs_thread.queue.tail = NULL;
                }
                victoria_logs_thread.queue.size--;
                message = node->message;
                free(node);
            }
            
            pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
        } else {
            // No timeout or already expired, just check for messages
            message = vl_queue_dequeue();
        }
        
        // Process the message if we got one
        if (message) {
            // Check if this is the first log - send immediately
            if (!victoria_logs_thread.first_log_sent) {
                add_to_batch(message);
                if (flush_batch_internal()) {
                    victoria_logs_thread.first_log_sent = true;
                } else {
                    // First flush failed (VL not ready yet) - set short retry
                    // Keep batch for retry; set long timer to retry interval
                    clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
                    victoria_logs_thread.long_timer.tv_sec += VICTORIA_LOGS_RETRY_INTERVAL_SEC;
                }
            } else {
                // Add to batch
                add_to_batch(message);
                
                // Check if batch is full
                if (victoria_logs_thread.batch_count >= VICTORIA_LOGS_BATCH_SIZE) {
                    if (flush_batch_internal()) {
                        reset_long_timer();
                    } else {
                        // Flush failed - use shorter retry interval
                        clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
                        victoria_logs_thread.long_timer.tv_sec += VICTORIA_LOGS_RETRY_INTERVAL_SEC;
                    }
                }
            }
            
            // Reset short timer on each log
            clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.short_timer);
            victoria_logs_thread.short_timer.tv_sec += VICTORIA_LOGS_SHORT_TIMER_SEC;
            victoria_logs_thread.short_timer_active = true;
            
            free(message);
        }
        
        // Check for timer expiration
        clock_gettime(CLOCK_REALTIME, &now);
        
        // Check short timer (idle timeout)
        if (victoria_logs_thread.short_timer_active) {
            if (now.tv_sec > victoria_logs_thread.short_timer.tv_sec ||
                (now.tv_sec == victoria_logs_thread.short_timer.tv_sec && 
                 now.tv_nsec >= victoria_logs_thread.short_timer.tv_nsec)) {
                // Short timer expired - flush if there's anything and deactivate
                if (victoria_logs_thread.batch_count > 0) {
                    if (flush_batch_internal()) {
                        reset_long_timer();
                    } else {
                        // Flush failed - use shorter retry interval
                        clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
                        victoria_logs_thread.long_timer.tv_sec += VICTORIA_LOGS_RETRY_INTERVAL_SEC;
                    }
                }
                victoria_logs_thread.short_timer_active = false;
            }
        }
        
        // Check long timer (periodic flush during heavy load / retry after failure)
        if (now.tv_sec > victoria_logs_thread.long_timer.tv_sec ||
            (now.tv_sec == victoria_logs_thread.long_timer.tv_sec && 
             now.tv_nsec >= victoria_logs_thread.long_timer.tv_nsec)) {
            // Long timer expired - flush if anything pending, always reset
            if (victoria_logs_thread.batch_count > 0) {
                if (flush_batch_internal()) {
                    // Success - mark first log sent if it wasn't already
                    victoria_logs_thread.first_log_sent = true;
                    reset_long_timer();
                } else {
                    // Flush failed - retry sooner
                    clock_gettime(CLOCK_REALTIME, &victoria_logs_thread.long_timer);
                    victoria_logs_thread.long_timer.tv_sec += VICTORIA_LOGS_RETRY_INTERVAL_SEC;
                }
            } else {
                reset_long_timer();
            }
            victoria_logs_thread.short_timer_active = false;
        }
    }
    
    // Final flush before exiting
    if (victoria_logs_thread.batch_count > 0) {
        flush_batch_internal();
    }
    
    return NULL;
}

/**
 * Initialize VictoriaLogs configuration from environment variables
 */
bool init_victoria_logs(void) {
    // Check if VICTORIALOGS_URL is set
    const char* url = getenv("VICTORIALOGS_URL");
    if (!url || !url[0]) {
        victoria_logs_config.enabled = false;
        return true;  // Not an error, just disabled
    }
    
    // Validate URL format
    char host[256];
    int port;
    char path[1024];
    bool use_ssl;
    if (!parse_url(url, host, &port, path, &use_ssl)) {
        victoria_logs_config.enabled = false;
        return false;
    }
    
    victoria_logs_config.enabled = true;
    victoria_logs_config.url = strdup(url);
    
    // Parse log level
    const char* lvl = getenv("VICTORIALOGS_LVL");
    victoria_logs_config.min_level = victoria_logs_parse_level(lvl, LOG_LEVEL_DEBUG);
    
    // Cache Kubernetes metadata with fallbacks
    const char* ns = getenv("K8S_NAMESPACE");
    victoria_logs_config.k8s_namespace = strdup((ns && ns[0]) ? ns : "local");
    
    const char* pod = getenv("K8S_POD_NAME");
    if (pod && pod[0]) {
        victoria_logs_config.k8s_pod_name = strdup(pod);
    } else {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            victoria_logs_config.k8s_pod_name = strdup(hostname);
        } else {
            victoria_logs_config.k8s_pod_name = strdup("localhost");
        }
    }
    
    const char* container = getenv("K8S_CONTAINER_NAME");
    victoria_logs_config.k8s_container_name = strdup((container && container[0]) ? container : "hydrogen");
    
    const char* node = getenv("K8S_NODE_NAME");
    if (node && node[0]) {
        victoria_logs_config.k8s_node_name = strdup(node);
    } else {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            victoria_logs_config.k8s_node_name = strdup(hostname);
        } else {
            victoria_logs_config.k8s_node_name = strdup("localhost");
        }
    }
    
    // Host is same as node name for consistency
    victoria_logs_config.host = strdup(victoria_logs_config.k8s_node_name);
    
    // Initialize thread state
    victoria_logs_thread.running = false;
    victoria_logs_thread.shutdown = false;
    victoria_logs_thread.batch_buffer = NULL;
    victoria_logs_thread.batch_buffer_size = 0;
    victoria_logs_thread.batch_count = 0;
    victoria_logs_thread.short_timer_active = false;
    victoria_logs_thread.first_log_sent = false;
    
    // Allocate batch buffer
    victoria_logs_thread.batch_buffer = malloc(VICTORIA_LOGS_MAX_BATCH_BUFFER);
    if (!victoria_logs_thread.batch_buffer) {
        cleanup_victoria_logs();
        return false;
    }
    victoria_logs_thread.batch_buffer[0] = '\0';
    
    // Initialize queue
    if (!vl_queue_init()) {
        cleanup_victoria_logs();
        return false;
    }
    
    // Start worker thread
    if (pthread_create(&victoria_logs_thread.thread, NULL, victoria_logs_worker, NULL) != 0) {
        cleanup_victoria_logs();
        return false;
    }
    
    victoria_logs_thread.running = true;
    
    return true;
}

/**
 * Cleanup VictoriaLogs configuration
 */
void cleanup_victoria_logs(void) {
    // Signal shutdown
    victoria_logs_thread.shutdown = true;
    
    // Wake up the worker thread
    if (victoria_logs_thread.running) {
        pthread_mutex_lock(&victoria_logs_thread.queue.mutex);
        pthread_cond_signal(&victoria_logs_thread.queue.cond);
        pthread_mutex_unlock(&victoria_logs_thread.queue.mutex);
        
        // Wait for thread to finish
        pthread_join(victoria_logs_thread.thread, NULL);
        victoria_logs_thread.running = false;
    }
    
    // Cleanup queue
    vl_queue_cleanup();
    
    // Free batch buffer
    if (victoria_logs_thread.batch_buffer) {
        free(victoria_logs_thread.batch_buffer);
        victoria_logs_thread.batch_buffer = NULL;
    }
    
    // Free config strings
    free(victoria_logs_config.url);
    free(victoria_logs_config.k8s_namespace);
    free(victoria_logs_config.k8s_pod_name);
    free(victoria_logs_config.k8s_container_name);
    free(victoria_logs_config.k8s_node_name);
    free(victoria_logs_config.host);
    
    memset(&victoria_logs_config, 0, sizeof(victoria_logs_config));
    memset(&victoria_logs_thread, 0, sizeof(victoria_logs_thread));
}

/**
 * Check if VictoriaLogs is enabled
 */
bool victoria_logs_is_enabled(void) {
    return victoria_logs_config.enabled && victoria_logs_thread.running;
}

/**
 * Send a log message to VictoriaLogs
 *
 * This function quickly formats and enqueues the message.
 * It is non-blocking and returns immediately.
 */
bool victoria_logs_send(const char* subsystem, const char* message, int priority) {
    if (!victoria_logs_config.enabled || !victoria_logs_thread.running) {
        return false;
    }
    
    // Check log level
    if (priority < victoria_logs_config.min_level) {
        return true;  // Silently skip
    }
    
    // Get current timestamp with nanoseconds (thread-safe)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    const struct tm* tm_info = gmtime_r(&ts.tv_sec, &tm_buf);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
    snprintf(timestamp + 19, sizeof(timestamp) - 19, ".%09ldZ", ts.tv_nsec);
    
    const char* level_label = victoria_logs_get_priority_label(priority);
    
    // Escape the message for JSON
    char escaped_message[VICTORIA_LOGS_MAX_MESSAGE_SIZE];
    if (victoria_logs_escape_json(message, escaped_message, sizeof(escaped_message)) < 0) {
        return false;  // Message too large
    }
    
    // Build JSON message
    char json[VICTORIA_LOGS_MAX_MESSAGE_SIZE];
    int json_len = snprintf(json, sizeof(json),
        "{"
        "\"_time\":\"%s\","
        "\"_msg\":\"%s\","
        "\"level\":\"%s\","
        "\"subsystem\":\"%s\","
        "\"app\":\"hydrogen\","
        "\"kubernetes_namespace\":\"%s\","
        "\"kubernetes_pod_name\":\"%s\","
        "\"kubernetes_container_name\":\"%s\","
        "\"kubernetes_node_name\":\"%s\","
        "\"host\":\"%s\""
        "}",
        timestamp,
        escaped_message,
        level_label,
        subsystem,
        victoria_logs_config.k8s_namespace,
        victoria_logs_config.k8s_pod_name,
        victoria_logs_config.k8s_container_name,
        victoria_logs_config.k8s_node_name,
        victoria_logs_config.host
    );
    
    if (json_len < 0 || (size_t)json_len >= sizeof(json)) {
        return false;
    }
    
    // Enqueue the message (non-blocking)
    return vl_queue_enqueue(json);
}

/**
 * Flush any pending logs immediately (used during shutdown)
 */
void victoria_logs_flush(void) {
    if (!victoria_logs_thread.running) {
        return;
    }
    
    // Process all remaining queued messages
    char* message;
    while ((message = vl_queue_dequeue()) != NULL) {
        add_to_batch(message);
        free(message);
    }
    
    // Flush the batch
    if (victoria_logs_thread.batch_count > 0) {
        flush_batch_internal();
    }
}
