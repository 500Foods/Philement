-- Migration: courses_4094.lua
-- CATCHUP Phase 15: seed Enroll.StaffGrant (invokable)
--
-- Staff grant of one free seat. account_id and course_id are the target.
-- Paid courses are refused. An existing seat is not rewritten.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Enroll.StaffGrant

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4094"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "StaffGrant"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.StaffGrant (invokable)
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
-- Enroll.StaffGrant (CATCHUP Phase 15)
-- JWT POST /api/conduit/script. Staff or admin from params._hydrogen.roles.
-- Target is params.account_id and params.course_id, never the caller.
-- Free, published, not retired. An existing seat is left alone.
-- Writes user_enrollments source=free, renew_policy=free_renew,
-- expires_at 90 days out. Canvas enroll is best-effort when a link exists.
-- Does not create a Canvas user. Does not grant a paid seat.

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

local function positive(value)
    local n = tonumber(value)
    if not n or n < 1 or n ~= math.floor(n) then return nil end
    return n
end

local function present(value)
    if value == nil then return false end
    local text = tostring(value)
    return text ~= "" and text ~= "null" and text ~= "NULL"
end

local function nzi(value)
    local n = tonumber(value)
    if not n or n == 0 then return 0 end
    return n
end

local function staff_gate()
    local loaded, gate = pcall(require, "Enroll.StaffGate")
    if not loaded or type(gate) ~= "table" or type(gate.allow) ~= "function" then
        return fail("not_staff", "Staff check is unavailable")
    end
    local allowed, why = gate.allow(params)
    if allowed then return nil end
    if why == "lookup_error" then
        return fail("lookup_error", "Could not resolve staff access")
    end
    if why == "missing_identity" then
        return fail("missing_identity", "Sign in required")
    end
    return fail("not_staff", "Staff access is required")
end

local function try_log_event(opts)
    local loaded, mod = pcall(require, "Enroll.LogEvent")
    if not loaded or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("StaffGrant: history unavailable")
        return
    end
    local recorded = mod.record(opts)
    if type(recorded) == "table" and recorded.ok == false then
        H.log.warn("StaffGrant: history: %s", tostring(recorded.code or recorded.message))
    end
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local denied = staff_gate()
if denied then return denied end

local account_id = positive(params.account_id)
local course_id = positive(params.course_id)
if not account_id or not course_id then
    return fail("validation", "account_id and course_id are required")
end

