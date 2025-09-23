#ifndef LOG_QUEUE_MANAGER_H
#define LOG_QUEUE_MANAGER_H

#include <stdio.h>
#include <stdbool.h>
#include "../config/config_logging.h"

void* log_queue_manager(void* arg);
void init_file_logging(const char* log_file_path);
void close_file_logging(void);
bool should_log_to_console(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_file(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_database(const char* subsystem, int priority, const LoggingConfig* config);
bool should_log_to_notify(const char* subsystem, int priority, const LoggingConfig* config);

#endif // LOG_QUEUE_MANAGER_H
