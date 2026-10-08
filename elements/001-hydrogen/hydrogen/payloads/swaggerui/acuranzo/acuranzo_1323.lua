-- Migration: acuranzo_1323.lua
-- PRIORITIZE 2.8: seed Enroll.SyncEnrollments (invokable) Canvas→Helium backfill
--
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Diffs Canvas StudentEnrollment vs user_enrollments for the caller.
-- Inserts only when no row exists for (account_id, course_id) — archived
-- and superseded rows are sticky. Maps via courses.canvas_course_id.
-- Unmapped Canvas courses are skipped. No diagram (data seed).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-20 - Seed Enroll.SyncEnrollments invokable=1 (PRIORITIZE 2.8)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1323"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "SyncEnrollments"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.SyncEnrollments (invokable)
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
-- Enroll.SyncEnrollments (PRIORITIZE 2.8)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Backfill user_enrollments from Canvas seats. Archive is sticky:
-- insert only when no row exists for the pair. Secrets: CANVAS_*.

local HTTP_TIMEOUT = 15
local MAX_PAGES = 5

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

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function next_enrollment_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}user_enrollments
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function any_entitlement(account_id, course_id)
    local _er, qerr = H.query_sync([[
        SELECT enrollment_id
        FROM ${SCHEMA}user_enrollments
        WHERE account_id = :ACCOUNTID
          AND course_id = :COURSEID
        ORDER BY enrollment_id DESC
    ]], { ACCOUNTID = account_id, COURSEID = course_id })
    if qerr then return nil, qerr end
    local rows = qrows(_er)
    if rows and rows[1] then
        return tonumber(pick(rows[1], "enrollment_id", "ENROLLMENT_ID")), nil
    end
    return nil, nil
end

