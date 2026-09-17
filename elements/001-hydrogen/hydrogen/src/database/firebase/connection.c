/*
 * Firebase engine - connect, health GET, disconnect, cancel.
 *
 * Phase 3 talks to the Firestore emulator only. Production SA JWT is Phase 16.
 * Empty Pass plus firestore.googleapis.com fails closed.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "http.h"
#include "utils.h"
#include "connection.h"

void firebase_free_engine_connection(FirebaseConnection* fb) {
    if (!fb) {
        return;
    }
    free(fb->project);
    free(fb->database);
    free(fb->host);
    free(fb->schema);
    free(fb->base_url);
    free(fb);
}

bool firebase_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    if (!config || !connection) {
        return false;
    }

    const char* log_subsystem = designator ? designator : SR_DATABASE;

    if (firebase_host_is_production(config->host)) {
        log_this(log_subsystem,
                 "Firebase production host requires Phase 16 SA JWT; refusing connect",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    if (!firebase_config_is_emulator(config)) {
        log_this(log_subsystem, "Firebase Phase 3 supports the emulator only", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* project = firebase_resolved_project(config);
    if (!project) {
        log_this(log_subsystem, "Firebase connect: missing project id (User)", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* database = firebase_resolved_database(config);
    const char* host = firebase_resolved_host(config);
    int port = firebase_resolved_port(config);

    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        return false;
    }

    FirebaseConnection* fb = calloc(1, sizeof(FirebaseConnection));
    if (!fb) {
        free(db_handle);
        return false;
    }

    fb->project = strdup(project);
    fb->database = strdup(database);
    fb->host = strdup(host);
    fb->port = port;
    fb->emulator = true;
    if (config->schema && *config->schema) {
        fb->schema = strdup(config->schema);
    }
    fb->base_url = firebase_http_build_documents_url(host, port, project, database, true);

    if (!fb->project || !fb->database || !fb->host || !fb->base_url) {
        firebase_free_engine_connection(fb);
        free(db_handle);
        return false;
    }

    FirebaseHttpResponse* health = firebase_http_get(fb->base_url, NULL, false,
                                                     &fb->inflight, &fb->abort_requested);
    bool healthy = health && firebase_http_status_is_healthy(health->http_status);
    if (!healthy) {
        log_this(log_subsystem, "Firebase connect: emulator health GET failed", LOG_LEVEL_ERROR, 0);
        firebase_http_response_free(health);
        firebase_free_engine_connection(fb);
        free(db_handle);
        return false;
    }
    firebase_http_response_free(health);

    db_handle->designator = designator ? strdup(designator) : NULL;
    db_handle->engine_type = DB_ENGINE_FIREBASE;
    db_handle->connection_handle = fb;
    db_handle->config = config;
    db_handle->status = DB_CONNECTION_CONNECTED;
    db_handle->connected_since = time(NULL);
    db_handle->current_transaction = NULL;
    db_handle->prepared_statements = NULL;
    db_handle->prepared_statement_count = 0;
    db_handle->prepared_statement_lru_counter = NULL;
    pthread_mutex_init(&db_handle->connection_lock, NULL);
    db_handle->in_use = false;
    db_handle->last_health_check = time(NULL);
    db_handle->consecutive_failures = 0;

    *connection = db_handle;
    log_this(log_subsystem, "Firebase connection established (emulator)", LOG_LEVEL_TRACE, 0);
    return true;
}

bool firebase_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }

    FirebaseConnection* fb = (FirebaseConnection*)connection->connection_handle;
    if (fb) {
        if (fb->inflight) {
            fb->abort_requested = true;
        }
        firebase_free_engine_connection(fb);
        connection->connection_handle = NULL;
    }

    connection->status = DB_CONNECTION_DISCONNECTED;
    const char* log_subsystem = connection->designator ? connection->designator : SR_DATABASE;
    log_this(log_subsystem, "Firebase connection closed", LOG_LEVEL_TRACE, 0);
    return true;
}

bool firebase_health_check(DatabaseHandle* connection) {
    if (!connection) {
        log_this(SR_DATABASE, "Firebase health check: connection is NULL", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    if (connection->engine_type != DB_ENGINE_FIREBASE) {
        log_this(designator, "Firebase health check: wrong engine type %d", LOG_LEVEL_ERROR, 1,
                 connection->engine_type);
        return false;
    }

    FirebaseConnection* fb = (FirebaseConnection*)connection->connection_handle;
    if (!fb || !fb->base_url) {
        log_this(designator, "Firebase health check: missing connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    fb->abort_requested = false;
    FirebaseHttpResponse* health = firebase_http_get(fb->base_url, NULL, false,
                                                     &fb->inflight, &fb->abort_requested);
    bool ok = health && firebase_http_status_is_healthy(health->http_status);
    if (ok) {
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
    } else {
        connection->consecutive_failures++;
        log_this(designator, "Firebase health check failed", LOG_LEVEL_ERROR, 0);
    }
    firebase_http_response_free(health);
    return ok;
}

bool firebase_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    FirebaseConnection* fb = (FirebaseConnection*)connection->connection_handle;
    if (!fb) {
        return false;
    }
    fb->abort_requested = false;
    if (!firebase_health_check(connection)) {
        return false;
    }
    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;
    return true;
}

void firebase_cancel_inflight(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBASE) {
        return;
    }
    FirebaseConnection* fb = (FirebaseConnection*)connection->connection_handle;
    if (!fb || !fb->inflight) {
        const char* designator = connection->designator ? connection->designator : SR_DATABASE;
        log_this(designator, "Firebase cancel_inflight: no in-flight request", LOG_LEVEL_TRACE, 0);
        return;
    }
    fb->abort_requested = true;
}
