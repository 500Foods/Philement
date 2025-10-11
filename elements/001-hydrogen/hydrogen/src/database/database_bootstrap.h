/*
 * Database Bootstrap Query Header
 *
 * Provides functions for executing bootstrap queries after database connection
 * establishment, primarily for Lead queues to initialize the Query Table Cache (QTC).
 */

#ifndef DATABASE_BOOTSTRAP_H
#define DATABASE_BOOTSTRAP_H

#include <src/hydrogen.h>

// Forward declaration from database_queue.h
typedef struct DatabaseQueue DatabaseQueue;

/*
 * Execute bootstrap query after successful Lead DQM connection
 * This loads the Query Table Cache (QTC) and confirms connection health
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue);

#endif // DATABASE_BOOTSTRAP_H