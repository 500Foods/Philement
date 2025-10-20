/*
 * Time management and formatting utilities.
 * 
 * Provides functionality for:
 * - Server ready time tracking
 * - Duration formatting
 * - Time-based operations
 */

#ifndef UTILS_TIME_H
#define UTILS_TIME_H

#include <time.h>

// Server state tracking
void set_server_start_time(void);
time_t get_server_start_time(void);
void update_server_ready_time(void);
int is_server_ready_time_set(void);
time_t get_server_ready_time(void);

// Shutdown timing
void record_shutdown_start_time(void);
void record_shutdown_end_time(void);
double calculate_shutdown_time(void);

// Time formatting
void format_duration(time_t seconds, char *buffer, size_t buflen);

// Startup timing helpers
const char* get_system_start_time_string(void);
double calculate_startup_time(void);     // Time since current startup began
double calculate_total_runtime(void);    // Time since original startup

// Time calculation utilities
double calc_elapsed_time(const struct timespec *end, const struct timespec *start);

// New timing functions for shutdown improvements
void record_startup_complete_time(void);     // Record when startup finishes
void record_shutdown_initiate_time(void);    // Record when shutdown begins
double calculate_total_running_time(void);   // Time from startup complete to shutdown initiate
double calculate_total_elapsed_time(void);   // Time from original start to shutdown complete

#endif // UTILS_TIME_H
