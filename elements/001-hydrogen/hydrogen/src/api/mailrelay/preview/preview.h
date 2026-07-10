/*
 * Mail Relay Preview API Endpoint
 *
 * Implements POST /api/mailrelay/preview for authenticated, side-effect-free
 * template rendering. Returns the rendered subject and bodies, the resolved
 * from/reply-to addresses, and the list of macros that were resolved.
 */

#ifndef MAILRELAY_API_PREVIEW_H
#define MAILRELAY_API_PREVIEW_H

#include <microhttpd.h>

/*
 * Handle POST /api/mailrelay/preview requests.
 *
 * Requires a valid JWT Bearer token with a database claim and a "mail_send"
 * role. The request body must contain a template_key and optional params.
 * Recipients and idempotency keys are not required because preview never
 * touches the queue or SMTP.
 */
//@ swagger:path /api/mailrelay/preview
//@ swagger:method POST
//@ swagger:operationId mailrelayPreview
//@ swagger:tags "Mail Service"
//@ swagger:summary Preview a rendered mail template
//@ swagger:description Renders a mail template without enqueueing or sending. Requires a valid JWT Bearer token with a database claim and the mail_send role (JWT roles claim is a comma-separated list of role_id integers). Returns rendered subject and bodies, resolved from/reply_to from MailRelay defaults, and a deduplicated macros_used array. No queue or SMTP side effects.
//@ swagger:security bearerAuth
//@ swagger:request body application/json {"type":"object","required":["template_key"],"properties":{"template_key":{"type":"string","description":"Active mail template key","example":"mail.test"},"params":{"type":"object","description":"Template macro substitutions as string key-value pairs","additionalProperties":{"type":"string"},"example":{"NAME":"Ada"}}}}
//@ swagger:response 200 application/json {"type":"object","required":["success","template_key","subject","macros_used"],"properties":{"success":{"type":"boolean","example":true},"template_key":{"type":"string","example":"mail.test"},"from":{"type":"string","format":"email","description":"Resolved DefaultFrom when configured","example":"noreply@example.com"},"reply_to":{"type":"string","format":"email","description":"Resolved DefaultReplyTo when configured"},"subject":{"type":"string","example":"Hello Ada"},"text_body":{"type":"string","description":"Rendered text body when the template defines one"},"html_body":{"type":"string","description":"Rendered HTML body when the template defines one"},"macros_used":{"type":"array","items":{"type":"string"},"description":"Deduplicated macro names resolved during render","example":["NAME","APP_NAME","SERVER_NAME","TIMESTAMP"]}}}
//@ swagger:response 400 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","description":"Stable error code","enum":["MAIL_PARAM_MISSING","MAIL_INVALID_JSON","MAIL_METHOD_NOT_ALLOWED"],"example":"MAIL_PARAM_MISSING"},"message":{"type":"string","example":"template_key is required"}}}
//@ swagger:response 401 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_AUTH_REQUIRED"},"message":{"type":"string","example":"Valid JWT with database claim and mail_send role required"}}}
//@ swagger:response 403 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_AUTH_REQUIRED"},"message":{"type":"string","description":"Missing mail_send role","example":"mail_send role required"}}}
//@ swagger:response 404 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_TEMPLATE_NOT_FOUND"},"message":{"type":"string","example":"Template not found or inactive"}}}
//@ swagger:response 429 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_RATE_LIMITED"},"message":{"type":"string","example":"Rate limit exceeded"}}}
//@ swagger:response 503 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_DISABLED"},"message":{"type":"string","example":"Mail Relay is disabled"}}}
//@ swagger:response 500 application/json {"type":"object","required":["success","error","message"],"properties":{"success":{"type":"boolean","example":false},"error":{"type":"string","example":"MAIL_INTERNAL_ERROR"},"message":{"type":"string","example":"Internal server error"}}}
enum MHD_Result handle_mailrelay_preview_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

#endif /* MAILRELAY_API_PREVIEW_H */
