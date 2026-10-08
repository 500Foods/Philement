-- Migration: acuranzo_1372.lua
-- MCP Phase 10: seed Mcp.Sleep (timeout / cancel fixture)
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-26 - Sleep params.seconds capped at 60

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1372"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "Sleep"
-- ----------------------------------------------------------------------------
-- Forward: seed Mcp.Sleep
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
-- Mcp.Sleep (MCP Phase 10 fixture)
-- Sleeps params.seconds (capped at 60) for Test 47 timeout / cancel.

local seconds = 0
if type(params) == "table" then
    seconds = tonumber(params.seconds) or 0
end
if seconds < 0 then
    seconds = 0
end
if seconds > 60 then
    seconds = 60
end

local ms = math.floor(seconds * 1000)
if ms > 0 then
    H.sleep(ms)
end

H.set_result_json({
    content = { { type = "text", text = "slept" } },
    structuredContent = { seconds = seconds },
})
return 0
                ]==],
                'MCP fixture: sleep up to 60s for timeout/cancel tests',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"seconds":{"type":"number","minimum":0,"maximum":60}},"required":["seconds"]}}',
                '{"title":"Sleep","readOnlyHint":true,"destructiveHint":false}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Mcp.Sleep fixture tool'                                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mcp.Sleep

            Sleeps `params.seconds` (cap 60) via `H.sleep`.
            `readOnlyHint` + `destructiveHint=false`.
            `mcp_access=1`, `invokable=0`.
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
            WHERE group_name = 'Mcp'
              AND script_name = 'Sleep';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mcp.Sleep fixture tool'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mcp.Sleep
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
