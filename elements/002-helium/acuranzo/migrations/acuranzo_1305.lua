-- Migration: acuranzo_1305.lua
-- Phase 56: seed Mail.Notices.CourseExpiration (invokable) for expiring/expired mail
--
-- Band M user_enrollments is not ready (Phase 59). This job is the send-site
-- interface: fixture/self-test now, live SQL in Phase 59+. Default dry_run.
-- Design: FINISHLINE FL-51/55/56. Data-only seed (no diagram).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Mail.Notices.CourseExpiration invokable=1 (FINISHLINE FL-56)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1305"
cfg.GROUP_NAME = "Mail.Notices"
cfg.SCRIPT_NAME = "CourseExpiration"
-- ----------------------------------------------------------------------------
-- Forward: seed Mail.Notices.CourseExpiration (invokable)
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
-- Mail.Notices.CourseExpiration (Phase 56)
-- JWT POST /api/conduit/script. Default dry_run=true (no SMTP).
-- Live user_enrollments query is Phase 59+; v1 uses params.fixtures.
-- N=7 days, UTC, one notice per enrollment per window (FL-51/55).

local N_DAYS = 7
local LOOKBACK_DAYS = 1
local SETTINGS_URL = "https://www.500courses.com/#account"
local COURSE_URL = "https://www.500courses.com/#my-courses"
local CATALOG_URL = "https://www.500courses.com/#courses"

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function as01(v, default)
    if v == true or v == 1 or v == "1" then return 1 end
    if v == false or v == 0 or v == "0" then return 0 end
    return default
end

local function truthy(v)
    return v == true or v == 1 or v == "1" or v == "true"
end

local function has_token(list, token)
    local wrapped = "," .. tostring(list or "") .. ","
    return wrapped:find("," .. token .. ",", 1, true) ~= nil
end

local function has_mail_send(h)
    if type(h) ~= "table" then return true end
    local roles = tostring(h.roles or "")
    if has_token(roles, "1") then return true end
    if has_token(roles:lower(), "mail_send") then return true end
    return false
end

-- Civil days since 1970-01-01 (UTC calendar; no local tz).
local function days_from_civil(y, m, d)
    y = y - (m <= 2 and 1 or 0)
    local era = math.floor(y / 400)
    local yoe = y - era * 400
    local doy = math.floor((153 * (m + (m > 2 and -3 or 9)) + 2) / 5) + d - 1
    local doe = yoe * 365 + math.floor(yoe / 4) - math.floor(yoe / 100) + doy
    return era * 146097 + doe - 719468
end

local function ymd_days(s)
    local y, mo, d = tostring(s or ""):match("^(%d%d%d%d)%-(%d%d)%-(%d%d)")
    if not y then return nil end
    return days_from_civil(tonumber(y), tonumber(mo), tonumber(d))
end

local function today_days(as_of)
    if as_of and as_of ~= "" then
        local t = ymd_days(as_of)
        if t then return t end
    end
    local utc = os.date("!*t")
    return days_from_civil(utc.year, utc.month, utc.day)
end

local function clean(s, fallback)
    s = tostring(s or ""):gsub("%%", "")
    if s == "" then return fallback end
    return s
end

local function enroll_id(row)
    if row.enrollment_id ~= nil and tostring(row.enrollment_id) ~= "" then
        return tostring(row.enrollment_id)
    end
    return tostring(row.account_id or "?") .. "-" .. tostring(row.course_id or "?")
end

local function idem_key(kind, row)
    local id = enroll_id(row)
    if kind == "expiring" then
        local day = tostring(row.expires_at or ""):sub(1, 10)
        return "course-expiring:" .. id .. ":" .. day
    end
    return "course-expired:" .. id
end

local function classify(row, today)
    local email = row.email
    if type(email) ~= "string" or email == "" then
        return nil, "no_email"
    end
    local exp = ymd_days(row.expires_at)
    if not exp then
        return nil, "bad_expires_at"
    end
    local days_left = exp - today
    if days_left > 0 and days_left <= N_DAYS then
        if as01(row.notify_expiring, 1) ~= 1 then
            return nil, "pref_off"
        end
        return "expiring", days_left
    end
    if days_left <= 0 and days_left >= -LOOKBACK_DAYS then
        if as01(row.notify_expired, 1) ~= 1 then
            return nil, "pref_off"
        end
        return "expired", 0
    end
    return nil, "out_of_window"
