/*
 * System Upload API endpoint for the Hydrogen Project.
 * Provides file upload functionality via REST API.
 */

#ifndef HYDROGEN_SYSTEM_UPLOAD_H
#define HYDROGEN_SYSTEM_UPLOAD_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// Third-party includes
#include <microhttpd.h>
#include <jansson.h>

// Project includes
#include <src/config/config.h>
#include <src/state/state.h>

/**
 * Handles the /api/system/upload endpoint request.
 * Provides REST API access to file upload functionality with JSON responses.
 *
 * This endpoint supports:
 * - File upload with multipart form data
 * - Optional print queue integration
 * - G-code analysis and metadata extraction
 * - Preview image generation
 * - Structured JSON responses for API integration
 *
 * @param connection The MHD_Connection to send the response through
 * @param method The HTTP method (should be POST)
 * @param upload_data The upload data from the request
 * @param upload_data_size The size of upload data
 * @param con_cls Connection context pointer
 * @return MHD_Result indicating success or failure
 */
//@ swagger:path /api/system/upload
//@ swagger:method POST
//@ swagger:operationId uploadFile
//@ swagger:tags "System Service"
//@ swagger:summary File upload endpoint (API method)
//@ swagger:description Uploads files via REST API with structured JSON responses. For web-based uploads, use the alternative method described below.
//@ swagger:parameter file formData file true "File to upload"
//@ swagger:parameter print formData string false "Set to true to queue file for printing after upload"
//@ swagger:response 200 application/json {"type":"object","properties":{"files":{"type":"object"},"done":{"type":"boolean"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"error":{"type":"string"}}}
//@ swagger:response 413 application/json {"type":"object","properties":{"error":{"type":"string"}}}
//@ swagger:response 500 application/json {"type":"object","properties":{"error":{"type":"string"}}}
//@ swagger:notes
//@ swagger:note **Alternative Web Upload Method:**
//@ swagger:note For web-based file uploads (HTML forms), use the web server upload endpoint at the root level.
//@ swagger:note This method provides HTML form responses and direct browser integration:
//@ swagger:note - **URL**: `POST /upload` (relative to web server root)
//@ swagger:note - **Content-Type**: `multipart/form-data`
//@ swagger:note - **Form Fields**:
//@ swagger:note   - `file`: The file to upload (required)
//@ swagger:note   - `print`: Set to 'true' to queue for printing (optional)
//@ swagger:note - **Response**: HTML response with upload status
//@ swagger:note - **Features**: Progress tracking, G-code analysis, preview image extraction
//@ swagger:note Both methods share the same upload logic and configuration for consistency.
enum MHD_Result validate_upload_method(const char *method);

enum MHD_Result handle_system_upload_request(struct MHD_Connection *connection,
                                            const char *method,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls);

enum MHD_Result handle_system_upload_info_request(struct MHD_Connection *connection);

#endif /* HYDROGEN_SYSTEM_UPLOAD_H */
