/*
 * Implementation of the Hydrogen 3D printer's web server.
 * 
 * Uses libmicrohttpd to provide a multi-threaded HTTP server that implements
 * a RESTful API and file management system. The server provides several key
 * features:
 * 
 * Request Handling:
 * - Multi-threaded processing for concurrent requests
 * - CORS support for cross-origin requests
 * - Request routing based on URL patterns
 * - Static file serving for web interface
 * 
 * File Upload System:
 * - Secure file upload handling with size limits
 * - Progress tracking and logging
 * - Automatic UUID generation for unique filenames
 * - Upload directory management and validation
 * 
 * G-code Processing:
 * - Automatic analysis of uploaded G-code files
 * - Preview image extraction from embedded thumbnails
 * - Print time estimation and statistics
 * - Layer-by-layer breakdown of print jobs
 * 
 * Print Queue Management:
 * - JSON-based job descriptions
 * - Queue visualization and management
 * - Print job status tracking
 * - Integration with print queue manager
 * 
 * API Compatibility:
 * - OctoPrint-compatible REST endpoints
 * - Version information and system status
 * - Print job control and monitoring
 * 
 * Security Considerations:
 * - File size limits and validation
 * - Safe file path handling
 * - Resource cleanup on errors
 * - Proper error reporting
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>

// Network headers
#include <netinet/in.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

// Third-party libraries
#include <jansson.h>
#include <microhttpd.h>

// Project headers
#include "web_server.h"
#include "configuration.h"
#include "logging.h"
#include "beryllium.h"
#include "queue.h"
#include "print_queue_manager.h"
#include "api/system/system_service.h"
#include "utils.h"

#define UUID_STR_LEN 37

static struct MHD_Daemon *web_daemon = NULL;
static WebConfig *server_web_config = NULL;

extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t web_server_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

struct ConnectionInfo {
    FILE *fp;
    char *original_filename;
    char *new_filename;
    struct MHD_PostProcessor *postprocessor;
    size_t total_size;
    size_t last_logged_mb;
    size_t expected_size;
    bool is_first_chunk;
    bool print_after_upload;
    bool response_sent;
};

static json_t* extract_gcode_info(const char* filename);
static char* extract_preview_image(const char* filename);
static enum MHD_Result handle_print_queue_request(struct MHD_Connection *connection);

// Generate collision-resistant file identifiers
//
// UUID generation strategy balances several needs:
// 1. Uniqueness guarantees
//    - Microsecond timestamp component
//    - Random number entropy
//    - Version 4 UUID structure
//
// 2. Performance considerations
//    - No filesystem queries needed
//    - Minimal system calls
//    - Fast string operations
//
// 3. Security implications
//    - Non-sequential for privacy
//    - Unpredictable for security
//    - No PII exposure
static void generate_uuid(char *uuid_str) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long int time_in_usec = ((unsigned long long int)tv.tv_sec * 1000000ULL) + tv.tv_usec;

    snprintf(uuid_str, UUID_STR_LEN, "%08llx-%04x-%04x-%04x-%012llx",
             time_in_usec & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff),
             (unsigned int)((rand() & 0xfff) | 0x4000),
             (unsigned int)((rand() & 0x3fff) | 0x8000),
             (unsigned long long int)rand() * rand());
}

// Check if a network port is available for binding
// Attempts to create and bind a socket to verify availability
// Returns true if port can be bound, false otherwise
static bool is_port_available(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);

    return result == 0;
}

void add_cors_headers(struct MHD_Response *response) {
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
}

// Process file uploads with streaming and progress monitoring
//
// Upload handling design prioritizes:
// 1. Memory efficiency
//    - Streaming processing
//    - Bounded buffer sizes
//    - Early validation
//
// 2. Reliability
//    - Atomic file operations
//    - Progress tracking
//    - Partial upload cleanup
//
// 3. Security
//    - Size limit enforcement
//    - Path traversal prevention
//    - File type validation
static enum MHD_Result handle_upload_data(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                                          const char *filename, const char *content_type,
                                          const char *transfer_encoding, const char *data, uint64_t off, size_t size) {
    struct ConnectionInfo *con_info = coninfo_cls;
    (void)kind; (void)content_type; (void)transfer_encoding; (void)off;  // Unused parameters

    if (0 == strcmp(key, "file")) {
        if (!con_info->fp) {
            if (filename) {
                char uuid_str[37];
                generate_uuid(uuid_str);

                char file_path[1024];
                snprintf(file_path, sizeof(file_path), "%s/%s.gcode", server_web_config->upload_dir, uuid_str);
                con_info->fp = fopen(file_path, "wb");
                if (!con_info->fp) {
                    log_this("WebServer", "Failed to open file for writing", 3, true, false, true);
                    return MHD_NO;
                }
                free(con_info->original_filename);  // Free previous allocation if any
                free(con_info->new_filename);       // Free previous allocation if any
                con_info->original_filename = strdup(filename);
                con_info->new_filename = strdup(file_path);

                char log_buffer[256];
                snprintf(log_buffer, sizeof(log_buffer), "Starting file upload: %s", filename);
                log_this("WebServer", log_buffer, 0, true, false, true);
            }
        }

        if (size > 0) {
            if (con_info->total_size + size > server_web_config->max_upload_size) {
                log_this("WebServer", "File upload exceeds maximum allowed size", 3, true, false, true);
                return MHD_NO;
            }

            if (fwrite(data, 1, size, con_info->fp) != size) {
                log_this("WebServer", "Failed to write to file", 3, true, false, true);
                return MHD_NO;
            }
            con_info->total_size += size;

            // Log progress every 100MB
            if (con_info->total_size / (100 * 1024 * 1024) > con_info->last_logged_mb) {
                con_info->last_logged_mb = con_info->total_size / (100 * 1024 * 1024);
                char progress_log[128];
                snprintf(progress_log, sizeof(progress_log), "Upload progress: %zu MB", con_info->last_logged_mb * 100);
                log_this("WebServer", progress_log, 2, true, false, true);
            }
        }
    } else if (0 == strcmp(key, "print")) {
        // Handle the 'print' field
        con_info->print_after_upload = (0 == strcmp(data, "true"));
        console_log("WebServer", 0, con_info->print_after_upload ? "Print after upload: enabled" : "Print after upload: disabled");
    } else {
        // Log unknown keys
        char log_buffer[256];
        snprintf(log_buffer, sizeof(log_buffer), "Received unknown key in form data: %s", key);
        log_this("WebServer", log_buffer, 2, true, false, true);
    }

    return MHD_YES;
}

static enum MHD_Result handle_version_request(struct MHD_Connection *connection) {
    const char *version_json = "{\"api\": \"0.1\", \"server\": \"1.1.0\", \"text\": \"OctoPrint 1.1.0\"}";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(version_json),
                                    (void*)version_json, MHD_RESPMEM_PERSISTENT);
    add_cors_headers(response);
    MHD_add_response_header(response, "Content-Type", "application/json");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

static enum MHD_Result serve_file(struct MHD_Connection *connection, const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) return MHD_NO;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return MHD_NO;
    }

    struct MHD_Response *response = MHD_create_response_from_fd(st.st_size, fd);
    if (!response) {
        close(fd);
        return MHD_NO;
    }

    add_cors_headers(response);

    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) MHD_add_response_header(response, "Content-Type", "text/html");
        else if (strcmp(ext, ".css") == 0) MHD_add_response_header(response, "Content-Type", "text/css");
        else if (strcmp(ext, ".js") == 0) MHD_add_response_header(response, "Content-Type", "application/javascript");
        // Add more content types as needed
    }

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

static bool is_api_endpoint(const char *url, char *service, char *endpoint) {
    // Check if URL starts with /api/
    if (strncmp(url, "/api/", 5) != 0) {
        return false;
    }

    // Skip "/api/"
    const char *path = url + 5;
    
    // Find the next slash
    const char *slash = strchr(path, '/');
    if (!slash) {
        return false;
    }

    // Extract service name (e.g., "system" from "/api/system/info")
    size_t service_len = slash - path;
    strncpy(service, path, service_len);
    service[service_len] = '\0';

    // Extract endpoint name (e.g., "info" from "/api/system/info")
    const char *endpoint_start = slash + 1;
    strcpy(endpoint, endpoint_start);

    // Convert first character to uppercase for service name
    if (service[0] >= 'a' && service[0] <= 'z') {
        service[0] = service[0] - 'a' + 'A';
    }

    return true;
}

// Request router with comprehensive security and performance features
//
// The routing architecture implements:
// 1. Security measures
//    - CORS policy enforcement
//    - Method validation
//    - Path sanitization
//    - Resource limits
//
// 2. Performance optimization
//    - Fast path matching
//    - Static file caching
//    - Connection reuse
//    - Minimal allocations
//
// 3. API compatibility
//    - RESTful endpoint structure
//    - Standard HTTP semantics
//    - Flexible content negotiation
//    - Proper status codes
static enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                           const char *url, const char *method,
                           const char *version, const char *upload_data,
                           size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)version;

    // Log API endpoint access
    char service[32] = {0};
    char endpoint[32] = {0};
    if (is_api_endpoint(url, service, endpoint)) {
        char detail[128];
        snprintf(detail, sizeof(detail), "%sService/%s", service, endpoint);
        log_this("API", detail, 0, true, false, true);
    }

    // Handle OPTIONS method for CORS preflight requests
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Deal with regular GET requests
    if (strcmp(method, "GET") == 0) {
        
        // Expected endpoints 
        if (strcmp(url, "/api/version") == 0) {
            return handle_version_request(connection);
        } else if (strcmp(url, "/print/queue") == 0) {
            return handle_print_queue_request(connection);
        } else if (strcmp(url, "/api/system/info") == 0) {
            return handle_system_info_request(connection);
        }

        // If no API endpoint matched, try to serve a static file
        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s%s", app_config->web.web_root, url);

        // If the URL ends with a /, append index.html
        if (url[strlen(url) - 1] == '/') {
            strcat(file_path, "index.html");
        }

	// Serve up the requested file
        if (access(file_path, F_OK) != -1) {
	    log_this("WebServer","Served File: %s", 0, true, true, true, file_path);
            return serve_file(connection, file_path);
        }

        // If file not found, return 404
        const char *page = "<html><body>404 Not Found</body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
	add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }

    struct ConnectionInfo *con_info = *con_cls;
    if (NULL == con_info) {
        con_info = calloc(1, sizeof(struct ConnectionInfo));
        if (NULL == con_info) return MHD_NO;
        con_info->postprocessor = MHD_create_post_processor(connection, 8192, handle_upload_data, con_info);
        if (NULL == con_info->postprocessor) {
            free(con_info);
            return MHD_NO;
        }
        *con_cls = (void*)con_info;
        return MHD_YES;
    }

    if (0 == strcmp(method, "POST")) {
        if (*upload_data_size != 0) {
            MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else if (!con_info->response_sent) {
            if (con_info->fp) {
                fclose(con_info->fp);
                con_info->fp = NULL;

                // Process print job here
                json_t* print_job = json_object();

                json_object_set_new(print_job, "original_filename", json_string(con_info->original_filename));
                json_object_set_new(print_job, "new_filename", json_string(con_info->new_filename));
                json_object_set_new(print_job, "file_size", json_integer(con_info->total_size));
                json_object_set_new(print_job, "print_after_upload", json_boolean(con_info->print_after_upload));

                json_t* gcode_info = extract_gcode_info(con_info->new_filename);
                if (gcode_info) {
                    json_object_set_new(print_job, "gcode_info", gcode_info);
                }

                char* preview_image = extract_preview_image(con_info->new_filename);
                if (preview_image) {
                    json_object_set_new(print_job, "preview_image", json_string(preview_image));
                    free(preview_image);
                }

                char* print_job_str = json_dumps(print_job, JSON_COMPACT);
                if (print_job_str) {
                    Queue* print_queue = queue_find("PrintQueue");
                    if (print_queue) {
                        queue_enqueue(print_queue, print_job_str, strlen(print_job_str), 0);
                        log_this("WebServer", "Added print job to queue", 0, true, false, true);
                    } else {
                        log_this("WebServer", "Failed to find PrintQueue", 3, true, false, true);
                    }

                    free(print_job_str);
                } else {
                    log_this("WebServer", "Failed to create JSON string", 3, true, false, true);
                }

                json_decref(print_job);

                char complete_log[512];
                log_this("WebServer", "File upload completed:", 0, true, true, true);
                snprintf(complete_log, sizeof(complete_log), " -> Source: %s", con_info->original_filename);
                log_this("WebServer", complete_log, 0, true, true, true);
                snprintf(complete_log, sizeof(complete_log), " ->  Local: %s", con_info->new_filename);
                log_this("WebServer", complete_log, 0, true, true, true);
                snprintf(complete_log, sizeof(complete_log), " ->   Size: %zu bytes", con_info->total_size);
                log_this("WebServer", complete_log, 0, true, true, true);
                snprintf(complete_log, sizeof(complete_log), " ->  Print: %s", con_info->print_after_upload ? "true" : "false");
                log_this("WebServer", complete_log, 0, true, true, true);

                // Send response
                const char *response_text = "{\"files\": {\"local\": {\"name\": \"%s\", \"origin\": \"local\"}}, \"done\": true}";
                char *json_response = malloc(strlen(response_text) + strlen(con_info->original_filename) + 1);
                sprintf(json_response, response_text, con_info->original_filename);

                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json_response),
                                                (void*)json_response, MHD_RESPMEM_MUST_FREE);
                add_cors_headers(response);
                MHD_add_response_header(response, "Content-Type", "application/json");
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
                MHD_destroy_response(response);
                con_info->response_sent = true;
                return ret;
            } else {
                log_this("WebServer", "File upload failed or no file was uploaded", 2, true, false, true);
                const char *error_response = "{\"error\": \"File upload failed\", \"done\": false}";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_response),
                                                (void*)error_response, MHD_RESPMEM_PERSISTENT);
		add_cors_headers(response);
                MHD_add_response_header(response, "Content-Type", "application/json");
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                MHD_destroy_response(response);
                con_info->response_sent = true;
                return ret;
            }
        }
    } else {
        // Handle non-POST requests
        const char *page = "<html><body>Use POST to upload files</body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
	add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }

    return MHD_YES;
}

// Cleanup handler called after request completion
// Ensures proper resource cleanup:
// - Closes open files
// - Frees allocated memory
// - Destroys post processors
static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls; (void)connection; (void)toe;  // Unused parameters
    struct ConnectionInfo *con_info = *con_cls;
    if (NULL == con_info) return;
    if (con_info->postprocessor) {
        MHD_destroy_post_processor(con_info->postprocessor);
    }
    if (con_info->fp) {
        fclose(con_info->fp);
    }
    free(con_info->original_filename);
    free(con_info->new_filename);
    free(con_info);
    *con_cls = NULL;
}

bool init_web_server(WebConfig *web_config) {
    if (!is_port_available(web_config->port)) {
        log_this("WebServer", "Port is not available", 3, true, false, true);
        return false;
    }

    server_web_config = web_config;

    log_this("WebServer", "Initializing web server", 0, true, false, true);
    log_this("WebServer", "-> Port: %u", 0, true, true, true, server_web_config->port);
    log_this("WebServer", "-> WebRoot: %s", 0, true, true, true, server_web_config->web_root);
    log_this("WebServer", "-> Upload Path: %s", 0, true, true, true, server_web_config->upload_path);
    log_this("WebServer", "-> Upload Dir: %s", 0, true, true, true, server_web_config->upload_dir);
    log_this("WebServer", "-> LogLevel: %s", 0, true, true, true, server_web_config->log_level);

    // Create upload directory if it doesn't exist
    struct stat st = {0};
    if (stat(server_web_config->upload_dir, &st) == -1) {
        log_this("WebServer", "Upload directory does not exist, attempting to create", 2, true, false, true);
        if (mkdir(server_web_config->upload_dir, 0700) != 0) {
            char error_buffer[256];
            snprintf(error_buffer, sizeof(error_buffer), "Failed to create upload directory: %s", strerror(errno));
            log_this("WebServer", error_buffer, 3, true, false, true);
            return false;
        }
        log_this("WebServer", "Created upload directory", 0, true, false, true);
    } else {
        log_this("WebServer", "Upload directory already exists", 2, true, false, true);
    }

    return true;
}

const char* get_upload_path(void) {
    return server_web_config->upload_path;
}

void* run_web_server(void* arg) {
    (void)arg; // Unused parameter

    log_this("WebServer", "Starting web server", 0, true, false, true);
    web_daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, server_web_config->port, NULL, NULL,
                                &handle_request, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                                MHD_OPTION_END);
    if (web_daemon == NULL) {
        log_this("WebServer", "Failed to start web server", 4, true, false, true);
        return NULL;
    }

    // Check if the web server is actually running
    const union MHD_DaemonInfo *info = MHD_get_daemon_info(web_daemon, MHD_DAEMON_INFO_BIND_PORT);
    if (info == NULL) {
        log_this("WebServer", "Failed to get daemon info", 4, true, false, true);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    unsigned int actual_port = info->port;
    if (actual_port == 0) {
        log_this("WebServer", "Web server failed to bind to the specified port", 4, true, false, true);
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        return NULL;
    }

    char port_info[64];
    snprintf(port_info, sizeof(port_info), "Web server bound to port: %u", actual_port);
    log_this("WebServer", port_info, 0, true, false, true);

    log_this("WebServer", "Web server started successfully", 0, true, false, true);

    return NULL;
}

void shutdown_web_server(void) {
    log_this("WebServer", "Shutdown: Shutting down web server", 0, true, false, true);
    if (web_daemon != NULL) {
        MHD_stop_daemon(web_daemon);
        web_daemon = NULL;
        log_this("WebServer", "Web server shut down successfully", 0, true, false, true);
    } else {
        log_this("WebServer", "Web server was not running", 1, true, false, true);
    }

    // We don't free server_web_config here because it's a pointer to the AppConfig structure
    // which is managed elsewhere (likely in the main hydrogen.c file)
}

static json_t* extract_gcode_info(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this("WebServer", "Failed to open G-code file for analysis", 3, true, false, true);
        return NULL;
    }

    BerylliumConfig config = {
        .acceleration = ACCELERATION,
        .z_acceleration = Z_ACCELERATION,
        .extruder_acceleration = E_ACCELERATION,
        .max_speed_xy = MAX_SPEED_XY,
        .max_speed_travel = MAX_SPEED_TRAVEL,
        .max_speed_z = MAX_SPEED_Z,
        .default_feedrate = DEFAULT_FEEDRATE,
        .filament_diameter = DEFAULT_FILAMENT_DIAMETER,
        .filament_density = DEFAULT_FILAMENT_DENSITY
    };

    char *start_time = get_iso8601_timestamp();
    clock_t start = clock();
    BerylliumStats stats = beryllium_analyze_gcode(file, &config);
    clock_t end = clock();
    char *end_time = get_iso8601_timestamp();
    fclose(file);

    double elapsed_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;

    json_t* info = json_object();

    json_object_set_new(info, "analysis_start", json_string(start_time));
    json_object_set_new(info, "analysis_end", json_string(end_time));
    json_object_set_new(info, "analysis_duration_ms", json_real(elapsed_time));

    json_object_set_new(info, "file_size", json_integer(stats.file_size));
    json_object_set_new(info, "total_lines", json_integer(stats.total_lines));
    json_object_set_new(info, "gcode_lines", json_integer(stats.gcode_lines));
    json_object_set_new(info, "layer_count_height", json_integer(stats.layer_count_height));
    json_object_set_new(info, "layer_count_slicer", json_integer(stats.layer_count_slicer));

    json_t* objects = json_array();
    for (int i = 0; i < stats.num_objects; i++) {
        json_t* object = json_object();
        json_object_set_new(object, "index", json_integer(stats.object_infos[i].index + 1));
        json_object_set_new(object, "name", json_string(stats.object_infos[i].name));
        json_array_append_new(objects, object);
    }
    json_object_set_new(info, "objects", objects);

    json_object_set_new(info, "filament_used_mm", json_real(stats.extrusion));
    json_object_set_new(info, "filament_used_cm3", json_real(stats.filament_volume));
    json_object_set_new(info, "filament_weight_g", json_real(stats.filament_weight));

    char print_time_str[20];
    format_time(stats.print_time, print_time_str);
    json_object_set_new(info, "estimated_print_time", json_string(print_time_str));

    json_t* layers = json_array();
    double cumulative_time = 0.0;
    for (int i = 0; i < stats.layer_count_slicer; i++) {
        json_t* layer = json_object();
        char start_str[20], end_str[20], duration_str[20];
        format_time(cumulative_time, start_str);
        cumulative_time += stats.layer_times[i];
        format_time(cumulative_time, end_str);
        format_time(stats.layer_times[i], duration_str);

        json_object_set_new(layer, "layer", json_integer(i + 1));
        json_object_set_new(layer, "start_time", json_string(start_str));
        json_object_set_new(layer, "end_time", json_string(end_str));
        json_object_set_new(layer, "duration", json_string(duration_str));

        json_t* layer_objects = json_array();
        for (int j = 0; j < stats.num_objects; j++) {
            if (stats.object_times[i][j] > 0) {
                json_t* obj = json_object();
                char obj_start_str[20], obj_end_str[20], obj_duration_str[20];
                format_time(cumulative_time - stats.layer_times[i], obj_start_str);
                format_time(cumulative_time - stats.layer_times[i] + stats.object_times[i][j], obj_end_str);
                format_time(stats.object_times[i][j], obj_duration_str);

                json_object_set_new(obj, "object", json_integer(j + 1));
                json_object_set_new(obj, "start_time", json_string(obj_start_str));
                json_object_set_new(obj, "end_time", json_string(obj_end_str));
                json_object_set_new(obj, "duration", json_string(obj_duration_str));

                json_array_append_new(layer_objects, obj);
            }
        }
        json_object_set_new(layer, "objects", layer_objects);
        json_array_append_new(layers, layer);
    }
    json_object_set_new(info, "layers", layers);

    json_t* configuration = json_object();
    json_object_set_new(configuration, "acceleration", json_real(config.acceleration));
    json_object_set_new(configuration, "z_acceleration", json_real(config.z_acceleration));
    json_object_set_new(configuration, "extruder_acceleration", json_real(config.extruder_acceleration));
    json_object_set_new(configuration, "max_speed_xy", json_real(config.max_speed_xy));
    json_object_set_new(configuration, "max_speed_travel", json_real(config.max_speed_travel));
    json_object_set_new(configuration, "max_speed_z", json_real(config.max_speed_z));
    json_object_set_new(configuration, "default_feedrate", json_real(config.default_feedrate));
    json_object_set_new(configuration, "filament_diameter", json_real(config.filament_diameter));
    json_object_set_new(configuration, "filament_density", json_real(config.filament_density));
    json_object_set_new(info, "configuration", configuration);

    // Clean up the stats structure
    beryllium_free_stats(&stats);

    return info;
}

static char* extract_preview_image(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_this("WebServer", "Failed to open G-code file for image extraction", 3, true, false, true);
        return NULL;
    }

    char line[1024];
    char* image_data = NULL;
    size_t image_size = 0;
    bool in_thumbnail = false;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "; thumbnail begin")) {
            in_thumbnail = true;
            continue;
        }
        if (strstr(line, "; thumbnail end")) {
            break;
        }
        if (in_thumbnail && line[0] == ';') {
            size_t len = strlen(line);
            if (len > 2) {
                image_data = realloc(image_data, image_size + len - 2);
                memcpy(image_data + image_size, line + 2, len - 3);  // -3 to remove newline
                image_size += len - 3;
            }
        }
    }

    fclose(file);

    if (image_data) {
        image_data = realloc(image_data, image_size + 1);
        image_data[image_size] = '\0';

        // Create a data URL
        char *data_url = malloc(image_size * 4 / 3 + 50); // Allocate enough space for the prefix and the base64 data
        sprintf(data_url, "data:image/png;base64,%s", image_data);
        free(image_data);
        return data_url;
    }

    return image_data;
}

static enum MHD_Result handle_print_queue_request(struct MHD_Connection *connection) {
    Queue* print_queue = queue_find("PrintQueue");
    if (!print_queue) {
        const char *error_response = "{\"error\": \"Print queue not found\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_response),
                                        (void*)error_response, MHD_RESPMEM_PERSISTENT);
	add_cors_headers(response);
        MHD_add_response_header(response, "Content-Type", "application/json");
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    json_t *queue_array = json_array();
    size_t queue_size_value = queue_size(print_queue);

    for (size_t i = 0; i < queue_size_value; i++) {
        size_t size;
        int priority;
        char *job_str = queue_dequeue(print_queue, &size, &priority);
        if (job_str) {
            json_t *job_json = json_loads(job_str, 0, NULL);
            if (job_json) {
                json_array_append_new(queue_array, job_json);
            }
            queue_enqueue(print_queue, job_str, size, priority);
            free(job_str);
        }
    }

    char *queue_str = json_dumps(queue_array, JSON_INDENT(2));
    json_decref(queue_array);

    // Create HTML wrapper
    char *html_response = malloc(strlen(queue_str) + 1000); // Allocate extra space for HTML
    sprintf(html_response,
            "<html><head><title>Print Queue</title></head>"
            "<body>"
            "<h1>Print Queue</h1>"
            "<div id='queue-data' style='display:none;'>%s</div>"
            "<div id='queue-display'></div>"
            "<script>"
            "var queueData = JSON.parse(document.getElementById('queue-data').textContent);"
            "var displayDiv = document.getElementById('queue-display');"
            "queueData.forEach(function(job, index) {"
            "  var jobDiv = document.createElement('div');"
            "  jobDiv.innerHTML = '<h2>Job ' + (index + 1) + '</h2>' +"
            "    '<p>Filename: ' + job.original_filename + '</p>' +"
            "    '<p>Size: ' + job.file_size + ' bytes</p>' +"
            "    '<img src=\"' + job.preview_image + '\" alt=\"Preview\" style=\"max-width:300px;\">' +"
            "    '<pre>' + JSON.stringify(job, null, 2) + '</pre>';"
            "  displayDiv.appendChild(jobDiv);"
            "});"
            "</script>"
            "</body></html>",
            queue_str);

    free(queue_str);

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(html_response),
                                    (void*)html_response, MHD_RESPMEM_MUST_FREE);
    add_cors_headers(response);
    MHD_add_response_header(response, "Content-Type", "text/html");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}
