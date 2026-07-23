# Mail Guide for Hydrogen

Operator and developer guide for Hydrogen’s **Mail Relay** subsystem: how mail is produced, rewritten, queued, delivered, and how **Lua** participates at every useful stage.

This document assumes the full design in [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) is implemented (outbound, templates, API, Lua `H.mail`, events, OTP, inbound rewrite/routing, security gates). For pure Lua language help see [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md); for general scripting see [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md); for the `H` host API contract see [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md).

Subsystem overview: [mailrelay/README.md](/docs/H/core/subsystems/mailrelay/README.md).

---

## Table of Contents

1. [Mental model](#mental-model)
2. [Architecture and data flow](#architecture-and-data-flow)
3. [Configuration](#configuration)
4. [Templates and macros](#templates-and-macros)
5. [Producing mail (C, REST, Lua)](#producing-mail-c-rest-lua)
6. [Rewriting and adjusting mail in flight](#rewriting-and-adjusting-mail-in-flight)
7. [Debounce and coalescing](#debounce-and-coalescing)
8. [System events and Lua handlers](#system-events-and-lua-handlers)
9. [Inbound SMTP submission and route rewrites](#inbound-smtp-submission-and-route-rewrites)
10. [OTP / MFA mail](#otp--mfa-mail)
11. [Queue, workers, retry, persistence](#queue-workers-retry-persistence)
12. [Observability and status](#observability-and-status)
13. [Security and policy](#security-and-policy)
14. [Database objects (operators)](#database-objects-operators)
15. [Worked end-to-end scenarios](#worked-end-to-end-scenarios)
16. [Error codes and troubleshooting](#error-codes-and-troubleshooting)
17. [Related docs](#related-docs)

---

## Mental model

Mail Relay is **not** “open SMTP and hope.” It is a **controlled mail pipeline**:

1. A **producer** (REST, Lua `H.mail`, Notify, system event, inbound route) builds a **mail request**.
2. External/REST requests are **template-keyed** (`template_key` + params). Trusted **Lua** may also send **freeform** subject/body via `mailrelay_send_direct`.
3. The request is **validated**, optionally **rewritten** (routes, debounce, headers, recipients), then **enqueued**.
4. **Workers** claim queue items, render to **RFC 5322**, and deliver via **libcurl SMTP/SMTPS**.
5. Outcomes are recorded (attempts, status, metrics). Secrets and OTP plaintext never appear in normal logs.

**One delivery path.** REST, Lua, Notify, and events all enqueue through Mail Relay (`mailrelay_send_template`, `mailrelay_send_direct`, or `mailrelay_enqueue`). Scripts must not open SMTP sockets or call REST just to send mail.

**Rewrite is first-class.** You can change From/To/Cc, inject BCC watchers, swap templates, coalesce bursts, and reshape inbound submission into outbound templated mail—without callers knowing the SMTP topology.

---

## Architecture and data flow

```text
  REST /api/mailrelay/*     Lua H.mail / H.notify     System events
  Inbound SMTP (opt-in)     Notify compatibility      Internal C callers
              \                    |                    /
               \                   |                   /
                v                  v                  v
              +------------------------------------------+
              |  Policy: auth, roles, rate limits,       |
              |  recipient rules, idempotency            |
              +--------------------+---------------------+
                                   |
                                   v
               +------------------------------------------+
               |  Template resolve + macro render         |
               |  OR freeform subject/body (Lua only)     |
               |  (or inbound route rewrite → template)   |
               +--------------------+---------------------+
                                   |
                                   v
               +------------------------------------------+
               |  Optional debounce (debounce_key)        |
               +--------------------+---------------------+
                                   |
                                   v
              +------------------------------------------+
              |  mail_queue (memory + optional DB)       |
              |  priority + next_attempt_at              |
              +--------------------+---------------------+
                                   |
                                   v
              +------------------------------------------+
              |  Workers → RFC 5322 render → SMTP        |
              |  retry / permanent fail / attempts log   |
              +------------------------------------------+
```

### Layers

| Layer | Responsibility |
|-------|----------------|
| **Producer API** | `mailrelay_send_template`, `mailrelay_send_direct` (Lua freeform), `mailrelay_enqueue`, raw internal test paths |
| **Template engine** | `%MACRO%` expansion into subject/text/html (template mode only) |
| **Rewrite / routes** | Inbound and policy transforms (`rewrite_from`, `rewrite_to`, extra recipients, template swap) |
| **Debounce** | Collapse repeated keys into one mail with `%COUNT%` / `%SUMMARY%` |
| **Queue** | Priority, scheduling, optional durable `mail_queue` rows |
| **Workers** | Dequeue, send, retry classification |
| **SMTP transport** | libcurl to configured outbound servers |
| **Repository** | QueryRefs for templates, queue, attempts, OTP, routes |

### What is *not* rewritten by default

- Template **bodies** after successful render (unless a later extension hook rewrites them).
- Arbitrary caller subject/body on the public REST path (v1 **requires** `template_key`). Lua freeform is allowed separately via `H.mail`.
- SMTP credentials (never exposed to Lua or REST clients).

---

## Configuration

Mail Relay is **disabled by default**. Enable it explicitly.

### Minimal outbound example

```json
{
  "MailRelay": {
    "Enabled": true,
    "OutboundEnabled": true,
    "InboundEnabled": false,
    "Database": "Acuranzo",
    "DefaultFrom": "noreply@example.com",
    "DefaultReplyTo": "support@example.com",
    "AdminRecipients": [
      "ops@example.com"
    ],
    "Workers": 2,
    "Queue": {
      "MaxInMemory": 1000,
      "Persist": true,
      "RetryAttempts": 5,
      "InitialDelaySeconds": 5,
      "MaxDelaySeconds": 300,
      "DebounceSeconds": 30,
      "StaleTimeoutSeconds": 300
    },
    "Events": {
      "Enabled": true,
      "MaxEventsPerInterval": 10,
      "EventIntervalSeconds": 60,
      "Rules": {
        "system.server_started": "Mail.Events.ServerStarted",
        "jobs.failed": "Mail.Events.JobFailed"
      }
    },
    "Servers": [
      {
        "Host": "${env.SMTP_HOST}",
        "Port": "587",
        "Username": "${env.SMTP_USER}",
        "Password": "${env.SMTP_PASS}",
        "UseTLS": true,
        "TLSMode": "starttls",
        "TimeoutSeconds": 30
      }
    ]
  }
}
```

### Important knobs

| Setting | Meaning |
|---------|---------|
| `Enabled` | Subsystem on/off (disabled = clean skip at launch) |
| `OutboundEnabled` | Whether SMTP delivery is allowed |
| `InboundEnabled` | Whether the submission listener runs (never open-relay by default) |
| `Database` | Hydrogen DB connection for durable mail tables (`MailRelay.Database`) |
| `DefaultFrom` / `DefaultReplyTo` | Fallbacks when producers omit them |
| `AdminRecipients` | Default targets for built-in system event handlers |
| `Workers` | Concurrent SMTP worker threads |
| `Queue.*` | Capacity, retry, debounce window, persistence, stale recovery |
| `Events.Rules` | Map `event_key` → Lua script name (group.name) |
| `Servers[]` | Outbound SMTP endpoints (TLS/auth/timeout per server) |

Sensitive fields support `${env.VAR}` expansion. Config dumps mask passwords.

### Lifecycle

- Launch readiness validates outbound servers only when `OutboundEnabled` is true.
- Landing drains workers, flushes debounce, and shuts down cleanly.
- Durable “sending” rows older than `StaleTimeoutSeconds` are recovered on startup when `Queue.Persist` is true.

---

## Templates and macros

Templates live in **`mail_templates`** (not the generic polymorphic `templates` table). Each row has a stable **`template_key`**, subject/text/html bodies, and status (inactive/active/deprecated).

### Macro syntax

| Form | Meaning |
|------|---------|
| `%NAME%` | Required user or built-in macro |
| `%NAME\|default%` | Use value, or `default` if missing |
| `%%` | Literal `%` |

**Built-in macros** (always available when the producer supplies context):

| Macro | Typical source |
|-------|----------------|
| `%APP_NAME%` | App / config |
| `%SERVER_NAME%` | Server identity |
| `%TIMESTAMP%` | ISO-8601 string from producer |
| `%USER_EMAIL%` | Authenticated user, if any |
| `%REQUEST_ID%` | Correlation id |
| `%OTP_CODE%` | OTP send path only |

**No automatic HTML/text escaping.** Sanitize untrusted values in the producer (API layer, Lua script, or C caller) before they enter params.

Missing required macros (no default) fail with **`MAIL_PARAM_MISSING`**.

### Example template (`mail.test`)

Subject:

```text
Hello %NAME|friend% from %APP_NAME%
```

Text body:

```text
Hi %NAME|,

This is a test message from %SERVER_NAME% at %TIMESTAMP%.
Request: %REQUEST_ID|n/a%

— %APP_NAME%
```

HTML body:

```html
<p>Hi %NAME%,</p>
<p>This is a test message from <strong>%SERVER_NAME%</strong>
at %TIMESTAMP%.</p>
```

### Preview (no side effects)

`POST /api/mailrelay/preview` with JWT + `mail_send` role:

```json
{
  "template_key": "mail.test",
  "params": {
    "NAME": "Ada"
  }
}
```

Returns rendered `subject`, optional `text_body` / `html_body`, resolved `from` / `reply_to`, and `macros_used` — **without** enqueue or SMTP.

---

## Producing mail (C, REST, Lua)

All production paths converge on the templated producer.

### Internal C

Call `mailrelay_send_template()` (template_key, recipients, params, idempotency_key, priority, built-in macro context). Raw `mailrelay_send_raw` / `mailrelay_enqueue` are for workers and trusted internal tests—not external API clients.

### REST: send

`POST /api/mailrelay/send`  
Auth: Bearer JWT with database claim **and** `mail_send` role (role_id resolved by name via QueryRef, matched against the JWT `roles` claim of integer ids).

```json
{
  "template_key": "mail.test",
  "to": ["user@example.com"],
  "cc": [],
  "bcc": ["archive@example.com"],
  "params": {
    "NAME": "Ada"
  },
  "idempotency_key": "invoice-9981-notify",
  "priority": "normal"
}
```

Success:

```json
{
  "success": true,
  "message_id": "a1b2c3d4-...",
  "status": "queued"
}
```

Raw subject/body is **rejected** on this endpoint. Use templates.

### REST: status

`GET /api/mailrelay/status` — any valid JWT with database claim (no `mail_send` required). Returns counters: queued, sending, sent, failed, retrying, permanent_failures, last_success/failure, worker_count, queue_depth.

### Lua: `H.mail`

Scripts use the host API (async-first, same conventions as queries/HTTP).
Exactly one mode per call:

- **Template** — `template` / `template_key` + optional `params` (macro expansion).
- **Freeform** — `subject` + `text_body` / `html_body` / `body` (literal; no macros).

REST send remains template-only. Freeform is available only via Lua.

Template mode:

```lua
local h = H.mail.send({
    template = "mail.test",          -- or template_key
    to = "user@example.com",         -- string or { "a", "b" }
    cc = { "cc@example.com" },
    bcc = {},
    params = {
        NAME = "Ada",
        CUSTOM_NOTE = "Your report is ready",
    },
    idempotency_key = "report-2026-07-09",  -- optional; auto UUID if omitted
    priority = 0,                           -- optional integer
})

local res, err = H.wait(h)
if err then
    H.log.error("mail failed: %s", err)
    return 1
end
H.log.info("queued message_id=%s status=%s",
    tostring(res.message_id), tostring(res.status))
```

Freeform mode (body authored in the script):

```lua
local detail = "Job failed: " .. tostring(err_msg)
local res, err = H.mail.send_sync({
    to = "ops@example.com",
    subject = "Job failure",
    text_body = detail,              -- or body = detail
    -- html_body = "<p>" .. detail .. "</p>",
})
```

Mixing template and freeform fields is rejected (`MAIL_PARAM_MISSING`).

**Semantics of “sync”:** wait for **queue acceptance / render validation**, not for SMTP delivery (unless a test-only mode is enabled). Delivery is always worker-driven.

**Lua must not** call SMTP, shell out to `sendmail`, or HTTP-loop back to `/api/mailrelay/*` for ordinary sends. That would bypass rate limits, audit, and idempotency.

### Lua: `H.notify`

Notify is a **compatibility shim**. v1 always returns a stable deferred error
(`"notify: deferred to mailrelay rules"`) — no silent success, no channel→template
map yet. Prefer explicit event emission or `H.mail` (template or freeform).

```lua
local h = H.notify.send({
    channel = "email",
    to = "ops@example.com",
    body = "deprecated shape",
})
local res, err = H.wait(h)
-- res == nil, err == "notify: deferred to mailrelay rules"
```

Recommended practice: emit a named event or call `H.mail` with a template key
or freeform subject/body your org owns.

### Multiline bodies in Lua (templates stored as SQL)

When **authoring templates** in Helium migrations or seed scripts, use long strings:

```lua
local html = [=[
<html><body>
  <h1>Invoice for %NAME%</h1>
  <p>Amount: %AMOUNT|0.00%</p>
</body></html>
]=]
```

See [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) for `[[ ]]` / `[=[ ]=]` details.

---

## Rewriting and adjusting mail in flight

“Rewrite” means **changing envelope, recipients, template, or coalesced content** after the original intent is known but before SMTP. Hydrogen supports several rewrite points.

### Rewrite points (ordered)

| Stage | What can change | Who configures it |
|-------|-----------------|-------------------|
| **Producer** | template_key, params, to/cc/bcc, from, priority, idempotency | REST client, Lua, C, event handler |
| **Event Lua handler** | Entire mail-request table returned from script | `Events.Rules` scripts |
| **Inbound route** | `rewrite_from`, `rewrite_to`, `add_recipients_json`, `template_key`, domain filters | `mail_routes` rows |
| **Debounce flush** | `%COUNT%`, `%SUMMARY%`, single coalesced message | `debounce_key` + `DebounceSeconds` |
| **Default From/Reply-To** | Fill missing envelope fields | `DefaultFrom`, `DefaultReplyTo` |
| **Render** | Macro substitution into subject/bodies | Template + params |
| **RFC 5322 render** | Header order, MIME, Bcc exclusion from visible headers | C renderer (policy fixed) |

### Why rewrite exists

- **Compliance:** force BCC to an archive address.
- **Branding:** force From to a verified domain.
- **Canvas-style workflows:** inbound mail from a form becomes a templated internal ticket mail.
- **Noise control:** 500 identical alerts become one email with a count.
- **Security:** strip or replace untrusted From on inbound submission.

### Example: force archive BCC in an event handler

```lua
-- Script: Mail.Events.JobFailed (loaded as event rule)
return function(event)
    return {
        template_key = "ops.job_failed",
        to = event.params.owner_email or nil,
        -- rewrite: always archive
        bcc = { "mail-archive@example.com" },
        params = {
            JOB_ID = event.params.job_id or "unknown",
            ERROR = event.params.error or "",
            INSTANCE = event.params.instance_id or "",
        },
        priority = "high",
        debounce_key = "job-failed:" .. tostring(event.params.job_id or "none"),
    }
end
```

If `to` is nil, the C layer can fall back to `AdminRecipients` for known system templates (handler-dependent). Prefer explicit recipients.

### Example: Lua producer rewrites recipients based on query results

```lua
local res, err = H.query_sync([[
    SELECT email FROM accounts WHERE id = :id
]], { id = account_id })
if err or not res.rows[1] then
    H.log.error("no recipient")
    return 1
end

local email = res.rows[1].email
-- rewrite: internal domain goes to a different template
local template = "user.welcome"
if email:match("@internal%.example%.com$") then
    template = "user.welcome_internal"
end

return H.mail.send_sync({
    template = template,
    to = email,
    params = { NAME = res.rows[1].name or "friend" },
    idempotency_key = "welcome-" .. tostring(account_id),
})
```

### Example: C-level route rewrite fields (`mail_routes`)

When inbound is enabled, each active route can specify:

| Field | Effect |
|-------|--------|
| `source_network` | CIDR / network allow |
| `sender_domain` / `sender_pattern` | Match envelope sender |
| `recipient_domain` / `recipient_pattern` | Match RCPT TO |
| `auth_required` / `require_tls` | Session requirements |
| `template_key` | Force templated outbound instead of raw relay |
| `rewrite_from` | Replace From |
| `rewrite_to` | Replace primary To (or envelope recipient mapping) |
| `add_recipients_json` | Extra To/Cc/Bcc JSON |
| `priority` / `sort_seq` | Match order |

Conceptual application:

```text
Inbound message:
  MAIL FROM:<forms@partner.example>
  RCPT TO:<intake+site12@mail.example>
  Subject: New lead
  Body: ...

Matched route:
  sender_domain = partner.example
  template_key  = inbound.partner_lead
  rewrite_from  = noreply@example.com
  rewrite_to    = sales@example.com
  add_recipients_json = {"bcc":["crm-copy@example.com"]}

Outbound queued mail:
  From: noreply@example.com
  To: sales@example.com
  Bcc: crm-copy@example.com
  Template: inbound.partner_lead
  Params: extracted or mapped fields (subject snippet, body excerpt, …)
```

Inbound is **opt-in**, trusted/internal submission first, and **not** a public open relay.

### What “adjust” means for headers

Optional `headers` / `headers_json` on messages can carry extra headers (list-unsubscribe, correlation ids). The renderer must reject **header injection** (CR/LF in field values). Prefer params inside the body over free-form headers from untrusted input.

---

## Debounce and coalescing

Event storms (disk full alerts, flapping checks) should not produce thousands of emails.

### How it works

1. Producer sets `debounce_key` (e.g. `disk-full:/data`).
2. If `Queue.DebounceSeconds > 0`, the first message is held; later arrivals with the same key **refresh the window** and increment a count.
3. When the window expires, **one** message is enqueued.
4. Placeholders in subject/body are rewritten:
   - `%COUNT%` → number of arrivals
   - `%SUMMARY%` → short pluralized summary (e.g. `5 events`)

### Lua event handler with debounce

```lua
return function(event)
    local path = event.params.mount or "unknown"
    return {
        template_key = "ops.disk_alert",
        to = { "ops@example.com" },
        params = {
            MOUNT = path,
            DETAIL = event.params.detail or "",
            -- COUNT/SUMMARY filled at flush time if present in template
        },
        debounce_key = "disk:" .. path,
        priority = "high",
    }
end
```

Template subject:

```text
[%COUNT%x] Disk alert on %MOUNT%
```

Body:

```text
%SUMMARY% for mount %MOUNT%.
Latest detail: %DETAIL%
```

Shutdown **flushes** pending debounce entries so mail is not lost on landing.

---

## System events and Lua handlers

### Emitting events (C)

`mailrelay_event_emit(event_key, params)` — built-ins include `system.server_started` (after `READY FOR REQUESTS`) and `system.server_stopped` (during landing while enqueue still works).

### Config mapping

```json
"Events": {
  "Enabled": true,
  "MaxEventsPerInterval": 10,
  "EventIntervalSeconds": 60,
  "Rules": {
    "system.server_started": "Mail.Events.ServerStarted",
    "jobs.failed": "Mail.Events.JobFailed"
  }
}
```

### Handler contract (Lua)

Handlers run in a **fresh sandboxed Lua state** (same spirit as worker jobs). They receive an event table and **return a mail-request table** (or nil to suppress).

```lua
-- Mail.Events.ServerStarted
return function(event)
    H.log.info("event %s at %s", event.key, tostring(event.params.timestamp))
    return {
        template_key = "system.server_started",
        -- recipients: omit to use AdminRecipients for built-in templates
        params = {
            SERVER_NAME = event.params.server_name or "hydrogen",
            VERSION = event.params.version or "",
        },
        priority = "normal",
    }
end
```

Return shapes the C dispatcher understands (fields may be optional):

```lua
{
  template_key = "string",      -- required for enqueue
  to = "a@x" or { "a@x", "b@x" },
  cc = { ... },
  bcc = { ... },
  from = "optional@x",
  reply_to = "optional@x",
  params = { KEY = "value", ... },
  idempotency_key = "optional",
  priority = "normal" | "high" | number,
  debounce_key = "optional",
}
```

Returning `nil` or `{}` without `template_key` suppresses send (still may count as accepted emit). Raising an error is logged; the event is not partially delivered.

### Rate limits

Per `event_key` token bucket: `MaxEventsPerInterval` / `EventIntervalSeconds`. Excess emits fail closed (no send) with metrics/logs—never silent SMTP bypass.

### Default handlers

If no rule is configured, well-known keys may still use **built-in** handlers that mail `AdminRecipients` with seeded templates (`system.server_started`, `system.server_stopped`). Custom rules always win when present.

---

## Inbound SMTP submission and route rewrites

### Threat model (v1)

- **Submission-only** for trusted/internal clients.
- **Not** public MX / open relay.
- Prefer **auth + TLS** before DATA.
- Every accepted message becomes a **normal outbound queue item** after policy—usually via a **template**, not blind relay of the original body to arbitrary internet recipients.

Inbound SMTP is a normal submission dialogue (EHLO, optional STARTTLS/AUTH, MAIL FROM, RCPT TO, DATA). Success queues a message id; unauthorized attempts get permanent rejects and must not enqueue.

### Route matching algorithm (conceptual)

1. Load active `mail_routes` ordered by `priority` / `sort_seq`.
2. Filter by source network, TLS/auth flags.
3. Match sender domain/pattern and recipient domain/pattern.
4. First match wins.
5. Apply rewrites → build `MailRelaySendTemplateRequest` or structured message → `mailrelay_send_template` / enqueue.
6. Record audit fields (route_id, original envelope) in logs/DB without storing secrets.

### Full rewrite example (partner form → CRM)

**Route row (logical):**

```text
sender_domain     = forms.partner.com
recipient_pattern = lead-*@inbound.example.com
template_key      = inbound.partner_lead
rewrite_from      = noreply@example.com
rewrite_to        = crm-leads@example.com
add_recipients    = bcc:legal-archive@example.com
auth_required     = true
require_tls       = true
```

**Lua is optional here**—routes can be pure data. If you need extraction logic (parse subject tags, normalize phone fields), use an event-like script hook associated with the route or a post-accept script extension (Phase 13 style), still ending in `mailrelay_send_template`.

### Reject examples

| Case | Result |
|------|--------|
| External IP not in `source_network` | Reject |
| No matching route | Reject (fail closed) |
| Open-relay attempt (arbitrary RCPT) | Reject |
| TLS required but plaintext | Reject |
| Template missing after rewrite | Fail enqueue; do not half-send |

---

## OTP / MFA mail

OTP is a **mail feature consumed by auth**, not an auth bypass.

### Flow

1. Auth requests an OTP for purpose `login_mfa` or `email_verify` (password reset may share primitives).
2. Generator creates a code; **only a hash** is stored in `mail_otp_codes`.
3. Mail Relay sends `auth.otp_code` template with `%OTP_CODE%` in the body.
4. User submits code; verifier checks hash, expiry, attempts, purpose, consumed state.
5. On success, code is consumed once.

### Template sketch

```text
Subject: Your %APP_NAME% code

Your verification code is: %OTP_CODE%

It expires in a few minutes. If you did not request this, ignore this email.
```

### Lua (if auth orchestration is scripted)

```lua
-- Prefer dedicated auth APIs; illustrative only
local res, err = H.mail.send_sync({
    template = "auth.otp_code",
    to = user_email,
    params = {
        -- OTP_CODE is injected by the OTP producer path, not free-form Lua,
        -- when using the official OTP API. Do not log it.
    },
    idempotency_key = "otp-login-" .. user_id .. "-" .. window_id,
})
```

**Never** log OTP plaintext, put it in scoreboard state, or return it from REST except on the authenticated channel that requested it (and prefer not even then—send only via mail).

---

## Queue, workers, retry, persistence

### Status lifecycle (`mail_queue.status_a63`)

| Status | Meaning |
|--------|---------|
| pending | Waiting for a worker |
| sending | Claimed / in flight |
| sent | SMTP success |
| failed | Permanent failure |
| retrying | Scheduled for future attempt |

### Retry

- Transport marks results **retryable** (timeouts, connection errors, SMTP 4xx) vs permanent (5xx, policy).
- Backoff: `InitialDelaySeconds * 2^attempt`, capped by `MaxDelaySeconds`.
- Workers use time-aware dequeue so future retries do not block due items.

### Idempotency

Producers pass `idempotency_key`. A second send with the same key returns the existing `message_id` and status instead of double-mailing.

```lua
H.mail.send_sync({
    template = "billing.receipt",
    to = customer_email,
    params = { INVOICE = "9981" },
    idempotency_key = "receipt-9981",
})
```

### Priority

Higher priority dequeues first; FIFO within the same priority. Event handlers and API may set `priority`.

### Persistence

With `Queue.Persist=true` and a configured `Database`:

- Enqueue writes a durable row before memory queue acceptance.
- Attempts append to `mail_attempts`.
- Startup recovers stale `sending` rows older than `StaleTimeoutSeconds`.
- Multi-instance claim uses conditional updates (`instance_id`, `claim_token`) so only one worker owns a row.

---

## Observability and status

### Counters

From `mailrelay_get_status` / `GET /api/mailrelay/status`:

- lifetime queued / sent / failed / retrying / permanent_failures  
- current sending, queue_depth, worker_count  
- last_success / last_failure timestamps  

### Logging rules

- Subsystem tag: MailRelay.
- Log message ids, template keys, SMTP codes, durations.
- **Do not** log passwords, OTP codes, JWTs, or full bodies in normal levels.
- Lua `H.log.*` lines from event handlers follow the same redaction discipline.

### Attempts table

Each SMTP try stores attempt_number, server_index, success, smtp_code, duration_ms, error_class—enough for ops dashboards without body content.

---

## Security and policy

| Control | Behavior |
|---------|----------|
| Auth on send/preview | JWT + `mail_send` role_id |
| Auth on status | JWT + database claim |
| Template-only external send | No raw subject/body on public API |
| Rate limits | Per user/IP/template/event/global |
| Recipient validation | `mailrelay_is_valid_email`, optional allow/deny lists |
| Header injection | Reject CR/LF in header fields |
| TLS | Configurable outbound/inbound minimums |
| Open relay | Inbound fail-closed; no arbitrary external relay |
| Secrets | Env expansion; dumps mask passwords; test_02 style checks |

JWT `roles` claim is a **comma-separated list of role_id integers** (e.g. `"1,3"`), not role names. Gate by resolving name → id, then matching.

---

## Database objects (operators)

Created by Acuranzo migrations (numbers as in the plan; treat as the mail schema set):

| Table | Role |
|-------|------|
| `mail_templates` | Template definitions by `template_key` |
| `mail_queue` | Durable outbound items |
| `mail_attempts` | Per-try SMTP outcomes |
| `mail_events` | Optional durable event audit (when used) |
| `mail_otp_codes` | Hashed OTPs + purpose/status |
| `mail_routes` | Inbound match + rewrite rules |

Lookups cover queue status, template status, event status, OTP purpose/status, route status. Internal QueryRefs are **not** exposed on Conduit public endpoints.

Retention cleanups delete old terminal rows using cutoff timestamps computed in C (engine-agnostic).

---

## Worked end-to-end scenarios

### 1. Daily report from a Lua worker

```lua
local day = os.date("!%Y-%m-%d")
local q, err = H.query_sync([[
    SELECT name, total FROM report_lines WHERE report_day = :d
]], { d = day })
if err then
    H.log.error("report query: %s", err)
    return 1
end

local lines = { "Report for " .. day, string.rep("-", 40) }
for _, row in ipairs(q.rows) do
    lines[#lines + 1] = string.format("%-20s %10s",
        tostring(row.name), tostring(row.total))
end
local summary = table.concat(lines, "\n")

local res, merr = H.mail.send_sync({
    template = "reports.daily",
    to = { "ops@example.com", "finance@example.com" },
    params = {
        DAY = day,
        SUMMARY = summary,
        ROW_COUNT = tostring(#q.rows),
    },
    idempotency_key = "daily-report-" .. day,
})
if merr then
    H.log.error("mail: %s", merr)
    return 1
end
H.set_result("mail", res.message_id)
return 0
```

Template body uses `%SUMMARY%` and `%ROW_COUNT%`. No SMTP knowledge in the script.

### 2. REST preview then send

`POST /api/mailrelay/preview` then `POST /api/mailrelay/send` with the same `template_key`/`params`, Bearer JWT, and an `idempotency_key` on send. Use `jq` on the JSON responses.

### 3. Flapping alert coalesced to one mail

```lua
-- Called many times per minute from a monitor
return function(event)
    return {
        template_key = "ops.service_down",
        to = { "oncall@example.com" },
        params = {
            SERVICE = event.params.service,
            DETAIL = event.params.detail or "",
        },
        debounce_key = "down:" .. tostring(event.params.service),
        priority = "high",
    }
end
```

With `DebounceSeconds = 60`, a 40-flap burst becomes one email: subject `[40x] service api-gateway down`.

### 4. Inbound rewrite to templated sales mail

1. Partner posts form mail to `lead-west@inbound.example.com`.
2. Route matches `partner.com` + `lead-*@inbound.example.com`.
3. Rewrite From → `noreply@example.com`, To → `sales-west@example.com`, BCC archive.
4. Template `inbound.partner_lead` renders with params extracted from the original subject/body.
5. Worker delivers via corporate SMTP; partner never speaks to that SMTP host.

### 5. Server lifecycle mails

```json
"Rules": {
  "system.server_started": "Mail.Events.ServerStarted",
  "system.server_stopped": "Mail.Events.ServerStopped"
}
```

```lua
-- Mail.Events.ServerStarted
return function(event)
    return {
        template_key = "system.server_started",
        to = nil,  -- AdminRecipients
        params = {
            INSTANCE = event.params.instance_id or "",
            VERSION = event.params.version or "",
        },
    }
end
```

### 6. Escalate severity (rewrite template/recipients)

```lua
return function(event)
    local sev = event.params.severity or "info"
    local template = "ops.alert_info"
    local priority = "normal"
    local bcc = { "log@example.com" }

    if sev == "critical" then
        template = "ops.alert_critical"
        priority = "high"
        bcc = { "log@example.com", "oncall@example.com", "exec-page@example.com" }
    end

    return {
        template_key = template,
        to = { event.params.team_email or "ops@example.com" },
        bcc = bcc,
        params = {
            TITLE = event.params.title or "alert",
            BODY = event.params.body or "",
            SEVERITY = sev,
        },
        debounce_key = "alert:" .. tostring(event.params.fingerprint or event.params.title),
        priority = priority,
    }
end
```

This is the preferred “rewrite” style for application logic: **decide template + recipients + debounce in Lua**, leave SMTP to Mail Relay.

## Error codes and troubleshooting

### Stable API / producer codes

| Code | Meaning |
|------|---------|
| `MAIL_AUTH_REQUIRED` | Missing/invalid JWT or role |
| `MAIL_TEMPLATE_NOT_FOUND` | Unknown or inactive `template_key` |
| `MAIL_PARAM_MISSING` | Required macro without default |
| `MAIL_QUEUE_FULL` | In-memory/durable capacity exceeded |
| `MAIL_RECIPIENT_INVALID` | Bad email address |
| `MAIL_DISABLED` | Subsystem or outbound disabled |
| `MAIL_RATE_LIMITED` | Rate bucket exhausted |

Lua surfaces these as `H.wait` error strings (or structured fields when implemented).

### Checklist

Enabled + outbound? Launch Go? DB migrated + templates? Preview works? SMTP reachable? Status/`mail_attempts`? Events rule + rate limit? Inbound route/auth/TLS? Stable `idempotency_key`? Debounce window/flush?

---

## Related docs

| Document | Use |
|----------|-----|
| [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) | Implementation phases and working log |
| [mailrelay/README.md](/docs/H/core/subsystems/mailrelay/README.md) | Subsystem blurb |
| [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) | Lua + Hydrogen `H` intro |
| [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) | Pure Lua strings/tables/dates |
| [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) | Full `H.mail` / `H.notify` reference |
| [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) | Scripting subsystem roadmap |
| Helium migrations under `/elements/002-helium/acuranzo/migrations/` | Schema and seed templates |

### Design principles (recap)

1. **One enqueue path** for REST, Lua, Notify, events, and inbound-after-rewrite.  
2. **Templates first** for external/REST clients; trusted Lua may also send freeform subject/body.  
3. **Rewrite is policy**: routes, event handlers, debounce, and defaults—not ad-hoc SMTP hacks.  
4. **Lua decides intent**; C owns queue, SMTP, secrets, and audit.  
5. **Fail closed** on open-relay, missing templates, and rate limits.  
6. **Never log** credentials, OTP plaintext, or full bodies at normal levels.

---

*This guide is intentionally under 1000 lines. When behavior and this document disagree, trust the implementation and update the guide—[MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) remains the engineering working document.*
