/*
 * Database Migration Management Header
 *
 * Function declarations for migration validation and execution.
 */

#ifndef DATABASE_MIGRATIONS_H
#define DATABASE_MIGRATIONS_H

#include <stdbool.h>

// Forward declarations
struct DatabaseQueue;

/*
 * Validate migration files are available for the given database connection
 *
 * @param db_queue The Lead DQM queue for the database
 * @return true if migrations are valid or not configured, false on validation error
 */
bool database_migrations_validate(struct DatabaseQueue* db_queue);

/*
 * Execute auto migrations for the given database connection
 * This generates and executes SQL to populate the Queries table with migration information
 *
 * @param db_queue The Lead DQM queue for the database
 * @param connection The database connection to use (must already be locked)
 * @return true if auto migration succeeds, false on failure
 */
bool database_migrations_execute_auto(struct DatabaseQueue* db_queue, DatabaseHandle* connection);

/*
 * Execute test migrations for the given database connection
 * This validates migration execution without actually running them
 *
 * @param db_queue The Lead DQM queue for the database
 * @return true if test migration succeeds or is not enabled, false on failure
 */
bool database_migrations_execute_test(struct DatabaseQueue* db_queue);

#endif /* DATABASE_MIGRATIONS_H */