/*
 * System Readiness API Endpoint Implementation
 *
 * Implements the /api/system/readiness endpoint. Unlike /api/system/health (a static
 * liveness check), this endpoint reflects whether the server is ready to accept requests:
 * it returns HTTP 200 only once every enabled database's Lead DQM has completed its
 * conductor sequence (connect -> bootstrap -> migration -> additional queues), and HTTP 503
 * while startup/migrations are still in progress.
 *
 * This is the endpoint a Kubernetes readiness probe should target so that, while a pod is
 * still running migrations, the ingress (e.g. Traefik) routes traffic to other ready pods.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes (readiness snapshot)
#include <src/database/database.h>

// Local includes
#include "readiness.h"

// Handle GET /api/system/readiness requests
// Returns HTTP 200 with {"ready":true,...} when the server is Ready For Requests,
// or HTTP 503 with {"ready":false,...} and per-database progress while still starting.
enum MHD_Result handle_system_readiness_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling readiness endpoint request", LOG_LEVEL_DEBUG, 0);

    // Snapshot current database readiness
    DatabaseReadiness readiness;
    bool db_ready = database_get_readiness(&readiness);

    // The authoritative signal is the global server_ready flag (set once when the
    // "READY FOR REQUESTS" line is emitted). Fall back to the live snapshot for the
    // no-database case where readiness is decided at startup.
    bool ready = (server_ready != 0) || (db_ready && readiness.expected > 0);

    json_t *root = json_object();
    json_object_set_new(root, "ready", json_boolean(ready));

    json_object_set_new(root, "databases_expected", json_integer((json_int_t)readiness.expected));
    json_object_set_new(root, "databases_ready", json_integer((json_int_t)readiness.started));

    // Build "starting <db1>, <db2>... started <db3>, <db4>" style status plus arrays so a
    // continual poll shows databases moving from one side to the other.
    json_t *starting = json_array();
    json_t *started = json_array();

    char status[560];
    char starting_names[256] = {0};
    char started_names[256] = {0};
    size_t starting_count = 0;
    size_t started_count = 0;

    for (int i = 0; i < readiness.count; i++) {
        const DatabaseReadinessEntry *entry = &readiness.entries[i];
        if (entry->ready) {
            json_array_append_new(started, json_string(entry->name));
            if (started_count > 0) {
                strncat(started_names, ", ", sizeof(started_names) - strlen(started_names) - 1);
            }
            strncat(started_names, entry->name, sizeof(started_names) - strlen(started_names) - 1);
            started_count = started_count + 1;
        } else {
            json_array_append_new(starting, json_string(entry->name));
            if (starting_count > 0) {
                strncat(starting_names, ", ", sizeof(starting_names) - strlen(starting_names) - 1);
            }
            strncat(starting_names, entry->name, sizeof(starting_names) - strlen(starting_names) - 1);
            starting_count = starting_count + 1;
        }
    }

    json_object_set_new(root, "starting", starting);
    json_object_set_new(root, "started", started);

    if (ready) {
        snprintf(status, sizeof(status), "ready");
    } else if (readiness.expected == 0) {
        // No databases configured but not yet flagged ready (transient startup window).
        snprintf(status, sizeof(status), "starting");
    } else if (starting_count > 0 && started_count > 0) {
        snprintf(status, sizeof(status), "starting %s; started %s", starting_names, started_names);
    } else if (starting_count > 0) {
        snprintf(status, sizeof(status), "starting %s", starting_names);
    } else {
        snprintf(status, sizeof(status), "starting");
    }
    json_object_set_new(root, "status", json_string(status));

    // 200 when ready, 503 (Service Unavailable) while still starting so probes/ingress
    // can route traffic elsewhere until the pod is fully ready.
    unsigned int http_status = ready ? MHD_HTTP_OK : MHD_HTTP_SERVICE_UNAVAILABLE;

    // api_send_json_response takes ownership of root and decrefs it.
    return api_send_json_response(connection, root, http_status);
}
