/*
 * Database Management Header
 *
 * Function declarations for database addition and removal functionality.
 */

#ifndef DATABASE_MANAGE_H
#define DATABASE_MANAGE_H

// Add a database configuration
bool database_add_database(const char* name, const char* engine, const char* connection_string);

// Remove a database
bool database_remove_database(const char* name);

// Test database connectivity
bool database_test_connection(const char* database_name);

#endif // DATABASE_MANAGE_H