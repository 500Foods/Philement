// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <brotli/decode.h>

// Project headers
#include "web_server_swagger.h"
#include "web_server_core.h"
#include "web_server_compression.h"
#include "../logging/logging.h"

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

// Forward declarations
static bool extract_swagger_payload(const char *executable_path);
static bool load_swagger_files_from_tar(const uint8_t *tar_data, size_t tar_size);
static void free_swagger_files(void);
static char* get_server_url(struct MHD_Connection *connection, const WebConfig *config);
static char* create_dynamic_initializer(const char *base_content, const char *server_url, const WebConfig *config);

void cleanup_swagger_support(void);

bool init_swagger_support(WebConfig *config) {
    if (!config || !config->swagger.enabled) {
        return false;
    }

    // Get executable path
    char *executable_path = get_executable_path();
    if (!executable_path) {
        log_this("WebServer", "Failed to get executable path", LOG_LEVEL_ERROR);
        return false;
    }

    // Try to extract Swagger payload
    bool success = extract_swagger_payload(executable_path);
    free(executable_path);

    // Update configuration based on payload availability
    config->swagger.payload_available = success;
    
    if (success) {
        log_this("WebServer", "Swagger UI payload extracted successfully", LOG_LEVEL_INFO);
        log_this("WebServer", "Swagger UI files available:", LOG_LEVEL_INFO);
        
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
            
            log_this("WebServer", "-> %s (%s%s)", LOG_LEVEL_INFO,
                    swagger_files[i].name, size_display, 
                    swagger_files[i].is_compressed ? ", compressed" : "");
        }
    } else {
        log_this("WebServer", "No Swagger UI payload found in executable", LOG_LEVEL_WARN);
    }

    return success;
}

bool is_swagger_request(const char *url, const WebConfig *config) {
    if (!config || !config->swagger.enabled || !config->swagger.payload_available || 
        !config->swagger.prefix || !url) {
        return false;
    }

    return strncmp(url, config->swagger.prefix, strlen(config->swagger.prefix)) == 0;
}

enum MHD_Result handle_swagger_request(struct MHD_Connection *connection,
                                     const char *url,
                                     const WebConfig *config) {
    if (!connection || !url || !config) {
        return MHD_NO;
    }

    // First check if this is exactly the swagger prefix with no trailing slash
    // If so, redirect to the same URL with a trailing slash
    // This ensures all relative assets are correctly loaded
    size_t prefix_len = strlen(config->swagger.prefix);
    if (strcmp(url, config->swagger.prefix) == 0) {
        char *redirect_url;
        if (asprintf(&redirect_url, "%s/", url) != -1) {
            log_this("WebServer", "Redirecting %s to %s for proper relative path resolution", 
                    LOG_LEVEL_INFO, url, redirect_url);
                    
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
    log_this("WebServer", "Swagger request: Original URL: %s, Processed path: %s", 
             LOG_LEVEL_INFO, url, url_path);

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

    // If this is swagger-initializer.js, we need to modify it with the correct URL
    struct MHD_Response *response;
    char *dynamic_content = NULL;
    
    if (strcmp(url_path, "swagger-initializer.js") == 0) {
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

static bool extract_swagger_payload(const char *executable_path) {
    // Open the executable
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        return false;
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return false;
    }

    // Map the file
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (file_data == MAP_FAILED) {
        return false;
    }

    // Search for marker (it's appended after the tar.br)
    const char *marker = memmem(file_data, st.st_size,
                               SWAGGER_PAYLOAD_MARKER, 
                               strlen(SWAGGER_PAYLOAD_MARKER));

    if (!marker) {
        munmap(file_data, st.st_size);
        return false;
    }

    // Get tar.br size from the 8 bytes after the marker
    const uint8_t *size_bytes = (uint8_t*)(marker + strlen(SWAGGER_PAYLOAD_MARKER));
    size_t tar_size = 0;
    for (int i = 0; i < 8; i++) {
        tar_size = (tar_size << 8) | size_bytes[i];
    }

    // Extract tar.br data (it's before the marker)
    const uint8_t *tar_data = (uint8_t*)(marker - tar_size);
    
    // Load the files from the tar data
    bool success = load_swagger_files_from_tar(tar_data, tar_size);

    munmap(file_data, st.st_size);
    return success;
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
    // First decompress the Brotli data
    uint8_t *decompressed_data = NULL;
    size_t buffer_size = tar_size * 4; // Estimate max size
    decompressed_data = malloc(buffer_size);
    if (!decompressed_data) {
        log_this("WebServer", "Failed to allocate memory for Swagger UI payload", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Decompress the data
    BrotliDecoderResult result = BrotliDecoderDecompress(
        tar_size, tar_data,
        &buffer_size, decompressed_data);
    
    if (result != BROTLI_DECODER_RESULT_SUCCESS) {
        log_this("WebServer", "Failed to decompress Swagger UI payload", LOG_LEVEL_ERROR);
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
                          const WebConfig *config __attribute__((unused))) {
    // Host header is mandatory in HTTP/1.1
    const char *host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    if (!host) {
        log_this("WebServer", "No Host header in Swagger UI request", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Determine protocol (https or http) from X-Forwarded-Proto
    const char *forwarded_proto = MHD_lookup_connection_value(connection, 
                                                            MHD_HEADER_KIND, 
                                                            "X-Forwarded-Proto");
    const char *scheme = (forwarded_proto && strcmp(forwarded_proto, "https") == 0) 
                        ? "https" : "http";

    // Construct the base URL from the request information only
    char *url = NULL;
    if (asprintf(&url, "%s://%s", scheme, host) == -1) {
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
                                      const WebConfig *config) {
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
        "      tryItOutEnabled: true,\n"
        "      displayOperationId: true,\n"
        "      defaultModelsExpandDepth: 1,\n"
        "      defaultModelExpandDepth: 1,\n"
        "      docExpansion: \"list\"\n"
        "    });\n"
        "  });\n"
        "};", server_url, config->swagger.prefix, server_url, config->api_prefix) == -1) {
        return NULL;
    }

    return dynamic_content;
}