#ifndef LOG_QUEUE_MANAGER_H
#define LOG_QUEUE_MANAGER_H

#include <stdio.h>

void* log_queue_manager(void* arg);
void init_file_logging(const char* log_file_path);
void close_file_logging(void);

#endif // LOG_QUEUE_MANAGER_H
