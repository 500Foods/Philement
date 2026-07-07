/*
 * Mail Relay libcurl SMTP/SMTPS send helper.
 *
 * Renders a MailRelayMessage to RFC 5322 and delivers it to a single
 * OutboundServer via libcurl. The actual transport is behind a swappable
 * seam (mailrelay_smtp_transport_fn) so unit tests can assert on the fully
 * resolved request without touching the network.
 *
 * Secrets (password, message body) are never logged.
 */

#ifndef MAILRELAY_SMTP_H
#define MAILRELAY_SMTP_H

#include <stdbool.h>

#include <src/config/config_mail_relay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fully resolved SMTP request built once by mailrelay_smtp_send and handed to
 * the transport. Keeping resolution separate from delivery makes the envelope,
 * TLS scheme, credentials, timeout, and rendered payload directly assertable.
 */
typedef struct MailRelaySmtpRequest {
    char url[512];                              /* smtp:// or smtps:// host:port */
    int use_ssl;                                /* 0 none, 1 try STARTTLS, 2 require */
    int tls_mode;                               /* resolved MAIL_TLS_MODE_* */
    char mail_from[MV_ADDR_LEN];                /* envelope MAIL FROM */
    const char* recipients[MV_MAX_RECIPIENTS];  /* to + cc + bcc (bcc stays in envelope only) */
    int recipient_count;
    char username[256];
    char password[256];                         /* never logged */
    const char* ca_path;
    int timeout_seconds;
    int auth_mode;                              /* MAIL_AUTH_MODE_* */
    const char* payload;                        /* rendered RFC 5322 message */
    size_t payload_len;
} MailRelaySmtpRequest;

/*
 * Transport: performs the actual send. The default uses libcurl; tests swap it
 * with mailrelay_smtp_set_transport. Must fill out and return out->success.
 */
typedef bool (*mailrelay_smtp_transport_fn)(const MailRelaySmtpRequest* req,
                                            MailRelayResult* out);

/*
 * Render and send one message to a single OutboundServer.
 * Returns out->success and fills the structured result. On any error out->error
 * holds a stable, secret-free code.
 */
bool mailrelay_smtp_send(const MailRelayMessage* msg,
                         const OutboundServer* server,
                         const char* default_from,
                         const char* app_name,
                         MailRelayResult* out);

/* Install a mock transport for tests; pass NULL to restore the default. */
void mailrelay_smtp_set_transport(mailrelay_smtp_transport_fn fn);

/* Restore the libcurl transport. */
void mailrelay_smtp_reset_transport(void);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_SMTP_H */
