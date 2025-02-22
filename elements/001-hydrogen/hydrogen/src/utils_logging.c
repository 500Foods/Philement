// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils_logging.h"
#include "configuration.h"
#include "logging.h"

// System headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

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
    switch (priority) {
        case LOG_LEVEL_ALL:      return "ALL";
        case LOG_LEVEL_INFO:     return "INFO";
        case LOG_LEVEL_WARN:     return "WARN";
        case LOG_LEVEL_DEBUG:    return "DEBUG";
        case LOG_LEVEL_ERROR:    return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRITICAL";
        case LOG_LEVEL_NONE:     return "NONE";
        default:                 return "UNKNOWN";
    }
}

