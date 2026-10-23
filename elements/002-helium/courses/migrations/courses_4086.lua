-- Migration: courses_4086.lua
-- CATCHUP Phase 14: seed Stripe.Mode (invokable)
--
-- JWT POST /api/conduit/script. Reads STRIPE_MODE only.
-- Returns test or live. Never reads STRIPE_SECRET_KEY.
-- Dashboard links in Course Manager use this word.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed Stripe.Mode invokable=1 (CATCHUP Phase 14)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4086"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "Mode"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.Mode (invokable)
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
-- Stripe.Mode (CATCHUP Phase 14)
-- Returns the pod mode word. Does not read the secret key.

local function getenv(k)
    if type(os) == "table" and type(os.getenv) == "function" then
        local v = os.getenv(k)
        if v and v ~= "" then return v end
    end
    return ""
end

local mode = string.lower(getenv("STRIPE_MODE"))
if mode ~= "live" then
    mode = "test"
end

H.set_result_json({ ok = true, mode = mode })
return 0
                ]==],
                'CATCHUP Phase 14: test or live from STRIPE_MODE',
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
        'Seed Stripe.Mode invokable script'                                 AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.Mode

            Inserts `Stripe.Mode` with `invokable = 1`. The result is
            `{ ok, mode }` and mode is `test` or `live`. An empty
            STRIPE_MODE is test. The secret key is not read.
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
            WHERE group_name = 'Stripe'
              AND script_name = 'Mode';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.Mode script'                                         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.Mode
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
