-- Migration: courses_4093.lua
-- CATCHUP Phase 15: seed Enroll.StaffGate (not invokable)
--
-- Module only. Resolves params._hydrogen.roles through roles.name.
-- Staff or admin allows the call. A lookup error denies.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Enroll.StaffGate module

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4093"
cfg.GROUP_NAME = "Enroll"
cfg.SCRIPT_NAME = "StaffGate"
-- ----------------------------------------------------------------------------
-- Forward: seed Enroll.StaffGate (not invokable)
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
-- Enroll.StaffGate (CATCHUP Phase 15)
-- Module. require("Enroll.StaffGate").allow(params)
-- Not JWT-invokable. StaffGrant, StaffArchive, and StaffRenew call it.
-- params._hydrogen.roles is the CSV of integer role_ids.
-- A resolved roles.name of staff or admin allows the call.
-- A lookup error denies. This module does not read the target account.

local M = {}

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

local function add_id(ids, value)
    local text = tostring(value or ""):gsub("[%[%]\"']", "")
    local digits = text:match("^%s*(%d+)%s*$")
    local n = tonumber(digits)
    if n and n > 0 then
        ids[#ids + 1] = n
    end
end

local function role_ids(roles)
    local ids = {}
    if type(roles) == "string" then
        for part in string.gmatch(roles, "[^,]+") do
            add_id(ids, part)
        end
    elseif type(roles) == "number" then
        add_id(ids, roles)
    elseif type(roles) == "table" then
        for _, value in pairs(roles) do
            add_id(ids, value)
        end
    end
    return ids
end

function M.allow(params)
    if type(params) ~= "table" then
        return false, "validation"
    end
    local hydrogen = params._hydrogen
    if type(hydrogen) ~= "table" then
        return false, "missing_identity"
    end
    local ids = role_ids(hydrogen.roles)
    if #ids == 0 then
        return false, "not_staff"
    end
    for _, id in ipairs(ids) do
        local res, err = H.query_sync([[
            SELECT name
            FROM ${SCHEMA}roles
            WHERE role_id = :ROLEID
        ]], { ROLEID = id })
        if err then
            return false, "lookup_error"
        end
        local rows = qrows(res)
        local name = pick(rows and rows[1], "name", "NAME")
        if type(name) == "string" then
            local lower = string.lower(name)
            if lower == "staff" or lower == "admin" then
                return true
            end
        end
    end
    return false, "not_staff"
end

return M
                ]==],
                'CATCHUP Phase 15: staff role check module',
                0,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Enroll.StaffGate'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Enroll.StaffGate

            Inserts Enroll.StaffGate with invokable 0. Callers require it.
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
              AND script_name = 'StaffGate';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Enroll.StaffGate'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Enroll.StaffGate
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
