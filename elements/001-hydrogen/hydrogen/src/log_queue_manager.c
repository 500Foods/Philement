#include "log_queue_manager.h"
#include "queue.h"
#include "configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <jansson.h>
#include <signal.h>
#include <unistd.h>

extern volatile sig_atomic_t keep_running;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

void* log_queue_manager(void* arg) {
    Queue* log_queue = (Queue*)arg;

    while (keep_running) {
        size_t message_size;
        int priority;
        char* message = queue_dequeue(log_queue, &message_size, &priority);
        if (message) {
            // Parse the JSON string to extract the log message details
            json_error_t error;
            json_t* json = json_loads(message, 0, &error);
            if (json) {
                const char* subsystem = json_string_value(json_object_get(json, "subsystem"));
                const char* details = json_string_value(json_object_get(json, "details"));
                bool logConsole = json_boolean_value(json_object_get(json, "LogConsole"));
                bool logDatabase = json_boolean_value(json_object_get(json, "LogDatabase"));
                bool logFile = json_boolean_value(json_object_get(json, "LogFile"));

                // Format the log message
                char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 4];
                snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

                char formatted_subsystem[25];
                snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-20.20s ]", subsystem);

                char timestamp[24];
                time_t now;
                struct tm* tm_info;
                time(&now);
                tm_info = localtime(&now);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

                // Output the log message to console and/or file based on flags
                if (logConsole) {
                    printf("%s  %s  %s  %s\n", timestamp, formatted_priority, formatted_subsystem, details);
                }

                if (logDatabase) {
                    // Implement file logging here
                    // For now, we'll just print a message
                    printf("(Database logging not implemented yet)\n");
                }

                if (logFile) {
                    // Implement file logging here
                    // For now, we'll just print a message
                    printf("(File logging not implemented yet)\n");
                }

                json_decref(json);
            } else {
                fprintf(stderr, "Error parsing JSON: %s\n", error.text);
            }

            free(message);
        } else {
            pthread_mutex_lock(&terminate_mutex);
            if (keep_running) {
                pthread_cond_wait(&terminate_cond, &terminate_mutex);
            }
            pthread_mutex_unlock(&terminate_mutex);
        }
    }

    return NULL;
}
