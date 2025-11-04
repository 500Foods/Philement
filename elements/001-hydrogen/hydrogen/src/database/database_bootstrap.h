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
 * This ALWAYS loads migration information (AVAIL/LOAD/APPLY)
 * This ALWAYS populates QTC with query templates for Conduit and migrations
 *
 * @param db_queue The database queue to execute bootstrap on
 */
void database_queue_execute_bootstrap_query(DatabaseQueue* db_queue);

/*
 * Populate QTC from bootstrap query results
 * Called separately when QTC needs to be refreshed without re-running bootstrap query
 * Note: This is currently not implemented as bootstrap query always populates QTC
 *
 * @param db_queue The database queue with bootstrap data to populate QTC from
 */
void database_queue_populate_qtc_from_bootstrap(DatabaseQueue* db_queue);

#endif // DATABASE_BOOTSTRAP_H