end

local function select_notices(rows, today)
    local seen = {}
    local notices = {}
    local skipped = {}
    for _, row in ipairs(rows or {}) do
        local kind, extra = classify(row, today)
        if not kind then
            skipped[#skipped + 1] = {
                enrollment_id = row.enrollment_id,
                reason = extra,
            }
        else
            local key = idem_key(kind, row)
            if seen[key] then
                skipped[#skipped + 1] = {
                    enrollment_id = row.enrollment_id,
                    reason = "already_queued",
                    idempotency_key = key,
                }
            else
                seen[key] = true
                local days_left = extra
                notices[#notices + 1] = {
                    kind = kind,
                    enrollment_id = row.enrollment_id,
                    account_id = row.account_id,
                    course_id = row.course_id,
                    email = row.email,
                    first_name = clean(row.first_name, "there"),
                    course_title = clean(row.course_title, "your course"),
                    expires_at = tostring(row.expires_at or ""),
                    days_left = days_left,
                    template_key = (kind == "expiring")
                        and "user.course_expiring"
                        or "user.course_expired",
                    idempotency_key = key,
                }
            end
        end
    end
    return notices, skipped
end

local function builtin_fixtures()
    return {
        {
            enrollment_id = 1, account_id = 9004, course_id = 1,
            email = "ada@example.com", first_name = "Ada",
            course_title = "Vermillion", expires_at = "2026-08-20T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 2, account_id = 9005, course_id = 1,
            email = "off@example.com", first_name = "Off",
            course_title = "Vermillion", expires_at = "2026-08-20T12:00:00Z",
            notify_expiring = 0, notify_expired = 1,
        },
        {
            enrollment_id = 3, account_id = 9006, course_id = 1,
            email = "later@example.com", first_name = "Later",
            course_title = "Vermillion", expires_at = "2026-08-21T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 4, account_id = 9007, course_id = 2,
            email = "ended@example.com", first_name = "Ended",
            course_title = "Canvas 101", expires_at = "2026-08-13T06:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 5, account_id = 9008, course_id = 2,
            email = "old@example.com", first_name = "Old",
            course_title = "Canvas 101", expires_at = "2026-08-10T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 6, account_id = 9009, course_id = 3,
            email = "soon@example.com", first_name = "Soon",
            course_title = "Lua Basics", expires_at = "2026-08-16T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 7, account_id = 9010, course_id = 1,
            email = "", first_name = "NoMail",
            course_title = "Vermillion", expires_at = "2026-08-16T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
        {
            enrollment_id = 8, account_id = 9011, course_id = 1,
            email = "default@example.com", first_name = "Default",
            course_title = "Vermillion", expires_at = "2026-08-20T12:00:00Z",
        },
        {
            enrollment_id = 1, account_id = 9004, course_id = 1,
            email = "ada@example.com", first_name = "Ada",
            course_title = "Vermillion", expires_at = "2026-08-20T12:00:00Z",
            notify_expiring = 1, notify_expired = 1,
        },
    }
end

local function expected_self_test()
    return {
        { enrollment_id = 1, kind = "expiring", days_left = 7 },
        { enrollment_id = 4, kind = "expired", days_left = 0 },
        { enrollment_id = 6, kind = "expiring", days_left = 3 },
        { enrollment_id = 8, kind = "expiring", days_left = 7 },
    }
end

local function run_self_test()
    local today = ymd_days("2026-08-13")
    local notices, skipped = select_notices(builtin_fixtures(), today)
    local expect = expected_self_test()
    local cases = {}
    local ok = true
    if #notices ~= #expect then
        ok = false
        cases[#cases + 1] = {
            name = "count",
            ok = false,
            message = "got " .. tostring(#notices) .. " expected " .. tostring(#expect),
        }
    end
    for i, exp in ipairs(expect) do
        local got = notices[i]
        local pass = got
            and got.enrollment_id == exp.enrollment_id
            and got.kind == exp.kind
            and got.days_left == exp.days_left
        if not pass then ok = false end
        cases[#cases + 1] = {
            name = "notice_" .. tostring(i),
            ok = pass and true or false,
            expected = exp,
            got = got,
        }
    end
    local reasons = {}
    for _, s in ipairs(skipped) do
        reasons[s.reason] = (reasons[s.reason] or 0) + 1
    end
    local function need(reason, n)
        local pass = reasons[reason] == n
        if not pass then ok = false end
        cases[#cases + 1] = {
            name = "skip_" .. reason,
            ok = pass and true or false,
            expected = n,
            got = reasons[reason] or 0,
        }
    end
    need("pref_off", 1)
    need("out_of_window", 2)
    need("no_email", 1)
    need("already_queued", 1)
    return ok, cases, notices, skipped
end

local function mail_params(n)
    local p = {
        FIRST_NAME = n.first_name,
        COURSE_TITLE = n.course_title,
        EXPIRES_AT = n.expires_at,
        SETTINGS_URL = SETTINGS_URL,
    }
    if n.kind == "expiring" then
        p.DAYS_LEFT = tostring(n.days_left or N_DAYS)
        p.COURSE_URL = COURSE_URL
    else
        p.CATALOG_URL = CATALOG_URL
    end
    return p
end

local function send_one(n, dry_run)
    if dry_run then
        return { queued = false, dry_run = true }
    end
    if type(H) ~= "table" or type(H.mail) ~= "table" or type(H.mail.send_sync) ~= "function" then
        return { queued = false, error = "mail_unavailable" }
    end
    local res, err = H.mail.send_sync({
        template = n.template_key,
        to = n.email,
        params = mail_params(n),
        idempotency_key = n.idempotency_key,
    })
    if err then
        return { queued = false, error = tostring(err) }
    end
    local mid = nil
    if type(res) == "table" then mid = res.message_id end
    return { queued = true, message_id = mid }
end

if type(params) ~= "table" then
    params = {}
end

if truthy(params.self_test) then
    local ok, cases, notices, skipped = run_self_test()
    H.set_result_json({
        ok = ok,
        self_test = true,
        dry_run = true,
        as_of = "2026-08-13T12:00:00Z",
        n_days = N_DAYS,
        cases = cases,
        notices = notices,
        skipped = skipped,
    })
    return 0
end

local h = params._hydrogen
if not has_mail_send(h) then
    return fail("forbidden", "mail_send role required")
end

local dry_run = true
if params.dry_run == false or params.dry_run == 0 or params.dry_run == "0" then
    dry_run = false
end
if truthy(params.send) then
    dry_run = false
end

local rows = params.fixtures
local source = "fixtures"
if type(rows) ~= "table" or #rows == 0 then
    H.set_result_json({
        ok = true,
        dry_run = dry_run,
        source = "none",
        n_days = N_DAYS,
        notices = {},
        skipped = {},
        message = "user_enrollments not wired (Phase 59+); pass fixtures or self_test=true",
    })
    return 0
end

local today = today_days(params.as_of)
local notices, skipped = select_notices(rows, today)
local sent = 0
local errors = 0
for _, n in ipairs(notices) do
    local r = send_one(n, dry_run)
    n.queued = r.queued
    n.dry_run = r.dry_run
    n.message_id = r.message_id
    n.error = r.error
    if r.queued then sent = sent + 1 end
    if r.error then errors = errors + 1 end
end

H.set_result_json({
    ok = errors == 0,
    dry_run = dry_run,
    source = source,
    n_days = N_DAYS,
    as_of = params.as_of,
    notice_count = #notices,
    sent = sent,
    error_count = errors,
    notices = notices,
    skipped = skipped,
})
return 0
                ]==],
                'Phase 56: expiring/expired notice job (fixture dry-run; live SQL Phase 59+)',
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
        'Seed Mail.Notices.CourseExpiration invokable script'               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mail.Notices.CourseExpiration

            Inserts `Mail.Notices.CourseExpiration` with `invokable = 1`
            (FINISHLINE Phase 56). Default `dry_run=true`. Client invoke
            requires JWT `mail_send` (role id 1). Orchestrator (no
            `_hydrogen`) is allowed.

            Band M `user_enrollments` is not present. Pass `fixtures` or
            `self_test=true`. Live query is Phase 59+. Weekly digest is
            not this job. No diagram (data seed only).
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
            WHERE group_name = 'Mail.Notices'
              AND script_name = 'CourseExpiration';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mail.Notices.CourseExpiration script'                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mail.Notices.CourseExpiration
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
