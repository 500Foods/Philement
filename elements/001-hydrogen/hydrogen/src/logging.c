#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "configuration.h"
#include "queue.h"

void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...) {
    char details[1024];  // Increased buffer size to accommodate formatted string
    va_list args;
    va_start(args, LogFile);
    vsnprintf(details, sizeof(details), format, args);
    va_end(args);

    // Create a JSON string with the log message details
    char json_message[2048];  // Increased buffer size
    snprintf(json_message, sizeof(json_message),
             "{\"subsystem\":\"%s\",\"details\":\"%s\",\"priority\":%d,\"LogConsole\":%s,\"LogDatabase\":%s,\"LogFile\":%s}",
             subsystem, details, priority, LogConsole ? "true" : "false", LogDatabase ? "true" : "false", LogFile ? "true" : "false");

    // Enqueue the log message
    Queue* log_queue = queue_find("SystemLog");
    if (log_queue) {
        queue_enqueue(log_queue, json_message, strlen(json_message), priority);
    } else {
        fprintf(stderr, "Error: SystemLog queue not found\n");
    }
}
