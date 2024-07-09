#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <stdbool.h>
#include <stdlib.h>

// Initialize the web server
bool init_web_server(unsigned int port, const char* upload_path, const char* upload_dir, size_t config_max_upload_size);

// Run the web server (this will be called in a separate thread)
void* run_web_server(void* arg);

// Shutdown the web server
void shutdown_web_server(void);

// Get the configured upload path
const char* get_upload_path(void);

#endif // WEB_INTERFACE_H
