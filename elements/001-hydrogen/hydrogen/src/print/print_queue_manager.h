/*
 * Print Job Management for Reliable 3D Printing
 * 
 */

#ifndef PRINT_QUEUE_MANAGER_H
#define PRINT_QUEUE_MANAGER_H

#include <stdio.h>
#include "../queue/queue.h"

extern Queue* print_queue;

void* print_queue_manager(void* arg);
int init_print_queue(void);  // Returns 1 on success, 0 on failure
void shutdown_print_queue(void);

#endif // PRINT_QUEUE_MANAGER_H
