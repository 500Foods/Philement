#!/usr/bin/env node
/*
 * Tiny mock Keycloak (Phase 9 + Phase 11 + Phase 12 + Phase 14 + Phase 22).
 *
 * Serves just enough surface for `test_42_oidc_rp.sh` to exercise the
 * Hydrogen OIDC RP discovery + JWKS + token-exchange + id-token
 * validation + callback chain:
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
 *   GET /realms/test/protocol/openid-connect/auth   (Phase 14)
 *       → Skips the user-login UI entirely and 302-redirects the
 *         browser straight back to the supplied redirect_uri with
 *         `?code=test-code-ok&state=<echoed>`. The mock takes the
 *         flow's word that the user authenticated; the real login
 *         UX is irrelevant to the OIDC RP code paths under test.
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

// A throwaway keypair used by the `idTokenInvalid` test mode to sign
// an id_token that the Hydrogen RP cannot verify (unknown `kid`). The
// public half is never exported via the JWKS endpoint, so Hydrogen's
// id_token validator rejects the signature → exercises the
// `id_token_*` error branch in oidc_rp_callback.c.
const { privateKey: badPrivateKey } = crypto.generateKeyPairSync('rsa', {
    modulusLength: 2048,
});

// Test-only mode toggles. The Hydrogen RP drives /token indirectly
// (server → IdP POST), so the blackbox test sets these via the
// `_test/set-mode` admin endpoint. They let a single mock instance
// flip between the happy path and the various failure branches the
// /callback handler must handle, without restarting the process.
//
//   tokenError   : if non-empty, /token returns 400 {"error":<tokenError>}
//                   (e.g. "invalid_grant") instead of a token bundle.
//   noIdToken    : if true, the 200 token bundle omits `id_token`
//                   entirely (→ /callback `server_error` path).
//   idTokenInvalid: if true, sign the id_token under badPrivateKey
//                   (unknown kid) so Hydrogen's validator fails.
const testMode = { tokenError: '', noIdToken: false, idTokenInvalid: false };

// Sign a payload as a compact JWS (RS256) under our private key.
// `payloadObj` is plain JSON; this function adds nothing beyond what
// the caller specifies.
function signJwt(payloadObj, signingKey = privateKey, kid = KID) {
    const header = {
        alg: 'RS256',
        kid,
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

// Phase 14: per-issued-code state map. The /auth handler stashes the
// nonce + client_id supplied by the RP at authorization-request time
// here, keyed by the canned `code = "test-code-ok"`. The /token
// handler retrieves them so the signed id_token's `nonce` and `aud`
// claims match what the RP originally requested. Without this, the
// Hydrogen-side id_token validator would reject the token with
// nonce_mismatch.
//
// Phases 11+12 used the form-parameter fallback (the test harness
// passed nonce + client_id directly to /token); the Phase 14 redirect
// flow doesn't propagate them through the browser, so we need this
// in-memory bridge. The map is bounded by the canned single-code
// design (max 1 entry); a future revision that issues per-request
// codes would need an LRU + TTL.
const issuedCodes = new Map();

// Phase 14: handle GET /auth.
//
// Skips the user-login UI entirely and redirects the browser back
// to the supplied `redirect_uri` with `?code=test-code-ok&state=<echoed>`.
// Captures the request's `nonce` and `client_id` so the eventual
// /token call can sign an id_token with matching claims.
//
// Inputs (URL-encoded query params per OIDC Core 1.0 §3.1.2.1):
//   - redirect_uri (required)
//   - state        (required — echoed back unmodified)
//   - nonce        (optional — defaults to "test-nonce")
//   - client_id    (optional — defaults to "lithium-web")
//   - everything else (response_type, scope, code_challenge*) is
//     accepted but ignored.
function handleAuthGet(req, res) {
    const url = new URL(req.url || '/', `http://localhost:${port}`);
    const redirectUri = url.searchParams.get('redirect_uri');
    const state       = url.searchParams.get('state');
    const nonce       = url.searchParams.get('nonce') || 'test-nonce';
    const clientId    = url.searchParams.get('client_id') || 'lithium-web';

    if (!redirectUri || !state) {
        send(res, 400, {
            error: 'invalid_request',
            error_description: 'redirect_uri and state are required',
        });
        return;
    }

    issuedCodes.set('test-code-ok', { nonce, aud: clientId });

    const target = new URL(redirectUri);
    target.searchParams.set('code', 'test-code-ok');
    target.searchParams.set('state', state);

    res.writeHead(302, {
        Location: target.toString(),
        'Cache-Control': 'no-store',
        'Content-Length': 0,
    });
    res.end();
}

// Phase 11+12: handle POST /token.
//
// Accepts the canned `code === 'test-code-ok'` and returns a 200
// JSON response with a real RS256-signed id_token + a placeholder
// access_token. Phase 14 update: the id_token's `aud` and `nonce`
// claims are pulled from the per-code map populated by /auth. If
// the code was set up directly (e.g. by older Phase 11 unit-style
// tests calling /token alone), the form-parameter fallbacks still
// apply.
//
// Any other code returns 400 invalid_grant. Wrong grant_type
// returns 400 unsupported_grant_type.
function handleTokenPost(req, res) {
    readBody(req, (body) => {
        const params = parseForm(body);

        // Test-mode: force a token-error response (e.g. invalid_grant)
        // without needing a real bad code. Exercises the /callback
        // `token_<x>` error branch.
        if (testMode.tokenError) {
            send(res, 400, {
                error: testMode.tokenError,
                error_description: `mock forced token error: ${testMode.tokenError}`,
            });
            return;
        }

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
        const issued = issuedCodes.get('test-code-ok') || {};
        // Prefer the per-code map (Phase 14 redirect flow); fall back
        // to form parameters (Phase 11/12 direct-call style).
        const aud   = issued.aud   || params.client_id || 'lithium-web';
        const nonce = issued.nonce || params.nonce     || 'test-nonce';

        const idToken = signJwt(
            {
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
                // Phase 22: realm_access.roles for IDP_REALM_ROLES / MERGE mapping.
                // A single stable role name so test assertions are predictable.
                realm_access: { roles: ['test-role'] },
            },
            // Test-mode: sign under an unknown key so Hydrogen's id_token
            // validator rejects the signature.
            testMode.idTokenInvalid ? badPrivateKey : privateKey,
            testMode.idTokenInvalid ? 'unknown-test-key' : KID);

        const tokenBundle = {
            access_token: 'phase11-placeholder-access-token',
            token_type: 'Bearer',
            expires_in: 300,
            // refresh_token deliberately omitted; Hydrogen does not
            // consume it (plan non-goal #4).
            scope: 'openid profile email',
        };
        // Happy path normally includes id_token; the test-mode sends it as
        // a JSON null so Hydrogen's token parser sees a "successful" exchange
        // but with an empty id_token → exercises the /callback
        // server_error (missing id_token) branch (oidc_rp_callback.c ~433).
        if (!testMode.noIdToken) {
            tokenBundle.id_token = idToken;
        } else {
            tokenBundle.id_token = null;
        }

        send(res, 200, tokenBundle);
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
    // Phase 14: authorization endpoint. Match by path prefix so the
    // query string can ride along.
    if (req.method === 'GET' &&
        url.startsWith(`/realms/${REALM}/protocol/openid-connect/auth`)) {
        handleAuthGet(req, res);
        return;
    }
    if (req.method === 'POST' &&
        url === `/realms/${REALM}/protocol/openid-connect/token`) {
        handleTokenPost(req, res);
        return;
    }
    // Test-only control surface used by the blackbox suite (test_42) to
    // flip the mock between the happy path and the various /token /
    // id_token failure branches. Not part of the OIDC contract.
    if (req.method === 'POST' &&
        url === `/realms/${REALM}/_test/set-mode`) {
        readBody(req, (body) => {
            const params = parseForm(body);
            testMode.tokenError = params.tokenError || '';
            testMode.noIdToken = params.noIdToken === '1' || params.noIdToken === 'true';
            testMode.idTokenInvalid = params.idTokenInvalid === '1' || params.idTokenInvalid === 'true';
            send(res, 200, { ok: true, mode: { ...testMode } });
        });
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
