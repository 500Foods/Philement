// System Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Third-Party Libraries
#include <jansson.h>

// Project Libraries
#include "log_queue_manager.h"
#include "logging.h"
#include "queue.h"
#include "configuration.h"

extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

static FILE* log_file = NULL;

static void cleanup_log_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    close_file_logging();
}

static void process_log_message(const char* message, int priority) {
    json_error_t error;
    json_t* json = json_loads(message, 0, &error);
    if (json) {
        const char* subsystem = json_string_value(json_object_get(json, "subsystem"));
        const char* details = json_string_value(json_object_get(json, "details"));
        bool logConsole = json_boolean_value(json_object_get(json, "LogConsole"));
        bool logDatabase = json_boolean_value(json_object_get(json, "LogDatabase"));
        bool logFile = json_boolean_value(json_object_get(json, "LogFile"));

        char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
        snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

        char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
        snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

        char timestamp[32];
        struct timeval tv;
        struct tm* tm_info;
        gettimeofday(&tv, NULL);
        tm_info = localtime(&tv.tv_sec);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        char timestamp_ms[36];
        snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

        char log_entry[1024];  // Increase buffer size
        snprintf(log_entry, sizeof(log_entry), "%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, details);

        if (logConsole) {
            printf("%s", log_entry);
        }

        if (logFile && log_file) {
            fputs(log_entry, log_file);
            fflush(log_file);  // Ensure the log is written immediately
        }

        if (logDatabase) {
            // TODO: Implement database logging
        }

        json_decref(json);
    } else {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
    }
}

void init_file_logging(const char* log_file_path) {
    if (log_file) {
        fclose(log_file);
    }
    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Error: Unable to open log file: %s\n", log_file_path);
    }
}

void close_file_logging() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void* log_queue_manager(void* arg) {
    Queue* log_queue = (Queue*)arg;

    pthread_cleanup_push(cleanup_log_queue_manager, NULL);

    log_this("LogQueueManager", "Log queue manager started", 0, true, true, true);

    while (!log_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(log_queue) == 0 && !log_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (log_queue_shutdown && queue_size(log_queue) == 0) {
            log_this("LogQueueManager", "Shutdown: Log Queue Manager processing final messages", 0, true, true, true);
        }

        while (queue_size(log_queue) > 0) {
            size_t message_size;
            int priority;
            char* message = queue_dequeue(log_queue, &message_size, &priority);
            if (message) {
                process_log_message(message, priority);
                free(message);
            }
        }
    }

    log_this("LogQueueManager", "Shutdown: Log Queue Manager exiting", 0, true, true, true);

    pthread_cleanup_pop(1);
    return NULL;
}
