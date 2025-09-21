// Database Types Header
// Common type definitions for database subsystem

#ifndef DATABASE_TYPES_H
#define DATABASE_TYPES_H

// Database engine types
typedef enum {
    DB_ENGINE_POSTGRESQL = 0,
    DB_ENGINE_SQLITE,
    DB_ENGINE_MYSQL,
    DB_ENGINE_DB2,
    DB_ENGINE_AI,         // For future AI/ML query processing
    DB_ENGINE_MAX
} DatabaseEngine;

#endif // DATABASE_TYPES_H