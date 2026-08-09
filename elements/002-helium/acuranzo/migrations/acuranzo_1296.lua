-- Migration: acuranzo_1296.lua
-- LUA_CLIENT Phase 6: seed Api.Echo fixture for conduit script invoke

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Api.Echo returns params via H.set_result_json

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1296"
cfg.GROUP_NAME = "Api"
cfg.SCRIPT_NAME = "Echo"
-- ----------------------------------------------------------------------------
-- Forward: seed Api.Echo
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
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Api.Echo (LUA_CLIENT Phase 6 fixture)
-- Returns the job params table (including server-injected params._hydrogen)
-- via H.set_result_json for POST /api/conduit/script smoke tests.
-- Optional: params.probe_health = true → GET local system health via H.http.

local out = {}
if type(params) == "table" then
    for k, v in pairs(params) do
        out[k] = v
    end
end

if type(params) == "table" and params.probe_health == true then
    local base = "http://127.0.0.1:5000"
    if type(os) == "table" and type(os.getenv) == "function" then
        local env_base = os.getenv("HYDROGEN_PROBE_BASE")
        if env_base and env_base ~= "" then
            base = env_base:gsub("/+$", "")
        end
    end
    local url = base .. "/api/system/health"
    local res, err = H.http.get_sync(url, { Accept = "application/json" }, { timeout = 5 })
    if res then
        out.probe = {
            ok = (res.status >= 200 and res.status < 300),
            status = res.status,
            body_len = type(res.body) == "string" and #res.body or 0,
        }
    else
        out.probe = { ok = false, error = tostring(err) }
    end
end

H.set_result_json(out)
return 0
                ]==],
                'LUA_CLIENT fixture: echo params (+ optional probe_health)',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Api.Echo script fixture'                                      AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Api.Echo

            Inserts `Api.Echo` for client-facing script invoke tests
            (`POST /api/conduit/script`). Echoes `params` via
            `H.set_result_json`. Optional `params.probe_health` exercises
            outbound `H.http.get_sync` against system health.
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
            WHERE group_name = 'Api'
              AND script_name = 'Echo';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Api.Echo script fixture'                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Api.Echo
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
