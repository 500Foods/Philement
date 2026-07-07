/*
 * Mail Relay public internal API implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_test_seams.h>

bool mailrelay_init(void) {
    mailrelay_reset_seams();
    return true;
}

bool mailrelay_send_raw(const MailRelayMessage* msg,
                        const OutboundServer* server,
                        const char* default_from,
                        const char* app_name,
                        MailRelayResult* out) {
    if (!msg || !server || !out) {
        if (out) {
            mailrelay_result_init(out);
            snprintf(out->error, sizeof(out->error), "MAIL_RAW_INVALID_ARGS");
        }
        return false;
    }
    return mailrelay_smtp_send(msg, server, default_from, app_name, out);
}
