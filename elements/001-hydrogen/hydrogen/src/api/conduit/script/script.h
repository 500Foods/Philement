/*
 * Conduit Script Invoke API — POST /api/conduit/script
 * and GET /api/conduit/script/{job_id} (LUA_CLIENT Phases 4–5).
 */

#ifndef HYDROGEN_CONDUIT_SCRIPT_H
#define HYDROGEN_CONDUIT_SCRIPT_H

#include <stdbool.h>
#include <stddef.h>

#include <microhttpd.h>
#include <jansson.h>

#include <src/api/auth/auth_service.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting_invoke.h>

/* Phase 0 design-lock defaults (overridable via Scripting.ClientInvoke*) */
#define CONDUIT_SCRIPT_PARAMS_MAX_BYTES  (256 * 1024)
#define CONDUIT_SCRIPT_TIMEOUT_DEFAULT_S 15
#define CONDUIT_SCRIPT_TIMEOUT_MAX_S     60
#define CONDUIT_SCRIPT_RESULT_MAX_BYTES  (1024 * 1024)

/* Effective limits from app_config when set; else Phase 0 defaults. */
int conduit_script_params_max_bytes(void);
int conduit_script_timeout_default_s(void);
int conduit_script_timeout_max_s(void);
int conduit_script_result_max_bytes(void);

/*
 * Parsed POST body fields (non-owning pointers into request_json lifetime
 * except where noted).
 */
typedef struct ConduitScriptRequest {
    const char *script;          /* required Group.Name */
    json_t *params;              /* optional object; may be NULL */
    bool wait;                   /* default true if omitted */
    int timeout_seconds;         /* clamped 1..60, default 15 */
} ConduitScriptRequest;

char *conduit_script_extract_job_id(const char *path);

bool conduit_script_is_enabled(void);

/*
 * Parse and validate POST JSON into *out. Rejects client-supplied
 * params._hydrogen (400). On failure sets *error_code / *error_message.
 */
bool conduit_script_parse_post_json(json_t *request_json,
                                    ConduitScriptRequest *out,
                                    const char **error_code,
                                    const char **error_message);

/*
 * Build filtered claims object for params._hydrogen. Caller owns result.
 * Returns NULL on OOM.
 */
json_t *conduit_script_claims_to_hydrogen(const jwt_claims_t *claims);

/*
 * Merge client params + _hydrogen into a heap JSON string for submit.
 * Caller frees. Returns NULL on error (*error_code set).
 */
char *conduit_script_build_params_json(const ConduitScriptRequest *req,
                                       const jwt_claims_t *claims,
                                       const char **error_code,
                                       const char **error_message);

/*
 * Build Phase 0 job outcome body. Caller owns. status_str is body status.
 * entry may be NULL (async pending). result defaults to {} if completed
 * and result_json unset.
 */
json_t *conduit_script_build_job_response(const char *status_str,
                                          const char *job_id,
                                          const char *script_name,
                                          const ScoreboardEntry *entry,
                                          long elapsed_ms);

json_t *conduit_script_error_json(const char *error_code, const char *message);

/* Non-static helpers (Unity / no-static-in-src policy). */
void conduit_script_set_string_if(json_t *obj, const char *key, const char *val);
const char *conduit_script_job_status_string(ScoreboardJobStatus st);
long conduit_script_elapsed_ms_from_entry(const ScoreboardEntry *entry);

/* Map invoke error → HTTP status + stable error code (static strings). */
void conduit_script_map_invoke_error(ScriptingInvokeError err,
                                     unsigned int *http_status_out,
                                     const char **error_code_out,
                                     const char **message_out);

/* Map wait result → body status string (static). */
const char *conduit_script_wait_status_name(ScriptingWaitResult wr);

/*
 * Test seams (Unity). NULL = production.
 */
typedef ScriptingInvokeError (*conduit_script_submit_fn)(
    const char *script_name,
    const char *params_json,
    const ScoreboardJobLimits *limits,
    int fetch_timeout_seconds,
    char **job_id_out);

typedef ScriptingWaitResult (*conduit_script_wait_fn)(
    const char *job_id,
    int timeout_seconds,
    ScoreboardEntry **out_entry);

void conduit_script_set_submit_hook(conduit_script_submit_fn fn);
void conduit_script_set_wait_hook(conduit_script_wait_fn fn);

/*
 * JWT seam: when set, replaces extract_and_validate_jwt + claims check.
 * On true, *jwt_out must have valid claims (caller frees with free_jwt_claims).
 * On false, handler assumes a 401 was already queued (or will send generic 401).
 */
typedef bool (*conduit_script_jwt_fn)(struct MHD_Connection *connection,
                                      jwt_validation_result_t *jwt_out);
void conduit_script_set_jwt_hook(conduit_script_jwt_fn fn);

//@ swagger:path /api/conduit/script
//@ swagger:method POST
//@ swagger:operationId invokeConduitScript
//@ swagger:tags "Conduit Service"
//@ swagger:summary Invoke a named DB-backed Lua script
//@ swagger:description Submits a trusted database script (Group.Name) with JSON params. Requires JWT. Default wait=true blocks until terminal status or timeout. Injects params._hydrogen from JWT claims. Returns job status and result payload.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["script"],"properties":{"script":{"type":"string","description":"Canonical script id Group.Name","example":"Api.Echo"},"params":{"type":"object","description":"Opaque params object passed to the script"},"wait":{"type":"boolean","description":"If true (default), wait for completion","example":true},"timeout_seconds":{"type":"integer","description":"Wait budget 1-60, default 15","example":15}}}
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string","enum":["completed","failed","killed","timeout","pending","running"]},"job_id":{"type":"string"},"script":{"type":"string"},"result":{"type":"object"},"error":{"type":"string","nullable":true}}}
//@ swagger:response 202 application/json {"type":"object","properties":{"status":{"type":"string","example":"pending"},"job_id":{"type":"string"},"script":{"type":"string"}}}
//@ swagger:response 400 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string"},"message":{"type":"string"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"script_not_found"}}}
//@ swagger:response 405 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"method_not_allowed"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"scripting_disabled"},"message":{"type":"string"}}}

//@ swagger:path /api/conduit/script/{job_id}
//@ swagger:method GET
//@ swagger:operationId getConduitScriptJob
//@ swagger:tags "Conduit Service"
//@ swagger:summary Get script job status by id
//@ swagger:description Returns scoreboard status for a previously submitted script job. JWT required; only the submitting subject may read the job.
//@ swagger:security bearerAuth
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string"},"job_id":{"type":"string"},"result":{"type":"object"}}}
//@ swagger:response 403 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"forbidden"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"job_not_found"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"scripting_disabled"}}}

enum MHD_Result handle_conduit_script_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    const char *path);

#endif /* HYDROGEN_CONDUIT_SCRIPT_H */
