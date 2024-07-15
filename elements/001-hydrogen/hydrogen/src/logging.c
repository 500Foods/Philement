// System Libraries
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

// Project Libraries
#include "logging.h"
#include "configuration.h"
#include "queue.h"
#include "utils.h"

extern volatile sig_atomic_t log_queue_shutdown;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...) {
    pthread_mutex_lock(&log_mutex);

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
    if (log_queue && !log_queue_shutdown) {
        queue_enqueue(log_queue, json_message, strlen(json_message), priority);

        // Signal the log queue manager
        pthread_cond_signal(&terminate_cond);

        // If this is a shutdown-related log, sleep to ensure logging completes
        //if ((strcmp(subsystem, "Shutdown") == 0) || (strcmp(subsystem,"Initialization") == 0) || (strcasestr(format, "shutdown") != NULL)) {
        //    usleep(100000);  // Sleep for 100ms
       // }
    } else if (LogConsole) {
        // If the log queue is shutting down or not available, print to console as a fallback
        printf("%s: %s\n", subsystem, details);
    }

    pthread_mutex_unlock(&log_mutex);
}
