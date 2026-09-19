-- Migration: acuranzo_1311.lua
-- Creates the orders table (STRIPE_PLAN Phase 1)
--
-- One row per checkout attempt. Filled by Stripe.Checkout (Phase 5);
-- flipped by Stripe.Webhook / Enroll.PaidCourse (Phase 7–8).
-- No Hydrogen C. Currency is VARCHAR_20 lowercase (no CHAR_3 macro).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Create orders (STRIPE_PLAN Phase 1)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "orders"
cfg.MIGRATION = "1311"
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
                order_id                ${INTEGER}          NOT NULL,
                order_number            ${VARCHAR_50}       NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                stripe_intent_id        ${VARCHAR_100}              ,
                status                  ${VARCHAR_20}       NOT NULL DEFAULT 'pending',
                currency                ${VARCHAR_20}       NOT NULL,
                total_cents             ${INTEGER}          NOT NULL,
                items_json              ${JSON}             NOT NULL,
                idempotency_key         ${VARCHAR_128}      NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(order_id),
                ${UNIQUE}(order_number),
                ${UNIQUE}(idempotency_key)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_account
                ON ${SCHEMA}${TABLE}(account_id);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_intent
                ON ${SCHEMA}${TABLE}(stripe_intent_id);

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

            STRIPE_PLAN Phase 1 — checkout / PaymentIntent ledger.

            Empty after APPLY. `Stripe.Checkout` (Phase 5) INSERTs
            `pending`. `Stripe.Webhook` / `Enroll.PaidCourse`
            (Phase 7–8) flip `completed` / `failed`.
            `user_enrollments.order_id` (acuranzo_1307) is the
            logical FK (no SQL FK).

            ## Schema

            - **order_id**: Surrogate PK (MAX+1 in Lua).
            - **order_number**: Human-facing unique handle (support).
            - **account_id**: Hydrogen `accounts.account_id` (JWT
              `user_id`). Logical FK — no SQL FK.
            - **stripe_intent_id**: Stripe PaymentIntent `pi_…`.
              NULL until the PI is created. No UNIQUE (NULL
              portability); uniqueness is a Lua invariant. Index
              for webhook lookup.
            - **status**: `pending` | `completed` | `failed`.
              Default `pending`.
            - **currency**: lowercase ISO 4217 (`cad`/`usd`/`eur`/`gbp`),
              same as `course_prices`. Not CHAR(3) — no `CHAR_3`
              macro; do not use `total_cents_cad`.
            - **total_cents**: Settlement minor units (Lua re-reads
              `course_prices`; do not trust client cents).
            - **items_json**: Checkout line snapshot.
            - **idempotency_key**: Stripe Idempotency-Key
              (`checkout:{account_id}:…`). UNIQUE.

            `COMMON_CREATE` supplies audit timestamps — do **not**
            add a second domain `created_at` (FL-29c).

            ## Indexes

            - PRIMARY KEY on `order_id`.
            - UNIQUE on `order_number`.
            - UNIQUE on `idempotency_key`.
            - INDEX `(account_id)` — account order history.
            - INDEX `(stripe_intent_id)` — webhook idempotency.

            Additive. Existing `accounts` / `user_enrollments` /
            `course_prices` rows are not touched.
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

            Exact undo of the forward CREATE. Indexes drop with the
            table. Production rollback is not automated.
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

            This is the JSON Diagram code for the ${TABLE} table.
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
                            {
                                "name": "order_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "order_number",
                                "datatype": "${VARCHAR_50}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "stripe_intent_id",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "currency",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "total_cents",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "items_json",
                                "datatype": "${JSON}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "idempotency_key",
                                "datatype": "${VARCHAR_128}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
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
