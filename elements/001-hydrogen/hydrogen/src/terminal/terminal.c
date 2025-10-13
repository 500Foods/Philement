// Global includes
#include <src/hydrogen.h>

// Local includes
#include "terminal.h"
#include "terminal_session.h"
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// Web server headers for compression and CORS support
#include <src/webserver/web_server_core.h>
#include <src/webserver/web_server_compression.h>

// Include std headers for filesystem operations
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Forward declaration for payload cache functions
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files, size_t *num_files, size_t *capacity);
bool is_payload_cache_available(void);

// Forward declarations for compression and filesystem functions
bool brotli_file_exists(const char *file_path, char *br_file_path, size_t br_buffer_size);
bool client_accepts_brotli(struct MHD_Connection *connection);
void add_brotli_header(struct MHD_Response *response);

// Forward declaration for filesystem serving function
enum MHD_Result serve_file_from_path(struct MHD_Connection *connection, const char *file_path);

// Global state for terminal subsystem
static const TerminalConfig *global_terminal_config = NULL;
static bool terminal_initialized = false;

// Structure to hold in-memory terminal files (similar to SwaggerFile)
typedef struct {
    char *name;         // File name (e.g., "terminal.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} TerminalFile;

// Global terminal files state
static TerminalFile *terminal_files = NULL;
static size_t num_terminal_files = 0;

/**
 * Format file size for display
 * Helper function to format file size in human-readable format
 */
void format_file_size(size_t size, char *buffer, size_t buffer_size) {
    if (size < 1024) {
        snprintf(buffer, buffer_size, "%zu bytes", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1fK", (double)size / 1024.0);
    } else {
        snprintf(buffer, buffer_size, "%.1fM", (double)size / (1024.0 * 1024.0));
    }
}

/**
 * URL validator for terminal subsystem
 * This function validates whether a URL should be handled by the terminal subsystem
 */
bool terminal_url_validator(const char *url) {
    return is_terminal_request(url, global_terminal_config);
}

/**
 * Request handler wrapper for terminal subsystem
 * This function wraps the main terminal request handler for use by the web server
 */
enum MHD_Result terminal_request_handler(void *cls __attribute__((unused)),
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method __attribute__((unused)),
                                       const char *version __attribute__((unused)),
                                       const char *upload_data __attribute__((unused)),
                                       size_t *upload_data_size __attribute__((unused)),
                                       void **con_cls __attribute__((unused))) {
    return handle_terminal_request(connection, url, global_terminal_config);
}

/**
 * Initialize terminal support
 * This function sets up the terminal subsystem for operation
 */
bool init_terminal_support(TerminalConfig *config) {
    // Store config globally for validator
    global_terminal_config = config;

    // Check shutdown flags

    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_TERMINAL, "Cannot initialize terminal during shutdown", LOG_LEVEL_STATE, 0);
        terminal_initialized = false;
        return false;
    }

    // Only proceed if we're in startup phase
    if (!server_starting || server_stopping || web_server_shutdown) {
        log_this(SR_TERMINAL, "Cannot initialize - invalid system state", LOG_LEVEL_STATE, 0);
        return false;
    }

    // Skip if already initialized or disabled
    if (terminal_initialized || !config || !config->enabled) {
        if (terminal_initialized) {
            log_this(SR_TERMINAL, "Already initialized", LOG_LEVEL_STATE, 0);
        }
        return terminal_initialized;
    }

    // Initialize terminal session manager for WebSocket integration
    if (!init_session_manager(config->max_sessions, config->idle_timeout_seconds)) {
        log_this(SR_TERMINAL, "Failed to initialize terminal session manager", LOG_LEVEL_ERROR, 0);
        return false;
    }
    log_this(SR_TERMINAL, "Terminal session manager initialized", LOG_LEVEL_STATE, 0);

    // Determine serving mode based on WebRoot configuration
    bool is_payload_mode = false;

    if (config->webroot && strlen(config->webroot) > 0) {
        if (strncmp(config->webroot, "PAYLOAD:", 8) == 0) {
            is_payload_mode = true;
        }
    } else {
        // Legacy behavior: default to payload mode if no WebRoot specified
        is_payload_mode = true;
    }

    // Only check payload cache availability if we're in payload mode
    if (is_payload_mode && !is_payload_cache_available()) {
        log_this(SR_TERMINAL, "Payload cache not available - has payload subsystem launched?", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Initialize file arrays based on mode
    num_terminal_files = 0;

    if (is_payload_mode) {
        // Get Terminal files from payload cache
        PayloadFile *payload_files = NULL;
        size_t num_payload_files = 0;
        size_t capacity = 0;

        bool success = get_payload_files_by_prefix("terminal/", &payload_files, &num_payload_files, &capacity);

        if (!success) {
            log_this(SR_TERMINAL, "Failed to get terminal files from payload cache", LOG_LEVEL_ERROR, 0);
            terminal_initialized = false;
            return false;
        }

        // Convert PayloadFile array to TerminalFile array (compatibility layer)
        terminal_files = calloc(num_payload_files, sizeof(TerminalFile));
        if (!terminal_files) {
            log_this(SR_TERMINAL, "Failed to allocate terminal files array", LOG_LEVEL_ERROR, 0);
            free(payload_files);
            return false;
        }

        num_terminal_files = 0;

        for (size_t i = 0; i < num_payload_files; i++) {
            // Clean the file name to remove prefixes (e.g., "terminal/terminal.html" -> "terminal.html")
            const char *clean_name = payload_files[i].name;
            if (strncmp(payload_files[i].name, "terminal/", 9) == 0) {
                clean_name = payload_files[i].name + 9; // Skip "terminal/" prefix
            }

            terminal_files[i].name = strdup(clean_name); // Use cleaned name
            terminal_files[i].data = payload_files[i].data;
            terminal_files[i].size = payload_files[i].size;
            terminal_files[i].is_compressed = payload_files[i].is_compressed;

            if (!terminal_files[i].name) {
                log_this(SR_TERMINAL, "Failed to allocate memory for file name", LOG_LEVEL_ERROR, 0);
                // Cleanup already allocated names
                for (size_t j = 0; j < i; j++) {
                    free(terminal_files[j].name);
                }
                free(terminal_files);
                free(payload_files);
                return false;
            }

            num_terminal_files++;
        }

        // Free the PayloadFile array (but not the individual files - we keep references)
        free(payload_files);
        log_this(SR_TERMINAL, "Initialized in PAYLOAD mode with %zu files", LOG_LEVEL_STATE, 1, num_terminal_files);
    } else {
        // Initialize empty by design - this will force filesystem-only serving
        terminal_files = NULL;
        log_this(SR_TERMINAL, "Initialized in FILESYSTEM mode (no payload files loaded)", LOG_LEVEL_STATE, 0);
    }

    // Update configuration based on payload availability
    terminal_initialized = true;

    // Log the configurable index page setting
    if (config->index_page) {
        log_this(SR_TERMINAL, "Initialized with index page: %s", LOG_LEVEL_STATE, 1, config->index_page);
    } else {
        log_this(SR_TERMINAL, "Initialized with default index page", LOG_LEVEL_STATE, 0);
    }

    // Log each file's details (only for payload mode)
    if (is_payload_mode) {
        for (size_t i = 0; i < num_terminal_files; i++) {
            char size_display[32];
            format_file_size(terminal_files[i].size, size_display, sizeof(size_display));

            log_this(SR_TERMINAL, "-> %s (%s%s)", LOG_LEVEL_STATE, 3,
                terminal_files[i].name,
                size_display,
                terminal_files[i].is_compressed ? ", compressed" : "");
        }
    }

    return true;
}

/**
 * Check if a URL is a terminal request
 * This function determines if the given URL should be handled by the terminal subsystem
 */
bool is_terminal_request(const char *url, const TerminalConfig *config) {
    if (!config || !config->enabled || !config->web_path || !url) {
        return false;
    }

    size_t prefix_len = strlen(config->web_path);

    // Check exact match (for redirect)
    if (strcmp(url, config->web_path) == 0) {
        return true;
    }

    // Check if URL starts with prefix and is followed by either:
    // - end of string
    // - a slash (followed by any valid path)
    if (strncmp(url, config->web_path, prefix_len) == 0 &&
        (url[prefix_len] == '\0' || url[prefix_len] == '/')) {
        return true;
    }

    return false;
}

/**
 * Serve a file from filesystem path
 * This function serves files directly from the filesystem
 */
enum MHD_Result serve_file_from_path(struct MHD_Connection *connection, const char *file_path) {
    // Check if client accepts Brotli compression
    bool accepts_brotli = client_accepts_brotli(connection);

    // If client accepts Brotli, check if we have a pre-compressed version
    char br_file_path[PATH_MAX];
    bool use_br_file = accepts_brotli && brotli_file_exists(file_path, br_file_path, sizeof(br_file_path));

    // If we're using a pre-compressed file, use that instead
    const char *path_to_serve = use_br_file ? br_file_path : file_path;

    // Open the file
    int fd = open(path_to_serve, O_RDONLY);
    if (fd == -1) {
        log_this(SR_TERMINAL, "Failed to open file: %s", LOG_LEVEL_ERROR, 1, file_path);
        return MHD_NO;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        log_this(SR_TERMINAL, "Failed to stat file: %s", LOG_LEVEL_ERROR, 1, file_path);
        return MHD_NO;
    }

    struct MHD_Response *response = MHD_create_response_from_fd((size_t)st.st_size, fd);
    if (!response) {
        close(fd);
        log_this(SR_TERMINAL, "Failed to create response from file descriptor", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    add_cors_headers(response);

    // Set Content-Type based on the original file (not the .br version)
    const char *ext = strrchr(file_path, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) MHD_add_response_header(response, "Content-Type", "text/html");
        else if (strcmp(ext, ".css") == 0) MHD_add_response_header(response, "Content-Type", "text/css");
        else if (strcmp(ext, ".js") == 0) MHD_add_response_header(response, "Content-Type", "application/javascript");
        else if (strcmp(ext, ".json") == 0) MHD_add_response_header(response, "Content-Type", "application/json");
        else if (strcmp(ext, ".png") == 0) MHD_add_response_header(response, "Content-Type", "image/png");
        // Add more content types as needed
    }

    // If serving a .br file, add the Content-Encoding header
    if (use_br_file) {
        add_brotli_header(response);
        log_this(SR_TERMINAL, "Served pre-compressed Brotli file from filesystem: %s", LOG_LEVEL_STATE, 1, br_file_path);
    } else {
        log_this(SR_TERMINAL, "Served file from filesystem: %s", LOG_LEVEL_STATE, 1, file_path);
    }

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/**
 * Handle terminal requests
 * This function processes HTTP requests for the terminal subsystem
 */
enum MHD_Result handle_terminal_request(struct MHD_Connection *connection,
                                      const char *url,
                                      const TerminalConfig *config) {
    if (!connection || !url || !config) {
        return MHD_NO;
    }

    // First check if this is exactly the terminal prefix with no trailing slash
    // If so, redirect to the same URL with a trailing slash
    size_t prefix_len = strlen(config->web_path);
    if (strcmp(url, config->web_path) == 0) {
        char *redirect_url;
        if (asprintf(&redirect_url, "%s/", url) != -1) {
            log_this(SR_TERMINAL, "Redirecting %s to %s for proper relative path resolution", LOG_LEVEL_STATE, 2, url, redirect_url);

            struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Location", redirect_url);
            free(redirect_url);

            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Skip past the prefix to get the actual file path
    const char *url_path = url + prefix_len;

    // Check if this is a request for the root path
    if (!*url_path || strcmp(url_path, "/") == 0) {
        // Use the configurable index page
        const char *index_file = config->index_page ? config->index_page : "terminal.html";
        url_path = index_file;
        log_this(SR_TERMINAL, "Serving index page: %s", LOG_LEVEL_STATE, 1, url_path);
    } else if (*url_path == '/') {
        url_path++; // Skip leading slash for other paths
    }

    // Note: WebSocket connections are handled by the WebSocket server on port 5261
    // Client-side JavaScript will connect directly to port 5261 for terminal WebSocket connections

    // Log the URL processing for debugging
    log_this(SR_TERMINAL, "Request: Original URL: %s, Processed path: %s", LOG_LEVEL_STATE, 2, url, url_path);

    // Try to find the requested file - robust approach matching swagger.c
    TerminalFile *file = NULL;
    bool client_accepts_br = client_accepts_brotli(connection);

    // Debug logging for troubleshooting
    const char *accept_encoding = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Accept-Encoding");
    if (accept_encoding) {
        log_this(SR_TERMINAL, "Client Accept-Encoding: %s", LOG_LEVEL_STATE, 1, accept_encoding);
    } else {
        log_this(SR_TERMINAL, "No Accept-Encoding header from client", LOG_LEVEL_STATE, 0);
    }

    // Look for both compressed and uncompressed versions
    TerminalFile *compressed_file = NULL;
    TerminalFile *uncompressed_file = NULL;
    char br_path[256];

    // Prepare the compressed version path
    if (!strstr(url_path, ".br")) {
        snprintf(br_path, sizeof(br_path), "%s.br", url_path);
    }

    for (size_t i = 0; i < num_terminal_files; i++) {
        const char *name = terminal_files[i].name;
        if (!strstr(url_path, ".br") && strcmp(name, br_path) == 0) {
            compressed_file = &terminal_files[i];
        } else if (strcmp(name, url_path) == 0) {
            uncompressed_file = &terminal_files[i];
        }
    }

    // Prefer uncompressed file for browser compatibility, fallback to compressed
    if (uncompressed_file) {
        file = uncompressed_file;
        log_this(SR_TERMINAL, "Using uncompressed version of %s", LOG_LEVEL_STATE, 1, url_path);
    } else if (compressed_file && client_accepts_br) {
        file = compressed_file;
        log_this(SR_TERMINAL, "Using compressed version of %s (client supports brotli)", LOG_LEVEL_STATE, 1, url_path);
    } else if (compressed_file) {
        file = compressed_file;
        log_this(SR_TERMINAL, "Using compressed version of %s (forcing header for client compatibility)", LOG_LEVEL_STATE, 1, url_path);
    } else {
        log_this(SR_TERMINAL, "No version found for %s", LOG_LEVEL_ERROR, 1, url_path);
    }

    // Fallback handling for direct .br requests (if someone requests the compressed version directly)
    if (!file && strlen(url_path) > 3 &&
        strcmp(url_path + strlen(url_path) - 3, ".br") == 0) {
        char base_path[256];
        size_t path_len = strlen(url_path);
        if (path_len - 3U < sizeof(base_path)) {
            memcpy(base_path, url_path, path_len - 3U);
            base_path[path_len - 3U] = '\0';

            for (size_t i = 0; i < num_terminal_files; i++) {
                if (strcmp(terminal_files[i].name, base_path) == 0) {
                    file = &terminal_files[i];
                    break;
                }
            }
        }
    }

    if (!file) {
        // Determine serving mode based on WebRoot configuration
        bool serve_from_payload_only = false;
        bool serve_from_filesystem_only = false;
        const char* filesystem_root = NULL;

        if (config->webroot && strlen(config->webroot) > 0) {
            // Check if WebRoot specifies payload-only mode
            if (strncmp(config->webroot, "PAYLOAD:", 8) == 0) {
                serve_from_payload_only = true;
                log_this(SR_TERMINAL, "Configured for payload-only mode: %s", LOG_LEVEL_STATE, 1, config->webroot);
            } else {
                // Default to filesystem mode
                serve_from_filesystem_only = true;
                filesystem_root = config->webroot;
                log_this(SR_TERMINAL, "Configured for filesystem mode: %s", LOG_LEVEL_STATE, 1, config->webroot);
            }
        } else {
            // Legacy behavior: if no WebRoot configured, assume filesystem from current directory
            serve_from_filesystem_only = true;
            filesystem_root = ".";
            log_this(SR_TERMINAL, "No WebRoot configured, using current directory as fallback", LOG_LEVEL_STATE, 0);
        }

        // Handle filesystem fallback only if not in payload-only mode
        if (!serve_from_payload_only && serve_from_filesystem_only && filesystem_root) {
            // Build filesystem path using WebRoot
            char full_path[PATH_MAX];
            if (snprintf(full_path, sizeof(full_path), "%s/%s", filesystem_root, url_path) >= (int)sizeof(full_path)) {
                log_this(SR_TERMINAL, "File path too long: %s/%s", LOG_LEVEL_ERROR, 2, filesystem_root, url_path);
                return MHD_NO;
            }

            // Check if file exists on filesystem
            if (access(full_path, F_OK) != -1) {
                log_this(SR_TERMINAL, "Serving from filesystem: %s", LOG_LEVEL_STATE, 1, full_path);
                return serve_file_from_path(connection, full_path);
            }
        }

        log_this(SR_TERMINAL, "File not found: %s", LOG_LEVEL_STATE, 1, url_path);
        return MHD_NO; // File not found
    }

    // Note: Compression header will be added below if file->is_compressed is true
    // regardless of client acceptance, as per fixed protocol

    // For all terminal files, serve directly from memory
    struct MHD_Response *response = MHD_create_response_from_buffer(
        file->size,
        (void*)file->data,
        MHD_RESPMEM_PERSISTENT
    );

    if (!response) {
        return MHD_NO;
    }

    // Get the content type based on the file extension
    const char *content_type = NULL;
    const char *ext = strrchr(url_path, '.');
    if (ext) {
        // Skip .br extension if present
        if (strcmp(ext, ".br") == 0) {
            // Find the real extension before .br
            char clean_path[256];
            ptrdiff_t len = ext - url_path;
            if (len > 0 && (size_t)len < sizeof(clean_path)) {
                strncpy(clean_path, url_path, (size_t)len);
                clean_path[len] = '\0';
                ext = strrchr(clean_path, '.');
            }
        }

        if (ext) {
            if (strcmp(ext, ".html") == 0) {
                content_type = "text/html";
            } else if (strcmp(ext, ".css") == 0) {
                content_type = "text/css";
            } else if (strcmp(ext, ".js") == 0) {
                content_type = "application/javascript";
            } else if (strcmp(ext, ".json") == 0) {
                content_type = "application/json";
            } else if (strcmp(ext, ".png") == 0) {
                content_type = "image/png";
            }
        }
    }

    // If no extension found or unknown type, default to text/plain
    if (!content_type) {
        content_type = "text/plain";
    }

    // Add content type header
    MHD_add_response_header(response, "Content-Type", content_type);

    // Add compression header if serving compressed content
    // IMPORTANT: Always add header when serving compressed data, regardless of client support
    if (file->is_compressed) {
        add_brotli_header(response);
    }

    // Add CORS headers (from webserver core)
    add_cors_headers(response);

    // Queue response
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

/**
 * Cleanup terminal support
 * This function cleans up resources used by the terminal subsystem
 */
void cleanup_terminal_support(TerminalConfig *config __attribute__((unused))) {
    log_this(SR_TERMINAL, "Terminal subsystem cleanup called", LOG_LEVEL_STATE, 0);
    terminal_initialized = false;
    global_terminal_config = NULL;

    // Cleanup terminal session manager and all active sessions
    cleanup_session_manager();
    log_this(SR_TERMINAL, "Terminal session manager cleaned up", LOG_LEVEL_STATE, 0);

    // Free terminal files array and allocated names
    if (terminal_files) {
        for (size_t i = 0; i < num_terminal_files; i++) {
            free(terminal_files[i].name); // Free strdup'd names
        }
        free(terminal_files);
        terminal_files = NULL;
    }
    num_terminal_files = 0;

    log_this(SR_TERMINAL, "Terminal subsystem cleanup completed", LOG_LEVEL_STATE, 0);
}
