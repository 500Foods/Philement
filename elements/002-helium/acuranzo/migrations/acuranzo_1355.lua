-- Migration: acuranzo_1355.lua
-- PRIORITIZE 2.18: orders refund columns
--
-- Additive. status may be 'refunded' in addition to
-- pending|completed|failed. No diagram (column add; same as 1346).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - ADD orders refunded_at / refund_reason / stripe_refund_id

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "orders"
cfg.MIGRATION = "1355"
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
                ADD COLUMN refunded_at ${TIMESTAMP_TZ};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN refund_reason ${VARCHAR_500};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN stripe_refund_id ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add ${TABLE} refund columns'                                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: orders refund columns

            PRIORITIZE 2.18. `Stripe.Refund` records why a completed
            order was refunded. Dashboard-only refunds leave these
            NULL until the script runs.

            - **refunded_at**: when Helium recorded the refund.
            - **refund_reason**: operator note (never a missing named
              bind — Lua binds `''`).
            - **stripe_refund_id**: Stripe `re_…`.
            - **status** may be `refunded` (VARCHAR_20; no CHECK).
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
                DROP COLUMN stripe_refund_id;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN refund_reason;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN refunded_at;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} refund columns'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop orders refund columns

            Exact undo. `REORG` around DROP (DB2 SQL0668N rc7).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
