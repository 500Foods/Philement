# Lua Guide for Hydrogen

This guide is for human developers — and for people who know Python or another scripting language and want to write scripts that run inside Hydrogen. It starts with plain Lua, then covers the `H` host API Hydrogen installs into every script, then points at real Lua already in the tree, and finishes with worked examples (queries, jobs, mail stubs, Helium migrations, and report-style scripts).

If you only need the full API contract, use the reference:

- [Host API reference](/docs/H/core/subsystems/scripting/lua_api.md)
- [Scripting subsystem overview](/docs/H/core/subsystems/scripting/README.md)
- [Implementation plan](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md)

For pure Lua language features (strings, tables, numbers, dates, patterns) with no Hydrogen API, see [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md).

Helium migrations have a separate, migration-focused intro: [docs/He/LUA_INTRO.md](/docs/He/LUA_INTRO.md). This guide covers both in-server scripting and the multiline-string habits migrations need.

Hydrogen embeds **Lua 5.4**. Scripts run sandboxed: `io`, `os.execute`, `debug`, and native `package.loadlib` are off by default. Prefer the `H.*` API over raw OS libraries.

---

## Table of Contents

1. [Why Lua here?](#why-lua-here)
2. [Lua in 15 minutes (for Python people)](#lua-in-15-minutes-for-python-people)
3. [Strings and multiline strings](#strings-and-multiline-strings)
4. [Tables, nil, and common pitfalls](#tables-nil-and-common-pitfalls)
5. [How scripts run in Hydrogen](#how-scripts-run-in-hydrogen)
6. [The `H` API subset you will use most](#the-h-api-subset-you-will-use-most)
7. [Existing Lua in this project](#existing-lua-in-this-project)
8. [Worked examples](#worked-examples)
9. [Helium migrations (multiline SQL)](#helium-migrations-multiline-sql)
10. [Style, lint, and safety](#style-lint-and-safety)
11. [Where to go next](#where-to-go-next)

---

## Why Lua here?

| Need | Why Lua fits |
|------|----------------|
| Embedded in a C server | Tiny runtime, no separate process |
| SQL, HTML, JSON, email bodies | Long bracket strings avoid quote hell |
| Jobs that query DBs and call HTTP/LLM | One `H` table mirrors Conduit REST |
| Future report writer | Same language and same host API |
| Helium migrations | One Lua file → multi-engine SQL |

Compared with Python: no virtualenv, no separate interpreter process, arrays start at **1**, and the only complex type is the **table**.

---

## Lua in 15 minutes (for Python people)

### Comments and locals

```lua
-- single-line comment

--[[
  block comment
]]

local name = "report_daily"   -- always prefer local
local count = 0
local active = true
local missing = nil           -- like None
```

Globals work (`x = 1`) but pollute every context. Use `local` almost always.

### Numbers, strings, booleans

```lua
local n = 42
local pi = 3.14159
local s = "hello" .. " " .. "world"   -- concatenation is ..
local ok = true
local no = false
```

There is no integer/float distinction at the language level for most code you will write. Hydrogen's query binding still types values when they cross into SQL (see [Parameters](#parameters-named-and-typed)).

### Functions

```lua
local function greet(who)
    return "hello, " .. who
end

local result = greet("Hydrogen")

-- multiple return values (very common with H.wait)
local a, b = 1, 2

-- anonymous function
local f = function(x) return x * 2 end
```

### Conditionals and loops

```lua
if count == 0 then
    H.log.info("empty")
elseif count == 1 then
    H.log.info("one")
else
    H.log.info("many: %d", count)
end

for i = 1, 5 do
    -- i is 1,2,3,4,5
end

local i = 1
while i <= 5 do
    i = i + 1
end

-- iterate array part of a table (ordered)
for index, value in ipairs(rows) do
    -- ...
end

-- iterate all keys (order not guaranteed)
for key, value in pairs(obj) do
    -- ...
end
```

### `and` / `or` / truthiness

Only `false` and `nil` are falsey. `0` and `""` are **truthy**.

```lua
local x = maybe or "default"     -- common defaulting pattern
local y = ok and "yes" or "no"   -- ternary-ish (careful if "yes" is false)
```

### Errors

```lua
error("something went wrong")

local ok, err_or_result = pcall(function()
    error("boom")
end)
if not ok then
    H.log.error("caught: %s", tostring(err_or_result))
end
```

For host I/O (`H.query`, `H.http`, …) prefer the `result, err` pattern from `H.wait` over throwing.

---

## Strings and multiline strings

This is the single most important Lua feature for Hydrogen and Helium work.

### Quoted strings

```lua
local a = "double quotes"
local b = 'single quotes'
local c = "line1\nline2"     -- escapes work in short strings
local d = "say \"hi\""
```

### Long bracket strings: `[[ ... ]]`

```lua
local sql = [[
SELECT id, name
  FROM users
 WHERE status = 'active'
]]
```

No escape processing inside `[[...]]`. Quotes and backslashes are literal. Leading newline after `[[` is kept as written (fine for SQL).

### Nested / leveled brackets: `[=[ ... ]=]`

If the text itself contains `]]`, use a higher level:

```lua
local body = [=[
This SQL embeds a string that ends with ]]
and would break a plain [[...]] delimiter.
]=]
```

You can keep adding equals signs: `[==[ ... ]==]`, `[===[ ... ]===]`, …

**Migration habit:** use `[=[ ... ]=]` for SQL so accidental `]]` inside comments or generated fragments does not break the file.

```lua
-- Helium-style SQL blob
local sql = [=[
CREATE TABLE ${SCHEMA}${TABLE} (
    id   ${INTEGER} NOT NULL,
    name ${TEXT}    NOT NULL,
    ${COMMON_CREATE}
    ${PRIMARY}(id)
);
]=]
```

### When to use which

| Form | Use for |
|------|---------|
| `"..."` / `'...'` | Short identifiers, log formats, keys |
| `[[...]]` | SQL, JSON, HTML, email text without `]]` |
| `[=[...]=]` | SQL/HTML that may contain `]]` (migrations, templates) |
| `[==[...]==]` | Nested content that already uses `[=[...]=]` |

### Building strings

```lua
-- small
local msg = "user=" .. user_id

-- many pieces: prefer table.concat
local parts = {}
table.insert(parts, "SELECT ")
table.insert(parts, columns)
table.insert(parts, " FROM ")
table.insert(parts, table_name)
local sql = table.concat(parts)

-- format (Lua 5.4 string.format)
local line = string.format("job %s finished in %d ms", job_id, ms)
```

`H.log.info("job %s done", job_id)` already formats for you (see [Logging](#logging)).

---

## Tables, nil, and common pitfalls

### Tables are arrays and maps

```lua
-- array-like (1-based!)
local colors = { "red", "green", "blue" }
print(colors[1])   -- "red"   (NOT index 0)

-- map-like
local user = {
    id = 123,
    name = "Ada",
    active = true,
}

-- mixed
local row = {
    10, 20, 30,          -- array part
    label = "sample",    -- hash part
}

table.insert(colors, "yellow")
user.email = "ada@example.com"
```

### Length operator `#`

`#t` is the length of the **array part** up to the first hole. Prefer `ipairs` for row lists from queries.

```lua
H.log.info("got %d rows", #res.rows)
for _, row in ipairs(res.rows) do
    H.log.info("name=%s", tostring(row.name))
end
```

### Nil removes keys

```lua
user.email = nil   -- key gone
```

### Python → Lua cheat sheet

| Python | Lua |
|--------|-----|
| `None` | `nil` |
| `True` / `False` | `true` / `false` |
| `list[0]` | `list[1]` |
| `dict["k"]` | `t.k` or `t["k"]` |
| `len(x)` | `#x` (arrays) |
| `+` for strings | `..` |
| `f"{x}"` | `string.format("%s", x)` |
| `def f():` | `local function f()` |
| `except` | `pcall` / `H.wait` errors |
| modules | `require` (when allowed) |

---

## How scripts run in Hydrogen

### Two tiers

| Tier | Lifetime | Role |
|------|----------|------|
| **Orchestrator** | One long-lived `lua_State` per process | Scheduling loop; checks `H.shutdown_requested()` |
| **Workers** | Fresh `lua_State` per job | Run one script, then destroy the state |

Workers never reuse a state across jobs (same safety model as database migrations). The Orchestrator compiles once at launch.

### Enablement

Scripting is **off by default**. In config (section `Q` / `Scripting`):

```json
{
  "Scripting": {
    "Enabled": true,
    "DefaultDatabase": "Acuranzo",
    "Orchestrator": "Orchestrators.Orchestrator",
    "WorkerCount": 4,
    "DefaultQueryTimeout": 30,
    "AllowDBModuleLoad": true,
    "Sandbox": {
      "AllowOsTime": true,
      "AllowOsExecute": false,
      "AllowIo": false,
      "AllowDebug": false,
      "AllowLoadlib": false
    }
  }
}
```

See [scripting README](/docs/H/core/subsystems/scripting/README.md) for every option.

### Sandbox (what you cannot do by default)

- No `io.*` (files)
- No `os.execute` / `os.exit` / `os.remove`
- No `debug.*`
- No native `package.loadlib`
- Prefer `H.system.now()` over raw `os.time` when writing portable scripts

Use `H.http`, `H.query`, `H.log`, and `H.mail` instead of shelling out.

### Async-first handles

Blocking work returns a **handle** immediately. You join with `H.wait`:

```lua
local h = H.query("SELECT 1 AS n", {})
local res, err = H.wait(h)
```

Sync wrappers exist (`H.query_sync`, …) but the primitive is always handle + wait. Fan-out is natural:

```lua
local h1 = H.query(sql1, p1)
local h2 = H.query(sql2, p2)
local h3 = H.http.get("https://example.com/health")
local r1, r2, r3, e1, e2, e3 = H.wait(h1, h2, h3)
```

`H.wait` with `N` handles returns `2N` values: `r1..rN, e1..eN`. `err == nil` means success.

---

## The `H` API subset you will use most

Full list: [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md). Below is the subset most query/mail/report scripts need.

### Logging

```lua
H.log.trace(fmt, ...)
H.log.debug(fmt, ...)
H.log.info(fmt, ...)
H.log.warn(fmt, ...)
H.log.error(fmt, ...)
H.log.fatal(fmt, ...)
```

Uses Lua `string.format` semantics. Lines go through Hydrogen's logger (including VictoriaLogs) under the `Scripting` subsystem. Inside a worker job, messages are prefixed with `[job:<id>]`.

Format/arg mismatches are logged by the host; they do **not** raise into your script.

### System

```lua
H.system.uptime()       -- seconds since server start
H.system.now()          -- epoch seconds
H.system.now_iso()      -- ISO 8601 UTC string
H.system.instance_id()  -- stable UUID for this process
H.system.version()      -- Hydrogen version string
```

### Cooperative loops (Orchestrator)

```lua
while not H.shutdown_requested() do
    -- do work
    H.sleep(1000)   -- ms; returns early on shutdown
end
```

Never spin a tight loop without `H.sleep`.

### Progress and results (worker jobs)

```lua
H.set_current_state("Loading customers")
H.set_result("json", "workflow:ABCDE")
```

No-ops if called from the Orchestrator (it is not a scoreboard job).

### Database queries

```lua
-- default DB (Scripting.DefaultDatabase, or the only configured DB)
H.query(sql, params, opts?)          -> handle
H.query_sync(sql, params, opts?)     -> result, err

-- explicit DB name
H.altquery(db_name, sql, params, opts?)
H.altquery_sync(...)

-- JWT-scoped DB (same as REST auth_query)
H.authquery(token, sql, params, opts?)
H.authquery_sync(...)
```

`opts` may include `timeout` (seconds).

#### Parameters (named and typed)

SQL uses **named** binds (`:id`), not `?`. Pass a Lua table:

```lua
local h = H.query([[
    SELECT id, name FROM users
     WHERE id = :id AND active = :active
]], {
    id = 123,
    active = true,
    note = "hello",
})
```

Hydrogen maps values into typed parameter JSON (`INTEGER`, `FLOAT`, `STRING`, `BOOLEAN`, …). Nested tables become JSON objects. There is no positional binding.

#### Result table

```lua
{
    rows = { { id = 1, name = "Ada" }, ... },
    affected_rows = 0,           -- INSERT/UPDATE/DELETE
    column_names = { "id", "name" },
    elapsed_ms = 12,
}
```

#### Atomic claim (multi-instance safe)

```lua
local res, err = H.wait(H.query([[
    UPDATE tasks
       SET status = 'taken',
           claimed_by = :instance
     WHERE id = :id AND status = 'open'
]], {
    id = task_id,
    instance = H.system.instance_id(),
}))
if err then return false end
return res.affected_rows == 1   -- we own it
```

No transactions required for this pattern. See Phase 14 notes in the plan.

#### Service token for internal auth queries

```lua
local token = H.system_token("Acuranzo", 30)
local res, err = H.authquery_sync(token,
    "SELECT * FROM events LIMIT :n", { n = 10 })
```

### HTTP

```lua
local h = H.http.get(url, headers?, opts?)
local h = H.http.post(url, body?, headers?, opts?)
local res, err = H.wait(h)
-- res = { status, headers, body, elapsed_ms }
```

TLS verification is always on. Body cap for scripting HTTP is 16 MiB. Multiple GETs can be waited together for parallel fan-out.

### LLM

```lua
local h = H.llm.call({
    model = "grok-light",
    prompt = "Summarize:\n" .. text,
    max_tokens = 500,
    temperature = 0.2,
})
local res, err = H.wait(h)

local list, err2 = H.llm.list_sync()
-- list.models = { { name=..., model=..., provider=..., is_default=... }, ... }
```

You pass a **model name only**; keys and endpoints stay on the server.

### Mail and notify

`H.mail` queues through Mail Relay. Use **exactly one** mode:

- **Template** — `template` / `template_key` + optional `params`
- **Freeform** — `subject` + `text_body` / `html_body` / `body` (literal)

```lua
-- Template mode
local h = H.mail.send({
    template = "mail.test",
    to = "alice@example.com",
    params = { NAME = "Ada" },
})
local res, err = H.wait(h)
-- success: res = { message_id = "...", status = "queued" }

-- Freeform mode (body authored in the script)
local h2 = H.mail.send({
    to = "alice@example.com",
    subject = "Daily report",
    body = body_text,   -- alias for text_body
})
```

`H.notify` returns a stable deferred error (`"notify: deferred to mailrelay rules"`).
Prefer `H.mail` or system events. Full contract: [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md)
and [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md).

### Scoreboard (jobs)

```lua
local jobs = H.scoreboard.list()
local job  = H.scoreboard.get(job_id)
local id   = H.scoreboard.submit({
    script_name = "reports.daily",
    -- implementation may accept source or DB-backed name depending on phase
})
H.scoreboard.cancel(job_id)
```

Exact submit shapes evolve with DB-backed scripts; see [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md).

### GC helpers (long Orchestrator loops)

```lua
H.gc.collect()
H.gc.count()      -- KB
```

Workers rarely need this; `lua_close` cleans up at job end.

---

## Existing Lua in this project

Study these before inventing new patterns.

### Reference Orchestrator

[orchestrator.lua](/elements/001-hydrogen/hydrogen/src/scripting/orchestrator.lua) — long-lived loop: log start, tick, optional scoreboard submit, `H.sleep`, exit on `H.shutdown_requested()`.

```lua
local TICK_MS = 1000
H.log.info("Orchestrator: started")
local iterations = 0
while not H.shutdown_requested() do
    iterations = iterations + 1
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end
H.log.info("Orchestrator: shutdown after %d iteration(s)", iterations)
```

### Helium migrations

Thousands of lines of production Lua under:

- [elements/002-helium/acuranzo/migrations/](/elements/002-helium/acuranzo/migrations/)

Example bootstrap: [acuranzo_1000.lua](/elements/002-helium/acuranzo/migrations/acuranzo_1000.lua)

Pattern:

```lua
return function(engine, design_name, schema_name, cfg)
    local queries = {}
    cfg.TABLE = "queries"
    cfg.MIGRATION = "1000"
    table.insert(queries, { sql = [[
        CREATE TABLE ${SCHEMA}${QUERIES} (
            ...
        );
    ]] })
    return queries
end
```

Migration author guide: [docs/He/GUIDE.md](/docs/He/GUIDE.md). Anatomy: [docs/He/MIGRATION_ANATOMY.md](/docs/He/MIGRATION_ANATOMY.md).

### Test / tooling Lua

- [get_migration.lua](/elements/001-hydrogen/hydrogen/tests/lib/get_migration.lua) — CLI-style migration helper used by tests

### C host installation (for engineers)

The `H` table is installed in C under [src/scripting/](/elements/001-hydrogen/hydrogen/src/scripting/) (`lua_context.c`, `scripting_api_*.c`). You do not call those from Lua; they define what `H` can do.

---

## Worked examples

### 1. Hello, logging and system info

```lua
H.log.info("script starting on %s version %s",
    H.system.instance_id(), H.system.version())
H.log.info("uptime=%.0f now=%s", H.system.uptime(), H.system.now_iso())
return 0
```

### 2. Simple SELECT with error handling

```lua
local res, err = H.query_sync([[
    SELECT id, name, email
      FROM accounts
     WHERE status_a16 = :status
     LIMIT :lim
]], {
    status = 1,
    lim = 50,
})

if err then
    H.log.error("query failed: %s", err)
    return 1
end

H.log.info("rows=%d elapsed=%s ms", #res.rows, tostring(res.elapsed_ms))
for _, row in ipairs(res.rows) do
    H.log.info("account %s <%s>", tostring(row.name), tostring(row.email))
end
return 0
```

### 3. Fan-out: two queries + one HTTP check

```lua
local h_orders = H.query("SELECT count(*) AS c FROM orders", {})
local h_users  = H.query("SELECT count(*) AS c FROM accounts", {})
local h_health = H.http.get("https://example.com/health")

local o, u, health, e1, e2, e3 = H.wait(h_orders, h_users, h_health)
if e1 or e2 or e3 then
    H.log.error("fan-out failed: %s / %s / %s",
        tostring(e1), tostring(e2), tostring(e3))
    return 1
end

H.log.info("orders=%s users=%s http=%s",
    tostring(o.rows[1].c),
    tostring(u.rows[1].c),
    tostring(health.status))
return 0
```

### 4. Cross-database query

```lua
local res, err = H.altquery_sync("reporting", [[
    SELECT day, events
      FROM daily_stats
     WHERE day = :d
]], { d = "2026-07-01" })

if err then
    H.log.error("altquery: %s", err)
    return 1
end
return 0
```

### 5. Claim work, process, mark done

```lua
local function claim(task_id)
    local res, err = H.wait(H.query([[
        UPDATE tasks
           SET status = 'taken',
               claimed_by = :who,
               claimed_at = :now
         WHERE id = :id AND status = 'open'
    ]], {
        id = task_id,
        who = H.system.instance_id(),
        now = H.system.now(),
    }))
    if err then
        H.log.error("claim error: %s", err)
        return false
    end
    return res.affected_rows == 1
end

local function finish(task_id, ok)
    local status = ok and "done" or "failed"
    H.wait(H.query([[
        UPDATE tasks SET status = :st WHERE id = :id AND claimed_by = :who
    ]], {
        st = status,
        id = task_id,
        who = H.system.instance_id(),
    }))
end

-- worker body
H.set_current_state("claiming")
if not claim(task_id) then
    H.log.info("task %s already taken", tostring(task_id))
    return 0
end

H.set_current_state("working")
local ok = true
-- ... do work ...
finish(task_id, ok)
H.set_result("status", ok and "done" or "failed")
return ok and 0 or 1
```

### 6. Draft a report body with multiline strings

```lua
local res, err = H.query_sync([[
    SELECT name, total
      FROM report_lines
     WHERE report_day = :day
     ORDER BY name
]], { day = "2026-07-01" })

if err then
    H.log.error("%s", err)
    return 1
end

local lines = {}
table.insert(lines, "Daily report for 2026-07-01")
table.insert(lines, string.rep("-", 40))
for _, row in ipairs(res.rows) do
    table.insert(lines, string.format("%-20s %10s",
        tostring(row.name), tostring(row.total)))
end
local body = table.concat(lines, "\n")

local _, mail_err = H.mail.send_sync({
    to = "ops@example.com",
    subject = "Daily report 2026-07-01",
    body = body,
})
if mail_err then
    H.log.warn("mail failed: %s", mail_err)
    H.set_result("text", body)   -- still useful as job artifact metadata
end
return 0
```

### 7. LLM-assisted summary (optional)

```lua
local text = [[
(paste long operational notes here)
]]

local res, err = H.llm.call_sync({
    model = "grok-light",
    prompt = "Summarize in 5 bullets:\n" .. text,
    max_tokens = 400,
})
if err then
    H.log.error("llm: %s", err)
    return 1
end
H.log.info("summary: %s", tostring(res.response or res))
return 0
```

### 8. Mini Orchestrator poller

```lua
local INTERVAL_MS = 5000

H.log.info("poller up")
while not H.shutdown_requested() do
    local res, err = H.wait(H.query([[
        SELECT id FROM tasks WHERE status = 'open' LIMIT 20
    ]], {}))

    if err then
        H.log.error("poll failed: %s", err)
    else
        for _, row in ipairs(res.rows) do
            if claim(row.id) then
                H.scoreboard.submit({
                    script_name = "workers.process_task",
                    params_json = string.format('{"task_id":%s}', tostring(row.id)),
                })
            end
        end
    end

    H.gc.collect()
    H.sleep(INTERVAL_MS)
end
H.log.info("poller exit")
```

(`claim` as in example 5.)

### 9. HTML email body with nested brackets

```lua
local customer = "Ada"
local total = "42.00"

local html = [=[
<!DOCTYPE html>
<html>
<body>
  <h1>Invoice</h1>
  <p>Hello [=[customer]=] — oops, don't do that</p>
  <p>Hello ]=] .. customer .. [=[</p>
  <p>Total: $]=] .. total .. [=[</p>
</body>
</html>
]=]
```

Cleaner approach — build with concatenation or `string.format`:

```lua
local html = string.format([=[
<!DOCTYPE html>
<html><body>
  <h1>Invoice</h1>
  <p>Hello %s</p>
  <p>Total: $%s</p>
</body></html>
]=], customer, total)
```

### 10. Auth query with mint token

```lua
local token = H.system_token("Acuranzo", 60)
local res, err = H.authquery_sync(token, [[
    SELECT query_ref, name
      FROM queries
     WHERE query_status_a27 = 1
     LIMIT :n
]], { n = 5 })

if err then
    H.log.error("authquery: %s", err)
    return 1
end
for _, row in ipairs(res.rows) do
    H.log.info("Q#%s %s", tostring(row.query_ref), tostring(row.name))
end
return 0
```

---

## Helium migrations (multiline SQL)

Migrations are **not** Orchestrator/worker scripts. They are pure Lua functions that return SQL tables for the migration engine. They do not receive `H`. They still use the same language and the same multiline-string rules.

### Skeleton

```lua
-- Migration: acuranzo_NNNN.lua
-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-09 - Short description

return function(engine, design_name, schema_name, cfg)
    local queries = {}

    cfg.TABLE = "example_items"
    cfg.MIGRATION = "NNNN"

    table.insert(queries, { sql = [=[
        CREATE TABLE ${SCHEMA}${TABLE} (
            item_id   ${INTEGER} NOT NULL,
            name      ${TEXT}    NOT NULL,
            ${COMMON_CREATE}
            ${PRIMARY}(item_id)
        );
    ]=] })

    return queries
end
```

### Why `[=[ ]=]` in migrations

Migrations often embed SQL that contains string literals, JSON, or even documentation with `]]`. Leveled long strings prevent delimiter collisions. Macros like `${INTEGER}` and `${COMMON_CREATE}` are expanded per engine by Helium's `database_*.lua` modules.

### Engine branches

```lua
if engine == "postgresql" then
    table.insert(queries, { sql = [[ /* pg-only */ ]] })
elseif engine == "sqlite" then
    -- ...
end
```

Prefer macros over engine branches when a macro already exists.

### Learn more

- [Lua intro for migrations](/docs/He/LUA_INTRO.md)
- [Migration creation guide](/docs/He/GUIDE.md)
- [Why Lua / curiosities](/docs/H/CURIOSITIES.md)

Run `tests/test_98_luacheck.sh` after editing `.lua` files.

---

## Style, lint, and safety

### Style

1. **Always `local`** unless you intentionally export a module table.
2. **Snake_case** for locals and functions (`claim_task`, `report_day`).
3. **Multiline SQL** in `[[...]]` or `[=[...]=]`, not concatenated fragments of quoted strings.
4. **Named SQL parameters** (`:id`) with a params table — never string-concatenate user input into SQL.
5. **Check `err`** from every `H.wait` / `*_sync` call.
6. **Orchestrator loops** must call `H.shutdown_requested()` and `H.sleep`.
7. **One purpose per script** — pollers, workers, and report bodies stay separate.

### Lint

- [Test 98 — luacheck](/docs/H/tests/test_98_luacheck.md)
- Migration headers usually include `-- luacheck:` directives; follow neighboring files.

### Safety reminders

| Do | Don't |
|----|-------|
| `H.query` + named params | Build SQL with `.. user_input` |
| `H.http` / `H.mail` | `os.execute("curl ...")` |
| `H.log.*` | `print` for production diagnostics |
| `H.sleep` in loops | Busy-wait |
| Use `H.mail` (template or freeform) | Invent a second mail path / open SMTP |

### Config checklist before first run

1. `Scripting.Enabled = true`
2. `DefaultDatabase` set (or only one DB configured)
3. Orchestrator row present if you want the default scheduler (`Orchestrators.Orchestrator`)
4. Restart Hydrogen; watch logs for `Orchestrator: started` / scripting launch lines

---

## Where to go next

| Document | When |
|----------|------|
| [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md) | Pure Lua language features and stdlib recipes |
| [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md) | Full `H.*` contract |
| [scripting README](/docs/H/core/subsystems/scripting/README.md) | Config, ops, lifecycle |
| [LUA_PLAN_COMPLETE.md](/docs/H/plans/complete/LUA_PLAN_COMPLETE.md) | Design history and phases |
| [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md) | Mail Relay, rewrites, events, and Lua `H.mail` |
| [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) | Mail Relay implementation plan |
| [docs/He/LUA_INTRO.md](/docs/He/LUA_INTRO.md) | Migration-only Lua basics |
| [docs/He/GUIDE.md](/docs/He/GUIDE.md) | Writing Helium migrations |
| [Test 43 scripting](/docs/H/tests/test_43_scripting.md) | End-to-end scripting tests |
| [Lua in 15 minutes](https://tylerneylon.com/a/learn-lua/) | External quick language tour |
| [Lua 5.4 manual](https://www.lua.org/manual/5.4/) | Language reference |

### Mental model (one paragraph)

Write ordinary Lua with `local` variables and tables. Put SQL and templates in long bracket strings. Talk to Hydrogen only through `H`: log with `H.log`, read/write data with `H.query` / `H.altquery` / `H.authquery`, join async work with `H.wait`, and keep long loops polite with `H.sleep` and `H.shutdown_requested()`. For Helium migrations, return a list of SQL strings the same way existing `acuranzo_*.lua` files do — no `H` table, same multiline discipline. When the report writer lands, it will reuse this exact environment.

---

*Cap: this guide is intentionally under 1000 lines. Deep API details live in [lua_api.md](/docs/H/core/subsystems/scripting/lua_api.md).*
