#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Standard Libraries
#include <stdbool.h>
#include <stdlib.h>

// Project Libraries
#include "configuration.h"

// Initialize the web server
bool init_web_server(WebConfig *web_config);

// Run the web server (this will be called in a separate thread)
void* run_web_server(void* arg);

// Shutdown the web server
void shutdown_web_server(void);

// Get the configured upload path
const char* get_upload_path(void);

#endif // WEB_SERVER_H
