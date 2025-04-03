// System headers
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <linux/limits.h>

// Project headers
#include "web_server_request.h"
#include "web_server_core.h"
#include "web_server_upload.h"
#include "web_server_compression.h"
#include "../swagger/swagger.h"
#include "../threads/threads.h"
#include "../logging/logging.h"
#include "../state/state.h"
#include "../api/system/config/config.h"

static enum MHD_Result serve_file(struct MHD_Connection *connection, const char *file_path) {
    // Check if client accepts Brotli compression
    bool accepts_brotli = client_accepts_brotli(connection);
    
    // If client accepts Brotli, check if we have a pre-compressed version
    char br_file_path[PATH_MAX];
    bool use_br_file = accepts_brotli && brotli_file_exists(file_path, br_file_path, sizeof(br_file_path));
    
    // If we're using a pre-compressed file, use that instead
    const char *path_to_serve = use_br_file ? br_file_path : file_path;
    
    // Open the file
    int fd = open(path_to_serve, O_RDONLY);
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
    
    // Set Content-Type based on the original file (not the .br version)
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) MHD_add_response_header(response, "Content-Type", "text/html");
        else if (strcmp(ext, ".css") == 0) MHD_add_response_header(response, "Content-Type", "text/css");
        else if (strcmp(ext, ".js") == 0) MHD_add_response_header(response, "Content-Type", "application/javascript");
        // Add more content types as needed
    }
    
    // If serving a .br file, add the Content-Encoding header
    if (use_br_file) {
        add_brotli_header(response);
        log_this("WebServer", "Serving pre-compressed Brotli file: %s", LOG_LEVEL_STATE, br_file_path);
    }
    
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

static enum MHD_Result handle_version_request(struct MHD_Connection *connection) {
    const char *version_json = "{\"api\": \"0.1\", \"server\": \"1.1.0\", \"text\": \"OctoPrint 1.1.0\"}";
    size_t json_len = strlen(version_json);
    
    // Check if client accepts Brotli
    bool accepts_brotli = client_accepts_brotli(connection);
    
    if (accepts_brotli) {
        // Compress JSON with Brotli
        uint8_t *compressed_data = NULL;
        size_t compressed_size = 0;
        
        if (compress_with_brotli((const uint8_t *)version_json, json_len, 
                              &compressed_data, &compressed_size)) {
            // Create response with compressed data
            struct MHD_Response *response = MHD_create_response_from_buffer(
                compressed_size, compressed_data, MHD_RESPMEM_MUST_FREE);
            
            add_cors_headers(response);
            MHD_add_response_header(response, "Content-Type", "application/json");
            add_brotli_header(response);
            
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            MHD_destroy_response(response);
            return ret;
        } else {
            // Compression failed, fallback to uncompressed
            log_this("WebServer", "Brotli compression failed, serving uncompressed response", LOG_LEVEL_ALERT);
        }
    }
    
