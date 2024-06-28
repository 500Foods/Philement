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

//void log_this(const char* subsystem, const char* details, bool LogConsole, bool LogDatabase) {
    //(void)LogDatabase;  // Silence unused parameter warning
//
    //if (!LogConsole) {
        //return;  // For now, we only handle console logging
    //}
//
    //struct timeval tv;
    //gettimeofday(&tv, NULL);
//
    //struct tm* tm_info = localtime(&tv.tv_sec);
//
    //char timestamp[24];  // YYYY-MM-DD HH:MM:SS.mmm
    //strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
//
    //char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 4];  // "[LABEL ]"
    //snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));
//
    //char formatted_subsystem[25];  // [ subsystem           ]
    //snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-20.20s ]", subsystem);
//
    //printf("%s.%03ld  %s  %s\n", 
           //timestamp, 
           //tv.tv_usec / 1000,  // Convert microseconds to milliseconds
	   //formatted_priority,
           //formatted_subsystem, 
           //details);
//}
