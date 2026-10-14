# Test 63: Conduit Webhook

## Overview

The [`test_63_conduit_webhook.sh`](/elements/001-hydrogen/hydrogen/tests/test_63_conduit_webhook.sh)
script black-box tests webhook ingress over
`POST /api/conduit/webhook/{hook}` and `POST /webhook/{hook}` (the alias),
covering LUA_CLIENT Phase 14
([`src/api/conduit/webhook/webhook.c`](/elements/001-hydrogen/hydrogen/src/api/conduit/webhook/webhook.c)).
It is the first blackbox test for `webhook.c`, which previously had **0%**
blackbox coverage (Unity-only).

## Purpose

Webhook ingress is **unauthenticated by design** (no JWT): senders prove the
payload with an HMAC-SHA256 signature over the raw body, keyed by a secret the
config names via each hook's `SecretEnv`. Validates across all eight database
engines (parallel):

- `GET /api/conduit/webhook/{hook}` → **405** `method_not_allowed`
- Unknown hook, hook path with an extra slash, empty hook name → **404** `hook_not_found`
- Missing `SignatureHeader` value, wrong-length token, wrong-value token → **401** `invalid_signature`
- Hook whose `SecretEnv` names an unset variable → **401**
- Hook configured with no `SignatureHeader` → **401**
- Valid `v1=` (stripe-style), `sha256=` prefixed, and bare-hex signature tokens → **200** with `status=completed` and the `Api.Echo` params echo (`hook`, `body`, `headers`, `content_type`)
- Empty body signed over the empty payload → **200** with `result.body == ""`
- `sha256-timestamp` algorithm: header `t=<epoch>,v1=<hmac("<epoch>.<body>")>` → **200**
- `sha256-timestamp` hook whose header has no `t=` → **401**
- `GET`/`POST /webhook/{hook}` alias → **405**/**200**
- Hook bound to a nonexistent script → **404** `script_not_found`
- Reserved key smuggled in through a header value → **500** `internal_error` (reserved params leaked)
- Body larger than `API_MAX_POST_SIZE` (10 MiB) → **500** (SQLite only)
- `Webhooks.Enabled=false` → **503** `webhooks_disabled` (SQLite variant)
- `Scripting.Enabled=false` → **503** `scripting_disabled` (SQLite variant)

## Test Configuration

- **Test Name**: Conduit Webhook
- **Test Abbreviation**: CWH
- **Test Number**: 63
- **Version**: 1.0.0

## Port Assignment

Dedicated ports in the `1563x` range (outside the Linux ephemeral client range):

| Engine | Port |
|--------|------|
| PostgreSQL | 15630 |
| MySQL | 15631 |
| SQLite | 15632 |
| DB2 | 15633 |
| MariaDB | 15634 |
| Firebird | 15635 |
| YugabyteDB | 15636 |
| MSSQL | 15637 |
| SQLite (webhooks disabled) | 15638 |
| SQLite (scripting disabled) | 15639 |

## Configuration Files

- `hydrogen_test_63_conduit_webhook_postgres.json` … `hydrogen_test_63_conduit_webhook_mssql.json` (eight engines)
- `hydrogen_test_63_conduit_webhook_sqlite_wh_disabled.json` (Webhooks off, expect 503)
- `hydrogen_test_63_conduit_webhook_sqlite_scripting_disabled.json` (Scripting off, expect 503)

Each config mirrors the test_46 engine selection (live demo DBs, `Acuranzo`
connection, dedicated Fast/Medium/Cache workers) and adds a **Webhooks**
section. Five hooks are configured:

| Hook | SignatureHeader | Hmac | SecretEnv | Script |
|------|-----------------|------|-----------|--------|
| `echo` | `X-Signature-256` | `sha256` | `HYDROGEN_TEST_WEBHOOK_SECRET` | `Api.Echo` |
| `ts` | `X-Timestamp-Signature` | `sha256-timestamp` | `HYDROGEN_TEST_WEBHOOK_SECRET` | `Api.Echo` |
| `missing` | `X-Signature-256` | `sha256` | `HYDROGEN_TEST_WEBHOOK_SECRET` | `No.SuchScript` |
| `noenv` | `X-Signature-256` | `sha256` | `HYDROGEN_TEST_WEBHOOK_SECRET_UNSET` | `Api.Echo` |
| `nosig` | *(none)* | `sha256` | `HYDROGEN_TEST_WEBHOOK_SECRET` | `Api.Echo` |

The test exports `HYDROGEN_TEST_WEBHOOK_SECRET` before starting the instances,
so every Hydrogen process inherits it (`SecretEnv` names the variable; the
server reads it with `getenv()` per request). Signatures are computed with
`openssl dgst -sha256 -hmac`, matching `utils_hmac_sha256()` +
`conduit_webhook_bytes_to_hex()` server-side.

**SQLite** uses an isolated copy of `hydrodemo.sqlite` (copied via
`sqlite_online_backup`) seeded with the same idempotent LUA_CLIENT fixture as
test_46 (`Api.Echo` invokable + QueryRef #149; AutoMigration stays off because
the shared fixture has an APPLY hole at migration 1283).

## Prerequisites

- Hydrogen binary (via `find_hydrogen_binary`)
- `PAYLOAD_KEY`, `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_JWT_KEY`
- Live engines for non-SQLite variants (same fixtures as tests 40/43/46)

## Test Flow (per engine)

1. Start Hydrogen; wait for `READY FOR REQUESTS`
2. Wait for `/api/version` to answer (MHD bind can lag the log line)
3. Exercise error, signature-variant, alias, reserved, and (SQLite) oversize
   cases above
4. Graceful shutdown

## Suite-Load Resilience

The webhook invoke path is synchronous (submit + wait, fixed 15 s) with no
client timeout override, so under the heavy CPU/DB contention of tests 41/44
a job can take longer than 15 s. The signed happy-path cases retry up to
**3×** when the response is HTTP 200 with a non-`completed` status (or a 5xx),
and the unknown-script case retries on 5xx/000. Definitive 4xx answers are
never retried.

## Related

- Plan: [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) Phase 14
- API: [webhook.md](/docs/H/api/conduit/webhook.md)
- Unity coverage: `webhook_test_*`
- Sibling blackbox: test_46 (script invoke), test_43 (scripting)
