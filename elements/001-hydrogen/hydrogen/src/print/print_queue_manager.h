/*
 * Print Job Management for 3D Printing
 * 
 */

#ifndef PRINT_QUEUE_MANAGER_H
#define PRINT_QUEUE_MANAGER_H

#include <stdio.h>
#include <src/queue/queue.h>

extern Queue* print_queue;

void* print_queue_manager(void* arg);
int init_print_queue(void);  // Returns 1 on success, 0 on failure
void shutdown_print_queue(void);
void process_print_job(const char* job_data);
void cleanup_print_queue_manager(void* arg);

#endif // PRINT_QUEUE_MANAGER_H
