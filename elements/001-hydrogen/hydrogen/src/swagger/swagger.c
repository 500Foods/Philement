
// System headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <brotli/decode.h>

// Project headers
#include "swagger.h"
#include "../webserver/web_server_core.h"
#include "../webserver/web_server_compression.h"
#include "../payload/payload.h"
#include "../logging/logging.h"
#include "../config/config.h"
#include "../config/files/config_filesystem.h"

// Structure to hold in-memory Swagger files
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// Global state
static SwaggerFile *swagger_files = NULL;
static size_t num_swagger_files = 0;
static bool swagger_initialized = false;

// Forward declarations
static bool load_swagger_files_from_tar(const uint8_t *tar_data, size_t tar_size);
static void free_swagger_files(void);
static char* get_server_url(struct MHD_Connection *connection, const SwaggerConfig *config);
static char* create_dynamic_initializer(const char *base_content, const char *server_url, const SwaggerConfig *config);

bool init_swagger_support(SwaggerConfig *config) {
    // Check all shutdown flags atomically
    extern volatile sig_atomic_t server_stopping;
    extern volatile sig_atomic_t server_starting;
    extern volatile sig_atomic_t web_server_shutdown;
    
    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this("SwaggerUI", "Cannot initialize Swagger UI during shutdown", LOG_LEVEL_STATE, NULL);
        swagger_initialized = false;  // Reset initialization flag during shutdown
        return false;
    }

    // Only proceed if we're in startup phase and not shutting down
    if (!server_starting || server_stopping || web_server_shutdown) {
        log_this("SwaggerUI", "Cannot initialize - invalid system state", LOG_LEVEL_STATE, NULL);
        return false;
    }

    // Double-check shutdown state before proceeding
    if (server_stopping || web_server_shutdown) {
        log_this("SwaggerUI", "Shutdown initiated, aborting Swagger UI initialization", LOG_LEVEL_STATE, NULL);
        swagger_initialized = false;  // Reset initialization flag
        return false;
    }

    // Skip if already initialized or disabled
    if (swagger_initialized || !config || !config->enabled) {
        if (swagger_initialized) {
            log_this("SwaggerUI", "Already initialized", LOG_LEVEL_STATE, NULL);
        }
        return swagger_initialized;
    }

    // Get executable path
    char *executable_path = get_executable_path();
    if (!executable_path) {
        log_this("SwaggerUI", "Failed to get executable path", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Try to extract Swagger payload using the payload handler
    const AppConfig *app_config = get_app_config();
    PayloadData payload = {0};
    bool success = extract_payload(executable_path, app_config, PAYLOAD_MARKER, &payload);
    if (!success) {
        swagger_initialized = false;  // Reset initialization flag on failure
    }
    free(executable_path);

    if (!success) {
        log_this("SwaggerUI", "Failed to load UI files", LOG_LEVEL_ALERT, NULL);
        config->payload_available = false;
        return false;
    }

    // Load Swagger files from the payload
    success = load_swagger_files_from_tar(payload.data, payload.size);
    free_payload(&payload);

    // Update configuration based on payload availability
    config->payload_available = success;
    swagger_initialized = success;
    
    if (success) {
        log_this("SwaggerUI", "Files available:", LOG_LEVEL_STATE, NULL);
        
        // Log each file's details
        for (size_t i = 0; i < num_swagger_files; i++) {
            char size_display[32];
            if (swagger_files[i].size < 1024) {
                snprintf(size_display, sizeof(size_display), "%zu bytes", swagger_files[i].size);
            } else if (swagger_files[i].size < 1024 * 1024) {
                snprintf(size_display, sizeof(size_display), "%.1fK", swagger_files[i].size / 1024.0);
            } else {
                snprintf(size_display, sizeof(size_display), "%.1fM", swagger_files[i].size / (1024.0 * 1024.0));
            }
            
            log_this("SwaggerUI", "-> %s (%s%s)", LOG_LEVEL_STATE,
                    swagger_files[i].name, size_display, 
                    swagger_files[i].is_compressed ? ", compressed" : "");
        }
    } else {
        log_this("SwaggerUI", "Failed to load UI files from payload", LOG_LEVEL_ALERT, NULL);
    }

    return success;
}

bool is_swagger_request(const char *url, const SwaggerConfig *config) {
    if (!config || !config->enabled || !config->payload_available || 
        !config->prefix || !url) {
        return false;
    }

    size_t prefix_len = strlen(config->prefix);
    
    // Check exact match (for redirect)
    if (strcmp(url, config->prefix) == 0) {
        return true;
    }
    
    // Check with trailing slash
    if (strcmp(url, config->prefix) == 0 || 
        (strncmp(url, config->prefix, prefix_len) == 0 && 
         (url[prefix_len] == '/' || url[prefix_len] == '\0'))) {
        return true;
    }

    return false;
}

enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url,
                                     const SwaggerConfig *config) {
    if (!connection || !url || !config) {
        return MHD_NO;
    }

    // First check if this is exactly the swagger prefix with no trailing slash
    // If so, redirect to the same URL with a trailing slash
    // This ensures all relative assets are correctly loaded
    size_t prefix_len = strlen(config->prefix);
    if (strcmp(url, config->prefix) == 0) {
        char *redirect_url;
        if (asprintf(&redirect_url, "%s/", url) != -1) {
            log_this("SwaggerUI", "Redirecting %s to %s for proper relative path resolution", 
                    LOG_LEVEL_STATE, url, redirect_url);
                    
            struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
            MHD_add_response_header(response, "Location", redirect_url);
            free(redirect_url);
            
            add_cors_headers(response);
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, response);
            MHD_destroy_response(response);
            return ret;
        }
    }

    // Skip past the prefix to get the actual file path
    const char *url_path = url + prefix_len;
    
    // Check if this is a request for the root path
    if (!*url_path || strcmp(url_path, "/") == 0) {
        url_path = "index.html";
    } else if (*url_path == '/') {
        url_path++; // Skip leading slash for other paths
    }
    
    // Log the URL processing for debugging
    log_this("SwaggerUI", "Request: Original URL: %s, Processed path: %s", 
             LOG_LEVEL_STATE, url, url_path);

    // Try to find the requested file
    SwaggerFile *file = NULL;
    bool client_accepts_br = client_accepts_brotli(connection);
    
    // First try to find an exact match (handles direct .br requests)
    for (size_t i = 0; i < num_swagger_files; i++) {
        if (strcmp(swagger_files[i].name, url_path) == 0) {
            file = &swagger_files[i];
            break;
        }
    }

    // If not found and client accepts brotli, try the .br version
    if (!file && client_accepts_br && !strstr(url_path, ".br")) {
        char br_path[256];
        snprintf(br_path, sizeof(br_path), "%s.br", url_path);
        
        for (size_t i = 0; i < num_swagger_files; i++) {
            if (strcmp(swagger_files[i].name, br_path) == 0) {
                file = &swagger_files[i];
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
        if (path_len - 3 < sizeof(base_path)) {
            memcpy(base_path, url_path, path_len - 3);
            base_path[path_len - 3] = '\0';
            
            for (size_t i = 0; i < num_swagger_files; i++) {
                if (strcmp(swagger_files[i].name, base_path) == 0) {
                    file = &swagger_files[i];
                    break;
                }
            }
        }
    }

    if (!file) {
        return MHD_NO; // File not found
    }

    // Determine if we should serve compressed content
    bool serve_compressed = file->is_compressed && client_accepts_brotli(connection);

    // If this is swagger.json or swagger-initializer.js, we need to modify it
    struct MHD_Response *response;
    char *dynamic_content = NULL;
    
    if (strcmp(url_path, "swagger.json") == 0) {
        // Parse the original swagger.json
        json_error_t error;
        json_t *spec = json_loadb((const char*)file->data, file->size, 0, &error);
        if (!spec) {
            log_this("SwaggerUI", "Failed to parse swagger.json: %s", LOG_LEVEL_ERROR, error.text);
            return MHD_NO;
        }

        // Get the info object or create it if it doesn't exist
        json_t *info = json_object_get(spec, "info");
        if (!info) {
            info = json_object();
            json_object_set_new(spec, "info", info);
        }

        // Update metadata from config
        if (config->metadata.title) {
            json_object_set_new(info, "title", json_string(config->metadata.title));
        }
        if (config->metadata.description) {
            json_object_set_new(info, "description", json_string(config->metadata.description));
        }
        if (config->metadata.version) {
            json_object_set_new(info, "version", json_string(config->metadata.version));
        }

        // Update contact info if provided
        if (config->metadata.contact.name || 
            config->metadata.contact.email || 
            config->metadata.contact.url) {
            json_t *contact = json_object();
            if (config->metadata.contact.name) {
                json_object_set_new(contact, "name", json_string(config->metadata.contact.name));
            }
            if (config->metadata.contact.email) {
                json_object_set_new(contact, "email", json_string(config->metadata.contact.email));
            }
            if (config->metadata.contact.url) {
                json_object_set_new(contact, "url", json_string(config->metadata.contact.url));
            }
            json_object_set_new(info, "contact", contact);
        }

        // Update license info if provided
        if (config->metadata.license.name || config->metadata.license.url) {
            json_t *license = json_object();
            if (config->metadata.license.name) {
                json_object_set_new(license, "name", json_string(config->metadata.license.name));
            }
            if (config->metadata.license.url) {
                json_object_set_new(license, "url", json_string(config->metadata.license.url));
            }
            json_object_set_new(info, "license", license);
        }

        // Convert the modified spec back to JSON
        dynamic_content = json_dumps(spec, JSON_INDENT(2));
        json_decref(spec);

        if (!dynamic_content) {
            log_this("SwaggerUI", "Failed to serialize modified swagger.json", LOG_LEVEL_ERROR, NULL);
            return MHD_NO;
        }

        response = MHD_create_response_from_buffer(
            strlen(dynamic_content),
            dynamic_content,
            MHD_RESPMEM_MUST_FREE
        );

    } else if (strcmp(url_path, "swagger-initializer.js") == 0) {
        // Get the server's base URL
        char *server_url = get_server_url(connection, config);
        if (!server_url) {
            return MHD_NO;
        }
        
        // Create dynamic content with the correct URL
        dynamic_content = create_dynamic_initializer((const char*)file->data, server_url, config);
        free(server_url);
        
        if (!dynamic_content) {
            return MHD_NO;
        }
        
        response = MHD_create_response_from_buffer(
            strlen(dynamic_content),
            dynamic_content,
            MHD_RESPMEM_MUST_FREE
        );
    } else {
        // For all other files, serve directly from memory
        response = MHD_create_response_from_buffer(
            file->size,
            (void*)file->data,
            MHD_RESPMEM_PERSISTENT
        );
    }

    if (!response) {
        free(dynamic_content);
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
            strncpy(clean_path, url_path, ext - url_path);
            clean_path[ext - url_path] = '\0';
            ext = strrchr(clean_path, '.');
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

    // Add CORS headers
    add_cors_headers(response);

    // Queue response
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

void cleanup_swagger_support(void) {
    free_swagger_files();
}


static void free_swagger_files(void) {
    if (!swagger_files) {
        return;
    }

    for (size_t i = 0; i < num_swagger_files; i++) {
        free(swagger_files[i].name);
        free(swagger_files[i].data);
    }

    free(swagger_files);
    swagger_files = NULL;
    num_swagger_files = 0;
}

// TAR format constants
#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_SIZE_OFFSET 124
#define TAR_SIZE_LENGTH 12

static bool load_swagger_files_from_tar(const uint8_t *tar_data, size_t tar_size) {
    // Try to decompress with Brotli using streaming API
    uint8_t *decompressed_data = NULL;
    size_t buffer_size = tar_size * 4;  // Initial estimate
    BrotliDecoderState* decoder = NULL;
    bool success = false;
    
    // Create decoder state with custom allocator and configure for high compression
    decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        log_this("SwaggerUI", "Failed to create Brotli decoder instance", LOG_LEVEL_ERROR, NULL);
        return false;
    }

    // Enable large window support for high compression
    if (!BrotliDecoderSetParameter(decoder, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1)) {
        log_this("SwaggerUI", "Failed to set Brotli large window parameter", LOG_LEVEL_ERROR, NULL);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }
    
    // Allocate initial buffer with extra space for decompression
    buffer_size = tar_size * 32;  // Increased buffer size for high compression ratio
    decompressed_data = malloc(buffer_size);
    if (!decompressed_data) {
        log_this("SwaggerUI", "Failed to allocate decompression buffer", LOG_LEVEL_ERROR, NULL);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }

    // Log buffer allocation
    log_this("SwaggerUI", "Allocated decompression buffer: %zu bytes", LOG_LEVEL_STATE, buffer_size);
    
    // Set up streaming variables
    const uint8_t* next_in = tar_data;
    size_t avail_in = tar_size;
    uint8_t* next_out = decompressed_data;
    size_t avail_out = buffer_size;
    size_t total_out = 0;
    BrotliDecoderResult result;
    
    // Log detailed information about the input data
    char debug_bytes[128] = {0};
    char debug_bytes_end[128] = {0};
    for (size_t i = 0; i < (tar_size > 32 ? 32 : tar_size); i++) {
        sprintf(debug_bytes + (i * 3), "%02x ", tar_data[i]);
    }
    for (size_t i = 0; i < 32 && i < tar_size; i++) {
        sprintf(debug_bytes_end + (i * 3), "%02x ", tar_data[tar_size - 32 + i]);
    }
    log_this("SwaggerUI", "Decompression input analysis:", LOG_LEVEL_STATE, NULL);
    log_this("SwaggerUI", "- Input size: %zu bytes", LOG_LEVEL_STATE, tar_size);
    log_this("SwaggerUI", "- First 32 bytes: %s", LOG_LEVEL_STATE, debug_bytes);
    log_this("SwaggerUI", "- Last 32 bytes: %s", LOG_LEVEL_STATE, debug_bytes_end);
    log_this("SwaggerUI", "- Decompression buffer: %zu bytes", LOG_LEVEL_STATE, buffer_size);

    // Check if this looks like a tar file already
    if (tar_size > 262 && memcmp(tar_data + 257, "ustar", 5) == 0) {
        log_this("SwaggerUI", "Input appears to be uncompressed tar (ustar magic detected)", 
                 LOG_LEVEL_STATE, NULL);
        log_this("SwaggerUI", "Skipping Brotli decompression", LOG_LEVEL_STATE, NULL);
        // Copy the tar data directly
        memcpy(decompressed_data, tar_data, tar_size);
        buffer_size = tar_size;
        success = true;
        goto cleanup;
    }

    // Log Brotli decompression parameters
    log_this("SwaggerUI", "Starting Brotli decompression with:", LOG_LEVEL_STATE, NULL);
    log_this("SwaggerUI", "- Large window: enabled", LOG_LEVEL_STATE, NULL);
    log_this("SwaggerUI", "- Initial buffer: %zu bytes", LOG_LEVEL_STATE, buffer_size);
    
    // Decompress in chunks
    do {
        result = BrotliDecoderDecompressStream(decoder, &avail_in, &next_in,
                                              &avail_out, &next_out, &total_out);
        
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            // Need more output space
            size_t new_size = buffer_size * 2;
            uint8_t* new_buffer = realloc(decompressed_data, new_size);
            if (!new_buffer) {
                log_this("SwaggerUI", "Failed to expand decompression buffer", LOG_LEVEL_ERROR, NULL);
                goto cleanup;
            }
            
            // Update pointers after realloc
            next_out = new_buffer + total_out;
            avail_out = new_size - total_out;
            decompressed_data = new_buffer;
            buffer_size = new_size;
        }
    } while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT ||
             result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT ||
             (result == BROTLI_DECODER_RESULT_SUCCESS && avail_in > 0));
    
    if (result != BROTLI_DECODER_RESULT_SUCCESS) {
        const char* error = BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder));
        log_this("SwaggerUI", "Failed to decompress UI files: %s", LOG_LEVEL_ERROR, error);
        goto cleanup;
    }
    
    // Set actual size
    buffer_size = total_out;
    success = true;
    
    // Log detailed hex dump of decompressed data
    char decompressed_bytes[128] = {0};
    char decompressed_bytes_end[128] = {0};
    for (size_t i = 0; i < (buffer_size > 32 ? 32 : buffer_size); i++) {
        sprintf(decompressed_bytes + (i * 3), "%02x ", decompressed_data[i]);
    }
    for (size_t i = 0; i < 32 && i < buffer_size; i++) {
        sprintf(decompressed_bytes_end + (i * 3), "%02x ", decompressed_data[buffer_size - 32 + i]);
    }
    log_this("SwaggerUI", "Decompression successful: %zu bytes -> %zu bytes",
             LOG_LEVEL_STATE, tar_size, buffer_size);
    log_this("SwaggerUI", "Decompressed data first 32 bytes: %s", LOG_LEVEL_STATE, decompressed_bytes);
    log_this("SwaggerUI", "Decompressed data last 32 bytes: %s", LOG_LEVEL_STATE, decompressed_bytes_end);
    
