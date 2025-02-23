// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

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
#include "../utils/utils_threads.h"
#include "../logging/logging.h"

// External configuration
extern AppConfig *app_config;

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

bool is_api_endpoint(const char *url, char *service, char *endpoint) {
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
        log_this("WebServer", msg, LOG_LEVEL_INFO);
    }

    // Log API endpoint access
    char service[32] = {0};
    char endpoint[32] = {0};
    if (is_api_endpoint(url, service, endpoint)) {
        char detail[128];
        snprintf(detail, sizeof(detail), "%sService/%s", service, endpoint);
        log_this("API", detail, LOG_LEVEL_INFO);
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
        // API endpoints
        if (strcmp(url, "/api/version") == 0) {
            return handle_version_request(connection);
        } else if (strcmp(url, "/api/system/info") == 0) {
            return handle_system_info_request(connection);
        } else if (strcmp(url, "/api/system/health") == 0) {
            return handle_system_health_request(connection);
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
            log_this("WebServer", "Served File: %s", LOG_LEVEL_INFO, file_path);
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

    // Handle POST requests in web_server_upload.c
    if (strcmp(method, "POST") == 0) {
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
    log_this("WebServer", "Connection thread completed", LOG_LEVEL_INFO);
}