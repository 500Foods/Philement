-- Migration: acuranzo_1346.lua
-- PRIORITIZE 2.35: courses.retired (searchable, not buyable)
--
-- Lithium-only. Not unpublish (2.27/2.29) and not entitlement
-- revoke (2.18). Default 0. Catalog.SyncFromCanvas must not
-- write this column. No diagram (column add; same as 1333).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - ADD courses.retired (PRIORITIZE 2.35)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "courses"
cfg.MIGRATION = "1346"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN retired ${INTEGER_SMALL} NOT NULL DEFAULT 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add ${TABLE}.retired'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: courses.retired

            PRIORITIZE 2.35. Operator can mark a catalog row retired.

            - **retired**: INTEGER_SMALL NOT NULL DEFAULT 0. 1 = still
              listed and searchable (#147), not buyable / free-enrollable.
              Existing seats stay. Do not overload `published`.
            - Canvas `workflow_state` must not flip this (2.29).
            - Stripe Product deactivate rides on Catalog.Retire (1349).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN retired;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE}.retired'                                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop courses.retired

            Exact undo. `${REORG}` around DROP (DB2 SQL0668N rc7).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