    // If we get here, either client doesn't accept Brotli or compression failed
    // Serve uncompressed response
    struct MHD_Response *response = MHD_create_response_from_buffer(json_len,
                                    (void*)version_json, MHD_RESPMEM_PERSISTENT);
    add_cors_headers(response);
    MHD_add_response_header(response, "Content-Type", "application/json");
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

extern WebServerConfig *server_web_config;

bool is_api_endpoint(const char *url, char *service, char *endpoint) {
    if (!app_config || !app_config->api.prefix) {
        log_this("WebServer", "API configuration not available", LOG_LEVEL_ERROR);
        return false;
    }

    // Use the configured API prefix
    size_t prefix_len = strlen(app_config->api.prefix);
    // Check if URL starts with prefix followed by a slash
    if (strncmp(url, app_config->api.prefix, prefix_len) != 0 || 
        url[prefix_len] != '/') {
        return false;
    }
    // Skip the prefix and the following slash
    const char *path = url + prefix_len + 1;
    
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

// Helper function to build API paths with the current prefix configuration
static char* build_api_path(const char* endpoint_path, char* buffer, size_t buffer_size) {
    if (!app_config || !app_config->api.prefix) {
        log_this("WebServer", "API configuration not available", LOG_LEVEL_ERROR);
        return NULL;
    }
    snprintf(buffer, buffer_size, "%s%s", app_config->api.prefix, endpoint_path);
    return buffer;
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)version;

    // Register connection thread if this is a new request
    if (*con_cls == NULL) {
        extern ServiceThreads web_threads;
        add_service_thread(&web_threads, pthread_self());
        char msg[128];
        snprintf(msg, sizeof(msg), "New connection thread for %s %s", method, url);
        log_this("WebServer", msg, LOG_LEVEL_STATE);
    }

    // Log API endpoint access
    char service[32] = {0};
    char endpoint[32] = {0};
    if (is_api_endpoint(url, service, endpoint)) {
        char detail[128];
        snprintf(detail, sizeof(detail), "%sService/%s", service, endpoint);
        log_this("API", detail, LOG_LEVEL_STATE);
    }

    // Handle OPTIONS method for CORS preflight requests
    if (strcmp(method, "OPTIONS") == 0) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    // Handle GET requests
    if (strcmp(method, "GET") == 0) {
        // Check for Swagger UI requests first
        if (is_swagger_request(url, &app_config->web)) {
            return handle_swagger_request(connection, url, &app_config->web);
        }
        
        // API endpoints - use configurable API prefix
        char api_path[PATH_MAX];
        
        // Version endpoint
        if (strcmp(url, build_api_path("/version", api_path, sizeof(api_path))) == 0) {
            return handle_version_request(connection);
        } 
        // System info endpoint
        else if (strcmp(url, build_api_path("/system/info", api_path, sizeof(api_path))) == 0) {
            return handle_system_info_request(connection);
        } 
        // System health endpoint
        else if (strcmp(url, build_api_path("/system/health", api_path, sizeof(api_path))) == 0) {
            return handle_system_health_request(connection);
        } 
        // System test endpoint
        else if (strcmp(url, build_api_path("/system/test", api_path, sizeof(api_path))) == 0) {
            return handle_system_test_request(connection, method, upload_data, upload_data_size, con_cls);
        }
        // System config endpoint
        else if (strcmp(url, build_api_path("/system/config", api_path, sizeof(api_path))) == 0) {
            return handle_system_config_request(connection, method, upload_data, upload_data_size, con_cls);
        }

        // Try to serve a static file
        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s%s", app_config->web.web_root, url);

        // If the URL ends with a /, append index.html
        if (url[strlen(url) - 1] == '/') {
            strcat(file_path, "index.html");
        }

        // Serve up the requested file
        if (access(file_path, F_OK) != -1) {
            log_this("WebServer", "Served File: %s", LOG_LEVEL_STATE, file_path);
            return serve_file(connection, file_path);
        }

        // If file not found, return 404
        const char *page = "<html><body>404 Not Found</body></html>";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                        (void*)page, MHD_RESPMEM_PERSISTENT);
        add_cors_headers(response);
        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        MHD_destroy_response(response);
        return ret;
    }
    // Handle POST requests
    else if (strcmp(method, "POST") == 0) {
        // API endpoints that support POST
        char api_path[PATH_MAX];
        if (strcmp(url, build_api_path("/system/test", api_path, sizeof(api_path))) == 0) {
            return handle_system_test_request(connection, method, upload_data, upload_data_size, con_cls);
        }
        
        // Handle regular uploads
        return handle_upload_request(connection, upload_data, upload_data_size, con_cls);
    }

    // Handle unsupported methods
    const char *page = "<html><body>Method not supported</body></html>";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                    (void*)page, MHD_RESPMEM_PERSISTENT);
    add_cors_headers(response);
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);
    return ret;
}

void request_completed(void *cls, struct MHD_Connection *connection,
                      void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls; (void)connection; (void)toe;  // Unused parameters

    // Clean up connection info
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

    // Remove connection thread from tracking after cleanup
    extern ServiceThreads web_threads;
    remove_service_thread(&web_threads, pthread_self());
    log_this("WebServer", "Connection thread completed", LOG_LEVEL_STATE);
}