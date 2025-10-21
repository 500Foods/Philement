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
 * This loads migration information and optionally populates QTC based on populate_qtc flag
 */
void database_queue_execute_bootstrap_query_full(DatabaseQueue* db_queue, bool populate_qtc);

/*
 * Execute bootstrap query after successful Lead DQM connection
 * This loads migration information only (QTC population controlled separately)
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue);

/*
 * Execute bootstrap query and populate QTC
 * This loads both migration information and populates the Query Table Cache
 */
void database_queue_execute_bootstrap_query_with_qtc(DatabaseQueue* db_queue);

#endif // DATABASE_BOOTSTRAP_H