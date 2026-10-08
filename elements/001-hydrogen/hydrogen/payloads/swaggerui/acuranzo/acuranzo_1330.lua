-- Migration: acuranzo_1330.lua
-- PRIORITIZE 2.9: Enroll.Archive appends enrollment_events
--
-- Do not restamp 1324. After a real archive/unarchive write, require Enroll.LogEvent. Idempotent no-ops stay silent.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-20 - Archive logs enrolment events (PRIORITIZE 2.9)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1330"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "Archive"
-- ----------------------------------------------------------------------------
-- Forward: replace Enroll.Archive body
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Enroll.Archive (PRIORITIZE 2.7; 1330 LogEvent)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Hide overlay: SET/CLEAR user_enrollments.archived_at.
-- No Canvas unenroll. No completion gate. Own row only.
-- Archive SQL never binds NULL (Hydrogen drops missing named binds).

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local LOG_TAG = "Archive"
local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("%s: LogEvent unavailable: %s", LOG_TAG, tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("%s: LogEvent: %s", LOG_TAG, tostring(r.code or r.message))
    end
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

local enrollment_id = tonumber(params.enrollment_id)
if not enrollment_id or enrollment_id < 1 then
    return fail("validation", "enrollment_id is required")
end

local action = string.lower(tostring(params.action or ""))
if action ~= "archive" and action ~= "unarchive" then
    return fail("validation", "action must be archive or unarchive")
end

local _qr, qerr = H.query_sync([[
    SELECT enrollment_id, archived_at, course_id, canvas_course_id
    FROM ${SCHEMA}user_enrollments
    WHERE enrollment_id = :ENROLLMENTID
      AND account_id = :ACCOUNTID
]], { ENROLLMENTID = enrollment_id, ACCOUNTID = account_id })
if qerr then
    H.log.warn("Archive: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load enrollment")
end

local rows = qrows(_qr)
if not rows or not rows[1] then
    return fail("not_found", "Enrollment not found")
end

local is_archived = present(pick(rows[1], "archived_at", "ARCHIVED_AT"))
local want_archived = (action == "archive")

if is_archived == want_archived then
    H.set_result_json({
        ok = true,
        archived = want_archived,
        enrollment_id = enrollment_id,
        action = action,
    })
    return 0
end

local sql
if want_archived then
    sql = [[
        UPDATE ${SCHEMA}user_enrollments
           SET archived_at = NOW(),
               updated_at = NOW()
         WHERE enrollment_id = :ENROLLMENTID
           AND account_id = :ACCOUNTID
           AND archived_at IS NULL
    ]]
else
    sql = [[
        UPDATE ${SCHEMA}user_enrollments
           SET archived_at = NULL,
               updated_at = NOW()
         WHERE enrollment_id = :ENROLLMENTID
           AND account_id = :ACCOUNTID
           AND archived_at IS NOT NULL
    ]]
end

local _, werr = H.query_sync(sql, {
    ENROLLMENTID = enrollment_id,
    ACCOUNTID = account_id,
})
if werr then
    H.log.warn("Archive: write err: %s", tostring(werr))
    return fail("write_error", "Could not update archive state")
end

local course_id = tonumber(pick(rows[1], "course_id", "COURSE_ID"))
local canvas_course_id = tonumber(pick(rows[1], "canvas_course_id", "CANVAS_COURSE_ID"))
local canvas_user_id = 0
local _lr = H.query_sync([[
    SELECT canvas_user_id
    FROM ${SCHEMA}account_canvas_links
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
local lrows = qrows(_lr)
if lrows and lrows[1] then
    canvas_user_id = tonumber(pick(lrows[1], "canvas_user_id", "CANVAS_USER_ID")) or 0
end
local etype = "archived"
if not want_archived then
    etype = "unarchived"
end
try_log_event({
    account_id = account_id,
    course_id = course_id,
    canvas_user_id = canvas_user_id,
    canvas_course_id = canvas_course_id,
    enrollment_id = enrollment_id,
    event_type = etype,
    actor = "self",
})

H.set_result_json({
    ok = true,
    archived = want_archived,
    enrollment_id = enrollment_id,
    action = action,
})
return 0
                ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'Archive';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Archive logs enrollment_events'                                                AS name,
        [=[
            # Forward Migration ${MIGRATION}: Enroll.Archive LogEvent\n\n            Logs archived/unarchived on a real overlay write. Idempotent no-ops do not append. Canvas PUT is best-effort.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior body
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Enroll.Archive (PRIORITIZE 2.7)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Hide overlay: SET/CLEAR user_enrollments.archived_at.
-- No Canvas unenroll. No completion gate. Own row only.
-- Archive SQL never binds NULL (Hydrogen drops missing named binds).

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

local enrollment_id = tonumber(params.enrollment_id)
if not enrollment_id or enrollment_id < 1 then
    return fail("validation", "enrollment_id is required")
end

local action = string.lower(tostring(params.action or ""))
if action ~= "archive" and action ~= "unarchive" then
    return fail("validation", "action must be archive or unarchive")
end

local _qr, qerr = H.query_sync([[
    SELECT enrollment_id, archived_at
    FROM ${SCHEMA}user_enrollments
    WHERE enrollment_id = :ENROLLMENTID
      AND account_id = :ACCOUNTID
]], { ENROLLMENTID = enrollment_id, ACCOUNTID = account_id })
if qerr then
    H.log.warn("Archive: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load enrollment")
end

local rows = qrows(_qr)
if not rows or not rows[1] then
    return fail("not_found", "Enrollment not found")
end

local is_archived = present(pick(rows[1], "archived_at", "ARCHIVED_AT"))
local want_archived = (action == "archive")

if is_archived == want_archived then
    H.set_result_json({
        ok = true,
        archived = want_archived,
        enrollment_id = enrollment_id,
        action = action,
    })
    return 0
end

local sql
if want_archived then
    sql = [[
        UPDATE ${SCHEMA}user_enrollments
           SET archived_at = NOW(),
               updated_at = NOW()
         WHERE enrollment_id = :ENROLLMENTID
           AND account_id = :ACCOUNTID
           AND archived_at IS NULL
    ]]
else
    sql = [[
        UPDATE ${SCHEMA}user_enrollments
           SET archived_at = NULL,
               updated_at = NOW()
         WHERE enrollment_id = :ENROLLMENTID
           AND account_id = :ACCOUNTID
           AND archived_at IS NOT NULL
    ]]
end

local _, werr = H.query_sync(sql, {
    ENROLLMENTID = enrollment_id,
    ACCOUNTID = account_id,
})
if werr then
    H.log.warn("Archive: write err: %s", tostring(werr))
    return fail("write_error", "Could not update archive state")
end

H.set_result_json({
    ok = true,
    archived = want_archived,
    enrollment_id = enrollment_id,
    action = action,
})
return 0
                ]==]
            WHERE group_name = 'Enroll'
              AND script_name = 'Archive';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Enroll.Archive prior body'                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore prior script body
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
