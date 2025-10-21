/*
 * Web Server File Upload Handling
 * 
 * Provides file upload functionality:
 * - Upload request processing
 * - File streaming
 * - Progress tracking
 * - UUID generation
 */

#ifndef WEB_SERVER_UPLOAD_H
#define WEB_SERVER_UPLOAD_H

#include <src/globals.h>
#include <microhttpd.h>
#include "web_server_core.h"

void generate_uuid(char *uuid_str);

// Upload data processing
enum MHD_Result handle_upload_data(void *coninfo_cls, enum MHD_ValueKind kind,
                                 const char *key, const char *filename,
                                 const char *content_type, const char *transfer_encoding,
                                 const char *data, uint64_t off, size_t size);

// Main upload request handler
enum MHD_Result handle_upload_request(struct MHD_Connection *connection,
                                    const char *upload_data,
                                    size_t *upload_data_size,
                                    void **con_cls);

// G-code file analysis
json_t* extract_gcode_info(const char* filename);
char* extract_preview_image(const char* filename);

#endif // WEB_SERVER_UPLOAD_H
