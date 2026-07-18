/*
 * Mail Relay Send API Endpoint
 *
 * Implements POST /api/mailrelay/send for authenticated, templated mail
 * submission through the internal Mail Relay producer.
 */

#ifndef MAILRELAY_API_SEND_H
#define MAILRELAY_API_SEND_H

#include <microhttpd.h>
#include <src/mailrelay/mailrelay_template.h>
#include <src/mailrelay/mailrelay.h>

/*
 * Handle POST /api/mailrelay/send requests.
 *
 * Requires a valid JWT Bearer token with a database claim and a "mail_send"
 * role. The request body must contain template_key, to/cc/bcc recipients, an
 * idempotency key, and optional params/priority. from/reply_to are taken from
 * MailRelay.DefaultFrom/DefaultReplyTo; client overrides are not accepted.
 */
//@ swagger:path /api/mailrelay/send
//@ swagger:method POST
//@ swagger:operationId mailrelaySend
//@ swagger:tags "Mail Service"
//@ swagger:summary Queue a templated outbound email
//@ swagger:description Submits a template-keyed mail request to the Mail Relay queue. Requires a valid JWT Bearer token with a database claim and the mail_send role (JWT roles claim is a comma-separated list of role_id integers). Raw subject/body is not accepted. from and reply_to are taken from MailRelay.DefaultFrom and DefaultReplyTo; client overrides are ignored. On success the message is queued asynchronously (status queued).
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["template_key","idempotency_key"],"properties":{"template_key":{"type":"string","description":"Active mail template key","example":"mail.test"},"idempotency_key":{"type":"string","description":"Caller-supplied unique key for deduplication","example":"send-2026-07-10-001"},"to":{"type":"array","items":{"type":"string","format":"email"},"description":"Primary recipients (at least one of to/cc/bcc required)","example":["user@example.com"]},"cc":{"type":"array","items":{"type":"string","format":"email"},"description":"CC recipients"},"bcc":{"type":"array","items":{"type":"string","format":"email"},"description":"BCC recipients"},"params":{"type":"object","description":"Template macro substitutions as string key-value pairs","additionalProperties":{"type":"string"},"example":{"NAME":"Ada"}},"priority":{"type":"integer","description":"Queue priority (higher first; default 0)","example":0}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","message_id","status"],"properties":{"success":{"type":"boolean","example":true},"message_id":{"type":"string","description":"Queued message UUID","example":"a1b2c3d4-e5f6-7890-abcd-ef1234567890"},"status":{"type":"string","description":"Queue status after accept","example":"queued"}}}
//@ swagger:response 400 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Stable error code","enum":["MAIL_PARAM_MISSING","MAIL_RECIPIENT_INVALID","MAIL_INVALID_JSON","MAIL_METHOD_NOT_ALLOWED"],"example":"MAIL_PARAM_MISSING"},"message":{"type":"string","example":"template_key is required"}}}
//@ swagger:response 401 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_AUTH_REQUIRED"},"message":{"type":"string","example":"Valid JWT with database claim and mail_send role required"}}}
//@ swagger:response 403 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_AUTH_REQUIRED"},"message":{"type":"string","description":"Missing mail_send role","example":"mail_send role required"}}}
//@ swagger:response 404 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_TEMPLATE_NOT_FOUND"},"message":{"type":"string","example":"Template not found or inactive"}}}
//@ swagger:response 429 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_RATE_LIMITED"},"message":{"type":"string","example":"Rate limit exceeded"}}}
//@ swagger:response 503 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Service unavailable codes","enum":["MAIL_DISABLED","MAIL_QUEUE_FULL"],"example":"MAIL_DISABLED"},"message":{"type":"string","example":"Mail Relay is disabled"}}}
//@ swagger:response 500 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_INTERNAL_ERROR"},"message":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_mailrelay_send_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

/*
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly to exercise request parsing and producer-error mapping.
 */
bool parse_string_array(json_t* arr, const char* field_name,
                        const char* const** out_items, int* out_count,
                        char* err, size_t err_cap);
bool mailrelay_send_parse_template_params(json_t* obj, MailRelayTemplateParams* params,
                           char* err, size_t err_cap);
bool parse_send_request_json(json_t* request_json,
                             MailRelaySendTemplateRequest* req,
                             MailRelayTemplateParams* params,
                             const char* const** to_arr,
                             const char* const** cc_arr,
                             const char* const** bcc_arr,
                             char* err, size_t err_cap);
void mailrelay_send_parse_producer_error(const char* producer_err,
                          char* code, size_t code_cap,
                          char* message, size_t message_cap,
                          unsigned int* http_status);

#endif /* MAILRELAY_API_SEND_H */
