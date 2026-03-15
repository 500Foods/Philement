// Global includes
#include <src/hydrogen.h>

// Local includes
#include "web_server_request.h"
#include "web_server_core.h"
#include "web_server_upload.h"
#include "web_server_compression.h"
#include <src/swagger/swagger.h>
#include <src/api/api_service.h>
#include <src/api/api_utils.h>
#include <src/api/system/config/config.h>
#include <src/api/system/prometheus/prometheus.h>

// Function prototypes
bool format_http_date(time_t timestamp, char *buffer, size_t buffer_size);
time_t parse_http_date(const char *http_date);
bool build_static_etag(const struct stat *file_stat, bool use_br_file, char *etag, size_t etag_size);
void add_static_metadata_headers(struct MHD_Response *response, const struct stat *file_stat, const char *etag);
bool is_static_file_not_modified(struct MHD_Connection *connection, const char *etag, const struct stat *file_stat);

// Helper function to check if a file path matches a pattern
// Supports wildcard (*) matching
bool matches_pattern(const char* path, const char* pattern) {
    if (!path || !pattern) return false;

    // Exact match for "*" pattern
    if (strcmp(pattern, "*") == 0) {
        return true;
    }

    // Check if pattern ends with a file extension (e.g., ".js", ".wasm")
    if (pattern[0] == '.' && strlen(pattern) > 1) {
        // Check if path ends with this extension
        size_t pattern_len = strlen(pattern);
        size_t path_len = strlen(path);
        
        if (path_len >= pattern_len) {
            return strcmp(path + path_len - pattern_len, pattern) == 0;
        }
        return false;
    }

    // For other patterns, do simple substring matching
    return strstr(path, pattern) != NULL;
}

// Runtime HTTP metrics
static volatile size_t http_requests_in_flight = 0;
static volatile size_t http_requests_total = 0;
static volatile size_t http_api_post_contexts_current = 0;
static volatile size_t http_upload_contexts_current = 0;