cleanup:
    BrotliDecoderDestroyInstance(decoder);
    if (!success) {
        free(decompressed_data);
        return false;
    }
    
    // Verify we have a valid tar file
    if (buffer_size <= 262 || memcmp(decompressed_data + 257, "ustar", 5) != 0) {
        log_this("SwaggerUI", "Invalid tar format (missing ustar magic)", LOG_LEVEL_ERROR, NULL);
        free(decompressed_data);
        return false;
    }
    
    // Process the tar file
    const uint8_t *current = decompressed_data;
    const uint8_t *end = decompressed_data + buffer_size;
    size_t capacity = 16; // Initial capacity for swagger_files array
    
    swagger_files = malloc(capacity * sizeof(SwaggerFile));
    if (!swagger_files) {
        free(decompressed_data);
        return false;
    }
    
    num_swagger_files = 0;

    while (current + TAR_BLOCK_SIZE <= end) {
        // Check for end of tar (empty block)
        bool is_empty = true;
        for (size_t i = 0; i < TAR_BLOCK_SIZE; i++) {
            if (current[i] != 0) {
                is_empty = false;
                break;
            }
        }
        if (is_empty) break;

        // Get file name
        char name[TAR_NAME_SIZE + 1] = {0};
        memcpy(name, current, TAR_NAME_SIZE);
        
        // Get file size (octal string)
        char size_str[TAR_SIZE_LENGTH + 1] = {0};
        memcpy(size_str, current + TAR_SIZE_OFFSET, TAR_SIZE_LENGTH);
        size_t file_size = strtoul(size_str, NULL, 8);

        // Skip empty files or directories
        if (file_size == 0 || name[strlen(name) - 1] == '/') {
            current += TAR_BLOCK_SIZE;
            continue;
        }

        // Grow array if needed
        if (num_swagger_files == capacity) {
            capacity *= 2;
            SwaggerFile *new_files = realloc(swagger_files, capacity * sizeof(SwaggerFile));
            if (!new_files) {
                free_swagger_files();
                free(decompressed_data);
                return false;
            }
            swagger_files = new_files;
        }

        // Store file
        swagger_files[num_swagger_files].name = strdup(name);
        swagger_files[num_swagger_files].size = file_size;
        swagger_files[num_swagger_files].data = malloc(file_size);
        
        if (!swagger_files[num_swagger_files].name || 
            !swagger_files[num_swagger_files].data) {
            free_swagger_files();
            free(decompressed_data);
            return false;
        }

        // Copy file data
        current += TAR_BLOCK_SIZE;
        memcpy(swagger_files[num_swagger_files].data, current, file_size);
        
        // Check if this file is Brotli compressed (ends in .br)
        size_t name_len = strlen(name);
        swagger_files[num_swagger_files].is_compressed = 
            (name_len > 3 && strcmp(name + name_len - 3, ".br") == 0);


        num_swagger_files++;

        // Move to next file (aligned to block size)
        current += (file_size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE * TAR_BLOCK_SIZE;
    }

    free(decompressed_data);
    return num_swagger_files > 0;
}

