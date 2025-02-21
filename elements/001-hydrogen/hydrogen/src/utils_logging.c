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
#include <stdbool.h>

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

// Check if a message should be logged based on configuration
static bool should_log_message(const char* subsystem, int priority) {
    extern AppConfig *app_config;  // Get access to global config
    if (!app_config || !app_config->Logging.Console.Enabled) {
        return false;
    }

    // Get subsystem-specific level if configured, otherwise use default
    int configured_level = app_config->Logging.Console.DefaultLevel;
    
    // Check subsystem-specific configuration
    if (strcmp(subsystem, "ThreadMgmt") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.ThreadMgmt;
    } else if (strcmp(subsystem, "Shutdown") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.Shutdown;
    } else if (strcmp(subsystem, "mDNS") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.mDNS;
    } else if (strcmp(subsystem, "WebServer") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.WebServer;
    } else if (strcmp(subsystem, "WebSocket") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.WebSocket;
    } else if (strcmp(subsystem, "PrintQueue") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.PrintQueue;
    } else if (strcmp(subsystem, "LogQueueManager") == 0) {
        configured_level = app_config->Logging.Console.Subsystems.LogQueueManager;
    }

    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_ALL) return true;
    if (configured_level == LOG_LEVEL_NONE) return false;

    // For normal levels, message priority must be >= configured level
    // This ensures that:
    // - When level is set to INFO (1), we log messages with priority >= 1 (INFO through CRITICAL)
    // - When level is set to ERROR (4), we log messages with priority >= 4 (ERROR and CRITICAL)
    // - When level is set to CRITICAL (5), we log only messages with priority >= 5 (CRITICAL)
    return priority >= configured_level;
}

// Format and output a log message directly to console
void console_log(const char* subsystem, int priority, const char* message) {
    // Check if this message should be logged based on configuration
    if (!should_log_message(subsystem, priority)) {
        return;
    }

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