bool resolve_static_file_path(const char *url, char *resolved_path, size_t resolved_path_size) {
    struct stat path_stat;
    size_t url_len;

    if (!url || !resolved_path || resolved_path_size == 0 || !app_config ||
        !app_config->webserver.web_root) {
        return false;
    }

    url_len = strlen(url);
    if (url_len == 0) {
        return false;
    }

    if (snprintf(resolved_path, resolved_path_size, "%s%s",
                 app_config->webserver.web_root, url) >= (int)resolved_path_size) {
        return false;
    }

    if (stat(resolved_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
        return true;
    }

    if (url[url_len - 1] == '/' ||
        (stat(resolved_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode))) {
        char directory_path[PATH_MAX];
        const char *default_files[] = { "index.html", "Project1.html" };

        if (snprintf(directory_path, sizeof(directory_path), "%s", resolved_path) >= (int)sizeof(directory_path)) {
            return false;
        }

        for (size_t i = 0; i < sizeof(default_files) / sizeof(default_files[0]); i++) {
            if (snprintf(resolved_path, resolved_path_size, "%s%s%s",
                         directory_path,
                         directory_path[strlen(directory_path) - 1] == '/' ? "" : "/",
                         default_files[i]) >= (int)resolved_path_size) {
                return false;
            }

            if (stat(resolved_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode)) {
                return true;
            }
        }
    }

    return false;
}

void http_metrics_request_started(void) {
    __sync_add_and_fetch(&http_requests_in_flight, 1);
    __sync_add_and_fetch(&http_requests_total, 1);
}

void http_metrics_request_completed(void) {
    size_t current = (size_t)__sync_fetch_and_add(&http_requests_in_flight, 0);
    if (current > 0) {
        __sync_sub_and_fetch(&http_requests_in_flight, 1);
    }
}

void http_metrics_api_post_context_allocated(void) {
    __sync_add_and_fetch(&http_api_post_contexts_current, 1);
}

void http_metrics_api_post_context_freed(void) {
    size_t current = (size_t)__sync_fetch_and_add(&http_api_post_contexts_current, 0);
    if (current > 0) {
        __sync_sub_and_fetch(&http_api_post_contexts_current, 1);
    }
}

void http_metrics_upload_context_allocated(void) {
    __sync_add_and_fetch(&http_upload_contexts_current, 1);
}

void http_metrics_upload_context_freed(void) {
    size_t current = (size_t)__sync_fetch_and_add(&http_upload_contexts_current, 0);
    if (current > 0) {
        __sync_sub_and_fetch(&http_upload_contexts_current, 1);
    }
}

void get_http_runtime_metrics(HttpRuntimeMetrics *metrics) {
    if (!metrics) {
        return;
    }

    metrics->requests_in_flight = (size_t)__sync_fetch_and_add(&http_requests_in_flight, 0);
    metrics->requests_total = (size_t)__sync_fetch_and_add(&http_requests_total, 0);
    metrics->api_post_contexts_current = (size_t)__sync_fetch_and_add(&http_api_post_contexts_current, 0);
    metrics->upload_contexts_current = (size_t)__sync_fetch_and_add(&http_upload_contexts_current, 0);
    metrics->current_connections = 0;

    if (webserver_daemon) {
        const union MHD_DaemonInfo *conn_info =
            MHD_get_daemon_info(webserver_daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
        if (conn_info) {
            metrics->current_connections = conn_info->num_connections;
        }
    }
}

// Function to add custom headers to a response based on file path
void add_custom_headers(struct MHD_Response *response, const char* file_path, const WebServerConfig* web_config) {
    if (!response || !file_path || !web_config || !web_config->headers) {
        return;
    }

    // Extract just the filename from the full path
    const char* filename = strrchr(file_path, '/');
    if (filename) {
        filename++; // Skip the slash
    } else {
        filename = file_path;
    }

    // Check each header rule
    for (size_t i = 0; i < web_config->headers_count; i++) {
        const HeaderRule* rule = &web_config->headers[i];
        
        if (matches_pattern(filename, rule->pattern)) {
            MHD_add_response_header(response, rule->header_name, rule->header_value);
            log_this(SR_WEBSERVER, "Added custom header %s: %s for file %s", LOG_LEVEL_DEBUG, 3,
                    rule->header_name, rule->header_value, filename);
        }
    }
}

bool format_http_date(time_t timestamp, char *buffer, size_t buffer_size) {
    struct tm tm_value;

    if (!buffer || buffer_size == 0) {
        return false;
    }

    if (gmtime_r(&timestamp, &tm_value) == NULL) {
        return false;
    }

    return strftime(buffer, buffer_size, "%a, %d %b %Y %H:%M:%S GMT", &tm_value) > 0;
}

time_t parse_http_date(const char *http_date) {
    struct tm tm_value;
    const char *parse_end;

    if (!http_date) {
        return (time_t)-1;
    }

    memset(&tm_value, 0, sizeof(tm_value));
    parse_end = strptime(http_date, "%a, %d %b %Y %H:%M:%S GMT", &tm_value);
    if (!parse_end || *parse_end != '\0') {
        return (time_t)-1;
    }

    return timegm(&tm_value);
}

bool build_static_etag(const struct stat *file_stat, bool use_br_file, char *etag, size_t etag_size) {
    if (!file_stat || !etag || etag_size == 0) {
        return false;
    }

    return snprintf(etag, etag_size, "\"%llx-%llx-%llx-%c\"",
                    (unsigned long long)file_stat->st_ino,
                    (unsigned long long)file_stat->st_size,
                    (unsigned long long)file_stat->st_mtime,
                    use_br_file ? 'b' : 'r') < (int)etag_size;
}

void add_static_metadata_headers(struct MHD_Response *response, const struct stat *file_stat, const char *etag) {
    char last_modified[128];

    if (!response || !file_stat) {
        return;
    }

    MHD_add_response_header(response, "Cache-Control", "no-cache, must-revalidate");
    MHD_add_response_header(response, "Pragma", "no-cache");

    if (etag && etag[0] != '\0') {
        MHD_add_response_header(response, "ETag", etag);
    }

    if (format_http_date(file_stat->st_mtime, last_modified, sizeof(last_modified))) {
        MHD_add_response_header(response, "Last-Modified", last_modified);
    }
}

bool is_static_file_not_modified(struct MHD_Connection *connection, const char *etag, const struct stat *file_stat) {
    const char *if_none_match;
    const char *if_modified_since;
    time_t modified_since_time;

    if (!connection || !etag || !file_stat) {
        return false;
    }

    if_none_match = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "If-None-Match");
    if (if_none_match) {
        if (strcmp(if_none_match, "*") == 0 || strstr(if_none_match, etag) != NULL) {
            return true;
        }

        return false;
    }

    if_modified_since = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "If-Modified-Since");
    if (!if_modified_since) {
        return false;
    }

    modified_since_time = parse_http_date(if_modified_since);
    if (modified_since_time == (time_t)-1) {
        return false;
    }

    return file_stat->st_mtime <= modified_since_time;
}

enum MHD_Result serve_file_for_method(struct MHD_Connection *connection, const char *file_path, const char *method) {
    // Check if client accepts Brotli compression
    bool accepts_brotli = client_accepts_brotli(connection);
    char etag[128];
    
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

