-- Migration: courses_4095.lua
-- CATCHUP Phase 15: seed Enroll.StaffArchive (invokable)
--
-- Staff archive or unarchive of one seat. Canvas is updated first.
-- A Canvas failure leaves archived_at unchanged.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Enroll.StaffArchive

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4095"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "StaffArchive"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.StaffArchive (invokable)
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
-- Enroll.StaffArchive (CATCHUP Phase 15)
-- JWT POST /api/conduit/script. Staff or admin from params._hydrogen.roles.
-- Target is params.account_id, params.course_id, and params.action
-- archive or unarchive. The latest seat for that pair is used.
-- Archive removes the Canvas seat, then sets archived_at.
-- Unarchive restores Canvas, then clears archived_at.
-- Canvas failure leaves the Helium row unchanged.

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
        H.log.warn("StaffArchive: history unavailable")
        return
    end
    local recorded = mod.record(opts)
    if type(recorded) == "table" and recorded.ok == false then
        H.log.warn("StaffArchive: history: %s", tostring(recorded.code or recorded.message))
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

local action = string.lower(tostring(params.action or ""))
if action ~= "archive" and action ~= "unarchive" then
    return fail("validation", "action must be archive or unarchive")
end

local seat_rows, seat_err = H.query_sync([[
    SELECT enrollment_id, archived_at, course_id, canvas_course_id,
           canvas_enrollment_id, status,
           CASE
               WHEN status IN ('superseded', 'revoked') THEN 0
               WHEN expires_at IS NOT NULL AND expires_at <= NOW() THEN 0
               ELSE 1
           END AS entitled
    FROM ${SCHEMA}user_enrollments
    WHERE account_id = :ACCOUNTID
      AND course_id = :COURSEID
    ORDER BY enrollment_id DESC
]], { ACCOUNTID = account_id, COURSEID = course_id })
if seat_err then
    return fail("lookup_error", "Could not load the seat")
end
local seats = qrows(seat_rows)
local row = seats and seats[1]
if not row then
    return fail("not_found", "Enrollment not found")
end

local enrollment_id = nzi(pick(row, "enrollment_id", "ENROLLMENT_ID"))
local is_archived = present(pick(row, "archived_at", "ARCHIVED_AT"))
local want_archived = (action == "archive")
local canvas_course_id = nzi(pick(row, "canvas_course_id", "CANVAS_COURSE_ID"))
local canvas_enrollment_id = nzi(pick(row, "canvas_enrollment_id", "CANVAS_ENROLLMENT_ID"))
local entitled = nzi(pick(row, "entitled", "ENTITLED")) == 1

local canvas_user_id = 0
local link_rows = H.query_sync([[
    SELECT canvas_user_id
    FROM ${SCHEMA}account_canvas_links
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
local links = qrows(link_rows)
if links and links[1] then
    canvas_user_id = nzi(pick(links[1], "canvas_user_id", "CANVAS_USER_ID"))
end

if is_archived == want_archived then
    H.set_result_json({
        ok = true,
        archived = want_archived,
        enrollment_id = enrollment_id,
        account_id = account_id,
        course_id = course_id,
        action = action,
    })
    return 0
end

if not want_archived and not entitled then
    return fail("not_entitled", "This course is no longer available to restore")
end

local loaded, seat = pcall(require, "Enroll.CanvasSeat")
if not loaded or type(seat) ~= "table" then
    return fail("canvas_unconfigured", "Canvas is temporarily unavailable")
end

if want_archived then
    if canvas_course_id > 0 and canvas_user_id > 0 then
        local removed = seat.unenroll({
            canvas_user_id = canvas_user_id,
            canvas_course_id = canvas_course_id,
            canvas_enrollment_id = canvas_enrollment_id,
        })
        if type(removed) ~= "table" or removed.ok ~= true then
            return fail(
                (type(removed) == "table" and removed.code) or "canvas_unenroll_failed",
                "Could not remove Canvas seat")
        end
    end
    local _, write_err = H.query_sync([[
        UPDATE ${SCHEMA}user_enrollments
           SET archived_at = NOW(),
               canvas_enrollment_id = NULL,
               updated_at = NOW()
         WHERE enrollment_id = :EID
           AND account_id = :ACCOUNTID
           AND archived_at IS NULL
    ]], { EID = enrollment_id, ACCOUNTID = account_id })
    if write_err then
        return fail("write_error", "Could not update archive state")
    end
    try_log_event({
        account_id = account_id,
        course_id = course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
        enrollment_id = enrollment_id,
        event_type = "archived",
        actor = "staff",
    })
else
    if canvas_course_id == 0 or canvas_user_id == 0 then
        return fail("not_provisioned", "Canvas account is not ready yet")
    end
    local enrolled = seat.enroll({
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
    })
    if type(enrolled) ~= "table" or enrolled.ok ~= true then
        return fail(
            (type(enrolled) == "table" and enrolled.code) or "canvas_enroll_failed",
            "Could not restore Canvas seat")
    end
    local ceid = nzi(enrolled.canvas_enrollment_id)
    local write_err
    if ceid > 0 then
        _, write_err = H.query_sync([[
            UPDATE ${SCHEMA}user_enrollments
               SET archived_at = NULL,
                   canvas_enrollment_id = :CEID,
                   updated_at = NOW()
             WHERE enrollment_id = :EID
               AND account_id = :ACCOUNTID
               AND archived_at IS NOT NULL
        ]], { EID = enrollment_id, ACCOUNTID = account_id, CEID = ceid })
    else
        _, write_err = H.query_sync([[
            UPDATE ${SCHEMA}user_enrollments
               SET archived_at = NULL,
                   updated_at = NOW()
             WHERE enrollment_id = :EID
               AND account_id = :ACCOUNTID
               AND archived_at IS NOT NULL
        ]], { EID = enrollment_id, ACCOUNTID = account_id })
    end
    if write_err then
        return fail("write_error", "Could not update archive state")
    end
    try_log_event({
        account_id = account_id,
        course_id = course_id,
        canvas_user_id = canvas_user_id,
        canvas_course_id = canvas_course_id,
        enrollment_id = enrollment_id,
        event_type = "unarchived",
        actor = "staff",
    })
end

H.set_result_json({
    ok = true,
    archived = want_archived,
    enrollment_id = enrollment_id,
    account_id = account_id,
    course_id = course_id,
    action = action,
})
return 0
                ]==],
                'CATCHUP Phase 15: staff archive and unarchive',
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
        'Seed Enroll.StaffArchive'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.StaffArchive

            Inserts Enroll.StaffArchive with invokable 1.
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
              AND script_name = 'StaffArchive';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.StaffArchive'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.StaffArchive
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
