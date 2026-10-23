-- Migration: courses_4096.lua
-- CATCHUP Phase 15: seed Enroll.StaffRenew (invokable)
--
-- Staff renewal of a free_renew seat. Adds 90 days to expires_at.
-- paid_renew stays on checkout. Enroll.RenewFree stays unseeded.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Enroll.StaffRenew

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4096"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "StaffRenew"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.StaffRenew (invokable)
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
-- Enroll.StaffRenew (CATCHUP Phase 15)
-- JWT POST /api/conduit/script. Staff or admin from params._hydrogen.roles.
-- Target is params.account_id and params.course_id.
-- Extends expires_at by 90 days when renew_policy is free_renew.
-- Refuses paid_renew, no_renew, archived, revoked, and superseded.
-- Does not write an order. Enroll.RenewFree stays unseeded.

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
        H.log.warn("StaffRenew: history unavailable")
        return
    end
    local recorded = mod.record(opts)
    if type(recorded) == "table" and recorded.ok == false then
        H.log.warn("StaffRenew: history: %s", tostring(recorded.code or recorded.message))
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

local seat_rows, seat_err = H.query_sync([[
    SELECT enrollment_id, archived_at, renew_policy, status, canvas_course_id
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

local policy = string.lower(tostring(pick(row, "renew_policy", "RENEW_POLICY") or ""))
if policy == "paid_renew" then
    return fail("paid_renew", "Paid renewal stays on checkout.")
end
if policy ~= "free_renew" then
    return fail("no_renew", "This seat cannot be renewed.")
end
if present(pick(row, "archived_at", "ARCHIVED_AT")) then
    return fail("archived", "Unarchive the seat before renewing it.")
end
local status = string.lower(tostring(pick(row, "status", "STATUS") or ""))
if status == "revoked" or status == "superseded" then
    return fail("not_entitled", "This seat can no longer be renewed.")
end

local enrollment_id = nzi(pick(row, "enrollment_id", "ENROLLMENT_ID"))
local _, write_err = H.query_sync([[
    UPDATE ${SCHEMA}user_enrollments
       SET expires_at = NOW() + INTERVAL '90 days',
           updated_at = NOW()
     WHERE enrollment_id = :EID
       AND account_id = :ACCOUNTID
       AND course_id = :COURSEID
       AND archived_at IS NULL
]], { EID = enrollment_id, ACCOUNTID = account_id, COURSEID = course_id })
if write_err then
    return fail("write_error", "Could not renew the seat")
end

local fresh_rows, fresh_err = H.query_sync([[
    SELECT expires_at, renew_policy
    FROM ${SCHEMA}user_enrollments
    WHERE enrollment_id = :EID
      AND account_id = :ACCOUNTID
]], { EID = enrollment_id, ACCOUNTID = account_id })
if fresh_err then
    return fail("lookup_error", "Could not read the renewed seat")
end
local fresh = qrows(fresh_rows)
local saved = fresh and fresh[1]

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

try_log_event({
    account_id = account_id,
    course_id = course_id,
    canvas_user_id = canvas_user_id,
    canvas_course_id = nzi(pick(row, "canvas_course_id", "CANVAS_COURSE_ID")),
    enrollment_id = enrollment_id,
    event_type = "renewed",
    actor = "staff",
})

H.set_result_json({
    ok = true,
    enrollment_id = enrollment_id,
    account_id = account_id,
    course_id = course_id,
    expires_at = pick(saved, "expires_at", "EXPIRES_AT"),
    renew_policy = pick(saved, "renew_policy", "RENEW_POLICY") or "free_renew",
})
return 0
                ]==],
                'CATCHUP Phase 15: staff free renewal',
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
        'Seed Enroll.StaffRenew'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.StaffRenew

            Inserts Enroll.StaffRenew with invokable 1. free_renew only.
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
              AND script_name = 'StaffRenew';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.StaffRenew'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.StaffRenew
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