    if (!build_static_etag(&st, use_br_file, etag, sizeof(etag))) {
        close(fd);
        return MHD_NO;
    }

    if (is_static_file_not_modified(connection, etag, &st)) {
        struct MHD_Response *not_modified_response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
        if (!not_modified_response) {
            close(fd);
            return MHD_NO;
        }

        add_cors_headers(not_modified_response);
        add_custom_headers(not_modified_response, file_path, server_web_config);
        add_static_metadata_headers(not_modified_response, &st, etag);

        if (use_br_file) {
            add_brotli_header(not_modified_response);
        }

        close(fd);

        enum MHD_Result not_modified_ret = MHD_queue_response(connection, MHD_HTTP_NOT_MODIFIED, not_modified_response);
        MHD_destroy_response(not_modified_response);
        return not_modified_ret;
    }
    
    struct MHD_Response *response = MHD_create_response_from_fd((size_t)st.st_size, fd);
    if (!response) {
        close(fd);
        return MHD_NO;
    }
    
    add_cors_headers(response);
    
    // Add custom headers based on file path
    add_custom_headers(response, file_path, server_web_config);
    add_static_metadata_headers(response, &st, etag);
    
    // Set Content-Type based on the original file (not the .br version)
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        // Text formats
        if (strcmp(ext, ".html") == 0) MHD_add_response_header(response, "Content-Type", "text/html");
        else if (strcmp(ext, ".css") == 0) MHD_add_response_header(response, "Content-Type", "text/css");
        else if (strcmp(ext, ".js") == 0) MHD_add_response_header(response, "Content-Type", "application/javascript");
        else if (strcmp(ext, ".txt") == 0) MHD_add_response_header(response, "Content-Type", "text/plain");
        else if (strcmp(ext, ".json") == 0) MHD_add_response_header(response, "Content-Type", "application/json");
        else if (strcmp(ext, ".xml") == 0) MHD_add_response_header(response, "Content-Type", "application/xml");
        else if (strcmp(ext, ".csv") == 0) MHD_add_response_header(response, "Content-Type", "text/csv");
        
        // Image formats
        else if (strcmp(ext, ".svg") == 0) MHD_add_response_header(response, "Content-Type", "image/svg+xml");
        else if (strcmp(ext, ".png") == 0) MHD_add_response_header(response, "Content-Type", "image/png");
        else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) MHD_add_response_header(response, "Content-Type", "image/jpeg");
        else if (strcmp(ext, ".gif") == 0) MHD_add_response_header(response, "Content-Type", "image/gif");
        else if (strcmp(ext, ".ico") == 0) MHD_add_response_header(response, "Content-Type", "image/x-icon");
        else if (strcmp(ext, ".webp") == 0) MHD_add_response_header(response, "Content-Type", "image/webp");
        else if (strcmp(ext, ".bmp") == 0) MHD_add_response_header(response, "Content-Type", "image/bmp");
        else if (strcmp(ext, ".tif") == 0 || strcmp(ext, ".tiff") == 0) MHD_add_response_header(response, "Content-Type", "image/tiff");
        else if (strcmp(ext, ".avif") == 0) MHD_add_response_header(response, "Content-Type", "image/avif");
        
        // Font formats
        else if (strcmp(ext, ".woff") == 0) MHD_add_response_header(response, "Content-Type", "font/woff");
        else if (strcmp(ext, ".woff2") == 0) MHD_add_response_header(response, "Content-Type", "font/woff2");
        else if (strcmp(ext, ".ttf") == 0) MHD_add_response_header(response, "Content-Type", "font/ttf");
        else if (strcmp(ext, ".otf") == 0) MHD_add_response_header(response, "Content-Type", "font/otf");
        
        // Other common formats
        else if (strcmp(ext, ".pdf") == 0) MHD_add_response_header(response, "Content-Type", "application/pdf");
        else if (strcmp(ext, ".zip") == 0) MHD_add_response_header(response, "Content-Type", "application/zip");
        else if (strcmp(ext, ".wasm") == 0) MHD_add_response_header(response, "Content-Type", "application/wasm");
        // Add more content types as needed
    }
    
    // If serving a .br file, add the Content-Encoding header
    if (use_br_file) {
        add_brotli_header(response);
        log_this(SR_WEBSERVER, "Serving pre-compressed Brotli file: %s", LOG_LEVEL_DEBUG, 1, br_file_path);
    }
    
    // Queue the response - MHD automatically handles HEAD requests by suppressing
    // the body while preserving all headers (Content-Length, Content-Type, CORS, etc.)
    // No special HEAD handling needed: MHD_create_response_from_fd already knows the
    // correct Content-Length from st.st_size, and MHD will skip sending the body for HEAD.
    (void)method;
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

