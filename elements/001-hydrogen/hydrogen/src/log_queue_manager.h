/*
 * Log queue manager interface for the Hydrogen server.
 * 
 * Manages asynchronous processing of log messages from a queue, supporting
 * multiple output destinations (console, file, database). Provides thread-safe
 * initialization and cleanup of logging facilities.
 */

#ifndef LOG_QUEUE_MANAGER_H
#define LOG_QUEUE_MANAGER_H

#include <stdio.h>

void* log_queue_manager(void* arg);
void init_file_logging(const char* log_file_path);
void close_file_logging();

#endif // LOG_QUEUE_MANAGER_H
