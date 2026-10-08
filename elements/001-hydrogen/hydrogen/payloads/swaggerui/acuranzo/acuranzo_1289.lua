-- Migration: acuranzo_1289.lua
-- Phase 30: Provision.EnsureCanvasUser Lua + Orchestrator poll driver
--
-- Keeps Hydrogen Canvas-agnostic: outbound REST via H.http + env token;
-- durable link via raw SQL matching QueryRefs #144/#145/#146.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.3 - 2026-08-09 - Orchestrator: ${SCHEMA}scripts (not lithium), per-tick log, shutdown wording (see also 1299)
-- 1.0.2 - 2026-08-08 - DB2: VALUES seed (not SELECT/WHERE NOT EXISTS) avoids SQL1585N
-- 1.0.1 - 2026-08-08 - DB2: ${DUMMY_TABLE} before WHERE NOT EXISTS on script seed
-- 1.0.0 - 2026-08-08 - Initial EnsureCanvasUser + orchestrator submit every 60s

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1289"
cfg.GROUP_NAME = "Provision"
cfg.SCRIPT_NAME = "EnsureCanvasUser"
-- ----------------------------------------------------------------------------
-- Forward: seed Provision.EnsureCanvasUser + patch Orchestrator driver
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Provision.EnsureCanvasUser (Phase 30)
-- Poll accounts with an OIDC identity but no account_canvas_links row.
-- For each: Canvas email search -> create if missing -> #145 link / #146 touch.
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL").
-- No JSON library: minimal pattern parse of Canvas JSON bodies.

local function getenv(k, default)
    if type(os) == "table" and type(os.getenv) == "function" then
        local v = os.getenv(k)
        if v and v ~= "" then return v end
    end
    return default
end

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)
local ACCOUNT_PATH = "/api/v1/accounts/1"

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = 30 })
    if err then return nil, err end
    return res, nil
end

local function http_post_json(url, body)
    local headers = auth_headers()
    headers["Content-Type"] = "application/json"
    local res, err = H.http.post_sync(url, body, headers, {
        timeout = 30,
        content_type = "application/json",
    })
    if err then return nil, err end
    return res, nil
end

