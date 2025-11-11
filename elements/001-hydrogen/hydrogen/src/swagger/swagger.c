// Global includes
#include <src/hydrogen.h>

// Third-party libraries
#include <brotli/decode.h>

// Local includes
#include "swagger.h"
#include <src/webserver/web_server_core.h>
#include <src/webserver/web_server_compression.h>

// Structure to hold in-memory Swagger files
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// Function declarations
void free_swagger_files(void);

// Global state
SwaggerFile *swagger_files = NULL;
size_t num_swagger_files = 0;
bool swagger_initialized = false;
const SwaggerConfig *global_swagger_config = NULL;  // Global config for validator

/**
 * Web server validator wrapper - matches MHD signature
 */
bool swagger_url_validator(const char *url) {
    return is_swagger_request(url, global_swagger_config);
}

/**
 * Web server handler wrapper - matches MHD signature
 */
// cppcheck-suppress[constParameterPointer] - MHD callback signature requires void*
enum MHD_Result swagger_request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method __attribute__((unused)),
                                       const char *version __attribute__((unused)),
                                       const char *upload_data __attribute__((unused)),
                                       size_t *upload_data_size __attribute__((unused)),
                                       void **con_cls __attribute__((unused))) {
    const SwaggerConfig *config = (const SwaggerConfig *)cls;
    return handle_swagger_request(connection, url, config);
}




/**
 * Cleanup wrapper for shutdown
 */
void cleanup_swagger_support(void) {
    free_swagger_files();
}

// Forward declarations for main implementation functions

// Forward declaration for payload cache access
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files, size_t *num_files, size_t *capacity);
char* get_server_url(struct MHD_Connection *connection, const SwaggerConfig *config);
char* create_dynamic_initializer(const char *base_content, const char *server_url, const SwaggerConfig *config);
bool decompress_brotli_data(const uint8_t *compressed_data, size_t compressed_size, uint8_t **decompressed_data, size_t *decompressed_size);

/**
 * Initialize swagger support using payload cache files
 * This function takes pre-loaded files from payload cache and sets up memory structures
 */
