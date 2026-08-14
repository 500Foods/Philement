-- Migration: acuranzo_1309.lua
-- Phase 61: seed Account.MyCourses (invokable) for Reception My Courses list
--
-- JWT POST /api/conduit/script. Identity from params._hydrogen only.
-- Own user_enrollments + catalog join. view=active|past|archived|all.
-- No Hydrogen C. Data-only seed (no diagram).
-- auth_query is not used: it does not bind JWT user_id (FL-53).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Account.MyCourses invokable=1 (FINISHLINE FL-61)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1309"
cfg.GROUP_NAME = "Account"
cfg.SCRIPT_NAME = "MyCourses"
-- ----------------------------------------------------------------------------
-- Forward: seed Account.MyCourses (invokable)
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
-- Account.MyCourses (Phase 61)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Lists own user_enrollments joined to courses. Never accepts a
-- client account id. Tabs from FL-58 (derive expired; archive overlay).

local MAX_ROWS = 200

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function present(v)
    if v == nil then return false end
    local s = tostring(v)
    return s ~= "" and s ~= "null" and s ~= "NULL"
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

local function is_expired(expires_at, now_ts)
    if not present(expires_at) then return false end
    local a = parse_epoch(expires_at)
    local b = parse_epoch(now_ts)
    if not a or not b then return true end
    return a <= b
end

local function bucket_of(status, expires_at, archived_at, now_ts)
    if present(archived_at) then return "archived" end
    local st = string.lower(tostring(status or ""))
    if st == "pending" then return "pending" end
    if is_expired(expires_at, now_ts) or st == "superseded" then
        return "past"
    end
    if st == "active" or st == "completed" then return "active" end
    return "other"
end

local SELF_TEST = {
    { id = 1, status = "active", expires_at = nil, archived_at = nil, expect = "active" },
    { id = 2, status = "completed", expires_at = "2026-11-13T12:00:00Z",
      archived_at = nil, expect = "active" },
    { id = 3, status = "active", expires_at = "2026-05-13T12:00:00Z",
      archived_at = nil, expect = "past" },
    { id = 4, status = "completed", expires_at = "2026-05-13T12:00:00Z",
      archived_at = nil, expect = "past" },
    { id = 5, status = "superseded", expires_at = nil, archived_at = nil, expect = "past" },
    { id = 6, status = "active", expires_at = nil,
      archived_at = "2026-08-01T12:00:00Z", expect = "archived" },
    { id = 7, status = "active", expires_at = "2026-05-13T12:00:00Z",
      archived_at = "2026-08-01T12:00:00Z", expect = "archived" },
    { id = 8, status = "pending", expires_at = nil, archived_at = nil, expect = "pending" },
}

local function run_self_test()
    local now_ts = "2026-08-13T12:00:00Z"
    local items = {}
    local failed = 0
    for _, fx in ipairs(SELF_TEST) do
        local got = bucket_of(fx.status, fx.expires_at, fx.archived_at, now_ts)
        local ok = got == fx.expect
        if not ok then failed = failed + 1 end
        items[#items + 1] = {
            id = fx.id,
            expect = fx.expect,
            got = got,
            ok = ok,
        }
    end
    H.set_result_json({
        ok = failed == 0,
        self_test = true,
        now_ts = now_ts,
        items = items,
        failed = failed,
    })
    return 0
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

if params.self_test == true or params.self_test == 1 or params.self_test == "1"
        or params.self_test == "true" then
    return run_self_test()
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

local view = string.lower(tostring(params.view or "active"))
if view ~= "active" and view ~= "past" and view ~= "archived" and view ~= "all" then
    return fail("validation", "view must be active, past, archived, or all")
end

local _qr, qerr = H.query_sync([[
    SELECT
        ue.enrollment_id,
        ue.account_id,
        ue.course_id,
        ue.canvas_enrollment_id,
        ue.canvas_course_id,
        ue.status,
        ue.enrolled_at,
        ue.expires_at,
        ue.completed_at,
        ue.progress_percent,
        ue.progress_synced_at,
        ue.archived_at,
        ue.renew_policy,
        ue.source,
        c.code,
        c.slug,
        c.title,
        c.image_path,
        c.canvas_course_id AS catalog_canvas_course_id,
        c.pricing_type,
        CURRENT_TIMESTAMP AS now_ts
    FROM ${SCHEMA}user_enrollments ue
    LEFT JOIN ${SCHEMA}courses c
      ON c.course_id = ue.course_id
    WHERE ue.account_id = :ACCOUNTID
    ORDER BY ue.enrolled_at DESC
]], { ACCOUNTID = account_id })
if qerr then
    H.log.warn("MyCourses: list err: %s", tostring(qerr))
    return fail("query_error", "Could not load courses")
