# Hydrogen Examples

Practical, end-to-end examples for common Hydrogen tasks. Each example is self-contained with configuration, scripts, and verification steps, ready for a production deployment.

## Table of Contents

1. [Send a startup notification email](#1-send-a-startup-notification-email)
2. [Send a periodic server status report](#2-send-a-periodic-server-status-report)

---

## 1. Send a startup notification email

When Hydrogen finishes launching and reaches the `READY FOR REQUESTS` phase, it emits a `system.server_started` event through the Mail Relay subsystem. This event fires exactly once per successful startup, after all subsystems are running and the server is accepting requests. It is the purpose-built hook for sending yourself a notification email.

### Background: the READY FOR REQUESTS phase

At startup, Hydrogen launches all subsystems in registry order (Database, Logging, WebServer, MailRelay, Scripting, and so on). When every subsystem is running and all configured databases report ready, the server logs `READY FOR REQUESTS` and performs two actions at that exact point:

1. **Starts the Orchestrator** — `scripting_orchestrator_load_configured()` loads and runs the long-lived Orchestrator Lua script from the `scripts` table (or the no-DB path in `launch.c`).
2. **Emits the `system.server_started` event** — `mailrelay_event_emit("system.server_started", NULL)` dispatches the event through Mail Relay's event system (`launch.c:583-587` and `database.c:583-589`).

The `system.server_started` event is the cleanest hook for a startup notification because it is purpose-built for this use case: it fires once, at the right time, and the Mail Relay subsystem handles queueing, retry, and SMTP delivery for you.

### Three approaches

| Approach | When to use | Content control | Lua script needed |
| ---------- | ------------- | ----------------- | ------------------- |
| Built-in default handler | Simplest; no custom script | Template only | No |
| Custom event handler | Customize recipients, template, or params per event | Template + params via Lua | Yes |
| Orchestrator freeform | Write subject and body directly in Lua | Full freeform body in Lua | Yes |

The built-in default handler and the seeded `Mail.Events.ServerStarted` script both send to `AdminRecipients` using the `system.server_started` template (seeded by [migration 1280](/elements/002-helium/acuranzo/migrations/acuranzo_1280.lua)). If you only need to change who receives the email or the template wording, start with Option A or edit the seeded template. If you need to decide recipients or params dynamically, use Option B. If you need to write the email body as a Lua string, use Option C.

### Option A: Built-in default handler (simplest)

When `MailRelay.Events.Enabled` is `true` and no `Events.Rules` entry is configured for `system.server_started`, Hydrogen uses a built-in default Lua handler. This handler sends an email to the addresses in `MailRelay.AdminRecipients` using the `system.server_started` template. No custom Lua script is needed.

**Configuration:**

```json
{
    "MailRelay": {
        "Enabled": true,
        "OutboundEnabled": true,
        "AdminRecipients": ["you@example.com"],
        "Events": {
            "Enabled": true
        },
        "Servers": [
            {
                "Host": "${env.SMTP_HOST}",
                "Port": "${env.SMTP_PORT}",
                "Username": "${env.SMTP_USER}",
                "Password": "${env.SMTP_PASS}",
                "UseTLS": true,
                "TLSMode": 1,
                "AuthMode": 1,
                "TimeoutSeconds": 30
            }
        ]
    }
}
```

**What the email looks like** (from the seeded template in migration 1280):

```text
Subject: [1x] MailRelayEvent server_started <server_name>

Body:
Server <server_name> (Hydrogen) started at <ISO-8601 timestamp>.
1 event
```

To customize the wording, update the `system.server_started` row in the `mail_templates` table (or create a migration like [acuranzo_1282.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1282.lua) that modifies it). Template macros available: `%SERVER_NAME%`, `%APP_NAME%`, `%TIMESTAMP%`, `%COUNT%`, `%SUMMARY%`.

### Option B: Custom event handler with Lua (recommended for customization)

When you want to customize recipients, template, or params dynamically, configure an `Events.Rules` entry that maps `system.server_started` to a Lua handler script. The handler is loaded from the `scripts` table (via [QueryRef #087](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua)) and runs in a fresh sandboxed Lua state.

**Configuration:**

```json
{
    "MailRelay": {
        "Enabled": true,
        "OutboundEnabled": true,
        "AdminRecipients": ["you@example.com"],
        "Events": {
            "Enabled": true,
            "MaxEventsPerInterval": 10,
            "EventIntervalSeconds": 60,
            "Rules": {
                "system.server_started": "Mail.Events.ServerStarted"
            }
        },
        "Servers": [
            {
                "Host": "${env.SMTP_HOST}",
                "Port": "${env.SMTP_PORT}",
                "Username": "${env.SMTP_USER}",
                "Password": "${env.SMTP_PASS}",
                "UseTLS": true,
                "TLSMode": 1,
                "AuthMode": 1,
                "TimeoutSeconds": 30
            }
        ]
    }
}
```

The rule value `"Mail.Events.ServerStarted"` is split on the last `.` into `group_name = "Mail.Events"` and `script_name = "ServerStarted"`. The C loader (`mailrelay_event_resolve_source_owned` in [`mailrelay_events.c`](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_events.c)) fetches the `code` column from the `scripts` table row matching that group and name via [QueryRef #087](/elements/002-helium/acuranzo/migrations/acuranzo_1204.lua).

**The Lua handler script:**

The handler must define a global function named `handle_event`. It receives an event table and returns a mail-request table (or `nil` to suppress the send).

```lua
-- Script: Mail.Events.ServerStarted
-- Stored in the `scripts` table as group_name='Mail.Events', script_name='ServerStarted'

function handle_event(event)
    -- event fields available:
    --   event.event_key      — "system.server_started"
    --   event.timestamp      — ISO 8601 UTC string
    --   event.server_name    — from Server.ServerName config
    --   event.app_name       — same as server_name
    --   event.admin_recipients — array from MailRelay.AdminRecipients
    --   event.params         — empty table for system events

    -- Send to the configured admin recipients
    local recipients = event.admin_recipients

    -- You can customize recipients, template, and params here.
    -- For example, send to a different address on startup:
    -- local recipients = { "you@example.com", "ops-team@example.com" }

    return {
        template_key = "system.server_started",
        to = recipients,
        params = {
            SERVER_NAME = event.server_name,
            APP_NAME = event.app_name,
            TIMESTAMP = event.timestamp,
        },
        priority = 0,
    }
end
```

**Handler return table fields** (read by `mailrelay_event_dispatch_request` in [`mailrelay_events.c`](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_events.c)):

| Field | Type | Required | Description |
| ------- | ------ | ---------- | ------------- |
| `template_key` | string | Yes | Template key to render and send |
| `to` | array of strings | Yes | To recipient addresses |
| `cc` | array of strings | No | Cc recipient addresses |
| `bcc` | array of strings | No | Bcc recipient addresses |
| `from` | string | No | Override From (defaults to `MailRelay.DefaultFrom`) |
| `reply_to` | string | No | Override Reply-To (defaults to `MailRelay.DefaultReplyTo`) |
| `params` | table | No | Macro values for template rendering |
| `idempotency_key` | string | No | Idempotency key (auto UUID if omitted) |
| `priority` | integer | No | Queue priority, higher dequeues first (default 0) |
| `debounce_key` | string | No | Coalesces repeated events within `Queue.DebounceSeconds` |

Returning `nil` or a table without `template_key` suppresses the send. Template macros available in the handler's params: `%SERVER_NAME%`, `%APP_NAME%`, `%TIMESTAMP%`, `%COUNT%`, `%SUMMARY%`, `%REQUEST_ID%`, `%USER_EMAIL%`.

**Important:** Event handlers always use template mode — the C layer calls `mailrelay_send_template`. You cannot send freeform subject/body from an event handler. If you need to write the email body directly in Lua, use Option C.

**Storing the script in the database:**

The handler source lives in the `scripts` table. Migration [acuranzo_1281.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1281.lua) seeds a default `Mail.Events.ServerStarted` row. To customize it, either:

1. Update the `code` column of the existing `Mail.Events.ServerStarted` row in the `scripts` table, or
2. Create a new script row with a different `group_name`/`script_name` and update `Events.Rules` to point at it.

To insert a new script row, use [QueryRef #129](/elements/002-helium/acuranzo/migrations/acuranzo_1263.lua) (the "Insert Script" query) or run a SQL `INSERT` against the `scripts` table:

```sql
INSERT INTO scripts (group_name, script_name, script_type, status, code, summary)
VALUES
('Mail.Events', 'ServerStarted', 1, 1,
  'function handle_event(event)
     return {
         template_key = "system.server_started",
         to = event.admin_recipients,
         params = {
             SERVER_NAME = event.server_name,
             APP_NAME = event.app_name,
             TIMESTAMP = event.timestamp,
         },
     }
  end',
  'Custom startup notification handler');
```

### Option C: Orchestrator freeform email with a separate script (for full content control in Lua)

If you want to write the email subject and body directly in Lua (not via a template), use the Orchestrator. The Orchestrator is loaded at `READY FOR REQUESTS` and runs as a long-lived script. You add a one-shot section that loads a separate script from the `scripts` table, submits it as a job via the scoreboard, and lets the worker pool execute it. The script uses `H.mail.send_sync` in freeform mode, which accepts `subject`, `text_body`, `html_body`, `from`, `to`, `cc`, `bcc`, and `reply_to` directly.

**How the Orchestrator works with the scoreboard:**

The Orchestrator is the subsystem's long-running tier-2 context. It is not itself a scoreboard job — `H.set_current_state` and `H.set_result` are no-ops when called from it. Instead, the Orchestrator drives the scoreboard:

1. On each tick, it calls `H.scoreboard.list()` to see what jobs are in flight.
2. When it needs work done, it calls `H.scoreboard.submit({ script_name = "...", source = "..." })` to create a job entry. The worker pool picks up submitted jobs and executes them on dedicated pthreads.
3. The Orchestrator checks `H.shutdown_requested()` on every iteration and exits cleanly when true.

For a one-shot startup notification, the Orchestrator loads the script source from the `scripts` table (via [QueryRef #087](/elements/002-helium/acuranzo/migrations/acuranzo_1204.lua)), submits it as a job, and uses a flag to ensure it only fires once. The submitted script sends the email and then returns — it does not loop, so the worker pool reclaims its Lua state after completion.

**Configuration:**

```json
{
    "Scripting": {
        "Enabled": true,
        "DefaultDatabase": "Acuranzo",
        "Orchestrator": "Orchestrators.Orchestrator"
    },
    "MailRelay": {
        "Enabled": true,
        "OutboundEnabled": true,
        "AdminRecipients": ["you@example.com"],
        "Servers": [
            {
                "Host": "${env.SMTP_HOST}",
                "Port": "${env.SMTP_PORT}",
                "Username": "${env.SMTP_USER}",
                "Password": "${env.SMTP_PASS}",
                "UseTLS": true,
                "TLSMode": 1,
                "AuthMode": 1,
                "TimeoutSeconds": 30
            }
        ]
    }
}
```

**The separate startup notification script** (stored in the `scripts` table as `group_name='Orchestrators'`, `script_name='StartupNotify'`):

This script is a one-shot job. It composes an HTML email using a level-2 long string (`[=[ ... ]=]`) so that HTML attributes with double quotes don't conflict with the Lua string delimiter. It sets custom `from`, `to`, `cc`, `bcc`, and `reply_to` fields, then returns `0` to signal clean completion. Because the Orchestrator uses a flag to submit it only once, this script will never be launched again until the server restarts.

```lua
-- Orchestrators.StartupNotify
-- A one-shot job submitted by the Orchestrator at startup.
-- Sends a freeform HTML email and then returns (exits).
-- The Orchestrator's startup_notified flag ensures this is only
-- submitted once per process lifetime.

-- Compose the HTML body using a level-2 long string [=[ ]=]
-- so that HTML attributes with double quotes don't need escaping.
local html_template = [=[
<html>
  <body style="font-family: sans-serif; color: #333;">
    <h2 style="color: #2c3e50;">Hydrogen Server Started</h2>
    <table style="border-collapse: collapse; width: 100%;">
      <tr>
        <td style="padding: 4px 8px; border: 1px solid #ddd;"><strong>Instance</strong></td>
        <td style="padding: 4px 8px; border: 1px solid #ddd;">%s</td>
      </tr>
      <tr>
        <td style="padding: 4px 8px; border: 1px solid #ddd;"><strong>Version</strong></td>
        <td style="padding: 4px 8px; border: 1px solid #ddd;">%s</td>
      </tr>
      <tr>
        <td style="padding: 4px 8px; border: 1px solid #ddd;"><strong>Started at</strong></td>
        <td style="padding: 4px 8px; border: 1px solid #ddd;">%s</td>
      </tr>
      <tr>
        <td style="padding: 4px 8px; border: 1px solid #ddd;"><strong>Uptime</strong></td>
        <td style="padding: 4px 8px; border: 1px solid #ddd;">%.0f seconds</td>
      </tr>
      <tr>
        <td style="padding: 4px 8px; border: 1px solid #ddd;"><strong>Server</strong></td>
        <td style="padding: 4px 8px; border: 1px solid #ddd;">%s</td>
      </tr>
    </table>
    <p style="margin-top: 16px; color: #888; font-size: 12px;">
      This message was sent automatically by the Hydrogen Orchestrator.
    </p>
  </body>
</html>
]=]

local subject = string.format("[%s] Hydrogen started", H.system.instance_id())
local html_body = string.format(html_template,
    H.system.instance_id(),
    H.system.version(),
    H.system.now_iso(),
    H.system.uptime(),
    H.system.instance_id()
)

local res, err = H.mail.send_sync({
    from = "noreply@hydrogen.local",
    to = { "you@example.com", "ops-team@example.com" },
    cc = "ops@example.com",
    bcc = { "audit@example.com" },
    reply_to = "ops@example.com",
    subject = subject,
    html_body = html_body,
    text_body = string.format("Hydrogen %s started at %s (uptime: %.0fs)",
        H.system.version(), H.system.now_iso(), H.system.uptime()),
    priority = 5,
    idempotency_key = "startup-notify-" .. H.system.instance_id(),
})

if err then
    H.log.error("Startup notification failed: %s", err)
else
    H.log.info("Startup notification sent: %s", tostring(res.message_id))
end

-- Return 0 to signal clean completion. The worker pool reclaims
-- this Lua state after the function returns.
return 0
```

**The Orchestrator script** (stored as `Orchestrators.Orchestrator`):

This Orchestrator shows the full lifecycle: it reviews the scoreboard on every tick, submits the startup notification script exactly once (guarded by the `startup_notified` flag), and exits cleanly when `H.shutdown_requested()` returns true. The worker pool — not the Orchestrator itself — executes submitted jobs. When the Orchestrator calls `H.scoreboard.submit({ source = ... })`, the job enters the scoreboard's pending queue. Worker threads (configured via `Scripting.Workers`) pick up pending jobs, load the source into a fresh Lua state, and run it. The worker pool processes any job in the scoreboard, including the startup notification and any future jobs you submit.

```lua
-- Orchestrators.Orchestrator
-- Long-lived script loaded at READY FOR REQUESTS.
-- Drives the scoreboard: lists jobs, submits new ones, exits on shutdown.

H.log.info("Orchestrator: started")

local startup_notified = false
local TICK_MS = 1000
local iterations = 0

while not H.shutdown_requested() do
    iterations = iterations + 1

    -- Review the scoreboard: see what jobs are currently in flight.
    -- The worker pool is already draining pending jobs in parallel;
    -- this list call lets the Orchestrator make scheduling decisions.
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s) in scoreboard",
               iterations, #jobs)

    -- One-shot: submit the startup notification script exactly once.
    if not startup_notified then
        startup_notified = true

        -- Load the script source from the `scripts` table (QueryRef #087).
        local rows, qerr = H.query_sync([[
            SELECT code FROM scripts
             WHERE group_name = :group
               AND script_name = :name
        ]], { group = "Orchestrators", name = "StartupNotify" })

        if qerr or not rows or #rows == 0 then
            H.log.error("StartupNotify script not found: %s", tostring(qerr))
        else
            -- Submit the loaded source as a one-shot job.
            -- The worker pool picks it up and runs it; the script
            -- sends the email and returns 0, so it exits cleanly.
            -- Because startup_notified is now true, this block
            -- never executes again during this process lifetime.
            local job_id = H.scoreboard.submit({
                script_name = "Orchestrators.StartupNotify",
                source = rows[1].code,
            })
            if job_id then
                H.log.info("Orchestrator: submitted startup notify job %s", job_id)
            else
                H.log.warn("Orchestrator: submit startup notify returned nil")
            end
        end
    end

    -- The worker pool independently drains the scoreboard.
    -- Any job in the scoreboard (whether submitted by this Orchestrator
    -- or via the REST API) is picked up by a worker thread and executed.
    -- The Orchestrator does not need to manually run them.

    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
```

**Storing the scripts in the database:**

Both scripts live in the `scripts` table. Migration [acuranzo_1210.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1210.lua) seeds the `Orchestrators.Orchestrator` row. To add the `StartupNotify` script, insert a new row:

```sql
INSERT INTO scripts (group_name, script_name, script_type, status, code, summary)
VALUES
('Orchestrators', 'StartupNotify', 1, 1,
  'local html_template = [=[<html><body><h2>Started</h2></body></html>]=]
   local subject = string.format("[%s] Hydrogen started", H.system.instance_id())
   local html_body = string.format(html_template, H.system.instance_id())
   local res, err = H.mail.send_sync({
       from = "noreply@hydrogen.local",
       to = { "you@example.com" },
       subject = subject,
       html_body = html_body,
   })
   return 0',
  'One-shot startup notification with freeform HTML email');
```

See [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) for the full `H` host API reference, including `H.system.*`, `H.mail.send`, `H.sleep`, and `H.shutdown_requested`. See [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) for long-bracket string syntax (`[[ ... ]]` and `[=[ ... ]=]`).

### Complete production configuration

This is a full production-ready `hydrogen.json` that sends a startup email using Option B (custom event handler):

```json
{
    "Server": {
        "ServerName": "hydrogen-prod-01",
        "PayloadKey": "${env.PAYLOAD_KEY}",
        "LogFile": "/var/log/hydrogen.log"
    },
    "Databases": {
        "DefaultWorkers": 2,
        "Connections": {
            "Acuranzo": {
                "Enabled": true,
                "Type": "postgres",
                "Host": "${env.ACURANZO_DB_HOST}",
                "Port": "${env.ACURANZO_DB_PORT}",
                "Database": "${env.ACURANZO_DB_NAME}",
                "User": "${env.ACURANZO_DB_USER}",
                "Pass": "${env.ACURANZO_DB_PASS}",
                "Workers": 2
            }
        }
    },
    "WebServer": {
        "Enabled": true,
        "Port": 5000,
        "WebRoot": "/var/www/html"
    },
    "API": {
        "Enabled": true,
        "Prefix": "/api",
        "JWTSecret": "${env.JWT_SECRET}",
        "Headers": [
            ["/conduit/query", "Cache-Control", "max-age=60"]
        ]
    },
    "MailRelay": {
        "Enabled": true,
        "OutboundEnabled": true,
        "InboundEnabled": false,
        "Database": "Acuranzo",
        "DefaultFrom": "noreply@yourdomain.com",
        "DefaultReplyTo": "ops@yourdomain.com",
        "AdminRecipients": ["you@example.com"],
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
                "system.server_started": "Mail.Events.ServerStarted"
            }
        },
        "Servers": [
            {
                "Host": "${env.SMTP_HOST}",
                "Port": "${env.SMTP_PORT}",
                "Username": "${env.SMTP_USER}",
                "Password": "${env.SMTP_PASS}",
                "UseTLS": true,
                "TLSMode": 1,
                "CAPath": "",
                "AuthMode": 1,
                "TimeoutSeconds": 30
            }
        ]
    }
}
```

**Required environment variables:**

```bash
export SMTP_HOST="smtp.sendgrid.net"
export SMTP_PORT="587"
export SMTP_USER="apikey"
export SMTP_PASS="your-smtp-api-key"
export JWT_SECRET="your-jwt-signing-secret"
export PAYLOAD_KEY="your-payload-encryption-key"
export ACURANZO_DB_HOST="db.example.com"
export ACURANZO_DB_PORT="5432"
export ACURANZO_DB_NAME="hydrogen"
export ACURANZO_DB_USER="hydrogen"
export ACURANZO_DB_PASS="your-db-password"
```

### Customizing the email content

Use templates for structured, reusable content. Edit the `system.server_started` template in the `mail_templates` table to change the subject and body. The template uses `%MACRO%` syntax:

```text
Subject: [Hydrogen] Server started: %SERVER_NAME%

Body:
Hydrogen %APP_NAME% is now ready for requests.

Instance: %SERVER_NAME%
Started at: %TIMESTAMP%

This message was sent automatically.
```

Use the `|default` syntax for optional macros: `%COUNT|1%`, `%SUMMARY|1 event%`. Use `%%` for a literal percent sign.

For fully dynamic content written in Lua, use Option C (Orchestrator freeform with a separate script) with `H.mail.send_sync` and a `[=[ ... ]=]` long string for HTML bodies. See [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) for long-bracket string syntax (`[[ ... ]]` and `[=[ ... ]=]`).

### Verification

After starting Hydrogen with your configuration:

1. Watch the log for the `READY FOR REQUESTS` line, followed by `Event 'system.server_started' dispatched` (MailRelay subsystem).
2. Check the Mail Relay status endpoint:

   ```bash
   curl -H "Authorization: Bearer <your-jwt>" http://localhost:5000/api/mailrelay/status
   ```

3. Confirm the email arrives at your inbox. If it does not, check the logs for `MAIL_*` error codes (see [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md) section 16 for the full error code list).

---

## 2. Send a periodic server status report

This example shows a Lua script that runs inside the Orchestrator and sends an HTML status email every 6 hours. It gathers three metrics:

1. **REST API requests** — scraped from the local `/api/system/prometheus` endpoint via `H.http.get_sync` (the Prometheus text format includes `hydrogen_http_api_requests_total`).
2. **User logins (last 6h)** — counted from the `actions` table via `H.query_sync` (`action_type_a24 = 3` is a login action, `feature_a21 = 100` is the login feature).
3. **Unique web visitors (last 6h)** — counted as distinct `ip_address` values from the same login action rows.

The script is a long-running Orchestrator loop: it checks `H.shutdown_requested()` on every iteration, sleeps between checks, and fires the email when the 6-hour window elapses.

### How metrics are gathered

| Metric | Source | Method |
| -------- | -------- | -------- |
| REST API requests total | `/api/system/prometheus` | `H.http.get_sync` — parse `hydrogen_http_api_requests_total` from text |
| User logins (6h window) | `actions` table | `H.query_sync` — `COUNT(*)` where `action_type_a24 = 3` |
| Unique visitors (6h window) | `actions` table | `H.query_sync` — `COUNT(DISTINCT ip_address)` where `action_type_a24 = 3` |

The `/api/system/prometheus` endpoint does not require authentication. The Orchestrator can reach it at `http://localhost:<port>/api/system/prometheus` — use the `HYDROGEN_HTTP_PROBE_BASE` environment variable (or your WebServer port) to configure the base URL.

### The periodic status report script

Store this as `Orchestrators.StatusReport` in the `scripts` table and reference it from your Orchestrator:

```lua
-- Orchestrators.StatusReport
-- Runs inside the Orchestrator loop. Sends a status email at
-- scheduled UTC times with API request counts, login counts,
-- and unique visitor counts.
-- Uses H.http.get_sync for Prometheus metrics and H.query_sync
-- for database-backed login/visitor data.

-- Schedule: UTC times at which to send the report.
-- Each entry is a "HH:MM" string in UTC. The script wakes every
-- minute to check whether the current UTC time has passed the
-- next scheduled slot.
local SCHEDULE = { "00:00", "06:00", "12:00", "18:00" }

local function gather_api_requests(base_url)
    -- The Prometheus endpoint returns text/plain; parse the counter line.
    local res, err = H.http.get_sync(base_url .. "/api/system/prometheus",
        { Accept = "text/plain" }, { timeout = 10 })
    if err or not res or not res.body then
        H.log.warn("StatusReport: prometheus fetch failed: %s", tostring(err))
        return nil
    end
    -- Line looks like: hydrogen_http_api_requests_total 12345
    local count = res.body:match("hydrogen_http_api_requests_total%s+(%d+)")
    return tonumber(count)
end

local function gather_login_stats()
    -- action_type_a24 = 3  → login action
    -- feature_a21 = 100    → login feature
    -- created_at is a timestamp; filter to the last 6 hours
    -- (matching the SCHEDULE interval).
    local sql = [[
        SELECT
            COUNT(*)                              AS login_count,
            COUNT(DISTINCT ip_address)            AS unique_visitors
        FROM actions
        WHERE action_type_a24 = 3
          AND feature_a21 = 100
          AND ip_address IS NOT NULL
          AND created_at > (NOW() - INTERVAL '6 hours')
    ]]
    local rows, err = H.query_sync(sql, {}, { timeout = 15 })
    if err or not rows or #rows == 0 then
        H.log.warn("StatusReport: login query failed: %s", tostring(err))
        return nil, nil
    end
    return tonumber(rows[1].login_count) or 0,
           tonumber(rows[1].unique_visitors) or 0
end

local function send_status_email(api_requests, login_count, unique_visitors)
    -- Use [=[ ]=] so HTML double-quotes don't need escaping.
    local html = [=[
<html>
  <body style="font-family: sans-serif; color: #333;">
    <h2 style="color: #2c3e50;">Hydrogen 6-Hour Status Report</h2>
    <table style="border-collapse: collapse; width: 60%;">
      <tr style="background-color: #f0f0f0;">
        <td style="padding: 6px 12px; border: 1px solid #ddd;"><strong>Metric</strong></td>
        <td style="padding: 6px 12px; border: 1px solid #ddd;"><strong>Value</strong></td>
      </tr>
      <tr>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">Total API requests</td>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">%d</td>
      </tr>
      <tr>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">User logins (last 6h)</td>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">%d</td>
      </tr>
      <tr>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">Unique visitors (last 6h)</td>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">%d</td>
      </tr>
      <tr>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">Server uptime</td>
        <td style="padding: 6px 12px; border: 1px solid #ddd;">%.0f seconds</td>
      </tr>
    </table>
    <p style="margin-top: 16px; color: #888; font-size: 12px;">
      Generated at %s by the Hydrogen Orchestrator.
    </p>
  </body>
</html>
]=]

    local subject = string.format("Hydrogen status report — API: %d, Logins: %d, Visitors: %d",
        api_requests or 0, login_count or 0, unique_visitors or 0)

    local html_body = string.format(html,
        api_requests or 0,
        login_count or 0,
        unique_visitors or 0,
        H.system.uptime(),
        H.system.now_iso())

    local res, err = H.mail.send_sync({
        from = "noreply@hydrogen.local",
        to = { "ops@example.com" },
        cc = "team@example.com",
        reply_to = "ops@example.com",
        subject = subject,
        html_body = html_body,
        text_body = string.format(
            "Hydrogen 6-hour status report:\n"
          .. "  API requests: %d\n"
          .. "  User logins:  %d\n"
          .. "  Unique visitors: %d\n"
          .. "  Uptime: %.0f seconds\n",
            api_requests or 0, login_count or 0, unique_visitors or 0,
            H.system.uptime()),
        priority = 0,
        idempotency_key = "status-report-" .. H.system.instance_id() .. "-" .. os.time(),
    })

    if err then
        H.log.error("StatusReport: email failed: %s", err)
    else
        H.log.info("StatusReport: email sent: %s", tostring(res.message_id))
    end
end

-- Calculate the epoch timestamp of the next scheduled send.
local function next_scheduled_epoch(now_epoch)
    local utc = os.date("!*t", now_epoch)
    local seconds_since_midnight = utc.hour * 3600 + utc.min * 60 + utc.sec

    for _, time_str in ipairs(SCHEDULE) do
        local hh, mm = time_str:match("(%d%d):(%d%d)")
        local offset = (tonumber(hh) * 3600 + tonumber(mm) * 60) - seconds_since_midnight
        if offset > 0 then
            return now_epoch + offset
        end
    end

    -- No more scheduled times today; use the first slot tomorrow.
    local hh, mm = SCHEDULE[1]:match("(%d%d):(%d%d)")
    local first_offset = tonumber(hh) * 3600 + tonumber(mm) * 60
    return now_epoch + (24 * 3600) - seconds_since_midnight + first_offset
end

-- Gather metrics and send the status email.
local function gather_and_send()
    local base_url = os.getenv("HYDROGEN_HTTP_PROBE_BASE") or "http://localhost:5000"
    base_url = base_url:gsub("/+$", "")

    local api_requests = gather_api_requests(base_url)
    local login_count, unique_visitors = gather_login_stats()

    send_status_email(api_requests, login_count, unique_visitors)
end

-- Main loop: wake every minute, check if it's time to send.
local TICK_MS = 60000  -- 1 minute between checks
local next_send = 0    -- 0 means "send immediately on first run"

while not H.shutdown_requested() do
    local now = H.system.now()

    if now >= next_send then
        H.log.info("StatusReport: sending scheduled report")
        gather_and_send()
        next_send = next_scheduled_epoch(now)
        H.log.info("StatusReport: next send at epoch %d", next_send)
    end

    H.gc.collect()
    H.sleep(TICK_MS)
end

H.log.info("StatusReport: shutdown requested, exiting")
```

### Orchestrator integration

Add the status report to your Orchestrator's main loop. The Orchestrator loads the script source from the `scripts` table and submits it as a long-running job. The worker pool picks up the job and runs it; the StatusReport script has its own 6-hour loop with `H.sleep` and `H.shutdown_requested()` for clean shutdown.

```lua
-- Orchestrators.Orchestrator (excerpt — periodic status report section)
--
-- This shows how the Orchestrator submits a long-running periodic
-- script as a scoreboard job. The StatusReport job runs its own
-- 6-hour loop inside the worker pool.

local status_report_submitted = false

while not H.shutdown_requested() do
    -- ... existing orchestrator tick logic ...

    if not status_report_submitted then
        status_report_submitted = true

        local rows, qerr = H.query_sync([[
            SELECT code FROM scripts
             WHERE group_name = :group
               AND script_name = :name
        ]], { group = "Orchestrators", name = "StatusReport" })

        if qerr or not rows or #rows == 0 then
            H.log.error("StatusReport script not found: %s", tostring(qerr))
        else
            local job_id = H.scoreboard.submit({
                script_name = "Orchestrators.StatusReport",
                source = rows[1].code,
            })
            if job_id then
                H.log.info("Orchestrator: submitted status report job %s", job_id)
            end
        end
    end

    H.sleep(TICK_MS)
end
```

### Verification (6-hour report)

1. Start Hydrogen with the Orchestrator configured and the `Orchestrators.StatusReport` script seeded in the `scripts` table.
2. The script sends immediately on the first tick (since `next_send` starts at 0), then on schedule at 00:00, 06:00, 12:00, and 18:00 UTC.
3. In the logs, look for `StatusReport: sending scheduled report` and `StatusReport: email sent`, then confirm the email arrives.
4. To test the schedule without waiting 6 hours, temporarily set `SCHEDULE = { "00:00" }` or add a near-future time (e.g. `SCHEDULE = { "18:00" }` if it's currently 17:xx UTC) to trigger an immediate send on the next tick.

### Doc cross-references

| Topic | Document |
| ------- | ---------- |
| Full Mail Relay pipeline (templates, rewrites, debounce, events, Lua `H.mail`, OTP, security) | [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md) |
| Lua host API reference (`H.mail`, `H.system`, `H.query`, `H.http`, etc.) | [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) |
| Lua language intro and scripting patterns | [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md) |
| Pure Lua language features (strings, tables, dates, patterns) | [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) |
| Scripting subsystem config and lifecycle | [scripting README](/docs/H/core/subsystems/scripting/README.md) |
| Mail Relay implementation plan and working log | [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) |
| Mail Relay subsystem overview | [mailrelay README](/docs/H/core/subsystems/mailrelay/README.md) |
| Reference Orchestrator source | [orchestrator.lua](/elements/001-hydrogen/hydrogen/src/scripting/orchestrator.lua) |
| System event handler source (built-in + seeded) | [mailrelay_events.c](/elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_events.c) |
| System event template seeds (migration 1280) | [acuranzo_1280.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1280.lua) |
| Seeded event handler scripts (migration 1281) | [acuranzo_1281.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1281.lua) |
| Seeded Orchestrator script (migration 1210) | [acuranzo_1210.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1210.lua) |
| QueryRef #087: fetch script code from `scripts` table | [acuranzo_1204.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1204.lua) |
| `actions` table schema (login tracking) | [acuranzo_1006.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1006.lua) |
| Prometheus metrics formatter | [status_formatters.c](/elements/001-hydrogen/hydrogen/src/status/status_formatters.c) |
| HTTP request counter (api_requests) | [web_server_request.c](/elements/001-hydrogen/hydrogen/src/webserver/web_server_request.c) |
| READY FOR REQUESTS emission point | [launch.c](/elements/001-hydrogen/hydrogen/src/launch/launch.c) |
