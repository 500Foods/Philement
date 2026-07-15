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

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise the RFC 5322 rendering primitives directly. They are not
 * part of the stable public API.
 */
void render_grow(char** buf, const size_t* len, size_t* cap, size_t extra);
void render_mem(char** buf, size_t* len, size_t* cap, const char* data, size_t data_len);
void render_str(char** buf, size_t* len, size_t* cap, const char* s);
void render_header(char** buf, size_t* len, size_t* cap, const char* name, const char* value);
void render_header_list(char** buf, size_t* len, size_t* cap, const char* name, char* const* arr, int count);
char* render_boundary(const char* message_id);
void render_mime_part(char** buf, size_t* len, size_t* cap, const char* boundary, const char* part_type, const char* body);
void render_rfc822_date(char* buf, size_t cap, time_t t);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_RENDER_H */
