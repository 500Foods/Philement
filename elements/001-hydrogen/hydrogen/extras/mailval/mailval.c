/*
 * mailval main: parse options, initialize the store and TLS context, and
 * launch the SMTP / IMAP / JMAP listeners. One thread per protocol; the
 * process runs until terminated (SIGINT) for use as a long-lived fixture.
 */

#define _GNU_SOURCE 1
#include "mailval.h"
#include "util.h"
#include "tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

mv_config g_cfg;

#ifndef __SANITIZE_ADDRESS__
static void crash_handler(int sig) {
    void* frames[32];
    int n = backtrace(frames, 32);
    fprintf(stderr, "mailval: fatal signal %d\n", sig);
    backtrace_symbols_fd(frames, n, 2);
    _exit(1);
}
#endif

typedef struct {
    listen_spec spec;
    pthread_t tid;
} server_thread;

static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

static void* run_listener(void* arg) {
    listen_spec* spec = arg;
    mv_listen_and_serve(spec);
    return NULL;
}

int main(int argc, char** argv) {
    g_cfg.data_dir = ".";
    g_cfg.smtp_port = 0;
    g_cfg.imap_port = 0;
    g_cfg.jmap_port = 0;
    g_cfg.cert_file = NULL;
    g_cfg.key_file = NULL;
    g_cfg.verbose = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--smtp-port") == 0 && i + 1 < argc) g_cfg.smtp_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--imap-port") == 0 && i + 1 < argc) g_cfg.imap_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--jmap-port") == 0 && i + 1 < argc) g_cfg.jmap_port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--data-dir") == 0 && i + 1 < argc) g_cfg.data_dir = argv[++i];
        else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc) g_cfg.cert_file = argv[++i];
        else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) g_cfg.key_file = argv[++i];
        else if (strcmp(argv[i], "--verbose") == 0) g_cfg.verbose = true;
        else { fprintf(stderr, "mailval: unknown arg %s\n", argv[i]); return 2; }
    }

    if (!g_cfg.smtp_port && !g_cfg.imap_port && !g_cfg.jmap_port) {
        fprintf(stderr, "mailval: specify at least one of --smtp-port/--imap-port/--jmap-port\n");
        return 2;
    }

    store_init();

    SSL_CTX* ctx = NULL;
    if (g_cfg.cert_file && g_cfg.key_file) {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
        ctx = mv_tls_server_ctx(g_cfg.cert_file, g_cfg.key_file);
        if (!ctx) { fprintf(stderr, "mailval: failed to init TLS context\n"); return 1; }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN); /* writes to a closed client socket must fail, not kill the process */
#ifndef __SANITIZE_ADDRESS__
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
#endif

    server_thread servers[3];
    int n = 0;
    if (g_cfg.smtp_port) {
        servers[n].spec = (listen_spec){ "*", g_cfg.smtp_port, false, ctx, &proto_smtp };
        pthread_create(&servers[n].tid, NULL, run_listener, &servers[n].spec); n++;
    }
    if (g_cfg.imap_port) {
        servers[n].spec = (listen_spec){ "*", g_cfg.imap_port, false, ctx, &proto_imap };
        pthread_create(&servers[n].tid, NULL, run_listener, &servers[n].spec); n++;
    }
    if (g_cfg.jmap_port) {
        /* JMAP uses implicit TLS when a cert is configured, else plaintext HTTP. */
        servers[n].spec = (listen_spec){ "*", g_cfg.jmap_port, ctx != NULL, ctx, &proto_jmap };
        pthread_create(&servers[n].tid, NULL, run_listener, &servers[n].spec); n++;
    }

    if (g_cfg.verbose) fprintf(stderr, "mailval: running (smtp=%d imap=%d jmap=%d tls=%s)\n",
                               g_cfg.smtp_port, g_cfg.imap_port, g_cfg.jmap_port, ctx ? "yes" : "no");

    /* Run until interrupted. Listeners loop forever; we just wait. */
    while (!g_stop) sleep(1);

    fprintf(stderr, "mailval: shutting down\n");
    return 0;
}
