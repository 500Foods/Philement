// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "utils_time.h"

// Public interface declarations
void set_server_start_time(void);
time_t get_server_start_time(void);
void update_server_ready_time(void);
int is_server_ready_time_set(void);
time_t get_server_ready_time(void);
void record_shutdown_start_time(void);
void record_shutdown_end_time(void);
double calculate_shutdown_time(void);
void format_duration(time_t seconds, char *buffer, size_t buflen);
const char* get_system_start_time_string(void);
double calculate_startup_time(void);
void record_startup_complete_time(void);
void record_shutdown_initiate_time(void);
double calculate_total_running_time(void);
double calculate_total_elapsed_time(void);

// Internal state
static pthread_mutex_t ready_time_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec server_ready_time = {0, 0};  // Protected by ready_time_mutex
static struct timespec server_start_time = {0, 0};  // Updated on each startup/restart
static struct timespec original_start_time = {0, 0}; // Set only on first startup
static struct timespec shutdown_start_time = {0, 0}; // When shutdown begins
static struct timespec shutdown_end_time = {0, 0};   // When shutdown completes
static struct timespec startup_complete_time = {0, 0}; // When startup is fully complete
static struct timespec shutdown_initiate_time = {0, 0}; // When shutdown is initiated

// Private function declarations
static void format_iso_time(time_t t, char *buffer, size_t buflen);
static double calc_elapsed_time(const struct timespec *end, const struct timespec *start);

// Format time as ISO 8601 UTC timestamp
static void format_iso_time(time_t t, char *buffer, size_t buflen) {
    const struct tm *tm = gmtime(&t);
    strftime(buffer, buflen, "%Y-%m-%dT%H:%M:%SZ", tm);
}

// Calculate elapsed time in seconds with nanosecond precision
static double calc_elapsed_time(const struct timespec *end, const struct timespec *start) {
    double seconds = (double)(end->tv_sec - start->tv_sec);
    double nanoseconds = (double)(end->tv_nsec - start->tv_nsec);
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
    // Suppress unused variable warning
    (void)elapsed;
    
    // Comment out log messages for cleaner shutdown sequence
    // log_this(SR_LOGGING, "System shutdown initiated at %s", LOG_LEVEL_STATE, 1, iso_time);
    // log_this(SR_LOGGING, "System shutdown took %.3fs", LOG_LEVEL_STATE, 1, elapsed);
}

// Get the server start time
time_t get_server_start_time(void) {
    return server_start_time.tv_sec;
}

// Set the server start time (handles both initial startup and restarts)
void set_server_start_time(void) {
    // Get current time
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    // If this is first startup, set original start time
    if (original_start_time.tv_sec == 0) {
        original_start_time = current_time;
    }
    
    // Always update current startup time
    server_start_time = current_time;
    
    // Reset ready time for new startup sequence
    pthread_mutex_lock(&ready_time_mutex);
    server_ready_time.tv_sec = 0;
    server_ready_time.tv_nsec = 0;
    pthread_mutex_unlock(&ready_time_mutex);
}


// Format duration in human-readable format (e.g. "4d 1h 22m 0s")
void format_duration(time_t seconds, char *buffer, size_t buflen) {
    int days = (int)(seconds / 86400);
    int hours = (int)((seconds % 86400) / 3600);
    int minutes = (int)((seconds % 3600) / 60);
    int secs = (int)(seconds % 60);
    snprintf(buffer, buflen, "%dd %dh %dm %ds", days, hours, minutes, secs);
}

// Track when server_starting becomes false
void update_server_ready_time(void) {
    pthread_mutex_lock(&ready_time_mutex);
    if (server_ready_time.tv_sec == 0) {  // Only set it once
        if (!server_starting) {  // Check if we're no longer starting
            // Get high precision ready time
            clock_gettime(CLOCK_MONOTONIC, &server_ready_time);
            
            // Get wall clock time for ISO format
            time_t now = time(NULL);
            char iso_time[32];
            format_iso_time(now, iso_time, sizeof(iso_time));
            
            // Calculate and format startup duration using high precision time
            double elapsed = calc_elapsed_time(&server_ready_time, &server_start_time);

            log_group_begin();
            log_this(SR_STARTUP, "System started at %s", LOG_LEVEL_STATE, 1, iso_time);
            log_this(SR_STARTUP, "System startup took %.3fs", LOG_LEVEL_STATE, 1, elapsed);
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

// Get the formatted server start time string
const char* get_system_start_time_string(void) {
    static char time_buffer[32]; // Static buffer for thread safety
    time_t start_time = get_server_start_time();
    if (start_time > 0) {
        format_iso_time(start_time, time_buffer, sizeof(time_buffer));
    } else {
        snprintf(time_buffer, sizeof(time_buffer), "unknown");
    }
    return time_buffer;
}

// Calculate startup time (duration from current start to ready)
double calculate_startup_time(void) {
    // Return 0 if times aren't set yet
    if (server_start_time.tv_sec == 0) {
        return 0.0;
    }
    
    pthread_mutex_lock(&ready_time_mutex);
    
    // If ready time isn't set, use current time
    struct timespec end_time;
    if (server_ready_time.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
    } else {
        end_time = server_ready_time;
    }
    
    double elapsed = calc_elapsed_time(&end_time, &server_start_time);
    pthread_mutex_unlock(&ready_time_mutex);
    
    return elapsed;
}

// Record when startup is complete (called after startup sequence finishes)
void record_startup_complete_time(void) {
    clock_gettime(CLOCK_MONOTONIC, &startup_complete_time);
}

// Record when shutdown is initiated (called when shutdown sequence begins)
void record_shutdown_initiate_time(void) {
    clock_gettime(CLOCK_MONOTONIC, &shutdown_initiate_time);
}

// Calculate total running time (from startup complete to shutdown initiate)
double calculate_total_running_time(void) {
    // Return 0 if times aren't set
    if (startup_complete_time.tv_sec == 0 || shutdown_initiate_time.tv_sec == 0) {
        return 0.0;
    }
    
    return calc_elapsed_time(&shutdown_initiate_time, &startup_complete_time);
}

// Calculate total elapsed time (from original start to shutdown complete)
double calculate_total_elapsed_time(void) {
    // Return 0 if original start time isn't set
    if (original_start_time.tv_sec == 0) {
        return 0.0;
    }
    
    // If shutdown isn't complete, use current time
    struct timespec end_time;
    if (shutdown_end_time.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
    } else {
        end_time = shutdown_end_time;
    }
    
    return calc_elapsed_time(&end_time, &original_start_time);
}

// Calculate total runtime (duration from original start)
double calculate_total_runtime(void) {
    // Return 0 if original start time isn't set
    if (original_start_time.tv_sec == 0) {
        return 0.0;
    }
    
    // Get current time
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    return calc_elapsed_time(&current_time, &original_start_time);
}

// Calculate shutdown time (duration from shutdown start to end)
double calculate_shutdown_time(void) {
    // Return 0 if times aren't set yet
    if (shutdown_start_time.tv_sec == 0) {
        return 0.0;
    }
    
    // If end time isn't set, use current time
    struct timespec end_time;
    if (shutdown_end_time.tv_sec == 0) {
        clock_gettime(CLOCK_MONOTONIC, &end_time);
    } else {
        end_time = shutdown_end_time;
    }
    
    double elapsed = calc_elapsed_time(&end_time, &shutdown_start_time);
    
    return elapsed;
}
