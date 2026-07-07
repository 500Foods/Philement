/*
 * Mail Relay RFC 5322 message rendering.
 *
 * Produces a heap-allocated wire-format message string from a MailRelayMessage.
 * Uses the test seams for Date and Message-ID so render output is deterministic
 * in unit tests. CRLF line endings throughout.
 */

#ifndef MAILRELAY_RENDER_H
#define MAILRELAY_RENDER_H

#include <stdbool.h>
#include "mailrelay_message.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render m to a heap-allocated RFC 5322 string.
// default_from is used when m->from is NULL.
// app_name appears in the Message-ID domain portion.
// On success *out is set to a malloc'd buffer that the caller must free().
bool mailrelay_render_message(const MailRelayMessage* m, const char* default_from, const char* app_name, char** out);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_RENDER_H */