local function url_encode(s)
    s = tostring(s or "")
    return (s:gsub("([^%w%-_%.~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

-- Collect top-level JSON objects from an array body (best-effort).
local function each_object(body)
    local objs = {}
    if type(body) ~= "string" or body == "" then return objs end
    for obj in body:gmatch("%b{}") do
        objs[#objs + 1] = obj
    end
    return objs
end

local function object_id(obj)
    return tonumber(obj:match('"id"%s*:%s*(%d+)'))
end

local function object_field(obj, key)
    return obj:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function emails_equal(a, b)
    if not a or not b then return false end
    return string.lower(a) == string.lower(b)
end

-- Returns: canvas_user_id | nil, err_or_reason
local function canvas_find_by_email(email)
    local url = BASE .. ACCOUNT_PATH .. "/users?search_term=" .. url_encode(email)
        .. "&per_page=50"
    local res, err = http_get(url)
    if not res then
        return nil, "canvas_search_failed:" .. tostring(err)
    end
    if res.status < 200 or res.status >= 300 then
        return nil, "canvas_search_http_" .. tostring(res.status)
    end
    local matches = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            local sortable = object_field(obj, "sortable_name") or ""
            if emails_equal(login_id, email) or emails_equal(em, email) then
                matches[#matches + 1] = id
            elseif #matches == 0 and email ~= "" and (
                string.find(string.lower(login_id), string.lower(email), 1, true)
                or string.find(string.lower(em), string.lower(email), 1, true)
                or string.find(string.lower(sortable), string.lower(email), 1, true)
            ) then
                -- keep fuzzy candidates only if no exact match later
                matches[#matches + 1] = id
            end
        end
    end
    -- Prefer exact-only: re-scan for exact if we collected fuzzy noise
    local exact = {}
    for _, obj in ipairs(each_object(res.body or "")) do
        local id = object_id(obj)
        if id then
            local login_id = object_field(obj, "login_id") or ""
            local em = object_field(obj, "email") or ""
            if emails_equal(login_id, email) or emails_equal(em, email) then
                exact[#exact + 1] = id
            end
        end
    end
    if #exact == 1 then return exact[1], nil end
    if #exact > 1 then return nil, "email_ambiguous" end
    if #matches == 1 then return matches[1], nil end
    if #matches > 1 then return nil, "email_ambiguous" end
    return nil, "not_found"
end

local function canvas_create_user(email, name)
    local display = name
    if not display or display == "" then
        display = email
    end
    -- Canvas Admin API create user (account 1).
    local body = string.format(
        '{"user":{"name":%q,"skip_registration":true},"pseudonym":{"unique_id":%q,"send_confirmation":false},"communication_channel":{"type":"email","address":%q,"skip_confirmation":true}}',
        display, email, email
    )
    local url = BASE .. ACCOUNT_PATH .. "/users"
    local res, err = http_post_json(url, body)
    if not res then
        return nil, "canvas_create_failed:" .. tostring(err)
    end
    if res.status < 200 or res.status >= 300 then
        H.log.warn("EnsureCanvasUser: create HTTP %s body=%s",
            tostring(res.status), string.sub(tostring(res.body or ""), 1, 300))
        return nil, "canvas_create_http_" .. tostring(res.status)
    end
    local id = object_id(res.body or "")
    if not id then
        return nil, "canvas_create_no_id"
    end
    return id, nil
end

local function link_lookup(account_id)
    local _qr, err = H.query_sync([[
        SELECT link_id, account_id, canvas_user_id, canvas_email, last_seen_at
        FROM lithium.account_canvas_links
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if err then return nil, err end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1], nil end
    return nil, nil
end

local function link_insert(account_id, canvas_user_id, canvas_email)
    local _qr, err = H.query_sync([[
        INSERT INTO lithium.account_canvas_links (
            link_id, account_id, canvas_user_id, canvas_email, last_seen_at,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        )
        SELECT
            (SELECT COALESCE(MAX(link_id), 0) + 1 FROM lithium.account_canvas_links),
            a.account_id,
            :CANVASUSERID,
            :CANVASEMAIL,
            NOW(),
            '2025-01-01 00:00:00',
            '2035-01-01 00:00:00',
            0, NOW(), 0, NOW()
        FROM lithium.accounts a
        WHERE a.account_id = :ACCOUNTID
          AND NOT EXISTS (
              SELECT 1 FROM lithium.account_canvas_links acl
              WHERE acl.account_id = :ACCOUNTID
          )
        RETURNING link_id
    ]], {
        ACCOUNTID = account_id,
        CANVASUSERID = canvas_user_id,
        CANVASEMAIL = canvas_email,
    })
    if err then return nil, err end
    local rows = qrows(_qr)
    if rows and rows[1] then return rows[1].link_id or rows[1].LINK_ID, nil end
    return nil, nil
end

local function link_touch(link_id, canvas_email)
    local _, err = H.query_sync([[
        UPDATE lithium.account_canvas_links
        SET last_seen_at = NOW(),
            canvas_email = :CANVASEMAIL,
            updated_at = NOW()
        WHERE link_id = :LINKID
    ]], { LINKID = link_id, CANVASEMAIL = canvas_email })
    if err then
        H.log.warn("EnsureCanvasUser: touch failed link_id=%s err=%s",
            tostring(link_id), tostring(err))
    end
end

local function ensure_one(account_id, email, display_name)
    local existing, lerr = link_lookup(account_id)
    if lerr then
        H.log.warn("EnsureCanvasUser: lookup err account_id=%s: %s",
            tostring(account_id), tostring(lerr))
        return false
    end
    if existing then
        local lid = existing.link_id or existing.LINK_ID
        link_touch(lid, email)
        H.log.info("EnsureCanvasUser: already linked account_id=%s canvas_user_id=%s",
            tostring(account_id),
            tostring(existing.canvas_user_id or existing.CANVAS_USER_ID))
        return true
    end

    local canvas_id, ferr = canvas_find_by_email(email)
    if ferr == "email_ambiguous" then
        H.log.error("EnsureCanvasUser: email_ambiguous account_id=%s email=%s",
            tostring(account_id), tostring(email))
        return false
    end
    if ferr and ferr ~= "not_found" then
        H.log.warn("EnsureCanvasUser: search err account_id=%s: %s",
            tostring(account_id), tostring(ferr))
        return false
    end
    if not canvas_id then
        canvas_id, ferr = canvas_create_user(email, display_name)
        if not canvas_id then
            H.log.warn("EnsureCanvasUser: create failed account_id=%s: %s",
                tostring(account_id), tostring(ferr))
            return false
        end
        H.log.info("EnsureCanvasUser: created canvas_user_id=%s for account_id=%s",
            tostring(canvas_id), tostring(account_id))
    else
        H.log.info("EnsureCanvasUser: matched canvas_user_id=%s for account_id=%s",
            tostring(canvas_id), tostring(account_id))
    end

    local link_id, ierr = link_insert(account_id, canvas_id, email)
    if ierr then
        H.log.warn("EnsureCanvasUser: link insert err account_id=%s: %s",
            tostring(account_id), tostring(ierr))
        return false
    end
    if not link_id then
        -- race: another worker linked; re-check
        existing = select(1, link_lookup(account_id))
        if existing then
            link_touch(existing.link_id or existing.LINK_ID, email)
            return true
        end
        H.log.warn("EnsureCanvasUser: link insert returned 0 rows account_id=%s",
            tostring(account_id))
        return false
    end
    H.log.info("EnsureCanvasUser: linked account_id=%s -> canvas_user_id=%s link_id=%s",
        tostring(account_id), tostring(canvas_id), tostring(link_id))
    return true
end

-- ---- main ----
H.set_current_state("start")
if not TOKEN or TOKEN == "" then
    H.log.error("EnsureCanvasUser: CANVAS_API_KEY not set in environment")
    return
end

local _qr, err = H.query_sync([[
    SELECT a.account_id,
           oi.email,
           a.name,
           a.first_name,
           a.last_name
    FROM lithium.account_oidc_identities oi
    JOIN lithium.accounts a ON a.account_id = oi.account_id
    WHERE oi.email IS NOT NULL
      AND oi.email <> ''
      AND NOT EXISTS (
          SELECT 1 FROM lithium.account_canvas_links acl
          WHERE acl.account_id = a.account_id
      )
    ORDER BY a.account_id
    LIMIT 50
]])
if err then
    H.log.error("EnsureCanvasUser: candidate query failed: %s", tostring(err))
    return
end
local rows = qrows(_qr)
if not rows or #rows == 0 then
    H.log.info("EnsureCanvasUser: no unlinked OIDC accounts")
    return
end

H.log.info("EnsureCanvasUser: %d candidate(s)", #rows)
local ok_n, fail_n = 0, 0
for _, row in ipairs(rows) do
    local account_id = row.account_id or row.ACCOUNT_ID
    local email = row.email or row.EMAIL
    local name = row.name or row.NAME
    local fn = row.first_name or row.FIRST_NAME or ""
    local ln = row.last_name or row.LAST_NAME or ""
    local display = name
    if (not display or display == "") and (fn ~= "" or ln ~= "") then
        display = (fn .. " " .. ln):gsub("^%s+", ""):gsub("%s+$", "")
    end
    if ensure_one(account_id, email, display) then
        ok_n = ok_n + 1
    else
        fail_n = fail_n + 1
    end
end
H.log.info("EnsureCanvasUser: done ok=%d fail=%d", ok_n, fail_n)
H.set_current_state("done")
                ]==],
                'Phase 30: ensure Canvas user + account_canvas_links for OIDC accounts',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            -- Patch Orchestrator: every ~60s submit EnsureCanvasUser (source from DB).
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Orchestrators.Orchestrator (500 Courses — Phase 30 driver)
-- Periodic Provision.EnsureCanvasUser submit; per-iteration tick for lifecycle probes.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local TICK_MS = 1000
local ENSURE_EVERY = 60
local ensure_countdown = 0

H.log.info("Orchestrator: started (500courses provision driver)")

local function job_active(jobs, name)
    for _, j in ipairs(jobs or {}) do
        local sn = j.script_name or j.SCRIPT_NAME or ""
        local st = string.lower(tostring(j.status or j.STATUS or ""))
        if sn == name and (st == "pending" or st == "running") then
            return true
        end
    end
    return false
end

local function submit_ensure()
    if job_active(H.scoreboard.list(), "Provision.EnsureCanvasUser") then
        H.log.info("Orchestrator: EnsureCanvasUser already active; skip")
        return
    end
    local _qr, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Provision' AND script_name = 'EnsureCanvasUser'
        LIMIT 1
    ]])
    local rows = qrows(_qr)
    if err or not rows or not rows[1] then
        H.log.warn("Orchestrator: EnsureCanvasUser source missing: %s",
            tostring(err or "no row"))
        return
    end
    local src = rows[1].code or rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: EnsureCanvasUser empty code")
        return
    end
    local id = H.scoreboard.submit({
        script_name = "Provision.EnsureCanvasUser",
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted EnsureCanvasUser job %s", tostring(id))
    else
        H.log.warn("Orchestrator: EnsureCanvasUser submit failed")
    end
end

local iterations = 0
-- Run once shortly after boot so first-login lag is bounded.
ensure_countdown = 5

while not H.shutdown_requested() do
    iterations = iterations + 1
    ensure_countdown = ensure_countdown - 1
    if ensure_countdown <= 0 then
        ensure_countdown = ENSURE_EVERY
        local ok, err = pcall(submit_ensure)
        if not ok then
            H.log.warn("Orchestrator: ensure pcall err: %s", tostring(err))
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==],
                updated_at = ${NOW},
                summary = '500 Courses Orchestrator: EnsureCanvasUser every 60s (schema-safe scripts)'
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Provision.EnsureCanvasUser + Orchestrator driver'             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Phase 30 Canvas ensure via Lua

            1. Inserts `Provision.EnsureCanvasUser` worker script (H.http +
               account_canvas_links SQL).
            2. Replaces `Orchestrators.Orchestrator` body to submit that job
               every ~60s (source loaded from scripts table).

            Requires Scripting.Enabled, CANVAS_API_KEY env, QueryRefs/tables
            from 1283-1288. Orchestrator process must be restarted (or
            hydrogen rolled) to pick up the new Orchestrator source.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = 'Provision'
              AND script_name = 'EnsureCanvasUser';

            ${SUBQUERY_DELIMITER}

            -- Note: Orchestrator body is left as-is on reverse (would need
            -- the prior source snapshot). Operators can re-seed 1210 if needed.

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Provision.EnsureCanvasUser'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Provision.EnsureCanvasUser
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
