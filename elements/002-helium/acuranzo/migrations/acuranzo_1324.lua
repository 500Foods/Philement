-- Migration: acuranzo_1324.lua
-- PRIORITIZE 2.7: seed Enroll.Archive (invokable) hide overlay
--
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- SET/CLEAR user_enrollments.archived_at. No Canvas unenroll.
-- No completion gate — incomplete / in-progress / expired all allowed.
-- Own row only. Idempotent. No diagram (data seed).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-20 - Seed Enroll.Archive invokable=1 (PRIORITIZE 2.7)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1324"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "Archive"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.Archive (invokable)
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
                ]==],
                'PRIORITIZE 2.7: invokable archive overlay (no Canvas unenroll)',
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
        'Seed Enroll.Archive invokable script'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.Archive

            Inserts `Enroll.Archive` with `invokable = 1` for Reception
            `POST /api/conduit/script` (PRIORITIZE 2.7).

            The worker:

            1. Reads `params._hydrogen` (`user_id` / `sub`).
            2. Requires `enrollment_id` and `action` `archive` |
               `unarchive`.
            3. Loads the caller's own `user_enrollments` row. Missing
               or other-account → `not_found` (no leak).
            4. Sets or clears `archived_at` only. Does **not** change
               `status`, does **not** call Canvas, does **not** gate
               on `completed_at` / progress. Incomplete courses
               archive. Archive does not free the
               `(account_id, course_id)` slot (1307 invariant).
            5. Idempotent: already in the requested state returns
               `ok` without a write.

            Unarchive SQL uses a literal `archived_at = NULL` so
            Hydrogen never sees a missing named bind. No diagram.
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
              AND script_name = 'Archive';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.Archive script'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.Archive
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
