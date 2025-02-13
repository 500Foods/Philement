/*
 * Print queue manager interface for the Hydrogen 3D printer server.
 * 
 * Manages a queue of 3D print jobs, providing thread-safe job submission
 * and processing. Handles job metadata in JSON format and supports graceful
 * shutdown with remaining job cleanup.
 */

#ifndef PRINT_QUEUE_MANAGER_H
#define PRINT_QUEUE_MANAGER_H

#include <stdio.h>
#include "queue.h"

extern Queue* print_queue;

void* print_queue_manager(void* arg);
int init_print_queue(void);  // Returns 1 on success, 0 on failure
void shutdown_print_queue(void);

#endif // PRINT_QUEUE_MANAGER_H
