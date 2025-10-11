/*
 * System Recent API Endpoint Implementation
 * 
 * Implements the /api/system/recent endpoint that provides access
 * to the most recent log messages in reverse chronological order.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "recent.h"
#include "../system_service.h"
#include "../../../api/api_utils.h"

// Extract log lines from raw text into an array
// This function can be mocked in unit tests
char** extract_log_lines(const char *raw_text, size_t *line_count) {
    if (!raw_text || !line_count) return NULL;

    char **lines = NULL;
    *line_count = 0;

    // Make a copy of raw_text since strtok modifies it
    char *text_copy = strdup(raw_text);
    if (!text_copy) return NULL;

    // Split into lines and count them
    const char *line = strtok(text_copy, "\n");
    while (line) {
        char **new_lines = realloc(lines, (*line_count + 1) * sizeof(char*));
        if (!new_lines) {
            free(text_copy);
            for (size_t i = 0; i < *line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }
        lines = new_lines;

        lines[*line_count] = strdup(line);
        if (!lines[*line_count]) {
            free(text_copy);
            for (size_t i = 0; i < *line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            return NULL;
        }

        (*line_count)++;
        line = strtok(NULL, "\n");
    }

    free(text_copy);
    return lines;
}

// Build final text from lines in reverse order (newest to oldest)
// This function can be mocked in unit tests
char* build_reverse_log_text(char **lines, size_t line_count) {
    if (!lines || line_count == 0) return NULL;

    // Calculate total size needed
    size_t total_size = 0;
    for (size_t i = 0; i < line_count; i++) {
        if (lines[i]) {
            total_size += strlen(lines[i]) + 1;  // +1 for newline
        }
    }

    // Allocate final buffer
    char *processed_text = malloc(total_size + 1);  // +1 for null terminator
    if (!processed_text) return NULL;

    // Build final string in reverse order (newest to oldest)
    size_t pos = 0;
    for (size_t i = line_count; i > 0; i--) {
        size_t idx = i - 1;  // Convert to 0-based index
        if (lines[idx]) {
            size_t len = strlen(lines[idx]);
            memcpy(processed_text + pos, lines[idx], len);
            if (i > 1) {  // Add newline after all but the last line
                processed_text[pos + len] = '\n';
                pos += len + 1;
            } else {
                pos += len;
            }
        }
    }
    processed_text[pos] = '\0';

    return processed_text;
}

// Handle GET /api/system/recent requests
// Returns the most recent log messages in reverse chronological order
// Success: 200 OK with plain text response
// Error: 500 Internal Server Error with error details
enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection) {
    log_this(SR_API, "Handling recent logs request", LOG_LEVEL_STATE, 0);

    // Get all available messages from the rolling buffer
#ifdef UNITY_TEST_MODE
    // In test mode, use mock data
    char *raw_text = strdup("Test log line 1\nTest log line 2\nTest log line 3");
#else
    char *raw_text = log_get_last_n(500);  // Get all messages (buffer size is 500)
#endif
    if (!raw_text) {
        log_this(SR_API, "Failed to get log messages", LOG_LEVEL_ERROR, 0);
#ifdef UNITY_TEST_MODE
        // In test mode, return MHD_NO directly
        return MHD_NO;
#else
        json_t *error = json_pack("{s:s}", "error", "Failed to retrieve log messages");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
#endif
    }

    // Process the log text
    size_t line_count = 0;
    char **lines = extract_log_lines(raw_text, &line_count);
    if (!lines) {
        free(raw_text);
        log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

    char *processed_text = build_reverse_log_text(lines, line_count);
    if (!processed_text) {
        free(raw_text);
        for (size_t i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        log_this(SR_API, "Memory allocation failed", LOG_LEVEL_ERROR, 0);
        return MHD_NO;
    }

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
