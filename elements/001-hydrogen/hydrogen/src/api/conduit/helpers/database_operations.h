/*
 * Conduit Service Database Operations Helper Functions
 *
 * These helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 */

#ifndef CONDUIT_DATABASE_OPERATIONS_H
#define CONDUIT_DATABASE_OPERATIONS_H

#include <src/hydrogen.h>

/*
 * Advance past leading SQL whitespace and line/block comments, returning a
 * pointer to the first non-skipped character (or to the terminating NUL).
 */
const char* conduit_dbops_skip_sql_whitespace_and_comments(const char* sql);

#endif /* CONDUIT_DATABASE_OPERATIONS_H */
