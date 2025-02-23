/*
 * Web Server Print Queue Management
 * 
 * Provides print queue functionality:
 * - Print queue request handling
 * - Queue visualization
 * - Job status tracking
 * - Print job management
 */

#ifndef WEB_SERVER_PRINT_H
#define WEB_SERVER_PRINT_H

#include <microhttpd.h>
#include <jansson.h>

// Print queue request handling
enum MHD_Result handle_print_queue_request(struct MHD_Connection *connection);

// Print job status tracking
json_t* get_print_queue_status(void);

// Time formatting utilities
void format_time(double seconds, char *buffer);

#endif // WEB_SERVER_PRINT_H