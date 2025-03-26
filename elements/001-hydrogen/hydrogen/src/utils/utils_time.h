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
double calculate_startup_time(void);

#endif // UTILS_TIME_H