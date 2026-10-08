-- Migration: acuranzo_1308.lua
-- Phase 60: seed Enroll.SyncProgress (invokable) for Canvas progress cache
--
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Maps Canvas CourseProgress → user_enrollments.progress_percent /
-- completed_at / status=completed. Stale-while-revalidate: My Courses
-- reads Helium first (Phase 61); this script refreshes the cache.
-- No Hydrogen C. Data-only seed (no diagram).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Enroll.SyncProgress invokable=1 (FINISHLINE FL-60)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1308"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "SyncProgress"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.SyncProgress (invokable)
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
                invokable,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Enroll.SyncProgress (Phase 60)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Refresh Helium progress cache from Canvas CourseProgress.
-- Secrets: os.getenv("CANVAS_API_KEY"), os.getenv("CANVAS_BASE_URL").
-- Rate-limit is in this script (not C). LMS outage never throws.

local STALE_SECONDS = 300
local MAX_CALLS_CLIENT = 8
local MAX_CALLS_BATCH = 20
local HTTP_TIMEOUT = 10

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

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function truthy(v)
    return v == true or v == 1 or v == "1" or v == "true"
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function days_from_civil(y, m, d)
    y = y - (m <= 2 and 1 or 0)
    local era = math.floor(y / 400)
    local yoe = y - era * 400
    local doy = math.floor((153 * (m + (m > 2 and -3 or 9)) + 2) / 5) + d - 1
    local doe = yoe * 365 + math.floor(yoe / 4) - math.floor(yoe / 100) + doy
    return era * 146097 + doe - 719468
end

local function parse_epoch(s)
    local y, mo, d, h, mi, se = tostring(s or ""):match(
        "^(%d%d%d%d)%-(%d%d)%-(%d%d)[T ](%d%d):(%d%d):(%d%d)"
    )
    if not y then return nil end
    local days = days_from_civil(tonumber(y), tonumber(mo), tonumber(d))
    return days * 86400
        + tonumber(h) * 3600
        + tonumber(mi) * 60
        + tonumber(se)
end

local function age_seconds(synced_at, now_ts)
    local a = parse_epoch(synced_at)
    local b = parse_epoch(now_ts)
    if not a or not b then return nil end
    return b - a
end

local function map_progress(req_count, req_done, completed_at)
    req_count = tonumber(req_count) or 0
    req_done = tonumber(req_done) or 0
    local pct = 0
    if req_count > 0 then
        pct = math.floor((req_done * 100) / req_count)
        if pct > 100 then pct = 100 end
        if pct < 0 then pct = 0 end
    end
    local done = false
    local completed = tostring(completed_at or "")
    if completed ~= "" and completed ~= "null" then
        done = true
        pct = 100
    elseif req_count > 0 and req_done >= req_count then
        done = true
        pct = 100
    end
    return pct, done
end

local function object_num(obj, key)
    return tonumber(obj:match('"' .. key .. '"%s*:%s*(%d+)'))
end

local function object_field(obj, key)
    return obj:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function parse_course_progress(body)
    local obj = tostring(body or "")
    if obj == "" then return nil, "empty_body" end
    local req = object_num(obj, "requirement_count")
    local done = object_num(obj, "requirement_completed_count")
    if req == nil and done == nil then
        return nil, "unparsed"
    end
    local completed_at = object_field(obj, "completed_at")
    return {
        requirement_count = req or 0,
        requirement_completed_count = done or 0,
        completed_at = completed_at,
    }, nil
end

local SELF_TEST = {
    { id = 1, requirement_count = 10, requirement_completed_count = 0, completed_at = nil },
    { id = 2, requirement_count = 10, requirement_completed_count = 3, completed_at = nil },
    { id = 3, requirement_count = 10, requirement_completed_count = 10,
      completed_at = "2026-08-13T12:00:00Z" },
    { id = 4, requirement_count = 0, requirement_completed_count = 0, completed_at = nil },
    { id = 5, http_error = "canvas_unavailable" },
}

local function run_self_test()
    local synced = {}
    local skipped = {}
    local errors = {}
    for _, fx in ipairs(SELF_TEST) do
        if fx.http_error then
            errors[#errors + 1] = {
                enrollment_id = fx.id,
                code = fx.http_error,
            }
        else
            local pct, done = map_progress(
                fx.requirement_count,
                fx.requirement_completed_count,
                fx.completed_at
            )
            synced[#synced + 1] = {
                enrollment_id = fx.id,
                progress_percent = pct,
                completed = done,
            }
        end
    end
    H.set_result_json({
        ok = true,
        self_test = true,
        stale_seconds = STALE_SECONDS,
        synced = synced,
        skipped = skipped,
        errors = errors,
        synced_count = #synced,
        error_count = #errors,
    })
    return 0
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

