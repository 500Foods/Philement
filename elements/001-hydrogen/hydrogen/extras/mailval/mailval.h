/*
 * mailval - multi-protocol mail validator
 *
 * A standalone C mail server used as a test fixture for Hydrogen's Mail Relay.
 * It speaks SMTP (with STARTTLS / implicit TLS), IMAP (full), and JMAP (HTTPS),
 * all backed by one in-memory mailbox store so an SMTP-delivered message is
 * retrievable through IMAP and JMAP. Every session is transcribed to JSON for
 * blackbox assertions.
 *
 * This is a TEST FIXTURE, not a production server. It favors clarity and
 * coverage of protocol paths over performance or RFC completeness.
 */

#ifndef MAILVAL_H
#define MAILVAL_H

#include <stdbool.h>
#include <stdarg.h>
#include <openssl/ssl.h>

#include "capture.h"
#include "store.h"
#include "tls.h"
#include "listener.h"

#define MV_MAX_LINE 16384
#define MV_MAX_PATH 2048

/* Global runtime configuration (set by mailval.c from argv). */
typedef struct {
    const char* data_dir;   /* where captured transcripts are written */
    const char* cert_file;  /* PEM cert; if set, TLS is available */
    const char* key_file;   /* PEM key */
    int smtp_port;
    int imap_port;
    int jmap_port;
    bool verbose;
} mv_config;

extern mv_config g_cfg;

/* Protocol module: handle one accepted connection. */
typedef struct proto_module {
    const char* name;
    void (*handle)(mv_conn* conn, const char* peer, mv_capture* cap);
} proto_module;

/* Forwarded module entry points. */
extern proto_module proto_smtp;
extern proto_module proto_imap;
extern proto_module proto_jmap;

#endif /* MAILVAL_H */
