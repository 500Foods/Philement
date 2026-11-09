-- Migration: acuranzo_1485.lua
-- Reports.RewriteBackgroundColor — Phase 77 one-shot shape backgroundColor rewrite
--
-- Invokable script. Andrew runs it once after apply. Reverse deletes the
-- script only; it does not strip backgroundColor from reports.report.
-- QueryRef 175 stays unused.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 RewriteBackgroundColor

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1485"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "RewriteBackgroundColor"

local sql_new = [==[
-- Reports.RewriteBackgroundColor (Reporting Phase 77)
-- One-shot: add backgroundColor on shape items from fillPaint when missing.
-- Invoke once after apply (POST /api/conduit/script). Reverse of migration
-- 1485 deletes this script; it does not strip backgroundColor from rows
-- (forward is additive).

local H = params and params._hydrogen
if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
    error("Reports.RewriteBackgroundColor needs params._hydrogen")
end
local db = H.database
local schema = (type(H.schema) == "string" and H.schema ~= "" and H.schema)
    or (type(params.schema) == "string" and params.schema)
    or ""
local prefix = schema ~= "" and (schema:sub(-1) == "." and schema or (schema .. ".")) or ""

local function hex_ok(v)
    return type(v) == "string" and #v >= 4 and v:match("^#[0-9A-Fa-f]+$") ~= nil
end

local function from_paint(paint)
    if type(paint) ~= "table" then return nil end
    if hex_ok(paint.color) then return string.lower(paint.color) end
    local stops = paint.stops
    if type(stops) == "table" and type(stops[1]) == "table" and hex_ok(stops[1].color) then
        return string.lower(stops[1].color)
    end
    return nil
end

local function normalize(doc)
    if type(doc) ~= "table" then return doc, false end
    local changed = false
    for _, band in ipairs(doc.bands or {}) do
        for _, item in ipairs((band and band.items) or {}) do
            if type(item) == "table" then
                local tk = tonumber(item.typeKey)
                if (tk == 25 or tk == 26)
                    and (type(item.backgroundColor) ~= "string" or item.backgroundColor == "") then
                    local c = from_paint(item.fillPaint)
                    if c then item.backgroundColor = c; changed = true end
                end
            end
        end
    end
    return doc, changed
end

local cjson = cjson
if not cjson then
    local ok, mod = pcall(require, "cjson")
    if ok then cjson = mod end
end
if not cjson then error("cjson required") end

local select_sql = "SELECT report_id, rev_id, report FROM " .. prefix .. "reports"
local rows = H.altquery_sync(db, select_sql, {})
if type(rows) ~= "table" then
    error("select reports failed: " .. tostring(rows))
end

local updated, skipped, failed = 0, 0, 0
local errors = {}
for _, row in ipairs(rows) do
    local text = row.report
    if type(text) ~= "string" or text == "" then
        skipped = skipped + 1
    else
        local ok, doc = pcall(cjson.decode, text)
        if not ok or type(doc) ~= "table" then
            failed = failed + 1
            if #errors < 10 then errors[#errors + 1] = tostring(row.report_id) .. "/" .. tostring(row.rev_id) end
        else
            local nextDoc, changed = normalize(doc)
            if not changed then
                skipped = skipped + 1
            else
                local encoded = cjson.encode(nextDoc)
                local update_sql = "UPDATE " .. prefix .. "reports SET report = :REPORT WHERE report_id = :REPORT_ID AND rev_id = :REV_ID"
                local uok, uerr = pcall(H.altquery_sync, db, update_sql, {
                    TEXT = { REPORT = encoded },
                    INTEGER = { REPORT_ID = row.report_id, REV_ID = row.rev_id },
                })
                if not uok then
                    failed = failed + 1
                    if #errors < 10 then errors[#errors + 1] = tostring(uerr) end
                else
                    updated = updated + 1
                end
            end
        end
    end
end

if H.set_result_json then
    H.set_result_json({ ok = failed == 0, updated = updated, skipped = skipped, failed = failed, errors = errors })
end
return failed == 0 and 0 or 1

]==]

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
                group_name, script_name, script_type, schedule,
                next_run, last_run_start, last_run_end, status,
                code, summary, invokable, mcp_access, mcp_schema,
                mcp_annotations, ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1, NULL, NULL, NULL, NULL, 1,
                [==[
]====] .. sql_new .. [====[
                ]==],
                'One-shot: add backgroundColor from fillPaint on shape items in reports.report.',
                1, 0, NULL, NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Insert Reports.RewriteBackgroundColor'                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Insert Reports.RewriteBackgroundColor

            Invokable one-shot rewrite for shape backgroundColor.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.RewriteBackgroundColor'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.RewriteBackgroundColor

            Deletes the script. Does not strip backgroundColor from reports.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