/**
 * Get the server's base URL from the connection
 * 
 * Constructs the base URL (scheme://host[:port]) from the connection information
 * 
 * @param connection The MHD connection
 * @param config The web server configuration
 * @return Dynamically allocated string with the base URL, or NULL on error
 */
static char* get_server_url(struct MHD_Connection *connection, 
                          const SwaggerConfig *config __attribute__((unused))) {
    const AppConfig *app_config = get_app_config();
    if (!app_config) {
        log_this("SwaggerUI", "Failed to get app config", LOG_LEVEL_ERROR, NULL);
        return NULL;
    }

    // Get host from header or use localhost
    const char *host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    if (!host) {
        // If no Host header, construct from config
        char *url = NULL;
        if (asprintf(&url, "http://localhost:%d", app_config->web.port) == -1) {
            return NULL;
        }
        return url;
    }

    // Check if host already includes port
    if (strchr(host, ':')) {
        // Host includes port, use as-is
        char *url = NULL;
        if (asprintf(&url, "http://%s", host) == -1) {
            return NULL;
        }
        return url;
    }

    // Add port from config
    char *url = NULL;
    if (asprintf(&url, "http://%s:%d", host, app_config->web.port) == -1) {
        return NULL;
    }
    return url;
}

/**
 * Create a dynamic swagger-initializer.js with the correct server URL
 * 
 * @param base_content The original initializer content (unused as we generate content from scratch)
 * @param server_url The server's base URL from the incoming request
 * @param config The web server configuration containing the swagger prefix
 * @return Dynamically allocated string with the modified content, or NULL on error
 */