if truthy(params.self_test) then
    return run_self_test()
end

local h = params._hydrogen
local account_id = nil
local batch = false
if type(h) == "table" then
    account_id = tonumber(h.user_id) or tonumber(h.sub)
    if not account_id then
        return fail("missing_identity", "Sign in required")
    end
else
    batch = true
end

local force = truthy(params.force)
local dry_run = truthy(params.dry_run)
local only_id = tonumber(params.enrollment_id) or 0
local max_calls = batch and MAX_CALLS_BATCH or MAX_CALLS_CLIENT

H.set_current_state("start")

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)

local function auth_headers()
    return {
        Authorization = "Bearer " .. TOKEN,
        Accept = "application/json",
    }
end

local function http_get(url)
    local res, err = H.http.get_sync(url, auth_headers(), { timeout = HTTP_TIMEOUT })
    if err then return nil, err end
    return res, nil
end

local sql = [[
    SELECT
        ue.enrollment_id,
        ue.account_id,
        ue.course_id,
        ue.canvas_course_id,
        ue.canvas_enrollment_id,
        ue.status,
        ue.progress_percent,
        ue.progress_synced_at,
        ue.completed_at,
        c.canvas_course_id AS catalog_canvas_course_id,
        acl.canvas_user_id,
        CURRENT_TIMESTAMP AS now_ts
    FROM ${SCHEMA}user_enrollments ue
    LEFT JOIN ${SCHEMA}courses c
      ON c.course_id = ue.course_id
    LEFT JOIN ${SCHEMA}account_canvas_links acl
      ON acl.account_id = ue.account_id
    WHERE ue.status IN ('active', 'completed')
]]
local bind = {}
if not batch then
    sql = sql .. " AND ue.account_id = :ACCOUNTID"
    bind.ACCOUNTID = account_id
end
if only_id > 0 then
    sql = sql .. " AND ue.enrollment_id = :ENROLLMENTID"
    bind.ENROLLMENTID = only_id
end

local _qr, qerr = H.query_sync(sql, bind)
if qerr then
    H.log.warn("SyncProgress: list err: %s", tostring(qerr))
    return fail("query_error", "Could not load enrollments")
end

local rows = qrows(_qr) or {}
local synced = {}
local skipped = {}
local errors = {}
local calls = 0
local lms_down = false

