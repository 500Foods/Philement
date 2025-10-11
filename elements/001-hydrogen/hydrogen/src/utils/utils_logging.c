// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "utils_logging.h"

// Thread-safe identifier generation with collision resistance
void generate_id(char *buf, size_t len) {
    if (len < ID_LEN + 1) {
        log_this(SR_LOGGING, "Buffer too small for ID", LOG_LEVEL_ERROR, 0);
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    size_t id_chars_len = strlen(ID_CHARS);
    for (int i = 0; i < ID_LEN; i++) {
        buf[i] = ID_CHARS[(size_t)(rand() % (int)id_chars_len)];
    }
    buf[ID_LEN] = '\0';
}

// Get the string representation of a log priority level
const char* get_priority_label(int priority) {
   
    // If we have a valid config, use the custom log level names if defined
    if (app_config) {
        const char* level_name = config_logging_get_level_name(&app_config->logging, priority);
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
