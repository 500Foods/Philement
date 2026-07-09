/*
 * Mail Relay Send API Endpoint
 *
 * Implements POST /api/mailrelay/send for authenticated, templated mail
 * submission through the internal Mail Relay producer.
 */

#ifndef MAILRELAY_API_SEND_H
#define MAILRELAY_API_SEND_H

#include <microhttpd.h>

/*
 * Handle POST /api/mailrelay/send requests.
 *
 * Requires a valid JWT Bearer token with a database claim and a "mail_send"
 * role. The request body must contain template_key, to/cc/bcc recipients, an
 * idempotency key, and optional params/priority. from/reply_to are taken from
 * MailRelay.DefaultFrom/DefaultReplyTo; client overrides are not accepted.
 */
enum MHD_Result handle_mailrelay_send_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls);

#endif /* MAILRELAY_API_SEND_H */