// Legacy serve_file function for backward compatibility
enum MHD_Result serve_file(struct MHD_Connection *connection, const char *file_path) {
    return serve_file_for_method(connection, file_path, "GET");
}

enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls) {
    (void)cls; (void)version;

    // Register connection thread if this is a new request
    if (*con_cls == NULL) {
        http_metrics_request_started();
        add_service_thread(&webserver_threads, pthread_self());
        char msg[128];
        snprintf(msg, sizeof(msg), "New connection thread for %s %s", method, url);
        log_this(SR_WEBSERVER, msg, LOG_LEVEL_DEBUG, 0);
    

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
            log_this(SR_API, detail, LOG_LEVEL_DEBUG, 0);
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

    // Handle GET and HEAD requests (HEAD is like GET but without body)
    if (strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
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

        // Check if URL doesn't end with "/" but resolves to a directory
        // If so, redirect to add trailing slash for better relative path handling
        size_t url_len = strlen(url);
        if (url_len > 0 && url[url_len - 1] != '/') {
            char temp_path[PATH_MAX];
            if (snprintf(temp_path, sizeof(temp_path), "%s%s",
                         app_config->webserver.web_root, url) < (int)sizeof(temp_path)) {
                struct stat path_stat;
                if (stat(temp_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
                    // This is a directory without trailing slash - redirect
                    char redirect_url[PATH_MAX];
                    if (snprintf(redirect_url, sizeof(redirect_url), "%s/", url) < (int)sizeof(redirect_url)) {
                        struct MHD_Response *redirect_response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
                        if (redirect_response) {
                            MHD_add_response_header(redirect_response, "Location", redirect_url);
                            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, redirect_response);
                            MHD_destroy_response(redirect_response);
                            log_this(SR_WEBSERVER, "Redirecting directory %s to %s", LOG_LEVEL_DEBUG, 2, url, redirect_url);
                            return ret;
                        }
                    }
                }
            }
        }

        // Serve exact files directly, or for directory requests only resolve index.html
        // and Project1.html as fallbacks.
        if (resolve_static_file_path(url, file_path, sizeof(file_path))) {
            log_this(SR_WEBSERVER, "Served File: %s", LOG_LEVEL_DEBUG, 1, file_path);
            return serve_file_for_method(connection, file_path, method);
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

    // Clean up connection info based on type
    // Both ConnectionInfo and ApiPostBuffer have a magic field as their first member
    // that we can use to determine the proper cleanup procedure
    if (NULL == con_cls || NULL == *con_cls) {
        // No context to clean up - just unregister thread
        http_metrics_request_completed();
        remove_service_thread(&webserver_threads, pthread_self());
        log_this(SR_WEBSERVER, "Connection thread completed (no context)", LOG_LEVEL_TRACE, 0);
        return;
    }
    
    // Read the magic number to identify the context type
    // Both structs have magic as their first uint32_t member
    const uint32_t *magic_ptr = (const uint32_t *)*con_cls;
    uint32_t magic = *magic_ptr;
    
    if (magic == CONNECTION_INFO_MAGIC) {
        // This is a file upload ConnectionInfo structure
        struct ConnectionInfo *con_info = (struct ConnectionInfo *)*con_cls;
        if (con_info->postprocessor) {
            MHD_destroy_post_processor(con_info->postprocessor);
        }
        if (con_info->fp) {
            fclose(con_info->fp);
        }
        free(con_info->original_filename);
        free(con_info->new_filename);
        free(con_info);
        http_metrics_upload_context_freed();
        log_this(SR_WEBSERVER, "Cleaned up ConnectionInfo (file upload)", LOG_LEVEL_TRACE, 0);
    }
    else if (magic == API_POST_BUFFER_MAGIC) {
        // This is an API POST buffer - use the api_utils cleanup
        api_free_post_buffer(con_cls);
        log_this(SR_WEBSERVER, "Cleaned up ApiPostBuffer (API request)", LOG_LEVEL_TRACE, 0);
    }
    else {
        // Unknown context type - log warning but don't crash
        // This could be legacy context or an uninitialized structure
        log_this(SR_WEBSERVER, "Unknown connection context type (magic=0x%08X) - freeing raw pointer",
                 LOG_LEVEL_ALERT, 1, magic);
        free(*con_cls);
    }
    
    *con_cls = NULL;

    // Remove connection thread from tracking after cleanup
    http_metrics_request_completed();
    remove_service_thread(&webserver_threads, pthread_self());
    log_this(SR_WEBSERVER, "Connection thread completed", LOG_LEVEL_TRACE, 0);
}
