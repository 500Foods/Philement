/*
 * System Jobs API Endpoint Implementation
 *
 * Returns the scripting subsystem's scoreboard job list as a JSON array.
 * Requires a valid JWT for authentication.
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>
#include <src/scripting/scoreboard_json.h>

// Unity test mocks: redirect calls when compiled for the Unity test suite
#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif
#if defined(USE_MOCK_API_UTILS)
#include <unity/mocks/mock_api_utils.h>
#endif
#if defined(USE_MOCK_SCOREBOARD_JSON)
#include <unity/mocks/mock_scoreboard_json.h>
#endif

#include "jobs.h"

enum MHD_Result handle_system_jobs_request(struct MHD_Connection *connection)
{
    log_this(SR_API, "Handling jobs endpoint request", LOG_LEVEL_DEBUG, 0);

    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    jwt_validation_result_t jwt_result = {0};

    if (!auth_header || !extract_and_validate_jwt(auth_header, &jwt_result) || !jwt_result.claims) {
        if (jwt_result.claims) {
            free_jwt_claims(jwt_result.claims);
        }
        log_this(SR_API, "Jobs endpoint authentication failed", LOG_LEVEL_ALERT, 0);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Authentication required"));
        return api_send_json_response(connection, error, MHD_HTTP_UNAUTHORIZED);
    }
    free_jwt_claims(jwt_result.claims);

    json_t *snapshot = scripting_scoreboard_snapshot_json(100, false);
    if (!snapshot) {
        log_this(SR_API, "Failed to generate scoreboard snapshot", LOG_LEVEL_ERROR, 0);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Failed to generate job list"));
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_t *jobs = json_object_get(snapshot, "jobs");
    if (!jobs) {
        scripting_free_job_list(snapshot);
        log_this(SR_API, "Scoreboard snapshot missing jobs array", LOG_LEVEL_ERROR, 0);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Job list not available"));
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    json_incref(jobs);
    scripting_free_job_list(snapshot);

    return api_send_json_response(connection, jobs, MHD_HTTP_OK);
}
