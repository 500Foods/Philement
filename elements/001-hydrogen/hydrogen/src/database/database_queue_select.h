/*
 * Database Queue Selection Header
 *
 * Header file for intelligent queue selection algorithm used by Conduit service.
 */

#ifndef DATABASE_QUEUE_SELECT_H
#define DATABASE_QUEUE_SELECT_H

#include "dbqueue/dbqueue.h"

// Function prototypes

/*
 * Select optimal database queue for query execution
 *
 * @param database_name Name of the target database
 * @param queue_type_hint Preferred queue type (from QTC recommendation)
 * @param manager Database queue manager containing all queues
 * @return Selected DatabaseQueue or NULL if none available
 */
DatabaseQueue* select_optimal_queue(
    const char* database_name,
    const char* queue_type_hint,
    DatabaseQueueManager* manager
);

/*
 * Update last request time for a queue (called when query is submitted)
 *
 * @param db_queue Queue to update timestamp for
 */
void update_queue_last_request_time(DatabaseQueue* db_queue);

#endif // DATABASE_QUEUE_SELECT_H