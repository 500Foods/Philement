# Conduit Signed Webhook (`/api/conduit/webhook/{hook}`)

## Overview

Unauthenticated POST of a **raw** body. C looks up `{hook}` in config,
verifies HMAC, and submits **one** configured invokable Lua script.
Vendor-blind. Not a Stripe (or GitHub) module.

`{hook}` is a **config key**, not a script name the caller picks.

## Paths

| Path | Role |
|------|------|
| `POST /api/conduit/webhook/{hook}` | Canonical. Only this path is in Swagger. |
| `POST /api/webhook/{hook}` | Alias, same handler |
| `POST /webhook/{hook}` | Alias, same handler (exact `/webhook/{name}` prefix; will not steal `/swagger`) |

## Configuration

```json
"Webhooks": {
  "Enabled": true,
  "Hooks": [
    {
      "Name": "stripe",
      "SecretEnv": "STRIPE_WEBHOOK_SECRET",
      "SignatureHeader": "Stripe-Signature",
      "Hmac": "sha256-timestamp",
      "Script": "Stripe.Webhook"
    }
  ]
}
```

| Field | Meaning |
|-------|---------|
| `Name` | URL `{hook}` (case-insensitive match) |
| `SecretEnv` | Environment variable **name** holding the HMAC secret (`getenv` at request time) |
| `SignatureHeader` | Header to read (e.g. `Stripe-Signature`, `X-Hub-Signature-256`) |
| `Hmac` | `sha256` (HMAC of raw body) or `sha256-timestamp` (HMAC of `t` + `.` + body, Stripe-style) |
| `Script` | `Group.Name` with `scripts.invokable = 1` |

Missing section → disabled. Unknown hook → **404**, no Lua. Bad or missing
signature / secret → **401**, no Lua.

## HMAC compare

1. Compute HMAC-SHA256 (`utils_hmac_sha256`) of the signed payload.
2. Hex-encode (lowercase).
3. Compare constant-time to the header token:
   - `v1=<hex>` from a comma list, or
   - `sha256=<hex>`, or
   - the whole header.

`sha256-timestamp` uses header `t=` plus `.` plus the raw body as the
signed payload (Stripe `Stripe-Signature`).

## Script params

No JWT. No `params._hydrogen`.

```json
{
  "hook": "stripe",
  "body": "<raw request body as a string>",
  "headers": { "Stripe-Signature": "…", "Content-Type": "…" },
  "content_type": "application/json"
}
```

The Lua script parses `params.body` and routes (e.g. Stripe
`payment_intent.succeeded` → `Enroll.PaidCourse`).

## Not this endpoint

- JWT `POST /api/conduit/script` — interactive SPA invoke
- Public/Cap scripts (LUA_CLIENT Phase 12) — still deferred
- `/api/stripe/webhook` — will not exist

## Related

- [script.md](/docs/H/api/conduit/script.md)
- [LUA_CLIENT_COMPLETE.md](/docs/H/plans/complete/LUA_CLIENT_COMPLETE.md) Phase 14
- Reception `STRIPE_PLAN.md` Phase 7
