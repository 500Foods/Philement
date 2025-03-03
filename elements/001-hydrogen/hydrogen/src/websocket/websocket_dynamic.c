/*
 * Dynamic WebSocket Server Adapter
 *
 * This file demonstrates how to dynamically load libwebsockets only when needed,
 * allowing the application to start successfully even if libwebsockets is not
 * installed on the system. It provides graceful fallbacks and clear error messaging
 * when the library is missing.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "../utils/utils_dependency.h" // For dynamic library loading
#include "../logging/logging.h"
#include "websocket_server.h"          // Public interface

// Define the real WebSocket library dependency
#define WEBSOCKET_LIB "libwebsockets.so"
#define WEBSOCKET_VERSION "4.3.0"

// Define fallback status values (only if not already defined)
#ifndef LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT
#define LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT 1
#endif

#ifndef LWS_SERVER_OPTION_EXPLICIT_VHOSTS
#define LWS_SERVER_OPTION_EXPLICIT_VHOSTS 2
#endif

// Global library handle
static LibraryHandle *websocket_lib = NULL;

// Forward declarations of wrapper functions
static bool initialize_dynamic_websocket_library(void);
static void cleanup_dynamic_websocket_library(void);

// Forward declaration of libwebsockets structures
struct lws_context_creation_info;
struct lws_context;

// Function pointer types for ISO C compliance
typedef struct lws_context_creation_info* (*CreateInfoFunc)(void);
typedef struct lws_context* (*CreateContextFunc)(struct lws_context_creation_info*);
typedef int (*StartServerFunc)(struct lws_context*);
typedef void (*DestroyContextFunc)(struct lws_context*);

/*
 * Initialize the WebSocket server using dynamic loading
 *
 * This function:
 * 1. Attempts to dynamically load libwebsockets
 * 2. Retrieves function pointers for required functions
 * 3. Initializes the WebSocket server if library is available
 * 4. Returns descriptive error if library is missing
 *
 * Returns: 0 on success, error code on failure
 */
int init_websocket_server_dynamic(int port, const char *protocol, const char *key) {
    // Unused parameters - mark them to avoid compiler warnings
    (void)protocol;
    (void)key;
    // First, try to load the libwebsockets library
    if (!initialize_dynamic_websocket_library()) {
        log_this("WebSocket", "Cannot initialize WebSocket server - libwebsockets not available", 
                LOG_LEVEL_WARN);
        // This is not a critical error - the application can still run without WebSockets
        return -1;
    }
    
    // Function pointers for required libwebsockets functions - with proper typing
    void *info_func_ptr = get_library_function(websocket_lib, "lws_create_context_info");
    void *ctx_func_ptr = get_library_function(websocket_lib, "lws_create_context");
    
    // Function pointer casting
    // These casts are necessary for any dynamic loading system
    // Suppress ISO C pedantic warnings for these casts only
    CreateInfoFunc create_info = NULL;
    CreateContextFunc create_context = NULL;
    
    if (info_func_ptr) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        create_info = (CreateInfoFunc)info_func_ptr;
#pragma GCC diagnostic pop
    }
    
    if (ctx_func_ptr) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        create_context = (CreateContextFunc)ctx_func_ptr;
#pragma GCC diagnostic pop
    }
    
    // Check if we have the required function pointers
    if (!create_info || !create_context) {
        log_this("WebSocket", "Required functions not found in libwebsockets", LOG_LEVEL_WARN);
        cleanup_dynamic_websocket_library();
        return -1;
    }
    
    // Now use the properly cast function pointers to initialize the WebSocket server
    // This is a simplified example - in a real implementation, there would be
    // many more function calls and proper error handling
    
    // Create context info
    struct lws_context_creation_info *info = NULL;
    if (create_info) {
        info = create_info();
    }
    if (!info) {
        log_this("WebSocket", "Failed to create WebSocket context info", LOG_LEVEL_ERROR);
        cleanup_dynamic_websocket_library();
        return -1;
    }
    
    // Set required options
    info->port = port;
    info->options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT | 
                    LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
    
    // Create context (with proper typing)
    struct lws_context *context = NULL;
    if (create_context && info) {
        context = create_context(info);
    }
    if (!context) {
        log_this("WebSocket", "Failed to create WebSocket context", LOG_LEVEL_ERROR);
        // Assume the info structure needs to be freed - in a real implementation
        // this would depend on the specific library's memory management
        free(info);
        cleanup_dynamic_websocket_library();
        return -1;
    }
    
    // Store the context and other necessary information for later use
    // This would be done through your existing WebSocket server implementation
    
    log_this("WebSocket", "WebSocket server initialized successfully with dynamic loading", 
            LOG_LEVEL_INFO);
    
    return 0;
}

/*
 * Start the dynamically loaded WebSocket server
 * 
 * This function:
 * 1. Checks if the library is loaded
 * 2. Uses function pointers to start the server
 * 3. Provides appropriate error messages if library is missing
 *
 * Returns: 0 on success, error code on failure
 */
