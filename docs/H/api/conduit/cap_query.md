# Cap-Protected Conduit Query Endpoint (`cap_query`)

## Overview

`/api/conduit/cap_query` executes pre-defined **protected** database queries by reference after verifying a ChaCha (Cap) token. Protected queries are stored in the database with `query_type_a28 = 11` and are cached in the Query Table Cache (QTC) during bootstrap.

Unlike the public `/api/conduit/query` endpoint, `cap_query` is anonymous: it does not require a JWT. Instead, the caller supplies a Cap response token (`chachaToken`) and a site identifier (`chachaSite`), which Hydrogen verifies against the configured ChaCha siteverify endpoint before executing the query.

## Base URL

```text
http://your-server:port/api/conduit/cap_query
```

## Configuration

Add the following keys to the `WebServer` section of your Hydrogen configuration (use `${env.*}` references; do not commit real secrets):

```json
{
  "WebServer": {
    "ChaChaServer": "${env.CHACHA_SERVER}",
    "ChaChaSiteID": "${env.CHACHA_SITEID}",
    "ChaChaSecret": "${env.CHACHA_SECRET}"
  }
}
```

| Key | Purpose |
|-----|---------|
| `ChaChaServer` | Base URL of the Cap siteverify endpoint, e.g. `https://challenges.cloudflare.com/turnstile/v0` |
| `ChaChaSiteID` | Default site identifier (optional; the request body may override it via `chachaSite`) |
| `ChaChaSecret` | Secret key used to authenticate the server-side siteverify call |

Environment resolution happens at config load time. The secret is never written to non-DUMP logs or error responses.

## Request Format

```bash
curl -X POST http://localhost:5000/api/conduit/cap_query \
  -H "Content-Type: application/json" \
  -d '{
    "query_ref": 85,
    "database": "Demo_PG",
    "chachaToken": "<cap-response-token>",
    "chachaSite": "<site-id>",
    "params": {
      "STRING": {"name": "value"}
    }
  }'
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `query_ref` | integer | Yes | Protected query reference ID (`query_type_a28 = 11`) |
| `database` | string | Yes | Database name to execute against |
| `chachaToken` | string | Yes | Cap response token from the client |
| `chachaSite` | string | Yes | ChaCha site identifier for verification |
| `params` | object | No | Typed query parameters (same format as `/api/conduit/query`) |

## Response Formats

### Success

```json
{
  "success": true,
  "query_ref": 85,
  "database": "Demo_PG",
  "rows": [
    {"id": 1}
  ],
  "row_count": 1
}
```

### Fallback success

When the Cap siteverify endpoint returns a transient failure (HTTP 5xx or network timeout), Hydrogen accepts the request so that user submissions are not lost. The response contains the normal success payload plus a `cap_fallback` flag:

```json
{
  "success": true,
  "cap_fallback": true,
  "query_ref": 85,
  "database": "Demo_PG",
  "rows": [],
  "row_count": 0
}
```

Reviewers can identify these requests by the `cap_fallback` field.

### Error response

All Cap verification errors return HTTP 400 with the standard conduit error shape:

```json
{
  "success": false,
  "error": "CAP_TOKEN_MISSING",
  "message": "Missing required field: chachaToken"
}
```

## Error Codes

| Code | HTTP | Trigger |
|------|------|---------|
| `CAP_TOKEN_MISSING` | 400 | `chachaToken` is missing or empty |
| `CAP_TOKEN_INVALID` | 400 | Verifier returned `success:false` (explicitly rejected token) |
| `CAP_VERIFY_FAILED` | 400 | Verifier returned HTTP 4xx, malformed response, or configuration error |
| `CAP_STATEMENT_TYPE_NOT_ALLOWED` | 400 | Protected query SQL is not one of SELECT/INSERT/UPDATE/DELETE |

## Verification Flow

1. Parse and validate the JSON body.
2. Extract `chachaToken` and `chachaSite`.
3. POST `{secret, response}` to `${ChaChaServer}/${chachaSite}/siteverify`.
4. On explicit rejection → `CAP_TOKEN_INVALID`.
5. On transient failure (5xx/timeout) → continue with `cap_fallback: true`.
6. On success → look up the protected query (`query_type_a28 = 11`) in QTC.
7. Enforce statement-type restrictions (protected queries may be SELECT/INSERT/UPDATE/DELETE; DDL is rejected).
8. Force type-11 queries onto the **slow** queue.
9. Execute the query and return the result.

## Queue and Statement Restrictions

- Protected queries always run on the **slow** queue, regardless of the queue type stored in QTC.
- Public queries (`query_type_a28 = 10`) are restricted to `SELECT` statements.
- Protected queries may use `SELECT`, `INSERT`, `UPDATE`, or `DELETE`. DDL statements (`CREATE`, `DROP`, `ALTER`, etc.) are rejected.

## Testing

### Blackbox

The blackbox script `tests/test_56_cap_query.sh` exercises both negative and positive paths using the unified 7-engine config:

```bash
./tests/test_56_cap_query.sh
```

Negative paths verify `CAP_TOKEN_MISSING` and `CAP_VERIFY_FAILED`. Positive paths use the Cap fallback path (an unreachable `CHACHA_SERVER`) to execute protected INSERT queries QueryRef #085 (course suggestions) and QueryRef #086 (contact submissions) without requiring a real browser-solved token.

### Forcing a fallback

To test the fallback path, point `ChaChaServer` at a URL that returns HTTP 502/503 or does not respond, then send a valid-looking request. Hydrogen will accept the request and return `cap_fallback: true`.

## Implementation Files

```directory
src/api/conduit/
├── cap/
│   ├── cap_query.c          # Endpoint handler
│   └── cap_query.h          # Handler prototype and error contract
└── helpers/
    ├── cap_verify.c         # Token verification helper
    └── cap_verify.h         # Verification result enum
```

## Related Documentation

- [Conduit REST API Reference](/docs/H/core/subsystems/conduit/conduit_api.md) - All conduit endpoints
- [Conduit Implementation Plan](/docs/H/plans/CONDUIT.md) - Full implementation details
- [Cap Query Implementation Plan](/docs/H/plans/CAP_PLAN_QUERY.md) - This work's planning document
- [Configuration Secrets](/docs/H/SECRETS.md) - Environment variable and secret handling
