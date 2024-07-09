#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "configuration.h"
#include "queue.h"

void log_this(const char* subsystem, const char* details, int priority, bool LogConsole, bool LogDatabase, bool LogFile) {
    // Create a JSON string with the log message details
    char json_message[1024];
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

