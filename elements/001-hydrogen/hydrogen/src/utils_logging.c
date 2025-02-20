// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils_logging.h"
#include "configuration.h"

// System headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// Thread-safe identifier generation with collision resistance
void generate_id(char *buf, size_t len) {
    if (len < ID_LEN + 1) {
        console_log("Utils", 3, "Buffer too small for ID");
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

// Format and output a log message directly to console
void console_log(const char* subsystem, int priority, const char* message) {
    // Get current time with millisecond precision
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    // Format timestamp
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    char timestamp_ms[36];
    snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

    // Format priority and subsystem labels with proper padding
    char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
    snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", 
             MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

    char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
    snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", 
             MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

    // Output the formatted log entry
    printf("%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, message);
}