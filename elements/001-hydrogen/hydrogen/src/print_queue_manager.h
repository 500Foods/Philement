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

void* print_queue_manager(void* arg);
void init_print_queue();
void shutdown_print_queue();

#endif // PRINT_QUEUE_MANAGER_H
