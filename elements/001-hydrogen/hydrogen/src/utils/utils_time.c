// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System headers
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

// Project headers
#include "utils_time.h"
#include "../logging/logging.h"

// Public interface declarations
void set_server_start_time(void);
time_t get_server_start_time(void);
void update_server_ready_time(void);
int is_server_ready_time_set(void);
time_t get_server_ready_time(void);
void record_shutdown_start_time(void);
void record_shutdown_end_time(void);
void format_duration(time_t seconds, char *buffer, size_t buflen);

// External declarations
extern volatile sig_atomic_t server_starting;

// Internal state
static pthread_mutex_t ready_time_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec server_ready_time = {0, 0};  // Protected by ready_time_mutex
static struct timespec server_start_time = {0, 0};  // Set once at startup
static struct timespec shutdown_start_time = {0, 0}; // When shutdown begins
static struct timespec shutdown_end_time = {0, 0};   // When shutdown completes

// Private function declarations
static void format_iso_time(time_t t, char *buffer, size_t buflen);
static double calc_elapsed_time(const struct timespec *end, const struct timespec *start);

// Format time as ISO 8601 UTC timestamp
static void format_iso_time(time_t t, char *buffer, size_t buflen) {
    struct tm *tm = gmtime(&t);
    strftime(buffer, buflen, "%Y-%m-%dT%H:%M:%SZ", tm);
}

// Calculate elapsed time in seconds with nanosecond precision
static double calc_elapsed_time(const struct timespec *end, const struct timespec *start) {
    double seconds = end->tv_sec - start->tv_sec;
    double nanoseconds = end->tv_nsec - start->tv_nsec;
    return seconds + (nanoseconds / 1000000000.0);
}

// Record shutdown start time (called from signal handler)
void record_shutdown_start_time(void) {
    clock_gettime(CLOCK_MONOTONIC, &shutdown_start_time);
}

// Record shutdown end time and log duration
void record_shutdown_end_time(void) {
    clock_gettime(CLOCK_MONOTONIC, &shutdown_end_time);
    
    // Get wall clock time for ISO format
    time_t now = time(NULL);
    char iso_time[32];
    format_iso_time(now, iso_time, sizeof(iso_time));
    
    // Calculate elapsed time
    double elapsed = calc_elapsed_time(&shutdown_end_time, &shutdown_start_time);
    
    // Log timestamp and duration
    log_this("Utils", "System shutdown initiated at %s", LOG_LEVEL_STATE, iso_time);
    log_this("Utils", "System shutdown took %.3fs", LOG_LEVEL_STATE, elapsed);
}

// Get the server start time
time_t get_server_start_time(void) {
    return server_start_time.tv_sec;
}

// Set the server start time (called once during startup)
void set_server_start_time(void) {
    if (server_start_time.tv_sec == 0) {  // Only set it once
        clock_gettime(CLOCK_MONOTONIC, &server_start_time);
    }
}


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
    if (server_ready_time.tv_sec == 0) {  // Only set it once
        volatile sig_atomic_t starting = server_starting;  // Get current state
        if (!starting) {  // Check if we're no longer starting
            // Get high precision ready time
            clock_gettime(CLOCK_MONOTONIC, &server_ready_time);
            
            // Get wall clock time for ISO format
            time_t now = time(NULL);
            char iso_time[32];
            format_iso_time(now, iso_time, sizeof(iso_time));
            
            // Calculate and format startup duration using high precision time
            double elapsed = calc_elapsed_time(&server_ready_time, &server_start_time);

            log_group_begin();
            log_this("Startup", "System started at %s", LOG_LEVEL_STATE, iso_time);
            log_this("Startup", "System startup took %.3fs", LOG_LEVEL_STATE, elapsed);
            log_group_end();
        }
    }
    pthread_mutex_unlock(&ready_time_mutex);
}

// Check if server ready time has been set
int is_server_ready_time_set(void) {
    pthread_mutex_lock(&ready_time_mutex);
    int is_set = (server_ready_time.tv_sec > 0);
    pthread_mutex_unlock(&ready_time_mutex);
    return is_set;
}

// Get the server ready time
time_t get_server_ready_time(void) {
    pthread_mutex_lock(&ready_time_mutex);
    time_t ready_time = server_ready_time.tv_sec;
    pthread_mutex_unlock(&ready_time_mutex);
    return ready_time;
}