int start_websocket_server_dynamic(void) {
    // Check if library is loaded
    if (!websocket_lib || !websocket_lib->is_loaded) {
        log_this("WebSocket", "Cannot start WebSocket server - library not loaded", 
                LOG_LEVEL_WARN);
        return -1;
    }
    
    // Function pointer for starting the server
    void *start_func_ptr = get_library_function(websocket_lib, "lws_start_server");
    StartServerFunc start_server = NULL;
    
    if (start_func_ptr) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        start_server = (StartServerFunc)start_func_ptr;
#pragma GCC diagnostic pop
    }
    
    if (!start_server) {
        log_this("WebSocket", "Required function 'lws_start_server' not found", 
                LOG_LEVEL_WARN);
        return -1;
    }
    
    // Call the properly cast start function (placeholder for actual implementation)
    // In a real implementation, you would pass the stored context
    int result = -1;
    if (start_server) {
        result = start_server(NULL);
    }
    
    if (result != 0) {
        log_this("WebSocket", "Failed to start WebSocket server", LOG_LEVEL_ERROR);
        return -1;
    }
    
    log_this("WebSocket", "WebSocket server started successfully", LOG_LEVEL_INFO);
    return 0;
}

/*
 * Shutdown the dynamically loaded WebSocket server
 *
 * This function:
 * 1. Checks if the library is loaded
 * 2. Uses function pointers to shutdown the server
 * 3. Unloads the library when done
 *
 * Returns: none
 */
void shutdown_websocket_server_dynamic(void) {
    // Check if library is loaded
    if (!websocket_lib || !websocket_lib->is_loaded) {
        // Nothing to do
        return;
    }
    
    // Function pointer for shutting down the server
    void *destroy_func_ptr = get_library_function(websocket_lib, "lws_context_destroy");
    DestroyContextFunc destroy_context = NULL;
    
    if (destroy_func_ptr) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        destroy_context = (DestroyContextFunc)destroy_func_ptr;
#pragma GCC diagnostic pop
    }
    
    if (destroy_context) {
        // Call the properly cast destroy function (placeholder for actual implementation)
        // In a real implementation, you would pass the stored context
        destroy_context(NULL);
        log_this("WebSocket", "WebSocket server shutdown successfully", LOG_LEVEL_INFO);
    } else {
        log_this("WebSocket", "Required function 'lws_context_destroy' not found", 
                LOG_LEVEL_WARN);
    }
    
    // Cleanup and unload the library
    cleanup_dynamic_websocket_library();
}

/*
 * Helper function to initialize the WebSocket library
 */
static bool initialize_dynamic_websocket_library(void) {
    // Don't reload if already loaded
    if (websocket_lib && websocket_lib->is_loaded) {
        return true;
    }
    
    // Free previous handle if it exists
    if (websocket_lib) {
        unload_library(websocket_lib);
        websocket_lib = NULL;
    }
    
    // Load the library with RTLD_LAZY | RTLD_GLOBAL
    // RTLD_GLOBAL is often required for libraries that use plugins
    websocket_lib = load_library(WEBSOCKET_LIB, RTLD_LAZY | RTLD_GLOBAL);
    
    // Check if loading was successful
    if (websocket_lib && websocket_lib->is_loaded) {
        log_this("WebSocket", "Successfully loaded %s (Version: %s)", 
                LOG_LEVEL_INFO, WEBSOCKET_LIB, websocket_lib->version);
        return true;
    } else {
        log_this("WebSocket", "Failed to load %s - WebSocket functionality will be disabled", 
                LOG_LEVEL_WARN, WEBSOCKET_LIB);
        return false;
    }
}

/*
 * Helper function to cleanup the WebSocket library
 */
static void cleanup_dynamic_websocket_library(void) {
    if (websocket_lib) {
        unload_library(websocket_lib);
        websocket_lib = NULL;
    }
}

/* 
 * Example of how to safely call a dynamically loaded function with proper
 * ISO C compliance and fallback handling
 */
int example_get_websocket_connection_count(void) {
    // Example of safely calling a function
    int result = 0;
    void *context = NULL;  // Would be stored from initialization in real code
    
    // Basic check for library availability
    if (!websocket_lib || !websocket_lib->is_loaded) {
        return 0;  // Default fallback
    }
    
    // Get function pointer
    void *func_ptr = get_library_function(websocket_lib, "lws_get_connection_count");
    if (func_ptr) {
        // Cast to appropriate function type and call (with warning suppressed)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        int (*get_count_func)(void*) = (int (*)(void*))func_ptr;
#pragma GCC diagnostic pop
        result = get_count_func(context);
    } else {
        // Fallback behavior
        log_this("WebSocket", "Function lws_get_connection_count not available - using fallback", 
                LOG_LEVEL_WARN);
    }
    
    return result;
}

/*
 * Example of safely calling a void-returning dynamically loaded function with
 * ISO C compliance and fallback handling
 */
void example_websocket_log_connections(void) {
    void *context = NULL;  // Would be stored from initialization in real code
    
    // Basic check for library availability
    if (!websocket_lib || !websocket_lib->is_loaded) {
        // Fallback behavior
        log_this("WebSocket", "WebSocket library not loaded - using fallback", 
                LOG_LEVEL_WARN);
        return;
    }
    
    // Get function pointer
    void *func_ptr = get_library_function(websocket_lib, "lws_log_connections");
    if (func_ptr) {
        // Cast to appropriate function type and call (with warning suppressed)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        void (*log_func)(void*) = (void (*)(void*))func_ptr;
#pragma GCC diagnostic pop
        log_func(context);
    } else {
        // Fallback behavior
        log_this("WebSocket", "Function lws_log_connections not available - using fallback", 
                LOG_LEVEL_WARN);
    }
}