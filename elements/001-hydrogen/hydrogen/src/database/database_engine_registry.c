/*
 * Database Engine Registry
 *
 * Engine registry lifecycle: initialize from configured connection types,
 * register/lookup engine interface pointers by enum or by name.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/mutex/mutex.h>

// Forward declarations for database engines
DatabaseEngineInterface* postgresql_get_interface(void);
DatabaseEngineInterface* sqlite_get_interface(void);
DatabaseEngineInterface* mysql_get_interface(void);
DatabaseEngineInterface* db2_get_interface(void);

// Global engine registry
static DatabaseEngineInterface* engine_registry[DB_ENGINE_MAX] = {NULL};
static pthread_mutex_t engine_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static bool engine_system_initialized = false;

bool database_engine_init(void) {
    if (engine_system_initialized) {
        return true;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    memset(engine_registry, 0, sizeof(engine_registry));

    int postgres_count = 0, mysql_count = 0, sqlite_count = 0, db2_count = 0;

    if (app_config && app_config->databases.connection_count > 0) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            const DatabaseConnection* conn = &app_config->databases.connections[i];
            if (conn->enabled && conn->type) {
                const char* engine_type = conn->type;
                if (strcmp(engine_type, "postgresql") == 0 || strcmp(engine_type, "postgres") == 0) {
                    postgres_count++;
                } else if (strcmp(engine_type, "mysql") == 0) {
                    mysql_count++;
                } else if (strcmp(engine_type, "sqlite") == 0) {
                    sqlite_count++;
                } else if (strcmp(engine_type, "db2") == 0) {
                    db2_count++;
                }
            }
        }
    }

    if (postgres_count > 0) {
        DatabaseEngineInterface* postgresql_engine = postgresql_get_interface();
        if (postgresql_engine) {
            log_this(SR_DATABASE, "- Registering PostgreSQL engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
                postgresql_engine->name ? postgresql_engine->name : "NULL",
                DB_ENGINE_POSTGRESQL);
            engine_registry[DB_ENGINE_POSTGRESQL] = postgresql_engine;
        } else {
            log_this(SR_DATABASE, "CRITICAL ERROR: Failed to get PostgreSQL engine interface!", LOG_LEVEL_ERROR, 0);
        }
    } else {
        log_this(SR_DATABASE, "- Skipping PostgreSQL engine", LOG_LEVEL_TRACE, 0);
    }

    if (sqlite_count > 0) {
        DatabaseEngineInterface* sqlite_engine = sqlite_get_interface();
        if (sqlite_engine) {
            log_this(SR_DATABASE, "- Registering SQLite engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
                sqlite_engine->name ? sqlite_engine->name : "NULL",
                DB_ENGINE_SQLITE);
            engine_registry[DB_ENGINE_SQLITE] = sqlite_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping SQLite engine", LOG_LEVEL_TRACE, 0);
    }

    if (mysql_count > 0) {
        DatabaseEngineInterface* mysql_engine = mysql_get_interface();
        if (mysql_engine) {
            log_this(SR_DATABASE, "- Registering MySQL engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
                mysql_engine->name ? mysql_engine->name : "NULL",
                DB_ENGINE_MYSQL);
            engine_registry[DB_ENGINE_MYSQL] = mysql_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping MySQL engine", LOG_LEVEL_TRACE, 0);
    }

    if (db2_count > 0) {
        DatabaseEngineInterface* db2_engine = db2_get_interface();
        if (db2_engine) {
            log_this(SR_DATABASE, "- Registering DB2 engine: %s at index %d", LOG_LEVEL_DEBUG, 2,
                db2_engine->name ? db2_engine->name : "NULL",
                DB_ENGINE_DB2);
            engine_registry[DB_ENGINE_DB2] = db2_engine;
        }
    } else {
        log_this(SR_DATABASE, "- Skipping DB2 engine", LOG_LEVEL_TRACE, 0);
    }

    engine_system_initialized = true;
    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);

    return true;
}

bool database_engine_register(DatabaseEngineInterface* engine) {
    if (!engine || !engine_system_initialized) {
        return false;
    }

    if (engine->engine_type >= DB_ENGINE_MAX) {
        log_this(SR_DATABASE, "Invalid engine type for registration", LOG_LEVEL_ERROR, 0);
        return false;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return false;
    }

    if (engine_registry[engine->engine_type] != NULL) {
        log_this(SR_DATABASE, "Engine already registered for this type", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&engine_registry_lock);
        return false;
    }

    engine_registry[engine->engine_type] = engine;

    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);

    return true;
}

DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type) {
    return database_engine_get_with_designator(engine_type, SR_DATABASE);
}

DatabaseEngineInterface* database_engine_get_with_designator(DatabaseEngine engine_type, const char* designator) {
    if (engine_type >= DB_ENGINE_MAX || !engine_system_initialized) {
        log_this(designator, "database_engine_get: Invalid engine_type or system not initialized", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, designator);
    if (result != MUTEX_SUCCESS) {
        log_this(designator, "database_engine_get: Failed to lock engine_registry_lock, result=%s", LOG_LEVEL_ERROR, 1, mutex_result_to_string(result));
        return NULL;
    }

    DatabaseEngineInterface* engine = engine_registry[engine_type];

    MUTEX_UNLOCK(&engine_registry_lock, designator);

    return engine;
}

DatabaseEngineInterface* database_engine_get_by_name(const char* name) {
    if (!name || !engine_system_initialized) {
        return NULL;
    }

    MutexResult result = MUTEX_LOCK(&engine_registry_lock, SR_DATABASE);
    if (result != MUTEX_SUCCESS) {
        return NULL;
    }

    for (int i = 0; i < DB_ENGINE_MAX; i++) {
        if (engine_registry[i] && strcmp(engine_registry[i]->name, name) == 0) {
            DatabaseEngineInterface* engine = engine_registry[i];
            MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);
            return engine;
        }
    }

    MUTEX_UNLOCK(&engine_registry_lock, SR_DATABASE);
    return NULL;
}
