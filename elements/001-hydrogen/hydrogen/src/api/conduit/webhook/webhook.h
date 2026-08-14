/*
 * Conduit signed webhook — POST /api/conduit/webhook/{hook}
 * (LUA_CLIENT Phase 14). Aliases share this handler.
 */

#ifndef HYDROGEN_CONDUIT_WEBHOOK_H
#define HYDROGEN_CONDUIT_WEBHOOK_H

#include <stdbool.h>
#include <stddef.h>

#include <microhttpd.h>
#include <jansson.h>

#include <src/api/api_utils.h>
#include <src/config/config_webhooks.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting_invoke.h>

#define CONDUIT_WEBHOOK_TIMEOUT_DEFAULT_S 15

char *conduit_webhook_extract_hook(const char *path);
char *conduit_webhook_extract_hook_from_url(const char *url);

char *conduit_webhook_extract_kv(const char *header, const char *key);
char *conduit_webhook_extract_signature_token(const char *header);
char *conduit_webhook_extract_timestamp(const char *header);
char *conduit_webhook_bytes_to_hex(const unsigned char *bytes, size_t len);
bool conduit_webhook_hex_equal(const char *a, const char *b);

bool conduit_webhook_verify_hmac(const unsigned char *body, size_t body_len,
                                 const char *secret,
                                 const char *header_value,
                                 const char *hmac_algo);

char *conduit_webhook_build_params_json(const char *hook,
                                        const unsigned char *body,
                                        size_t body_len,
                                        const char *content_type,
                                        json_t *headers);

json_t *conduit_webhook_error_json(const char *error_code, const char *message);

typedef ScriptingInvokeError (*conduit_webhook_submit_fn)(
    const char *script_name,
    const char *params_json,
    const ScoreboardJobLimits *limits,
    int fetch_timeout_seconds,
    char **job_id_out);

typedef ScriptingWaitResult (*conduit_webhook_wait_fn)(
    const char *job_id,
    int timeout_seconds,
    ScoreboardEntry **out_entry);

void conduit_webhook_set_submit_hook(conduit_webhook_submit_fn fn);
void conduit_webhook_set_wait_hook(conduit_webhook_wait_fn fn);

typedef enum MHD_Result (*conduit_webhook_send_json_fn)(
    struct MHD_Connection *connection,
    json_t *json_obj,
    unsigned int status_code);
void conduit_webhook_set_send_json_hook(conduit_webhook_send_json_fn fn);
enum MHD_Result conduit_webhook_send_json(struct MHD_Connection *connection,
                                          json_t *json_obj,
                                          unsigned int status_code);

typedef ApiBufferResult (*conduit_webhook_buffer_fn)(
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    ApiPostBuffer **buffer_out);
void conduit_webhook_set_buffer_hook(conduit_webhook_buffer_fn fn);

json_t *conduit_webhook_collect_headers(struct MHD_Connection *connection,
                                        const char *signature_header);
enum MHD_Result send_webhook_error(struct MHD_Connection *connection,
                                   const char *error_code,
                                   const char *message,
                                   unsigned int http_status);
enum MHD_Result handle_webhook_post(struct MHD_Connection *connection,
                                    const char *hook_name,
                                    const char *upload_data,
                                    size_t *upload_data_size,
                                    void **con_cls);

//@ swagger:path /api/conduit/webhook/{hook}
//@ swagger:method POST
//@ swagger:operationId invokeConduitWebhook
//@ swagger:tags "Conduit Service"
//@ swagger:summary Deliver a signed webhook to one configured Lua script
//@ swagger:description Unauthenticated. {hook} is a config key, not a script name. C verifies HMAC against the configured secret and header, then submits the configured invokable script with params {hook, body, headers, content_type}. No JWT. No params._hydrogen.
//@ swagger:response 200 application/json {"type":"object","properties":{"status":{"type":"string"},"job_id":{"type":"string"},"script":{"type":"string"},"result":{"type":"object"}}}
//@ swagger:response 401 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"invalid_signature"}}}
//@ swagger:response 404 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"hook_not_found"}}}
//@ swagger:response 405 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"method_not_allowed"}}}
//@ swagger:response 503 application/json {"type":"object","properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"webhooks_disabled"}}}

enum MHD_Result handle_conduit_webhook_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls,
    const char *path);

#endif /* HYDROGEN_CONDUIT_WEBHOOK_H */
