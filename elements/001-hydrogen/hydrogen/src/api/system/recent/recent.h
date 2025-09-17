/*
 * System Recent API Endpoint Header
 * 
 * Declares the /api/system/recent endpoint that provides access
 * to the most recent log messages in reverse chronological order.
 */

#ifndef HYDROGEN_RECENT_H
#define HYDROGEN_RECENT_H

// Network headers
#include <microhttpd.h>

/**
 * Handles the /api/system/recent endpoint request.
 * Returns the most recent log messages in reverse chronological order.
 *
 * @param connection The MHD_Connection to send the response through
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/recent
//@ swagger:method GET
//@ swagger:operationId getSystemRecent
//@ swagger:tags "System Service"
//@ swagger:summary Recent log messages endpoint
//@ swagger:description Returns the most recent log messages from the system in reverse chronological order
//@ swagger:response 200 application/json {"type":"object","properties":{"messages":{"type":"array","items":{"type":"object","properties":{"timestamp":{"type":"string"},"level":{"type":"string"},"component":{"type":"string"},"message":{"type":"string"}}}}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string"}}}

// Function declarations
enum MHD_Result handle_system_recent_request(struct MHD_Connection *connection);

/**
 * Extract log lines from raw text into an array.
 * This function is made non-static to enable unit testing.
 *
 * @param raw_text The raw log text to process
 * @param line_count Pointer to store the number of lines extracted
 * @return Array of strings, each containing one log line, or NULL on error
 */
char** extract_log_lines(const char *raw_text, size_t *line_count);

/**
 * Build final text from log lines in reverse order (newest to oldest).
 * This function is made non-static to enable unit testing.
 *
 * @param lines Array of log line strings
 * @param line_count Number of lines in the array
 * @return Processed text with lines in reverse order, or NULL on error
 */
char* build_reverse_log_text(char **lines, size_t line_count);

#endif /* HYDROGEN_RECENT_H */
