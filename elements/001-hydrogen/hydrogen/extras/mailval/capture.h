/*
 * Per-session transcript capture. Every command/reply and selected metadata
 * field is recorded and flushed to a JSON file when the session closes, so
 * blackbox tests can assert on captured mail without parsing protocol streams.
 */

#ifndef MV_CAPTURE_H
#define MV_CAPTURE_H

typedef struct mv_capture mv_capture;

/* Create a capture for one session. proto is "smtp"/"imap"/"jmap". */
mv_capture* mv_capture_create(const char* data_dir, const char* proto, const char* peer);

/* Record a command received from the client. */
void mv_capture_command(mv_capture* c, const char* line);

/* Record a reply sent to the client. */
void mv_capture_reply(mv_capture* c, const char* line);

/* Record a structured metadata key/value (e.g. envelope, recipients). */
void mv_capture_meta(mv_capture* c, const char* key, const char* value);

/* Flush and free. Always call at session end. */
void mv_capture_close(mv_capture* c);

#endif /* MV_CAPTURE_H */