static char* create_dynamic_initializer(const char *base_content __attribute__((unused)), 
                                      const char *server_url,
                                      const SwaggerConfig *config) {
    // Get the API prefix from the global config
    const AppConfig *app_config = get_app_config();
    if (!app_config || !app_config->api.prefix) {
        log_this("SwaggerUI", "API configuration not available", LOG_LEVEL_ERROR, NULL);
        return NULL;
    }

    // Create the new initializer content with server URL update
    char *dynamic_content = NULL;
    if (asprintf(&dynamic_content,
        "window.onload = function() {\n"
        "  fetch('%s%s/swagger.json').then(response => response.json()).then(spec => {\n"
        "    // Update server URL to match current host\n"
        "    // Using configured API prefix instead of hardcoded value\n"
        "    spec.servers = [{url: '%s%s', description: 'Current server'}];\n"
        "    window.ui = SwaggerUIBundle({\n"
        "      spec: spec,\n"
        "      dom_id: '#swagger-ui',\n"
        "      deepLinking: true,\n"
        "      presets: [\n"
        "        SwaggerUIBundle.presets.apis,\n"
        "        SwaggerUIStandalonePreset\n"
        "      ],\n"
        "      plugins: [\n"
        "        SwaggerUIBundle.plugins.DownloadUrl\n"
        "      ],\n"
        "      layout: \"StandaloneLayout\",\n"
        "      tryItOutEnabled: %s,\n"
        "      displayOperationId: %s,\n"
        "      defaultModelsExpandDepth: %d,\n"
        "      defaultModelExpandDepth: %d,\n"
        "      showExtensions: %s,\n"
        "      showCommonExtensions: %s,\n"
        "      docExpansion: \"%s\",\n"
        "      syntaxHighlight: {\n"
        "        theme: \"%s\"\n"
        "      }\n"
        "    });\n"
        "  });\n"
        "};", 
        server_url, config->prefix,
        server_url, app_config->api.prefix,
        config->ui_options.try_it_enabled ? "true" : "false",
        config->ui_options.display_operation_id ? "true" : "false",
        config->ui_options.default_models_expand_depth,
        config->ui_options.default_model_expand_depth,
        config->ui_options.show_extensions ? "true" : "false",
        config->ui_options.show_common_extensions ? "true" : "false",
        config->ui_options.doc_expansion,
        config->ui_options.syntax_highlight_theme) == -1) {
        return NULL;
    }

    return dynamic_content;
}