-- Migration: courses_4128.lua
-- CATCHUP Phase 22: seed Build.InspectPass (invokable)
--
-- Inserts Build.InspectPass with invokable 1. inspect -> checklist
-- plus one ledger row. Human Inspect Pass. Stub: no Canvas call. No diagram.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Seed Build.InspectPass (invokable)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4128"
cfg.GROUP_NAME = "Build"
cfg.SCRIPT_NAME = "InspectPass"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_STATE = "char(50)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_STATE = cfg.VARCHAR_50
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end
-- ----------------------------------------------------------------------------
-- Forward: seed Build.InspectPass (invokable)
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
-- Build.InspectPass (CATCHUP Phase 22 / COURSEBUILDER CB-25)
-- JWT POST /api/conduit/script with params.run_id. Staff only.
-- Moves the run inspect -> checklist and writes one ledger row.
-- Wrong state returns { ok=false, code="wrong_state", state=... }.
-- Human Inspect Pass. Stub: no Canvas call.

local loaded, gate = pcall(require, "Build.Gate")
if not loaded or type(gate) ~= "table" or type(gate.transition) ~= "function" then
    H.set_result_json({ ok = false, code = "unavailable", message = "Build.Gate is unavailable" })
    return 0
end
return gate.transition(params, { from = "inspect", to = "checklist", script = "Build.InspectPass" })
                ]==],
                'CATCHUP Phase 22: Course Builder: inspect to checklist',
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
        'Seed Build.InspectPass'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Build.InspectPass

            Inserts Build.InspectPass with invokable 1. inspect -> checklist plus one ledger row. Human Inspect Pass. Stub: no Canvas call. No diagram.
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
            WHERE group_name = 'Build'
              AND script_name = 'InspectPass';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Build.InspectPass'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Build.InspectPass
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