local function grant_entitlement(account_id, course_id, canvas_course_id, source, renew_policy, lifetime)
    if not account_id or not course_id then
        return "missing_ids"
    end
    local existing, eerr = any_entitlement(account_id, course_id)
    if eerr then return "entitlement_lookup_failed" end
    if existing then return "exists" end
    local eid = next_enrollment_id()
    local expires_sql = lifetime and "NULL" or "NOW() + INTERVAL '90 days'"
    local sql = [[
        INSERT INTO ${SCHEMA}user_enrollments (
            enrollment_id, account_id, course_id,
            canvas_enrollment_id, canvas_course_id,
            status, enrolled_at, expires_at,
            completed_at, progress_percent, progress_synced_at,
            archived_at, renew_policy, source, order_id,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :EID, :ACCOUNTID, :COURSEID,
            NULL, :CANVASCOURSE,
            'active', NOW(), ]] .. expires_sql .. [[,
            NULL, 0, NULL,
            NULL, :RENEWPOLICY, :SOURCE, NULL,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]]
    local canvas_bind = canvas_course_id
    if canvas_bind == nil then canvas_bind = 0 end
    local _, ierr = H.query_sync(sql, {
        EID = eid,
        ACCOUNTID = account_id,
        COURSEID = course_id,
        CANVASCOURSE = canvas_bind,
        RENEWPOLICY = renew_policy,
        SOURCE = source,
    })
    if ierr then
        local again = select(1, any_entitlement(account_id, course_id))
        if again then return "exists" end
        return "entitlement_error"
    end
    return nil
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

H.set_current_state("start")

local BASE = (getenv("CANVAS_BASE_URL", "https://canvas.500courses.com")):gsub("/+$", "")
local TOKEN = getenv("CANVAS_API_KEY", nil)
local INTRO_COURSE_ID = tonumber(getenv("CANVAS_INTRO_COURSE_ID", "1001")) or 1001

if not TOKEN or TOKEN == "" then
    H.log.error("SyncEnrollments: CANVAS_API_KEY not set in environment")
    return fail("canvas_unconfigured", "Enrollment sync is temporarily unavailable")
end

local _lr, lerr = H.query_sync([[
    SELECT canvas_user_id
    FROM ${SCHEMA}account_canvas_links
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if lerr then
    H.log.warn("SyncEnrollments: link lookup err: %s", tostring(lerr))
    return fail("link_lookup_failed", "Could not resolve Canvas account")
end
local lrows = qrows(_lr)
if not lrows or not lrows[1] then
    H.set_current_state("done")
    H.set_result_json({
        ok = true,
        inserted = 0,
        skipped = 0,
        scanned = 0,
        unlinked = true,
    })
    return 0
end
local canvas_user_id = tonumber(pick(lrows[1], "canvas_user_id", "CANVAS_USER_ID"))
if not canvas_user_id then
    H.set_current_state("done")
    H.set_result_json({
        ok = true,
        inserted = 0,
        skipped = 0,
        scanned = 0,
        unlinked = true,
    })
    return 0
end

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

local function collect_course_ids(body, into, seen)
    for cid in tostring(body or ""):gmatch('"course_id"%s*:%s*(%d+)') do
        local n = tonumber(cid)
        if n and not seen[n] then
            seen[n] = true
            into[#into + 1] = n
        end
    end
end

local canvas_ids = {}
local seen = {}
local page = 1
while page <= MAX_PAGES do
    local url = BASE .. "/api/v1/users/" .. tostring(canvas_user_id)
        .. "/enrollments?type[]=StudentEnrollment&state[]=active&state[]=completed"
        .. "&per_page=100&page=" .. tostring(page)
    local res, herr = http_get(url)
    if not res then
        H.log.warn("SyncEnrollments: canvas list err: %s", tostring(herr))
        return fail("canvas_unavailable", "Could not list Canvas enrollments")
    end
    if res.status == 401 or res.status == 403 then
        return fail("canvas_forbidden", "Could not list Canvas enrollments")
    end
    if res.status < 200 or res.status >= 300 then
        return fail("canvas_http_" .. tostring(res.status), "Could not list Canvas enrollments")
    end
    local before = #canvas_ids
    collect_course_ids(res.body or "", canvas_ids, seen)
    local gained = #canvas_ids - before
    if gained < 100 then
        break
    end
    page = page + 1
end

local inserted = 0
local skipped = 0
local errors = 0

for _, canvas_course_id in ipairs(canvas_ids) do
    local _cr, cerr = H.query_sync([[
        SELECT course_id, pricing_type
        FROM ${SCHEMA}courses
        WHERE canvas_course_id = :CID
    ]], { CID = canvas_course_id })
    if cerr then
        H.log.warn("SyncEnrollments: catalog err course=%s: %s",
            tostring(canvas_course_id), tostring(cerr))
        errors = errors + 1
    else
        local crows = qrows(_cr)
        if not crows or not crows[1] then
            skipped = skipped + 1
        else
            local helium_id = tonumber(pick(crows[1], "course_id", "COURSE_ID"))
            local pricing = string.lower(tostring(
                pick(crows[1], "pricing_type", "PRICING_TYPE") or ""))
            local source = "free"
            local renew_policy = "free_renew"
            local lifetime = false
            if canvas_course_id == INTRO_COURSE_ID then
                source = "intro"
                renew_policy = "free_renew"
                lifetime = true
            elseif pricing == "paid" then
                source = "paid"
                renew_policy = "paid_renew"
                lifetime = false
            elseif pricing == "free" then
                source = "free"
                renew_policy = "free_renew"
                lifetime = false
            else
                source = "free"
                renew_policy = "free_renew"
                lifetime = false
            end
            local gerr = grant_entitlement(
                account_id, helium_id, canvas_course_id,
                source, renew_policy, lifetime)
            if gerr == "exists" then
                skipped = skipped + 1
            elseif gerr then
                H.log.warn("SyncEnrollments: grant err course=%s: %s",
                    tostring(helium_id), tostring(gerr))
                errors = errors + 1
            else
                inserted = inserted + 1
            end
        end
    end
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    inserted = inserted,
    skipped = skipped,
    errors = errors,
    scanned = #canvas_ids,
    unlinked = false,
})
return 0
                ]==],
                'PRIORITIZE 2.8: invokable Canvas→Helium enrollment reconcile',
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
        'Seed Enroll.SyncEnrollments invokable script'                      AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.SyncEnrollments

            Inserts `Enroll.SyncEnrollments` with `invokable = 1` for
            Reception `POST /api/conduit/script` (PRIORITIZE 2.8).

            The worker:

            1. Reads `params._hydrogen` (`user_id` / `sub`).
            2. Resolves `account_canvas_links.canvas_user_id`. Unlinked
               callers return `ok=true, unlinked=true, inserted=0`.
            3. Lists Canvas `StudentEnrollment` (active+completed).
            4. Maps each `course_id` to `${SCHEMA}courses`. Unmapped
               Canvas courses are skipped.
            5. Inserts `user_enrollments` only when **no row at all**
               exists for `(account_id, course_id)` — archived stays
               archived. Intro (`CANVAS_INTRO_COURSE_ID`) is
               `source=intro` / lifetime; free is `source=free` /
               90 days; paid is `source=paid` / `paid_renew` / 90 days.

            Secrets stay in env (`CANVAS_*`). No diagram.
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
              AND script_name = 'SyncEnrollments';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.SyncEnrollments script'                              AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.SyncEnrollments
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
