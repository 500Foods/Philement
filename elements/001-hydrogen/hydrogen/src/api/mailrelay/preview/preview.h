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
enum MHD_Result handle_mailrelay_preview_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

#endif /* MAILRELAY_API_PREVIEW_H */
