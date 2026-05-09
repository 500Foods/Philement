#!/usr/bin/env node
/*
 * Tiny mock Keycloak (Phase 9 + Phase 11 + Phase 12).
 *
 * Serves just enough surface for `test_42_oidc_rp.sh` to exercise the
 * Hydrogen OIDC RP discovery + JWKS + token-exchange + id-token
 * validation paths:
 *
 *   GET /realms/test/.well-known/openid-configuration
 *       → JSON discovery doc that points at this same server.
 *
 *   GET /realms/test/protocol/openid-connect/certs
 *       → JWKS document with one freshly-generated RSA public key
 *         (kid=test-key-1). The keypair is generated on every server
 *         start; the public half is exported as a JWK and the private
 *         half is retained in memory for /token signing.
 *
 *   POST /realms/test/protocol/openid-connect/token   (Phase 11+12)
 *       → On valid grant_type=authorization_code with the canned
 *         code "test-code-ok", responds 200 with a real RS256-signed
 *         id_token + access_token bundle. The id_token claims include
 *         iss, sub, aud, iat, exp, and a nonce derived from the
 *         optional `nonce` form parameter (if absent, defaults to
 *         "test-nonce" so unit-test fixtures can predict it).
 *         On any other code, responds 400 {"error":"invalid_grant"}.
 *         On a missing or wrong grant_type, 400
 *         {"error":"unsupported_grant_type"}.
 *
 *   GET /health
 *       → 200 "ok" — used by the test harness to wait for readiness.
 *
 * Usage:
 *   node tests/lib/mock_keycloak/server.js [port]   (default 7042)
 *
 * Sends a single line "READY <port>" to stdout once the listener is
 * accepting connections; the test harness greps for that line. SIGTERM
 * triggers a clean shutdown.
 *
 * Dependencies: Node built-ins only (`node:http`, `node:process`,
 * `node:crypto`). No npm install required.
 */

import http from 'node:http';
import process from 'node:process';
import crypto from 'node:crypto';

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

// Phase 12: generate a fresh RSA-2048 keypair on startup. The public
// half is exported as a JWK and served via the JWKS endpoint; the
// private half is retained for signing /token responses. A fresh key
// per process means tests are insensitive to leaked key material from
// previous runs and removes any need to commit PEM files to the repo.
//
// `KID` is fixed for Phase 12 since the only rotation flow exercised
// in this phase happens through the Hydrogen-side test seam. If a
// future phase needs to test mock-side key rotation (e.g. an
// integration test where Hydrogen's JWKS cache is invalidated by a
// real change in the mock's served keys), expand to a
// `Map<kid, {publicKey, privateKey}>` and add a `/admin/rotate-keys`
// endpoint that mints a new entry. The current shape is the simplest
// thing that works; the upgrade path is straightforward.
const KID = 'test-key-1';
const { publicKey, privateKey } = crypto.generateKeyPairSync('rsa', {
    modulusLength: 2048,
});

const PUBLIC_JWK = publicKey.export({ format: 'jwk' });

const JWKS_DOC = {
    keys: [
        {
            kid: KID,
            kty: PUBLIC_JWK.kty,
            alg: 'RS256',
            use: 'sig',
            n: PUBLIC_JWK.n,
            e: PUBLIC_JWK.e,
        },
    ],
};

// Sign a payload as a compact JWS (RS256) under our private key.
// `payloadObj` is plain JSON; this function adds nothing beyond what
// the caller specifies.
function signJwt(payloadObj) {
    const header = {
        alg: 'RS256',
        kid: KID,
        typ: 'JWT',
    };
    const headerB64 = Buffer.from(JSON.stringify(header), 'utf8')
        .toString('base64url');
    const payloadB64 = Buffer.from(JSON.stringify(payloadObj), 'utf8')
        .toString('base64url');
    const signingInput = `${headerB64}.${payloadB64}`;

    const sig = crypto.createSign('RSA-SHA256')
        .update(signingInput)
        .sign(privateKey);
    const sigB64 = sig.toString('base64url');

    return `${signingInput}.${sigB64}`;
}

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

// Read the full request body up to a sanity cap, then call cb(body).
// Used by the /token endpoint. Caps body at 64 KiB — plenty for an
// OAuth2 form payload, well below anything that would constitute an
// abuse signal.
function readBody(req, cb) {
    const chunks = [];
    let total = 0;
    req.on('data', (c) => {
        total += c.length;
        if (total > 65536) {
            req.destroy();
            return;
        }
        chunks.push(c);
    });
    req.on('end', () => cb(Buffer.concat(chunks).toString('utf8')));
}

// Parse application/x-www-form-urlencoded into a plain object.
function parseForm(body) {
    const out = {};
    if (!body) return out;
    for (const part of body.split('&')) {
        if (!part) continue;
        const eq = part.indexOf('=');
        const k = eq < 0 ? part : part.slice(0, eq);
        const v = eq < 0 ? '' : part.slice(eq + 1);
        try {
            out[decodeURIComponent(k.replace(/\+/g, ' '))] =
                decodeURIComponent(v.replace(/\+/g, ' '));
        } catch {
            // Ignore malformed entries — the canned fixtures the tests
            // use are always well-formed.
        }
    }
    return out;
}

// Phase 11+12: handle POST /token.
//
// Accepts the canned `code === 'test-code-ok'` and returns a 200
// JSON response with a real RS256-signed id_token + a placeholder
// access_token. The id_token's `aud` claim is taken from the
// `client_id` form parameter (or `lithium-web` if absent), and its
// `nonce` claim is taken from the `nonce` form parameter (or the
// canned default `"test-nonce"` if absent). All other claims are
// deterministic so the test harness can assert against them.
//
// Any other code returns 400 invalid_grant. Wrong grant_type
// returns 400 unsupported_grant_type.
function handleTokenPost(req, res) {
    readBody(req, (body) => {
        const params = parseForm(body);

        if (params.grant_type !== 'authorization_code') {
            send(res, 400, {
                error: 'unsupported_grant_type',
                error_description:
                    `mock supports authorization_code only (got: ${params.grant_type || '(missing)'})`,
            });
            return;
        }

        if (params.code !== 'test-code-ok') {
            send(res, 400, {
                error: 'invalid_grant',
                error_description: 'authorization code expired, revoked, or never issued',
            });
            return;
        }

        const now = Math.floor(Date.now() / 1000);
        const aud = params.client_id || 'lithium-web';
        const nonce = params.nonce || 'test-nonce';

        const idToken = signJwt({
            iss: ISSUER,
            sub: 'mock-sub-12345',
            aud,
            iat: now,
            exp: now + 300,
            nonce,
            email: 'mockuser@example.com',
            email_verified: true,
            preferred_username: 'mockuser',
            name: 'Mock User',
        });

        send(res, 200, {
            access_token: 'phase11-placeholder-access-token',
            token_type: 'Bearer',
            expires_in: 300,
            id_token: idToken,
            // refresh_token deliberately omitted; Hydrogen does not
            // consume it (plan non-goal #4).
            scope: 'openid profile email',
        });
    });
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
    if (req.method === 'POST' &&
        url === `/realms/${REALM}/protocol/openid-connect/token`) {
        handleTokenPost(req, res);
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
