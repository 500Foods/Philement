/*
 * TLS helpers built on OpenSSL. Supports a server context for implicit TLS
 * and a STARTTLS upgrade of an already-accepted cleartext connection.
 */

#ifndef MV_TLS_H
#define MV_TLS_H

#include <openssl/ssl.h>

/* Create a server SSL_CTX from PEM cert/key. Returns NULL on failure. */
SSL_CTX* mv_tls_server_ctx(const char* cert_file, const char* key_file);

/* Wrap an already-accepted fd in a TLS session (used for implicit TLS
 * listeners and for STARTTLS upgrades). Returns SSL* or NULL on failure. */
SSL* mv_tls_accept_fd(SSL_CTX* ctx, int fd);

#endif /* MV_TLS_H */
