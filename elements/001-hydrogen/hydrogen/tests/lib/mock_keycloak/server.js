#!/usr/bin/env node
/*
 * Tiny mock Keycloak (Phase 9).
 *
 * Serves just enough surface for `test_42_oidc_rp.sh` to exercise the
 * Hydrogen OIDC RP discovery + JWKS path:
 *
 *   GET /realms/test/.well-known/openid-configuration
 *       → JSON discovery doc that points at this same server.
 *
 *   GET /realms/test/protocol/openid-connect/certs
 *       → JWKS document with one fixture RSA key (kid=test-key-1).
 *
 *   GET /health
 *       → 200 "ok" — used by the test harness to wait for readiness.
 *
 * Future phases will add:
 *   POST /realms/test/protocol/openid-connect/token
 *   GET  /realms/test/protocol/openid-connect/auth
 *
 * Usage:
 *   node tests/lib/mock_keycloak/server.js [port]   (default 7042)
 *
 * Sends a single line "READY <port>" to stdout once the listener is
 * accepting connections; the test harness greps for that line. SIGTERM
 * triggers a clean shutdown.
 *
 * The fixture RSA key is intentionally a placeholder — no signing
 * happens here. Phase 12 (ID token validation) will replace it with
 * a real keypair so the mock can sign id_tokens.
 */

import http from 'node:http';
import process from 'node:process';

const port = parseInt(process.argv[2], 10) || 7042;

// The realm path component matches what `hydrogen_test_42_oidc_rp.json`
// will configure for the issuer URL.
const REALM = 'test';
const ISSUER = `http://localhost:${port}/realms/${REALM}`;

const DISCOVERY_DOC = {
    issuer: ISSUER,
    authorization_endpoint:
        `${ISSUER}/protocol/openid-connect/auth`,
    token_endpoint:
        `${ISSUER}/protocol/openid-connect/token`,
    userinfo_endpoint:
        `${ISSUER}/protocol/openid-connect/userinfo`,
    jwks_uri:
        `${ISSUER}/protocol/openid-connect/certs`,
    end_session_endpoint:
        `${ISSUER}/protocol/openid-connect/logout`,
    response_types_supported: ['code'],
    subject_types_supported: ['public'],
    id_token_signing_alg_values_supported: ['RS256'],
    scopes_supported: ['openid', 'profile', 'email'],
    token_endpoint_auth_methods_supported:
        ['client_secret_basic', 'client_secret_post'],
    code_challenge_methods_supported: ['S256'],
};

// Placeholder JWKS — Phase 12 will swap in a real keypair so the mock
// can sign id_tokens. Phase 9 only validates *parsing* and *caching*.
const JWKS_DOC = {
    keys: [
        {
            kid: 'test-key-1',
            kty: 'RSA',
            alg: 'RS256',
            use: 'sig',
            n: 'placeholder-n-base64url',
            e: 'AQAB',
        },
    ],
};

function send(res, status, body, contentType = 'application/json') {
    const buf = typeof body === 'string'
        ? Buffer.from(body, 'utf8')
        : Buffer.from(JSON.stringify(body), 'utf8');
    res.writeHead(status, {
        'Content-Type': contentType,
        'Content-Length': buf.length,
        'Cache-Control': 'no-store',
    });
    res.end(buf);
}

const server = http.createServer((req, res) => {
    const url = req.url || '/';
    if (req.method === 'GET' && url === '/health') {
        send(res, 200, 'ok', 'text/plain');
        return;
    }
    if (req.method === 'GET' &&
        url === `/realms/${REALM}/.well-known/openid-configuration`) {
        send(res, 200, DISCOVERY_DOC);
        return;
    }
    if (req.method === 'GET' &&
        url === `/realms/${REALM}/protocol/openid-connect/certs`) {
        send(res, 200, JWKS_DOC);
        return;
    }
    send(res, 404, { error: 'not_found', path: url });
});

server.on('listening', () => {
    // Single-line ready signal for the test harness. Flushing stdout
    // so the launcher can match it deterministically.
    process.stdout.write(`READY ${port}\n`);
});

server.on('error', (err) => {
    process.stderr.write(`mock_keycloak error: ${err.message}\n`);
    process.exit(1);
});

const shutdown = () => {
    server.close(() => process.exit(0));
    setTimeout(() => process.exit(0), 2000).unref();
};
process.on('SIGTERM', shutdown);
process.on('SIGINT', shutdown);

server.listen(port, '127.0.0.1');
