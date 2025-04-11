// System headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

// Project headers
#include "utils_logging.h"
#include "../config/config.h"
#include "../config/config_priority.h"
#include "../config/config_logging.h"
#include "../logging/logging.h"

// Thread-safe identifier generation with collision resistance
void generate_id(char *buf, size_t len) {
    if (len < ID_LEN + 1) {
        log_this("Utils", "Buffer too small for ID", LOG_LEVEL_ERROR);
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    size_t id_chars_len = strlen(ID_CHARS);
    for (int i = 0; i < ID_LEN; i++) {
        buf[i] = ID_CHARS[rand() % id_chars_len];
    }
    buf[ID_LEN] = '\0';
}

// Get the string representation of a log priority level
const char* get_priority_label(int priority) {
    // Get the application configuration
    const AppConfig* config = get_app_config();
    
    // If we have a valid config, use the custom log level names if defined
    if (config) {
        const char* level_name = config_logging_get_level_name(&config->logging, priority);
        if (level_name) {
            return level_name;
        }
    }
    
    // Fallback to default priority levels if config is not available
    if (priority >= 0 && priority < NUM_PRIORITY_LEVELS) {
        return DEFAULT_PRIORITY_LEVELS[priority].label;
    }
    
    return DEFAULT_PRIORITY_LEVELS[LOG_LEVEL_TRACE].label; // Default to TRACE for invalid values
}