end

local rows = qrows(_qr) or {}
local counts = { active = 0, past = 0, archived = 0, pending = 0, all = 0 }
local enrollments = {}

for _, row in ipairs(rows) do
    local now_ts = pick(row, "now_ts", "NOW_TS")
    local status = pick(row, "status", "STATUS")
    local expires_at = pick(row, "expires_at", "EXPIRES_AT")
    local archived_at = pick(row, "archived_at", "ARCHIVED_AT")
    local bucket = bucket_of(status, expires_at, archived_at, now_ts)
    counts.all = counts.all + 1
    if counts[bucket] ~= nil then
        counts[bucket] = counts[bucket] + 1
    end
    local include = view == "all" or bucket == view
    if include and #enrollments < MAX_ROWS then
        local canvas_course = tonumber(pick(row, "canvas_course_id", "CANVAS_COURSE_ID"))
            or tonumber(pick(row, "catalog_canvas_course_id", "CATALOG_CANVAS_COURSE_ID"))
        enrollments[#enrollments + 1] = {
            enrollment_id = tonumber(pick(row, "enrollment_id", "ENROLLMENT_ID")),
            course_id = tonumber(pick(row, "course_id", "COURSE_ID")),
            code = pick(row, "code", "CODE") or "",
            slug = pick(row, "slug", "SLUG") or "",
            title = pick(row, "title", "TITLE") or "",
            image_path = pick(row, "image_path", "IMAGE_PATH") or "",
            canvas_course_id = canvas_course,
            status = status,
            enrolled_at = pick(row, "enrolled_at", "ENROLLED_AT"),
            expires_at = expires_at,
            completed_at = pick(row, "completed_at", "COMPLETED_AT"),
            progress_percent = tonumber(pick(row, "progress_percent", "PROGRESS_PERCENT")) or 0,
            progress_synced_at = pick(row, "progress_synced_at", "PROGRESS_SYNCED_AT"),
            archived_at = archived_at,
            renew_policy = pick(row, "renew_policy", "RENEW_POLICY"),
            source = pick(row, "source", "SOURCE"),
            pricing_type = pick(row, "pricing_type", "PRICING_TYPE"),
            expired = is_expired(expires_at, now_ts),
            archived = present(archived_at),
            bucket = bucket,
        }
    end
end

H.set_result_json({
    ok = true,
    view = view,
    account_id = account_id,
    enrollments = enrollments,
    counts = counts,
})
return 0
                ]==],
                'Phase 61: invokable My Courses list (own rows only)',
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
        'Seed Account.MyCourses invokable script'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Account.MyCourses

            Inserts `Account.MyCourses` with `invokable = 1` for Reception
            `POST /api/conduit/script` (FINISHLINE Phase 61).

            `/api/conduit/auth_query` is **not** used: it does not inject
            JWT `user_id` into SQL params, so a type-11 QueryRef cannot
            enforce own-row-only (FL-53).

            The worker:

            1. Reads `params._hydrogen` (`user_id` / `sub`). Client
               `account_id` is never bound.
            2. `view=active|past|archived|all` (default `active`).
               Tabs from FL-58: Current = not archived, not expired,
               `status` in (`active`,`completed`). Past = not archived
               and (derived expired or `superseded`). Archived =
               `archived_at` set. In-term `completed` stays Current.
            3. LEFT JOIN `courses` for title / slug / image. Canvas
               course id is enrollment denorm, else catalog.
            4. Returns `renew_policy`, `expires_at`, progress cache,
               and `bucket`. Reception builds the Canvas deep link from
               `canvas.baseUrl` + `canvas_course_id` (no C helper).
            5. `self_test=true` classifies FL-58 fixtures (no SQL).

            No diagram (data seed). Table may be empty until FreeCourse
            / intro writers land — empty list is a valid payload.
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
            WHERE group_name = 'Account'
              AND script_name = 'MyCourses';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Account.MyCourses script'                                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Account.MyCourses
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
