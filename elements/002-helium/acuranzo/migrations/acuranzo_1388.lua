-- Migration: acuranzo_1388.lua
-- QueryRef #158 - Transaction Fixture Cleanup
--
-- Verify + cleanup for the transaction-block fixtures (#156 and #157). Runs
-- after every fixture invocation: its SELECT proves the committed final
-- state of TEST_TRANSACTIONS, then it drops both fixture tables. See
-- docs/H/plans/TRANSACTION_PLAN.md (Phase 4).
--
-- The stored SQL is engine-neutral: there are no engine-specific branches,
-- because query_cache_lookup() matches by query_ref only.
--
-- Cleanup contract:
--   - The drops run one per transaction block, primary table first, so a
--     failing drop never rolls back an earlier successful one and no residue
--     is left whatever happened before.
--   - The staging drop is allowed to fail with a "does not exist" class
--     error: on engines with transactional DDL the failed block in #157 rolled
--     the staging table back, and whenever the staging table was never created
--     (after #156, or on a repeat cleanup) there is simply nothing to clean.
--     The blackbox harness tolerates exactly that case.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
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
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            SELECT txn_id, run_label, note
              FROM ${SCHEMA}TEST_TRANSACTIONS
             ORDER BY txn_id;

            -- TRANSACTION_START
            DROP TABLE ${SCHEMA}TEST_TRANSACTIONS;
            -- TRANSACTION_END

            -- TRANSACTION_START
            DROP TABLE ${SCHEMA}TEST_TXN_STAGING;
            -- TRANSACTION_END
        ]=]
                                                                             AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Verify and cleanup for the transaction-block fixtures
            (QueryRef #156 - Transaction Block Smoke and #157 - Transaction
            Block Stress). Runs after every fixture invocation.

            ## Behaviour

            1. A bare `SELECT` proves the committed final state of
               `TEST_TRANSACTIONS` (one row after #156, two rows after a
               successful #157, one row after #157's forced failure).
            2. A one-statement transaction block drops `TEST_TRANSACTIONS`.
            3. A one-statement transaction block drops `TEST_TXN_STAGING`.

            The response payload is the usual per-statement array; the entries
            for the two drops show which cleanup statements ran.

            ## Cleanup contract

            - The primary table is dropped first, so a failing staging drop
              can never roll back a successful primary drop: no residue is
              left whatever happened before this query ran.
            - The staging drop is allowed to fail with a "does not exist"
              class error. That is the observable of QueryRef #157's
              per-engine DDL semantics: after #157's forced failure the
              staging table exists on Firebird/MySQL/MariaDB (DDL committed at
              the DDL) and does not exist on PostgreSQL/YugabyteDB/SQLite/DB2/
              MS SQL Server (DDL rolled back with the block). It is likewise
              simply absent after #156, or when this query is repeated.
              Tolerate that failure; it is the design, not a defect.
            - When even the proof SELECT finds no table, this query fails with
              "no such table" / equivalent at the first entry: also nothing to
              clean.

            ## Parameters

            None.

            ## Returns

            - The proof SELECT's rows as the first entry, then one entry per
              drop.

            ## Tables

            - `${SCHEMA}TEST_TRANSACTIONS` (created by #156 / #157).
            - `${SCHEMA}TEST_TXN_STAGING` (created by #157).

            ## Security Notes

            - `query_type_a28` is `TYPE_SQL` (1), so the query is reachable
              only through the runtime endpoint `/api/conduit/alt_query`.
              It is a test fixture: it reads the two tables above and drops
              them.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

    ${SUBQUERY_DELIMITER}

    UPDATE ${SCHEMA}${QUERIES}
    SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
    WHERE query_ref = ${MIGRATION}
    AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #158 - Transaction Fixture Cleanup
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    DELETE FROM ${SCHEMA}${TABLE}
    WHERE query_ref = ${QUERY_REF};

    ${SUBQUERY_DELIMITER}

    UPDATE ${SCHEMA}${QUERIES}
    SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
    WHERE query_ref = ${MIGRATION}
    AND query_type_a28 = ${TYPE_APPLIED_MIGRATION};
]]})

return queries end
