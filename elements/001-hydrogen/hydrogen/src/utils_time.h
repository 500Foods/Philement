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
void update_server_ready_time(void);
int is_server_ready_time_set(void);
time_t get_server_ready_time(void);

// Time formatting
void format_duration(time_t seconds, char *buffer, size_t buflen);

#endif // UTILS_TIME_H