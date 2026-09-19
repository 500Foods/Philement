-- Migration: acuranzo_1292.lua
-- Phase 35: course_prices table (Band G + STRIPE_PLAN Phase 3)
--
-- One row per (course_id, currency). stripe_price_id nullable until Band J.
-- Design: FINISHLINE FL-34 / STRIPE_PLAN.md Phase 3.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Create course_prices table (Phase 35 / FL-34)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_prices"
cfg.MIGRATION = "1292"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                price_id                ${INTEGER}          NOT NULL,
                course_id               ${INTEGER}          NOT NULL,
                currency                ${VARCHAR_20}       NOT NULL,
                unit_amount_cents       ${INTEGER}          NOT NULL,
                stripe_price_id         ${VARCHAR_100}              ,
                is_active               ${INTEGER_SMALL}    NOT NULL DEFAULT 1,
                ${COMMON_CREATE}
                ${PRIMARY}(price_id),
                ${UNIQUE}(course_id, currency)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_currency
                ON ${SCHEMA}${TABLE}(currency, is_active);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_course
                ON ${SCHEMA}${TABLE}(course_id);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            Band G Phase 35 + STRIPE_PLAN Phase 3 — per-currency prices.

            ## Schema

            - **price_id**: Surrogate PK.
            - **course_id**: Logical FK to courses.course_id (no SQL FK).
            - **currency**: lowercase ISO 4217 (`cad`,`usd`,`eur`,`gbp`).
            - **unit_amount_cents**: Minor units (Stripe-compatible).
            - **stripe_price_id**: NULL until Stripe Products/Prices exist.
            - **is_active**: Soft flag; UNIQUE(course_id, currency) is one
              row per pair in v1 (replace rather than version history).

            Launch set: cad/usd/eur/gbp. Free courses need no rows (API
            synthesizes 0). Paid courses need at least active `cad` before
            published=1 (app invariant).
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Drops course_prices. Safe before reversing courses (1291).
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {"name": "price_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "course_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "currency", "datatype": "${VARCHAR_20}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "unit_amount_cents", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "stripe_price_id", "datatype": "${VARCHAR_100}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "is_active", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
