-- Migration: acuranzo_1375.lua
-- MCP Phase 15: seed Mcp.Intro fixture prompt
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-27 - Optional topic argument; user message

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1375"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "Intro"
-- ----------------------------------------------------------------------------
-- Forward: seed Mcp.Intro
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
-- Mcp.Intro (MCP Phase 15 fixture prompt)
-- prompts/get returns a user message introducing Hydrogen MCP.

local topic = "Hydrogen MCP"
if type(params) == "table" and type(params.topic) == "string" and params.topic ~= "" then
    topic = params.topic
end

H.set_result_json({
    description = "Introduce Hydrogen MCP to an operator",
    messages = {
        {
            role = "user",
            content = {
                type = "text",
                text = "Explain " .. topic .. " to an operator. Tools are Group.Name. No generic SQL. params._hydrogen is server-injected.",
            },
        },
    },
})
return 0
                ]==],
                'MCP fixture: intro prompt',
                0,
                1,
                '{"arguments":[{"name":"topic","description":"Optional topic to introduce","required":false}]}',
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
        'Seed Mcp.Intro fixture prompt'                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mcp.Intro

            Fixture prompt with optional `topic` argument. Listed by
            `prompts/list`; hidden from `tools/list` because
            `mcp_schema` has `arguments` and no `inputSchema`.
            Two-segment `Group.Name` so `H.mcp.call` can load it.
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
              AND script_name = 'Intro';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mcp.Intro fixture prompt'                                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mcp.Intro
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
