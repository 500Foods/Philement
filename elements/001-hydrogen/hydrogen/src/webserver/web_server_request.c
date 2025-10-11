// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "web_server_request.h"
#include "web_server_core.h"
#include "web_server_upload.h"
#include "web_server_compression.h"
#include <src/swagger/swagger.h>
#include <src/api/api_service.h>
#include <src/api/system/config/config.h>
#include <src/api/system/prometheus/prometheus.h>

enum MHD_Result serve_file(struct MHD_Connection *connection, const char *file_path) {
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
    
    struct MHD_Response *response = MHD_create_response_from_fd((size_t)st.st_size, fd);
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
        log_this(SR_WEBSERVER, "Serving pre-compressed Brotli file: %s", LOG_LEVEL_STATE, 1, br_file_path);
    }
    
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)version;

    // Register connection thread if this is a new request
    if (*con_cls == NULL) {
        add_service_thread(&webserver_threads, pthread_self());
        char msg[128];
        snprintf(msg, sizeof(msg), "New connection thread for %s %s", method, url);
        log_this(SR_WEBSERVER, msg, LOG_LEVEL_STATE, 0);
    

        /*
        * Log API endpoint access using is_api_endpoint from the API service.
        * This function validates the URL against the configured prefix (e.g., "/api" or "/myapi")
        * and extracts the service and endpoint names. For example:
        * URL: "/api/system/health" -> service: "system", endpoint: "health"
        * URL: "/myapi/system/info" -> service: "system", endpoint: "info"
        * 
        * NOTE: Uploads happen in many parts, and we don't need to log each one, so this section
        * has been moved up into the above section so it just runs the one time, thanks.
        */
        char service[32] = {0};
        char endpoint[32] = {0};
        if (is_api_endpoint(url, service, endpoint)) {
            char detail[128];
            snprintf(detail, sizeof(detail), "%sService/%s", service, endpoint);
            log_this(SR_API, detail, LOG_LEVEL_STATE, 0);
        }
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
        // Check for Swagger UI requests if Swagger is enabled
        if (app_config && app_config->swagger.enabled && 
            is_swagger_request(url, &app_config->swagger)) {
            // Handle trailing slash redirect for Swagger UI root
            size_t url_len = strlen(url);
            const char* prefix = app_config->swagger.prefix;
            size_t prefix_len = strlen(prefix);
            
            if (url_len == prefix_len && strcmp(url, prefix) == 0) {
                struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
                char redirect_url[PATH_MAX];
                snprintf(redirect_url, sizeof(redirect_url), "%s/", prefix);
                MHD_add_response_header(response, "Location", redirect_url);
                enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
                MHD_destroy_response(response);
                return ret;
            }
            
            // Let Swagger subsystem handle its own requests
            return handle_swagger_request(connection, url, &app_config->swagger);
        }

    /*
     * Handle requests using the registered endpoint system.
     * Each subsystem (API, Swagger, etc.) registers its endpoints with prefix and handler.
     * This allows for flexible URL routing without hardcoding paths.
     */
    const WebServerEndpoint* web_endpoint = get_endpoint_for_url(url);
    if (web_endpoint) {
        return web_endpoint->handler(cls, connection, url, method, version,
                               upload_data, upload_data_size, con_cls);
    }

        // Try to serve a static file
        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s%s", app_config->webserver.web_root, url);

        // If the URL ends with a /, append index.html
        if (url[strlen(url) - 1] == '/') {
            strcat(file_path, "index.html");
        }

        // Serve up the requested file
        if (access(file_path, F_OK) != -1) {
            log_this(SR_WEBSERVER, "Served File: %s", LOG_LEVEL_STATE, 1, file_path);
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
        // Check for registered endpoint handler first
        const WebServerEndpoint* post_endpoint = get_endpoint_for_url(url);
        if (post_endpoint) {
            return post_endpoint->handler(cls, connection, url, method, version,
                                  upload_data, upload_data_size, con_cls);
        }
        
        // Handle regular uploads if no registered handler
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
    remove_service_thread(&webserver_threads, pthread_self());
    log_this(SR_WEBSERVER, "Connection thread completed", LOG_LEVEL_STATE, 0);
}