bool init_swagger_support_from_payload(SwaggerConfig *config, PayloadFile *payload_files, size_t num_payload_files) {
    if (!config) {
        log_this(SR_SWAGGER, "Invalid config parameter", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Store config globally for validator
    global_swagger_config = config;

    // Check all shutdown flags atomically

    // Prevent initialization during shutdown
    if (server_stopping || web_server_shutdown) {
        log_this(SR_SWAGGER, "Cannot initialize Swagger UI during shutdown", LOG_LEVEL_DEBUG, 0);
        swagger_initialized = false;
        return false;
    }

    // Only proceed if we're in startup phase
    if (!server_starting || server_stopping || web_server_shutdown) {
        log_this(SR_SWAGGER, "Cannot initialize - invalid system state", LOG_LEVEL_DEBUG, 0);
        return false;
    }

    // Skip if already initialized or disabled
    if (swagger_initialized || !config->enabled) {
        if (swagger_initialized) {
            log_this(SR_SWAGGER, "Already initialized", LOG_LEVEL_DEBUG, 0);
        }
        return swagger_initialized;
    }

    // Free any existing files first
    free_swagger_files();

    // Allocate SwaggerFile array
    swagger_files = calloc(num_payload_files, sizeof(SwaggerFile));
    if (!swagger_files) {
        log_this(SR_SWAGGER, "Failed to allocate swagger files array", LOG_LEVEL_ERROR, 0);
        return false;
    }

    num_swagger_files = 0;

    // Convert PayloadFile to SwaggerFile
    for (size_t i = 0; i < num_payload_files; i++) {
        // Clean the file name to remove prefixes (e.g., "swagger/index.html" -> "index.html")
        const char *clean_name = payload_files[i].name;
        if (strncmp(payload_files[i].name, "swagger/", 8) == 0) {
            clean_name = payload_files[i].name + 8; // Skip "swagger/" prefix
        }

        swagger_files[i].name = strdup(clean_name);
        swagger_files[i].data = payload_files[i].data; // Reference the payload data
        swagger_files[i].size = payload_files[i].size;
        swagger_files[i].is_compressed = payload_files[i].is_compressed;

        if (!swagger_files[i].name) {
            log_this(SR_SWAGGER, "Failed to allocate memory for file name", LOG_LEVEL_ERROR, 0);
            free_swagger_files();
            return false;
        }

        num_swagger_files++;
    }

    // Update configuration
    config->payload_available = true;
    swagger_initialized = true;

    log_this(SR_SWAGGER, "Loaded %zu swagger files from payload cache", LOG_LEVEL_DEBUG, 1, num_swagger_files);

    return true;
}

/**
 * Decompress Brotli-compressed data
 *
 * @param compressed_data The compressed data
 * @param compressed_size Size of compressed data
 * @param decompressed_data Output pointer for decompressed data (caller must free)
 * @param decompressed_size Output size of decompressed data
 * @return true if successful, false otherwise
 */
bool decompress_brotli_data(const uint8_t *compressed_data, size_t compressed_size,
                           uint8_t **decompressed_data, size_t *decompressed_size) {
    if (!compressed_data || !decompressed_data || !decompressed_size) {
        return false;
    }

    // Create Brotli decoder
    BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    if (!decoder) {
        log_this(SR_SWAGGER, "Failed to create Brotli decoder", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Start with 4x the compressed size
    size_t buffer_size = compressed_size * 4;
    uint8_t *output = malloc(buffer_size);
    if (!output) {
        log_this(SR_SWAGGER, "Failed to allocate decompression buffer", LOG_LEVEL_ERROR, 0);
        BrotliDecoderDestroyInstance(decoder);
        return false;
    }

    // Set up input/output parameters
    const uint8_t *next_in = compressed_data;
    size_t available_in = compressed_size;
    uint8_t *next_out = output;
    size_t available_out = buffer_size;
    size_t total_out = 0;

    // Decompress until done
    BrotliDecoderResult result;
    do {
        result = BrotliDecoderDecompressStream(
            decoder,
            &available_in, &next_in,
            &available_out, &next_out,
            &total_out
        );

        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            // Need more output space
            size_t current_position = (size_t)(next_out - output);
            buffer_size *= 2;
            uint8_t *new_buffer = realloc(output, buffer_size);
            if (!new_buffer) {
                log_this(SR_SWAGGER, "Failed to resize decompression buffer", LOG_LEVEL_ERROR, 0);
                free(output);
                BrotliDecoderDestroyInstance(decoder);
                return false;
            }
            
            output = new_buffer;
            next_out = output + current_position;
            available_out = buffer_size - current_position;
        } else if (result == BROTLI_DECODER_RESULT_ERROR) {
            log_this(SR_SWAGGER, "Brotli decompression error: %s", LOG_LEVEL_ERROR, 1,
                    BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder)));
            free(output);
            BrotliDecoderDestroyInstance(decoder);
            return false;
        }
    } while (result != BROTLI_DECODER_RESULT_SUCCESS);

    BrotliDecoderDestroyInstance(decoder);

    *decompressed_data = output;
    *decompressed_size = total_out;
    return true;
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
    
    // Check if URL starts with prefix and is followed by either:
    // - end of string
    // - a slash (followed by any valid path)
    if (strncmp(url, config->prefix, prefix_len) == 0 && 
        (url[prefix_len] == '\0' || url[prefix_len] == '/')) {
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
            log_this(SR_SWAGGER, "Redirecting %s to %s for proper relative path resolution", LOG_LEVEL_DEBUG, 2, url, redirect_url);
                    
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
        url_path = "swagger.html";
    } else if (*url_path == '/') {
        url_path++; // Skip leading slash for other paths
    }
    
    // Log the URL processing for debugging
    log_this(SR_SWAGGER, "Request: Original URL: %s, Processed path: %s", LOG_LEVEL_DEBUG, 2, url, url_path);

    // Try to find the requested file - prioritize uncompressed versions for browser compatibility
    SwaggerFile *file = NULL;
    bool client_accepts_br = client_accepts_brotli(connection);

    // Debug logging for troubleshooting
    const char *accept_encoding = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Accept-Encoding");
    if (accept_encoding) {
        log_this(SR_SWAGGER, "Client Accept-Encoding: %s", LOG_LEVEL_DEBUG, 1, accept_encoding);
    } else {
        log_this(SR_SWAGGER, "No Accept-Encoding header from client", LOG_LEVEL_DEBUG, 0);
    }

    // First try to find an exact match (handles direct .br requests)
    for (size_t i = 0; i < num_swagger_files; i++) {
        if (strcmp(swagger_files[i].name, url_path) == 0) {
            file = &swagger_files[i];
            const char *file_type = file->is_compressed ? "compressed" : "uncompressed";
            log_this(SR_SWAGGER, "Found exact match for %s (%s)", LOG_LEVEL_DEBUG, 2, url_path, file_type);
            break;
        }
    }

    // If not found, try to find both compressed and uncompressed versions
    if (!file) {
        char br_path[256];
        SwaggerFile *compressed_file = NULL;
        SwaggerFile *uncompressed_file = NULL;

        // Look for both .br and non-.br versions
        if (!strstr(url_path, ".br")) {
            snprintf(br_path, sizeof(br_path), "%s.br", url_path);
        }

        for (size_t i = 0; i < num_swagger_files; i++) {
            const char *name = swagger_files[i].name;
            if (!strstr(url_path, ".br") && strcmp(name, br_path) == 0) {
                compressed_file = &swagger_files[i];
            } else if (strcmp(name, url_path) == 0) {
                uncompressed_file = &swagger_files[i];
            }
        }

        // Prefer uncompressed file for browser compatibility
        if (uncompressed_file) {
            file = uncompressed_file;
            log_this(SR_SWAGGER, "Using uncompressed version of %s", LOG_LEVEL_DEBUG, 1, url_path);
        } else if (compressed_file && client_accepts_br) {
            file = compressed_file;
            log_this(SR_SWAGGER, "Using compressed version of %s (client supports brotli)", LOG_LEVEL_DEBUG, 1, url_path);
        } else if (compressed_file) {
            // Client doesn't support brotli but we only have compressed version
            // We'll decompress it on-the-fly below
            file = compressed_file;
            log_this(SR_SWAGGER, "Will decompress %s for client compatibility", LOG_LEVEL_DEBUG, 1, url_path);
        } else {
            log_this(SR_SWAGGER, "No version found for %s", LOG_LEVEL_ERROR, 1, url_path);
        }
    }

    // If still not found and the request was for a .br file,
    // try without the .br extension
    if (!file && strlen(url_path) > 3 && 
        strcmp(url_path + strlen(url_path) - 3, ".br") == 0) {
        char base_path[256];
        size_t path_len = strlen(url_path);
        if (path_len - 3U < sizeof(base_path)) {
            memcpy(base_path, url_path, path_len - 3U);
            base_path[path_len - 3U] = '\0';
            
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

    // Handle decompression for clients that don't support brotli
    uint8_t *decompressed_data = NULL;
    size_t decompressed_size = 0;
    bool needs_decompression = file->is_compressed && !client_accepts_br;
    
    if (needs_decompression) {
        if (!decompress_brotli_data(file->data, file->size, &decompressed_data, &decompressed_size)) {
            log_this(SR_SWAGGER, "Failed to decompress %s for client", LOG_LEVEL_ERROR, 1, url_path);
            return MHD_NO;
        }
        log_this(SR_SWAGGER, "Decompressed %s: %zu -> %zu bytes", LOG_LEVEL_DEBUG, 3,
                url_path, file->size, decompressed_size);
    }

    // If this is swagger.json or swagger-initializer.js, we need to modify it
    struct MHD_Response *response;
    char *dynamic_content = NULL;
    
    if (strcmp(url_path, "swagger.json") == 0) {
        // Parse the original swagger.json (use decompressed data if available)
        json_error_t error;
        const char *json_data = needs_decompression ? (const char*)decompressed_data : (const char*)file->data;
        size_t json_size = needs_decompression ? decompressed_size : file->size;
        json_t *spec = json_loadb(json_data, json_size, 0, &error);
        if (!spec) {
            log_this(SR_SWAGGER, "Failed to parse swagger.json: %s", LOG_LEVEL_ERROR, 1, error.text);
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

        // Get app config for API prefix
        if (!app_config || !app_config->api.prefix) {
            log_this(SR_SWAGGER, "API configuration not available", LOG_LEVEL_ERROR, 0);
            json_decref(spec);
            return MHD_NO;
        }

        // Update servers array with API prefix
        json_t *servers = json_array();
        json_t *server = json_object();
        char *server_url = get_server_url(connection, config);
        if (!server_url) {
            json_decref(spec);
            json_decref(servers);
            json_decref(server);
            return MHD_NO;
        }

        char *full_url;
        int asprintf_result = asprintf(&full_url, "%s%s", server_url, app_config->api.prefix);
        if (asprintf_result == -1) {
            free(server_url);
            json_decref(spec);
            json_decref(servers);
            json_decref(server);
            return MHD_NO;
        }
        free(server_url);

        json_object_set_new(server, "url", json_string(full_url));
        json_object_set_new(server, "description", json_string("Current server"));
        json_array_append_new(servers, server);
        json_object_set_new(spec, "servers", servers);
        free(full_url);

        // Log the server URL for debugging
        log_this(SR_SWAGGER, "Updated swagger.json with API prefix: %s", LOG_LEVEL_DEBUG, 1, app_config->api.prefix);

        // Convert the modified spec back to JSON
        dynamic_content = json_dumps(spec, JSON_INDENT(2));
        json_decref(spec);

        if (!dynamic_content) {
            log_this(SR_SWAGGER, "Failed to serialize modified swagger.json", LOG_LEVEL_ERROR, 0);
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
            free(decompressed_data);
            return MHD_NO;
        }
        
        // Create dynamic content with the correct URL (use decompressed data if available)
        const char *js_data = needs_decompression ? (const char*)decompressed_data : (const char*)file->data;
        dynamic_content = create_dynamic_initializer(js_data, server_url, config);
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
        // For all other files, serve directly from memory or decompressed data
        if (needs_decompression) {
            response = MHD_create_response_from_buffer(
                decompressed_size,
                (void*)decompressed_data,
                MHD_RESPMEM_MUST_FREE  // MHD will free the decompressed data
            );
            decompressed_data = NULL;  // Transfer ownership to MHD
        } else {
            response = MHD_create_response_from_buffer(
                file->size,
                (void*)file->data,
                MHD_RESPMEM_PERSISTENT
            );
        }
    }

    if (!response) {
        free(dynamic_content);
        free(decompressed_data);  // In case it wasn't transferred to MHD
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

    // Add compression header only if we're actually serving compressed content
    // (i.e., file is compressed AND client supports brotli)
    if (file->is_compressed && !needs_decompression) {
        log_this(SR_SWAGGER, "Serving compressed file: %s (Content-Encoding: br)", LOG_LEVEL_DEBUG, 1, url_path);
        add_brotli_header(response);
    } else if (needs_decompression) {
        log_this(SR_SWAGGER, "Serving decompressed file: %s", LOG_LEVEL_DEBUG, 1, url_path);
    } else {
        log_this(SR_SWAGGER, "Serving uncompressed file: %s", LOG_LEVEL_DEBUG, 1, url_path);
    }

    // Add CORS headers
    add_cors_headers(response);

    // Queue response
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}



void free_swagger_files(void) {
    if (!swagger_files) {
        return;
    }

    for (size_t i = 0; i < num_swagger_files; i++) {
        free(swagger_files[i].name);
        // CRITICAL: These data pointers are COPIES not references!
        // get_payload_files_by_prefix() allocates and copies the data
        // so we MUST free it here to prevent memory leaks
        free(swagger_files[i].data);
    }

    free(swagger_files);
    swagger_files = NULL;
    num_swagger_files = 0;
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
char* get_server_url(struct MHD_Connection *connection,
                          const SwaggerConfig *config __attribute__((unused))) {
    if (!app_config) {
        log_this(SR_SWAGGER, "Failed to get app config", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Get host from header or use localhost
    const char *host = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Host");
    if (!host) {
        // If no Host header, construct from config
        char *url = NULL;
        if (asprintf(&url, "http://localhost:%d", app_config->webserver.port) == -1) {
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
    if (asprintf(&url, "http://%s:%d", host, app_config->webserver.port) == -1) {
        return NULL;
    }
    return url;
}

/**
 * Cleanup wrapper for shutdown
 */
char* create_dynamic_initializer(const char *base_content __attribute__((unused)),
                                      const char *server_url,
                                      const SwaggerConfig *config) {
    // Validate parameters
    if (!config) {
        log_this(SR_SWAGGER, "NULL config parameter passed to create_dynamic_initializer", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Get the API prefix from the global config
    if (!app_config || !app_config->api.prefix) {
        log_this(SR_SWAGGER, "API configuration not available", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Create the new initializer content with server URL update
    char *dynamic_content = NULL;
    if (asprintf(&dynamic_content,
        "window.onload = function() {\n"
        "  fetch('%s%s/swagger.json').then(response => response.json()).then(spec => {\n"
        "    // Update server URL to match current host\n"
        "    // Use API prefix from app config\n"
        "    spec.servers = [{\n"
        "      url: '%s%s',\n"
        "      description: 'Current server'\n"
        "    }];\n"
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
