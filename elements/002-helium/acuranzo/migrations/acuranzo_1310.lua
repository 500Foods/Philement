-- Migration: acuranzo_1310.lua
-- Adds accounts.stripe_customer_id (STRIPE_PLAN Phase 1)
--
-- One Stripe Customer per Hydrogen account. Not a new users table.
-- Not on user_preferences (notification-only, FL-51) and not on
-- user_registration_meta (registration seed). Lua Stripe.EnsureCustomer
-- (Phase 2) writes this via os.getenv("STRIPE_SECRET_KEY") — never in git.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - ADD accounts.stripe_customer_id (STRIPE_PLAN Phase 1)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "accounts"
cfg.MIGRATION = "1310"
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
                ADD COLUMN stripe_customer_id ${VARCHAR_100};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add ${TABLE}.stripe_customer_id'                                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Add ${TABLE}.stripe_customer_id

            STRIPE_PLAN Phase 1 — store the Stripe Customer id on the
            account row (`cus_…`). NULL until `Stripe.EnsureCustomer`
            (Phase 2) creates or links one.

            ## Why accounts

            Plan says “account/prefs row, not a users table invented
            here”. `user_preferences` is notification flags only
            (acuranzo_1301 / FL-51). `user_registration_meta` is the
            Keycloak registration seed. Every authenticated user already
            has an `accounts` row.

            ## Notes

            - Nullable. Existing rows stay NULL.
            - No UNIQUE: multiple NULLs are not portable (DB2).
              One-customer-per-account is a Lua invariant.
            - No extra index: lookup is by `account_id` (PK).
            - No diagram (column add; same as acuranzo_1172 / 1297).
            - `VARCHAR_100` matches `course_prices.stripe_price_id`.
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
                DROP COLUMN stripe_customer_id;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE}.stripe_customer_id'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE}.stripe_customer_id

            Exact undo of the forward ADD. `REORG` before and after
            DROP (DB2 SQL0668N rc7).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
