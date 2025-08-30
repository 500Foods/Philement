/*
 * System Recent API Endpoint Implementation
 * 
 * Implements the /api/system/recent endpoint that provides access
 * to the most recent log messages in reverse chronological order.
 */

 // Global includes 
#include "../../../hydrogen.h"

// Local includes
#include "recent.h"
#include "../system_service.h"
#include "../../../api/api_utils.h"

// Handle GET /api/system/recent requests
// Returns the most recent log messages in reverse chronological order
// Success: 200 OK with plain text response
// Error: 500 Internal Server Error with error details
enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection) {
    log_this(SR_API, "Handling recent logs request", LOG_LEVEL_STATE);

    // Get all available messages from the rolling buffer
    char *raw_text = log_get_last_n(500);  // Get all messages (buffer size is 500)
    if (!raw_text) {
        log_this(SR_API, "Failed to get log messages", LOG_LEVEL_ERROR);
        json_t *error = json_pack("{s:s}", "error", "Failed to retrieve log messages");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
    }

    // Split into lines and count them
    char **lines = NULL;
    size_t line_count = 0;
    size_t total_size = 0;

    // First pass: count lines and calculate total size needed
    char *line = strtok(raw_text, "\n");
    while (line) {
        char **new_lines = realloc(lines, (line_count + 1) * sizeof(char*));
        if (!new_lines) {
            free(raw_text);
            for (size_t i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR);
            return MHD_NO;
        }
        lines = new_lines;

        lines[line_count] = strdup(line);
        if (!lines[line_count]) {
            free(raw_text);
            for (size_t i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR);
            return MHD_NO;
        }

        total_size += strlen(line) + 1;  // +1 for newline
        line_count++;
        line = strtok(NULL, "\n");
    }

    // Allocate final buffer
    char *processed_text = malloc(total_size + 1);  // +1 for null terminator
    if (!processed_text) {
        free(raw_text);
        for (size_t i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR);
        return MHD_NO;
    }

    // Build final string in reverse order (newest to oldest)
    size_t pos = 0;
    for (size_t i = line_count; i > 0; i--) {
        size_t idx = i - 1;  // Convert to 0-based index
        size_t len = strlen(lines[idx]);
        memcpy(processed_text + pos, lines[idx], len);
        if (i > 1) {  // Add newline after all but the last line
            processed_text[pos + len] = '\n';
            pos += len + 1;
        } else {
            pos += len;
        }
    }
    processed_text[pos] = '\0';

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
        MHD_RESPMEM_MUST_FREE  // MHD will free processed_text when done
    );

    if (!response) {
        log_this(SR_API, "Failed to create response", LOG_LEVEL_ERROR);
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