local function skip(id, code)
    skipped[#skipped + 1] = { enrollment_id = id, code = code }
end

local function err_row(id, code)
    errors[#errors + 1] = { enrollment_id = id, code = code }
end

if (not TOKEN or TOKEN == "") and #rows > 0 then
    H.log.error("SyncProgress: CANVAS_API_KEY not set in environment")
    H.set_result_json({
        ok = false,
        code = "canvas_unconfigured",
        message = "Progress sync is temporarily unavailable",
        synced = {},
        skipped = {},
        errors = {},
        synced_count = 0,
        error_count = 0,
    })
    return 0
end

for _, row in ipairs(rows) do
    local eid = tonumber(pick(row, "enrollment_id", "ENROLLMENT_ID"))
    local status = string.lower(tostring(pick(row, "status", "STATUS") or ""))
    local cached = tonumber(pick(row, "progress_percent", "PROGRESS_PERCENT")) or 0
    local already_done = status == "completed"
        or (tostring(pick(row, "completed_at", "COMPLETED_AT") or "") ~= "")
    local synced_at = pick(row, "progress_synced_at", "PROGRESS_SYNCED_AT")
    local now_ts = pick(row, "now_ts", "NOW_TS")
    local canvas_course = tonumber(pick(row, "canvas_course_id", "CANVAS_COURSE_ID"))
        or tonumber(pick(row, "catalog_canvas_course_id", "CATALOG_CANVAS_COURSE_ID"))
    local canvas_user = tonumber(pick(row, "canvas_user_id", "CANVAS_USER_ID"))

    local fetch = false
    if not eid then
        skip(0, "bad_row")
    elseif not canvas_course then
        skip(eid, "no_canvas_course")
    elseif not canvas_user then
        skip(eid, "unlinked")
    elseif (not force) and synced_at and now_ts then
        local age = age_seconds(synced_at, now_ts)
        if age and age >= 0 and age < STALE_SECONDS then
            skip(eid, "fresh")
        else
            fetch = true
        end
    else
        fetch = true
    end

    if fetch then
        if calls >= max_calls then
            skip(eid, "rate_limited")
        else
            calls = calls + 1
            local url = BASE .. "/api/v1/courses/" .. tostring(canvas_course)
                .. "/users/" .. tostring(canvas_user) .. "/progress"
            local res = http_get(url)
            if not res then
                lms_down = true
                err_row(eid, "canvas_unavailable")
            elseif res.status == 404 then
                skip(eid, "not_enrolled")
            elseif res.status == 401 or res.status == 403 then
                err_row(eid, "canvas_forbidden")
            elseif res.status < 200 or res.status >= 300 then
                if res.status >= 500 then lms_down = true end
                err_row(eid, "canvas_http_" .. tostring(res.status))
            else
                local prog, perr = parse_course_progress(res.body or "")
                if not prog then
                    err_row(eid, perr or "unparsed")
                else
                    local pct, done = map_progress(
                        prog.requirement_count,
                        prog.requirement_completed_count,
                        prog.completed_at
                    )
                    if already_done then
                        done = true
                        if pct < 100 then pct = 100 end
                    end
                    if not dry_run then
                        local completed_flag = done and 1 or 0
                        local _, uerr = H.query_sync([[
                            UPDATE ${SCHEMA}user_enrollments
                               SET progress_percent = :PCT,
                                   progress_synced_at = CURRENT_TIMESTAMP,
                                   completed_at = CASE
                                       WHEN :COMPLETED = 1
                                       THEN COALESCE(completed_at, CURRENT_TIMESTAMP)
                                       ELSE completed_at
                                   END,
                                   status = CASE
                                       WHEN :COMPLETED = 1 AND status = 'active'
                                       THEN 'completed'
                                       ELSE status
                                   END,
                                   updated_at = CURRENT_TIMESTAMP
                             WHERE enrollment_id = :EID
                               AND status IN ('active', 'completed')
                        ]], {
                            PCT = pct,
                            COMPLETED = completed_flag,
                            EID = eid,
                        })
                        if uerr then
                            H.log.warn("SyncProgress: update %s err: %s",
                                tostring(eid), tostring(uerr))
                            err_row(eid, "update_failed")
                        else
                            synced[#synced + 1] = {
                                enrollment_id = eid,
                                progress_percent = pct,
                                completed = done,
                                previous_percent = cached,
                            }
                        end
                    else
                        synced[#synced + 1] = {
                            enrollment_id = eid,
                            progress_percent = pct,
                            completed = done,
                            previous_percent = cached,
                            dry_run = true,
                        }
                    end
                end
            end
        end
    end
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    dry_run = dry_run,
    batch = batch,
    lms_down = lms_down,
    stale_seconds = STALE_SECONDS,
    calls = calls,
    synced = synced,
    skipped = skipped,
    errors = errors,
    synced_count = #synced,
    skipped_count = #skipped,
    error_count = #errors,
})
return 0
                ]==],
                'Phase 60: invokable Canvas progress sync (H.http CourseProgress)',
                1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Enroll.SyncProgress invokable script'                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.SyncProgress

            Inserts `Enroll.SyncProgress` with `invokable = 1` for Reception
            `POST /api/conduit/script` (FINISHLINE Phase 60).

            The worker:

            1. Client JWT: `params._hydrogen` (`user_id` / `sub`) — own
               current rows (`status IN active, completed`) only.
               Orchestrator (no `_hydrogen`) may batch stale rows.
            2. Skips rows fresher than 300s (`fresh`) unless `force=true`.
            3. Caps outbound LMS GETs (8 client / 20 batch). Extra rows
               skip `rate_limited`.
            4. `GET {CANVAS_BASE_URL}/api/v1/courses/:id/users/:id/progress`
               via `H.http.get_sync`. Maps `requirement_completed_count` /
               `requirement_count` → `progress_percent` (0–100). Sets
               `completed_at` + `status=completed` at 100% or when Canvas
               sends `completed_at`. Never un-completes.
            5. LMS outage / 5xx → `errors[].code=canvas_unavailable`,
               last known cache kept. HTTP 200 + `result.ok=true` with
               `lms_down=true`. Missing token → `ok=false`
               `canvas_unconfigured` (no throw).
            6. `self_test=true` classifies built-in fixtures (no HTTP, no
               UPDATE). `dry_run=true` classifies live rows without write.

            Secrets stay in env (`CANVAS_*`). No diagram (data seed).
            Does not patch Orchestrator — My Courses (Phase 61/62) invokes
            on demand. Table writers (FreeCourse / intro) are still open.
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
            WHERE group_name = 'Enroll'
              AND script_name = 'SyncProgress';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.SyncProgress script'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.SyncProgress
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
