-- Migration: acuranzo_1461.lua
-- zint.Svg invokable wrapper (Reporting Phase 54)
--
-- The barcode library is packed in the Hydrogen payload as lua/zint.
-- This row is the editor's POST /api/conduit/script entry.
-- output is forced off. filetype PDF returns { form, warning }.
-- SVG returns { svg, warning }. No lookup. No Reports.Render edit.
-- QueryRef 175 stays unused.
-- Reverse deletes the zint.Svg row.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-05 - Invokable zint.Svg wrapper
-- 1.0.1 - 2026-11-09 - Renumbered from acuranzo_1469.lua (Render split); content unchanged

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1461"
cfg.GROUP_NAME = "zint"
cfg.SCRIPT_NAME = "Svg"

local sql_new = [==[
-- zint.Svg (Reporting Phase 54). Invokable wrapper. The library lives in the payload.
-- output is forced off. filetype PDF returns { form, warning }. SVG returns { svg, warning }.
local src = type(params) == "table" and params or {}
local opts = {}
for k, v in pairs(src) do
    if k ~= "_hydrogen" and k ~= "output" then opts[k] = v end
end
opts.output = nil
local ok, fn = pcall(require, "zint.barcode")
if not ok or type(fn) ~= "function" then
    H.set_result_json({ error = "zint.barcode is not loaded" })
    return 0
end
local out, warning = fn(opts)
if out == nil then
    H.set_result_json({ error = tostring(warning or "barcode failed"), warning = warning })
    return 0
end
local ft = "SVG"
if opts.filetype and opts.filetype ~= "" then ft = tostring(opts.filetype):upper() end
if ft == "PDF" then
    H.set_result_json({ form = out, warning = warning })
else
    H.set_result_json({ svg = out, warning = warning })
end
return 0
]==]

-- ----------------------------------------------------------------------------
-- Forward: insert zint.Svg
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
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
]====] .. sql_new .. [====[
                ]==],
                'zint.Svg (Reporting Phase 54)',
                1,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Insert zint.Svg'                                                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: zint.Svg

            Inserts invokable group zint, script Svg. The library is
            require'd from the payload. PDF form output is a return
            value. Reports.Render is unchanged.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete zint.Svg
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete zint.Svg'                                                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete zint.Svg

            Deletes the group zint script Svg row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
