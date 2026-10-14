-- Migration: acuranzo_1388.lua
-- QueryRef #158 - Transaction Fixture Cleanup
--
-- Installs the verify-and-cleanup QueryRef for #156 and #157. APPLY inserts
-- that QueryRef (type 1). It does not execute the marker SQL. See
-- docs/H/plans/TRANSACTION_PLAN.md.
--
-- The stored SQL is one text per engine, expanded by macros at generation.
-- There is no engine branch: query_cache_lookup() matches by query_ref only.
--
-- Cleanup contract:
--   - The proof SELECT runs first.
--   - Each drop is its own transaction block, primary table first, so a
--     failing staging drop cannot roll back a successful primary drop.
--   - The staging drop is allowed to fail with a missing-table error. That
--     is the observable of #157's per-engine DDL semantics, and it is also
--     what happens when the staging table was never created.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-10-14 - Nest the QueryRef insert inside the forward code, matching acuranzo_1381. The 1.0.0 file stored the marker SQL as the migration's own code.
-- 1.0.0 - 2026-10-14 - Initial creation for TRANSACTION_PLAN Phase 4.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1388"
cfg.QUERY_REF = "158"
cfg.QUERY_NAME = "Transaction Fixture Cleanup"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #158 - Transaction Fixture Cleanup
-- ----------------------------------------------------------------------------
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
        ${QTC_FAST}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT txn_id, run_label, note
                      FROM ${SCHEMA}TEST_TRANSACTIONS
                     ORDER BY txn_id;

                    -- TRANSACTION_START
                    DROP TABLE ${SCHEMA}TEST_TRANSACTIONS;
                    -- TRANSACTION_END

                    -- TRANSACTION_START
                    DROP TABLE ${SCHEMA}TEST_TXN_STAGING;
                    -- TRANSACTION_END
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Verify and cleanup for QueryRef #156 (Transaction Block
                    Smoke) and QueryRef #157 (Transaction Block Stress). Run
                    it after every fixture invocation.

                    ## Behaviour

                    1. A bare SELECT reads `TEST_TRANSACTIONS`. Expect one row
                       after #156, two rows after a successful #157, and one
                       row (txn_id 1 only) after #157's forced failure. Block
                       one of #157 commits before the failing block.
                    2. A one-statement transaction block drops
                       `TEST_TRANSACTIONS`.
                    3. A one-statement transaction block drops
                       `TEST_TXN_STAGING`.

                    The two drops are separate blocks so a failing staging
                    drop cannot roll back the primary drop.

                    ## The staging drop

                    A missing-table error on the staging drop is the design.

                    - After #157 with `SHOULD_FAIL = 1`, the staging table
                      exists on Firebird, MySQL, and MariaDB (DDL committed
                      at the DDL) and does not exist on PostgreSQL,
                      YugabyteDB, SQLite, DB2, and MS SQL Server (DDL rolled
                      back with the block).
                    - After #156, and on a repeat of this query once both
                      tables are gone, the staging table is absent.
                    - When the proof SELECT finds no primary table, this
                      query fails on that first statement and the drops do
                      not run.

                    ## Parameters

                    None.

                    ## Tables

                    - `${SCHEMA}TEST_TRANSACTIONS` (created by #156 / #157).
                    - `${SCHEMA}TEST_TXN_STAGING` (created by #157).

                    ## Security Notes

                    - `query_type_a28` is `TYPE_SQL` (1), so the query is
                      reachable through `/api/conduit/alt_query`. It is a
                      test fixture: it reads the two tables above and drops
                      them.
                ]==]
                                                                                     AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Inserts the cleanup QueryRef. Running it proves the fixture
            rows and drops TEST_TRANSACTIONS and TEST_TXN_STAGING.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #158 - Transaction Fixture Cleanup
-- ----------------------------------------------------------------------------
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Deletes the runtime QueryRef. It does not drop leftover fixture
            tables. Those exist only after the fixture QueryRefs have run.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
