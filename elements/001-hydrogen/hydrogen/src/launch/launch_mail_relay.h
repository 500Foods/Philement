/*
 * Launch Mail Relay subsystem - internal helpers
 *
 * These helpers are NOT part of the stable public API. They are exposed
 * (non-static) solely so the Unity test framework can call them directly.
 */

#ifndef LAUNCH_MAIL_RELAY_H
#define LAUNCH_MAIL_RELAY_H

#include <stdbool.h>
#include <src/hydrogen.h>
#include <src/mailrelay/mailrelay_smtp.h>   // For MailRelaySmtpRequest
#include <src/mailrelay/mailrelay_result.h> // For MailRelayResult

// Register the Mail Relay subsystem for launch (registry callback)
void register_mail_relay_for_launch(void);

// Report a transport failure for an SMTP request into the result struct
bool launch_fail_transport(const MailRelaySmtpRequest* req, MailRelayResult* out);

// Block until the Mail Relay OTP cache has been populated (or timeout)
bool launch_wait_for_mailrelay_otp_cache(unsigned int timeout_ms);

// Block until a specific Mail Relay QueryRef is present in the QTC
bool launch_wait_for_mailrelay_qref(int query_ref, unsigned int timeout_ms);

// Ephemeral Lua probe of H.mail repository helpers (test seam)
void launch_mail_repo_probe(void);

#endif /* LAUNCH_MAIL_RELAY_H */