local account_rows, account_err = H.query_sync([[
    SELECT account_id
    FROM ${SCHEMA}accounts
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if account_err then
    return fail("lookup_error", "Could not load the account")
end
local accounts = qrows(account_rows)
if not accounts or not accounts[1] then
    return fail("not_found", "Account not found")
end

local course_rows, course_err = H.query_sync([[
    SELECT course_id, pricing_type, published, canvas_course_id, retired
    FROM ${SCHEMA}courses
    WHERE course_id = :COURSEID
]], { COURSEID = course_id })
if course_err then
    return fail("lookup_error", "Could not load the course")
end
local courses = qrows(course_rows)
local course = courses and courses[1]
if not course then
    return fail("not_found", "Course not found")
end
if nzi(pick(course, "retired", "RETIRED")) ~= 0 then
    return fail("not_offered", "Retired courses are not granted.")
end
if nzi(pick(course, "published", "PUBLISHED")) ~= 1 then
    return fail("not_offered", "The course is not published.")
end
local pricing = string.lower(tostring(pick(course, "pricing_type", "PRICING_TYPE") or ""))
if pricing ~= "free" then
    return fail("not_offered", "Free courses only. Paid seats stay on checkout.")
end

local seat_rows, seat_err = H.query_sync([[
    SELECT enrollment_id
    FROM ${SCHEMA}user_enrollments
    WHERE account_id = :ACCOUNTID
      AND course_id = :COURSEID
    ORDER BY enrollment_id DESC
]], { ACCOUNTID = account_id, COURSEID = course_id })
if seat_err then
    return fail("lookup_error", "Could not load the seat")
end
local seats = qrows(seat_rows)
local existing = seats and seats[1]
if existing then
    H.set_result_json({
        ok = true,
        already = true,
        enrollment_id = nzi(pick(existing, "enrollment_id", "ENROLLMENT_ID")),
        account_id = account_id,
        course_id = course_id,
    })
    return 0
end

local next_rows, next_err = H.query_sync([[
    SELECT COALESCE(MAX(enrollment_id), 0) + 1 AS next_id
    FROM ${SCHEMA}user_enrollments
]], {})
if next_err then
    return fail("lookup_error", "Could not allocate a seat id")
end
local next_list = qrows(next_rows)
local next_id = nzi(pick(next_list and next_list[1], "next_id", "NEXT_ID"))
if next_id < 1 then
    return fail("lookup_error", "Could not allocate a seat id")
end
local canvas_course_id = nzi(pick(course, "canvas_course_id", "CANVAS_COURSE_ID"))

local _, insert_err = H.query_sync([[
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
        'active', NOW(), NOW() + INTERVAL '90 days',
        NULL, 0, NULL,
        NULL, 'free_renew', 'free', NULL,
        NULL, NULL, 0, NOW(), 0, NOW()
    )
]], {
    EID = next_id,
    ACCOUNTID = account_id,
    COURSEID = course_id,
    CANVASCOURSE = canvas_course_id,
})
if insert_err then
    return fail("write_error", "Could not grant the seat")
end

local canvas_ok = false
local canvas_code = "not_linked"
local canvas_user_id = 0
local link_rows, link_err = H.query_sync([[
    SELECT canvas_user_id
    FROM ${SCHEMA}account_canvas_links
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if link_err then
    canvas_code = "lookup_error"
else
    local links = qrows(link_rows)
    canvas_user_id = nzi(pick(links and links[1], "canvas_user_id", "CANVAS_USER_ID"))
    if canvas_user_id > 0 and canvas_course_id > 0 then
        local loaded, seat = pcall(require, "Enroll.CanvasSeat")
        if not loaded or type(seat) ~= "table" or type(seat.enroll) ~= "function" then
            canvas_code = "canvas_unconfigured"
        else
            local enrolled = seat.enroll({
                canvas_user_id = canvas_user_id,
                canvas_course_id = canvas_course_id,
            })
            if type(enrolled) == "table" and enrolled.ok == true then
                canvas_ok = true
                canvas_code = ""
                local ceid = nzi(enrolled.canvas_enrollment_id)
                if ceid > 0 then
                    local _, write_err = H.query_sync([[
                        UPDATE ${SCHEMA}user_enrollments
                           SET canvas_enrollment_id = :CEID,
                               updated_at = NOW()
                         WHERE enrollment_id = :EID
                           AND account_id = :ACCOUNTID
                    ]], { CEID = ceid, EID = next_id, ACCOUNTID = account_id })
                    if write_err then
                        canvas_ok = false
                        canvas_code = "write_error"
                    end
                end
            else
                canvas_code = (type(enrolled) == "table" and enrolled.code) or "canvas_enroll_failed"
            end
        end
    end
end

try_log_event({
    account_id = account_id,
    course_id = course_id,
    canvas_user_id = canvas_user_id,
    canvas_course_id = canvas_course_id,
    enrollment_id = next_id,
    event_type = "enrolled",
    actor = "staff",
})

H.set_result_json({
    ok = true,
    already = false,
    enrollment_id = next_id,
    account_id = account_id,
    course_id = course_id,
    canvas_ok = canvas_ok,
    canvas_code = canvas_code,
})
return 0
                ]==],
                'CATCHUP Phase 15: staff free-seat grant',
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
        'Seed Enroll.StaffGrant'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.StaffGrant

            Inserts Enroll.StaffGrant with invokable 1. Free seats only.
            No diagram.
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
              AND script_name = 'StaffGrant';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.StaffGrant'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.StaffGrant
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
