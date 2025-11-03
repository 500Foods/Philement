/*
 * System AppConfig API Endpoint Implementation
 * 
 * Implements the /api/system/appconfig endpoint that provides the current
 * application configuration in plain text format.
 */

// Project includes 
#include <src/hydrogen.h>
#include <src/api/system/system_service.h>
#include <src/api/api_utils.h>

// Local includes
#include "appconfig.h"

// Handle GET /api/system/appconfig requests
// Returns current application configuration in plain text format
// Success: 200 OK with plain text response
// Error: 500 Internal Server Error with error details
enum MHD_Result handle_system_appconfig_request(struct MHD_Connection *connection) {
    log_this(SR_API, "Handling appconfig endpoint request", LOG_LEVEL_DEBUG, 0);

    // Get current configuration
    if (!app_config) {
        log_this(SR_API, "Failed to get application configuration", LOG_LEVEL_ERROR, 0);
        json_t *error = json_pack("{s:s}", "error", "Failed to get configuration");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
    }

    // Dump configuration to logs
    dumpAppConfig(app_config, NULL);

    // Get the dumped configuration from logs
    char *raw_text = log_get_messages(SR_CONFIG_CURRENT);
    if (!raw_text) {
        log_this(SR_API, "Failed to get configuration dump", LOG_LEVEL_ERROR, 0);
        json_t *error = json_pack("{s:s}", "error", "Failed to generate configuration");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
    }

    // Process the raw text to clean up format
    char *processed_text = NULL;
    size_t processed_size = 0;
    char **lines = NULL;
    size_t line_count = 0;

    // Find the position of "APPCONFIG" in the first line
    char *first_line = strtok(raw_text, "\n");
    if (!first_line) {
        free(raw_text);
        log_this(SR_API, "No configuration output found", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    const char *config_marker = strstr(first_line, "APPCONFIG");
    if (!config_marker) {
        free(raw_text);
        log_this(SR_API, "Could not find APPCONFIG marker", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    // Calculate the offset where content starts
    size_t content_offset = (size_t)(config_marker - first_line);

    // Process first line
    char **new_lines = malloc(sizeof(char*));
    if (!new_lines) {
        free(raw_text);
        log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }
    lines = new_lines;
    lines[line_count++] = strdup(config_marker);

    // Process remaining lines
    const char *line;
    while ((line = strtok(NULL, "\n"))) {
        new_lines = realloc(lines, (line_count + 1) * sizeof(char*));
        if (!new_lines) {
            free(raw_text);
            for (size_t i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }
        lines = new_lines;

        // Skip to the same position as APPCONFIG in all lines
        size_t line_len = strlen(line);
        if (line_len > content_offset) {
            lines[line_count] = strdup(line + content_offset);
        } else {
            lines[line_count] = strdup("");  // Line too short, use empty string
        }
        
        if (!lines[line_count]) {
            free(raw_text);
            for (size_t i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
            return MHD_NO;
        }
        
        line_count++;
    }

    // Calculate total size needed
    for (size_t i = 0; i < line_count; i++) {
        processed_size += strlen(lines[i]) + 1; // +1 for newline
    }

    // Allocate and build final string in reverse order
    processed_text = malloc(processed_size + 1); // +1 for null terminator
    if (!processed_text) {
        free(raw_text);
        for (size_t i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    size_t pos = 0;
    for (size_t i = line_count; i > 0; i--) {
        size_t len = strlen(lines[i-1]);
        memcpy(processed_text + pos, lines[i-1], len);
        if (i > 1) { // Add newline after all but the last line
            processed_text[pos + len] = '\n';
            pos += len + 1;
        } else {
            pos += len;
        }
    }
    processed_text[pos] = '\0'; // Properly terminate the string

    // Clean up temporary storage
    free(raw_text);
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    // Prepare response
    struct MHD_Response *response = MHD_create_response_from_buffer(
        strlen(processed_text),
        (void*)processed_text,
        MHD_RESPMEM_MUST_FREE  // MHD will free config_text when done
    );

    if (!response) {
        log_this(SR_API, "Failed to create response", LOG_LEVEL_ERROR, 0);
        free(processed_text);
        return MHD_NO;
    }

    // Set content type to plain text
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");

    // Send response
    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}
