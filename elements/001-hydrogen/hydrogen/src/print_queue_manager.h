/*
 * Print Job Management for Reliable 3D Printing
 * 
 * Why Robust Queue Management Matters:
 * 1. Print Reliability
 *    - Job state preservation
 *    - Progress tracking
 *    - Error recovery
 *    - Completion verification
 * 
 * 2. Resource Management
 *    Why This Approach?
 *    - Material planning
 *    - Time scheduling
 *    - Power management
 *    - System cooling
 * 
 * 3. Error Handling
 *    Why These Features?
 *    - Print failure recovery
 *    - Hardware protection
 *    - State preservation
 *    - Job restoration
 * 
 * 4. Job Prioritization
 *    Why This Matters?
 *    - Emergency handling
 *    - Resource optimization
 *    - Deadline management
 *    - User priorities
 * 
 * 5. System Safety
 *    Why These Checks?
 *    - Resource validation
 *    - Hardware readiness
 *    - Temperature control
 *    - Motion safety
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
