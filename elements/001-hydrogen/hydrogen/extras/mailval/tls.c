#include "tls.h"
#include "util.h"
#include <stdio.h>

SSL_CTX* mv_tls_server_ctx(const char* cert_file, const char* key_file) {
    if (!cert_file || !key_file) return NULL;
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        fprintf(stderr, "mailval: SSL_CTX_new failed\n");
        return NULL;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
    if (SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1) {
        fprintf(stderr, "mailval: cannot load cert %s\n", cert_file);
        SSL_CTX_free(ctx);
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) != 1) {
        fprintf(stderr, "mailval: cannot load key %s\n", key_file);
        SSL_CTX_free(ctx);
        return NULL;
    }
    if (SSL_CTX_check_private_key(ctx) != 1) {
        fprintf(stderr, "mailval: cert/key mismatch\n");
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

SSL* mv_tls_accept_fd(SSL_CTX* ctx, int fd) {
    if (!ctx) return NULL;
    SSL* ssl = SSL_new(ctx);
    if (!ssl) return NULL;
    SSL_set_fd(ssl, fd);
    SSL_set_accept_state(ssl);
    if (SSL_accept(ssl) != 1) {
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}
