// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils.h"
#include "logging.h"

// Forward declarations of static functions
static void init_all_service_threads(void);

// Initialize service threads when module loads
static void __attribute__((constructor)) init_utils(void) {
    init_all_service_threads();
    init_queue_memory(&log_queue_memory, NULL);
    init_queue_memory(&print_queue_memory, NULL);
}

// Update queue limits after configuration is loaded
void update_queue_limits_from_config(const AppConfig *config) {
    if (!config) return;
    
    update_queue_limits(&log_queue_memory, config);
    update_queue_limits(&print_queue_memory, config);
}

// Initialize all service thread tracking
static void init_all_service_threads(void) {
    init_service_threads(&logging_threads);
    init_service_threads(&web_threads);
    init_service_threads(&websocket_threads);
    init_service_threads(&mdns_threads);
    init_service_threads(&print_threads);
}