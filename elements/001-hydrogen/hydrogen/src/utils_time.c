// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils_time.h"
#include "logging.h"

// System headers
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

// Thread synchronization mutex
static pthread_mutex_t ready_time_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global state tracking
static time_t server_ready_time = 0;  // Protected by ready_time_mutex

// External state
extern volatile sig_atomic_t server_starting;

// Format duration in human-readable format (e.g. "4d 1h 22m 0s")
void format_duration(time_t seconds, char *buffer, size_t buflen) {
    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    snprintf(buffer, buflen, "%dd %dh %dm %ds", days, hours, minutes, secs);
}

// Track when server_starting becomes false
void update_server_ready_time(void) {
    pthread_mutex_lock(&ready_time_mutex);
    if (server_ready_time == 0) {  // Only set it once
        volatile sig_atomic_t starting = server_starting;  // Get current state
        if (!starting) {  // Check if we're no longer starting
            server_ready_time = time(NULL);
            log_this("Utils", "Server ready time recorded: %ld", 1, true, false, true, (long)server_ready_time);
        }
    }
    pthread_mutex_unlock(&ready_time_mutex);
}

// Check if server ready time has been set
int is_server_ready_time_set(void) {
    pthread_mutex_lock(&ready_time_mutex);
    int is_set = (server_ready_time > 0);
    pthread_mutex_unlock(&ready_time_mutex);
    return is_set;
}

// Get the server ready time
time_t get_server_ready_time(void) {
    pthread_mutex_lock(&ready_time_mutex);
    time_t ready_time = server_ready_time;
    pthread_mutex_unlock(&ready_time_mutex);
    return ready_time;
}