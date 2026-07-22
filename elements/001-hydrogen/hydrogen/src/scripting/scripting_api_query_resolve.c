/*
 * Scripting Subsystem - Database Resolution and Timeout Helpers
 *
 * Phase 13 of the LUA_PLAN: database queue resolution functions.
 */

#include <src/hydrogen.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"

#include <src/api/conduit/conduit_helpers.h>
#include <src/api/auth/auth_service.h>

DatabaseQueue* resolve_db_queue(const char* db_name, char** err_out) {
    if (err_out) *err_out = NULL;

    const char* target = db_name;
    if (!target || !*target) {
        if (!app_config) {
            if (err_out) {
                *err_out = strdup("H.query: no app_config available");
            }
            return NULL;
        }
        target = app_config->scripting.DefaultDatabase;
        if (!target || !*target) {
            if (app_config->databases.connection_count == 1 &&
                app_config->databases.connections[0].name &&
                app_config->databases.connections[0].name[0] != '\0') {
                target = app_config->databases.connections[0].name;
            } else {
                if (err_out) {
                    *err_out = strdup("H.query: no database specified and no "
                                      "scripting.DefaultDatabase configured");
                }
                return NULL;
            }
        }
    }

    if (!global_queue_manager) {
        if (err_out) {
            *err_out = strdup("H.query: database queue manager not available");
        }
        return NULL;
    }

    DatabaseQueue* dbq = database_queue_manager_get_database(
        global_queue_manager, target);
    if (!dbq) {
        if (err_out) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "H.query: database '%s' not found", target);
            *err_out = strdup(buf);
        }
        return NULL;
    }
    return dbq;
}

int get_default_query_timeout(void) {
    if (app_config && app_config->scripting.DefaultQueryTimeout > 0) {
        return app_config->scripting.DefaultQueryTimeout;
    }
    return 30;
}