/*
 * Log Queue Manager
 * 
 */
// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "log_queue_manager.h"
#include <src/config/config_logging.h>

// Public interface declarations
void init_file_logging(const char* log_file_path);
void close_file_logging(void);
void* log_queue_manager(void* arg);

// External declarations
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Internal state
static FILE* log_file = NULL;

// Private function declarations
void cleanup_log_queue_manager(void* arg);
bool should_log_to_console(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_file(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_database(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_notify(const char* subsystem, int priority, const LoggingConfig* config);
void process_log_message(const char* message, int priority);

// Thread cleanup handler with guaranteed file closure
void cleanup_log_queue_manager(void* arg) {
    (void)arg;  // Unused parameter
    
    // Remove this thread from tracking before cleanup
    remove_service_thread(&logging_threads, pthread_self());
    
    close_file_logging();
}

// Process log messages with structured formatting and routing
bool should_log_to_console(const char* subsystem, int priority, const LoggingConfig* config) {
    if (!config->console.enabled) {
        return false;
    }

    int configured_level = get_subsystem_level_console(config, subsystem);
    
    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_TRACE) return true;
    if (configured_level == LOG_LEVEL_QUIET) return false;

    // For normal levels, message priority must be >= configured level
    return priority >= configured_level;
}

bool should_log_to_file(const char* subsystem, int priority, const LoggingConfig* config) {
    if (!config->file.enabled) {
        return false;
    }

    int configured_level = get_subsystem_level_file(config, subsystem);
    
    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_TRACE) return true;
    if (configured_level == LOG_LEVEL_QUIET) return false;

    // For normal levels, message priority must be >= configured level
    return priority >= configured_level;
}

bool should_log_to_notify(const char* subsystem, int priority, const LoggingConfig* config) {
    if (!config->notify.enabled) {
        return false;
    }

    int configured_level = get_subsystem_level_notify(config, subsystem);
    
    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_TRACE) return true;
    if (configured_level == LOG_LEVEL_QUIET) return false;

    // For normal levels, message priority must be >= configured level
    return priority >= configured_level;
}

bool should_log_to_database(const char* subsystem, int priority, const LoggingConfig* config) {
    if (!config->database.enabled) {
        return false;
    }

    int configured_level = get_subsystem_level_database(config, subsystem);
    
    // Special handling for ALL and NONE
    if (configured_level == LOG_LEVEL_TRACE) return true;
    if (configured_level == LOG_LEVEL_QUIET) return false;

    // For normal levels, message priority must be >= configured level
    return priority >= configured_level;
}

void process_log_message(const char* message, int priority) {
    if (!app_config) return;

    json_error_t error;
    json_t* json = json_loads(message, 0, &error);
    if (json) {
        const char* subsystem = json_string_value(json_object_get(json, "subsystem"));
        const char* details = json_string_value(json_object_get(json, "details"));
        bool logConsole = json_boolean_value(json_object_get(json, "LogConsole"));
        bool logDatabase = json_boolean_value(json_object_get(json, "LogDatabase"));
        bool logFile = json_boolean_value(json_object_get(json, "LogFile"));
        bool logNotify = json_boolean_value(json_object_get(json, "LogNotify"));

        char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
        snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

        char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
        snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

        char timestamp[32];
        struct timeval tv;
        const struct tm* tm_info;
        gettimeofday(&tv, NULL);
        tm_info = localtime(&tv.tv_sec);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        char timestamp_ms[36];
        snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

        char log_entry[1024];  // Increase buffer size
        snprintf(log_entry, sizeof(log_entry), "%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, details);

        // Apply filtering for each destination
        // Prevent duplicate console output for thread management messages (which are already output directly)
        if (logConsole && should_log_to_console(subsystem, priority, &app_config->logging)) {
            // Skip console output for ThreadMgmt and LogQueueManager messages during startup 
            // to avoid duplicate output with different timestamp formats
            if (!(strcmp(subsystem, "ThreadMgmt") == 0 || 
                 (strcmp(subsystem, "LogQueueManager") == 0 && 
                  strstr(details, "Log queue manager started") != NULL))) {
                printf("%s", log_entry);
            }
        }

        if (logFile && log_file && should_log_to_file(subsystem, priority, &app_config->logging)) {
            fputs(log_entry, log_file);
          // fflush(log_file);  // Ensure the log is written immediately
        }

        if (logDatabase && should_log_to_database(subsystem, priority, &app_config->logging)) {
            // TODO: Implement database logging when needed
        }

        if (logNotify && should_log_to_notify(subsystem, priority, &app_config->logging)) {
            // Get the configured notifier type
            const char* notifier = app_config->notify.notifier;
            if (notifier && strcmp(notifier, "SMTP") == 0 && app_config->notify.smtp.host) {
                // Format message for email
                char subject[256];
                snprintf(subject, sizeof(subject), "[%s] %s Alert", 
                        app_config->server.server_name ? app_config->server.server_name : "Hydrogen",
                        priority >= LOG_LEVEL_ERROR ? "Error" : "Warning");

                // TODO: Call SMTP send function (to be implemented in notify subsystem)
                // For now, we'll log that we would send a notification
                fprintf(stderr, "Would send SMTP notification to %s: Subject: %s, Message: %s", 
                        app_config->notify.smtp.host, subject, log_entry);
            }
        }

        json_decref(json);
    } else {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
    }
}

// Initialize file-based logging
// Opens the specified log file in append mode
// Ensures previous file handle is closed
// Handles file access errors
void init_file_logging(const char* log_file_path) {
    if (log_file) {
        fclose(log_file);
    }
    log_file = fopen(log_file_path, "a");
    if (!log_file) {
        fprintf(stderr, "Error: Unable to open log file: %s\n", log_file_path);
    }
}

void close_file_logging(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// Log queue manager implementing producer-consumer pattern
void* log_queue_manager(void* arg) {
    Queue* log_queue = (Queue*)arg;

    // Register this thread with the logging service
    add_service_thread(&logging_threads, pthread_self());

    pthread_cleanup_push(cleanup_log_queue_manager, NULL);

    log_this(SR_QUEUES, "Log queue manager started", LOG_LEVEL_STATE, 0);

    while (!log_queue_shutdown) {
        pthread_mutex_lock(&terminate_mutex);
        while (queue_size(log_queue) == 0 && !log_queue_shutdown) {
            pthread_cond_wait(&terminate_cond, &terminate_mutex);
        }
        pthread_mutex_unlock(&terminate_mutex);

        if (log_queue_shutdown && queue_size(log_queue) == 0) {
            log_this(SR_QUEUES, "Shutdown: Log Queue Manager processing final messages", LOG_LEVEL_STATE, 0);
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

    log_this(SR_QUEUES, "Shutdown: Log Queue Manager exiting", LOG_LEVEL_STATE, 0);

    pthread_cleanup_pop(1);
    return NULL;
}
