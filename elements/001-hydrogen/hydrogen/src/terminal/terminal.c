// Global includes
#include "../hydrogen.h"

// Local includes
#include "terminal.h"
#include "../payload/payload.h"
#include "../payload/payload_cache.h"

// Web server headers for compression and CORS support
#include "../webserver/web_server_core.h"
#include "../webserver/web_server_compression.h"

// Forward declaration for payload cache functions
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files, size_t *num_files, size_t *capacity);
bool is_payload_cache_available(void);

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
enum MHD_Result terminal_request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method __attribute__((unused)),
                                       const char *version __attribute__((unused)),
                                       const char *upload_data __attribute__((unused)),
                                       size_t *upload_data_size __attribute__((unused)),
                                       void **con_cls __attribute__((unused))) {
    const TerminalConfig *config = (const TerminalConfig *)cls;
    return handle_terminal_request(connection, url, config);
}

/**
 * Initialize terminal support
 * This function sets up the terminal subsystem for operation
 */
bool init_terminal_support(TerminalConfig *config) {
    // Store config globally for validator
    global_terminal_config = config;

    // Check shutdown flags
    extern volatile sig_atomic_t web_server_shutdown;

    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_TERMINAL, "Cannot initialize terminal during shutdown", LOG_LEVEL_STATE);
        terminal_initialized = false;
        return false;
    }

    // Only proceed if we're in startup phase
    if (!server_starting || server_stopping || web_server_shutdown) {
        log_this(SR_TERMINAL, "Cannot initialize - invalid system state", LOG_LEVEL_STATE);
        return false;
    }

    // Skip if already initialized or disabled
    if (terminal_initialized || !config || !config->enabled) {
        if (terminal_initialized) {
            log_this(SR_TERMINAL, "Already initialized", LOG_LEVEL_STATE);
        }
        return terminal_initialized;
    }

    // Check if payload cache is available
    if (!is_payload_cache_available()) {
        log_this(SR_TERMINAL, "Payload cache not available - has payload subsystem launched?", LOG_LEVEL_ERROR);
        return false;
    }

    // Get Terminal files from payload cache
    PayloadFile *payload_files = NULL;
    size_t num_payload_files = 0;
    size_t capacity = 0;

    bool success = get_payload_files_by_prefix("terminal/", &payload_files, &num_payload_files, &capacity);

    if (!success) {
        log_this(SR_TERMINAL, "Failed to get terminal files from payload cache", LOG_LEVEL_ERROR);
        terminal_initialized = false;
        return false;
    }

    // Convert PayloadFile array to TerminalFile array (compatibility layer)
    terminal_files = calloc(num_payload_files, sizeof(TerminalFile));
    if (!terminal_files) {
        log_this(SR_TERMINAL, "Failed to allocate terminal files array", LOG_LEVEL_ERROR);
        free(payload_files);
        return false;
    }

    num_terminal_files = 0;

    for (size_t i = 0; i < num_payload_files; i++) {
        terminal_files[i].name = payload_files[i].name;
        terminal_files[i].data = payload_files[i].data;
        terminal_files[i].size = payload_files[i].size;
        terminal_files[i].is_compressed = payload_files[i].is_compressed;
        num_terminal_files++;
    }

    // Free the PayloadFile array (but not the individual files - we keep references)
    free(payload_files);

    // Update configuration based on payload availability
    terminal_initialized = true;

    // Log the configurable index page setting
    if (config->index_page) {
        log_this(SR_TERMINAL, "Initialized with index page: %s", LOG_LEVEL_STATE, config->index_page);
    } else {
        log_this(SR_TERMINAL, "Initialized with default index page", LOG_LEVEL_STATE);
    }

    log_this(SR_TERMINAL, "Terminal subsystem initialized with %zu files", LOG_LEVEL_STATE, num_terminal_files);

    // Log each file's details
    for (size_t i = 0; i < num_terminal_files; i++) {
        char size_display[32];
        if (terminal_files[i].size < 1024) {
            snprintf(size_display, sizeof(size_display), "%zu bytes", terminal_files[i].size);
        } else if (terminal_files[i].size < 1024 * 1024) {
            snprintf(size_display, sizeof(size_display), "%.1fK", (double)terminal_files[i].size / 1024.0);
        } else {
            snprintf(size_display, sizeof(size_display), "%.1fM", (double)terminal_files[i].size / (1024.0 * 1024.0));
        }

        log_this(SR_TERMINAL, "-> %s (%s%s)", LOG_LEVEL_STATE,
                terminal_files[i].name, size_display,
                terminal_files[i].is_compressed ? ", compressed" : "");
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
            log_this(SR_TERMINAL, "Redirecting %s to %s for proper relative path resolution",
                    LOG_LEVEL_STATE, url, redirect_url);

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
        log_this(SR_TERMINAL, "Serving index page: %s", LOG_LEVEL_STATE, url_path);
    } else if (*url_path == '/') {
        url_path++; // Skip leading slash for other paths
    }

    // Log the URL processing for debugging
    log_this(SR_TERMINAL, "Request: Original URL: %s, Processed path: %s",
             LOG_LEVEL_STATE, url, url_path);

    // Try to find the requested file
    TerminalFile *file = NULL;
    bool client_accepts_br = client_accepts_brotli(connection);

    // First try to find an exact match (handles direct .br requests)
    for (size_t i = 0; i < num_terminal_files; i++) {
        if (strcmp(terminal_files[i].name, url_path) == 0) {
            file = &terminal_files[i];
            break;
        }
    }

    // If not found and client accepts brotli, try the .br version
    if (!file && client_accepts_br && !strstr(url_path, ".br")) {
        char br_path[256];
        snprintf(br_path, sizeof(br_path), "%s.br", url_path);

        for (size_t i = 0; i < num_terminal_files; i++) {
            if (strcmp(terminal_files[i].name, br_path) == 0) {
                file = &terminal_files[i];
                break;
            }
        }
    }

    // If still not found and the request was for a .br file,
    // try without the .br extension
    if (!file && strlen(url_path) > 3 &&
        strcmp(url_path + strlen(url_path) - 3, ".br") == 0) {
        char base_path[256];
        size_t path_len = strlen(url_path);
        if (path_len > 3U && path_len - 3U < sizeof(base_path)) {
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
        log_this(SR_TERMINAL, "File not found: %s", LOG_LEVEL_STATE, url_path);
        return MHD_NO; // File not found
    }

    // Determine if we should serve compressed content
    bool serve_compressed = file->is_compressed && client_accepts_brotli(connection);

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
    if (serve_compressed) {
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
    log_this(SR_TERMINAL, "Terminal subsystem cleanup called", LOG_LEVEL_STATE);
    terminal_initialized = false;
    global_terminal_config = NULL;

    // Free terminal files array (but not individual file data - owned by payload cache)
    free(terminal_files);
    terminal_files = NULL;
    num_terminal_files = 0;

    log_this(SR_TERMINAL, "Terminal subsystem cleanup completed", LOG_LEVEL_STATE);
}
