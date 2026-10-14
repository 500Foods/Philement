-- Migration: acuranzo_1387.lua
-- QueryRef #157 - Transaction Block Stress
--
-- Proves the transaction-block behaviour #156 cannot: several blocks, DDL
-- before/between/inside blocks, a parameter-driven forced mid-block failure
-- (rollback proof), and the per-engine DDL semantics documented in
-- docs/H/plans/TRANSACTION_PLAN.md (Phase 4).
--
-- The stored SQL is engine-neutral: there are no engine-specific branches,
-- because query_cache_lookup() matches by query_ref only. The engines differ
-- only in how each one treats DDL inside a transaction block, and QueryRef
-- #158 makes that difference observable without any branching here.
--
-- The fixture owns its tables: TEST_TRANSACTIONS is created here, and
-- TEST_TXN_STAGING is created inside the block that can fail. Both are left
-- for QueryRef #158 (Transaction Fixture Cleanup). Always run #158 after an
-- invocation; it tolerates the nothing-to-clean case.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-14 - Initial creation for TRANSACTION_PLAN Phase 4.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1387"
cfg.QUERY_REF = "157"
cfg.QUERY_NAME = "Transaction Block Stress"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #157 - Transaction Block Stress
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
            CREATE TABLE ${SCHEMA}TEST_TRANSACTIONS
            (
                txn_id                          ${INTEGER}          NOT NULL,
                run_label                       ${VARCHAR_50}       NOT NULL,
                note                            ${TEXT}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(txn_id)
            );

            -- TRANSACTION_START
            INSERT INTO ${SCHEMA}TEST_TRANSACTIONS (txn_id, run_label, note, ${COMMON_FIELDS})
            VALUES (1, :RUN_LABEL, 'row from block one', ${COMMON_VALUES});
            -- TRANSACTION_END

            CREATE INDEX ${SCHEMA}TEST_TRANSACTIONS_idx_label ON ${SCHEMA}TEST_TRANSACTIONS (run_label);

            -- TRANSACTION_START
            CREATE TABLE ${SCHEMA}TEST_TXN_STAGING
            (
                stage_id                        ${INTEGER}          NOT NULL,
                run_label                       ${VARCHAR_50}       NOT NULL,
                note                            ${TEXT}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(stage_id)
            );
            -- STATEMENT
            INSERT INTO ${SCHEMA}TEST_TXN_STAGING (stage_id, run_label, note, ${COMMON_FIELDS})
            VALUES (1, :RUN_LABEL, 'staged row', ${COMMON_VALUES});
            -- STATEMENT
            INSERT INTO ${SCHEMA}TEST_TRANSACTIONS (txn_id, run_label, note, ${COMMON_FIELDS})
            VALUES (CASE WHEN :SHOULD_FAIL = 1 THEN 1 ELSE 2 END, :RUN_LABEL, 'row from block two', ${COMMON_VALUES});
            -- TRANSACTION_END

            SELECT txn_id, run_label, note
              FROM ${SCHEMA}TEST_TRANSACTIONS
             ORDER BY txn_id;
        ]=]
                                                                             AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Proves the transaction-block behaviour that QueryRef #156 cannot:
            several blocks, DDL before/between/inside blocks, and a forced
            mid-block failure.

            ## Behaviour

            1. `CREATE TABLE TEST_TRANSACTIONS` - bare statement (DDL before
               the blocks), autocommitted like any unmarked runtime query.
            2. Block 1 inserts the first row in its own transaction.
            3. `CREATE INDEX` - bare statement (DDL between the blocks).
            4. Block 2 creates `TEST_TXN_STAGING`, stages one row, then
               inserts the second primary row.
            5. The trailing verify SELECT reads the primary table.

            ## The forced failure

            Block 2's last statement inserts
            `CASE WHEN :SHOULD_FAIL = 1 THEN 1 ELSE 2 END` as txn_id. Row 1
            already exists, so with `SHOULD_FAIL = 1` that insert violates the
            primary key, block 2 rolls back, and the payload reports the
            failing block and statement plus the engine's message. With
            `SHOULD_FAIL = 0` it inserts row 2 and every block commits.

            ## Per-engine DDL semantics

            `TEST_TXN_STAGING` is created inside block 2, so a rollback of
            block 2 takes different paths per engine:

            - PostgreSQL, YugabyteDB, SQLite, DB2 and MS SQL Server treat DDL
              as transactional, so the staging table is rolled back with the
              block and does not exist afterwards.
            - Firebird, MySQL and MariaDB commit at the DDL, so the empty
              staging table survives the rollback.

            QueryRef #158's drop of `TEST_TXN_STAGING` is the observable: it
            fails with a "does not exist" class error on the transactional-DDL
            engines and succeeds on the commit-at-DDL engines.

            ## Parameters

            - `RUN_LABEL` (string, required): stamped into the rows so the
              caller can prove which invocation wrote them.
            - `SHOULD_FAIL` (integer, required): 0 commits block 2; 1 forces
              the duplicate-key failure that rolls it back.

            ## Returns

            - On success: one entry per executed statement, ending with the
              verify SELECT's rows.
            - On failure: everything collected so far, plus the failing
              block/statement numbers and the engine's message.

            ## Tables

            - `${SCHEMA}TEST_TRANSACTIONS` (created here, dropped by #158).
            - `${SCHEMA}TEST_TXN_STAGING` (created inside block 2, dropped by
              #158). No self-drop on either path.

            ## Security Notes

            - `query_type_a28` is `TYPE_SQL` (1), so the query is reachable
              only through the runtime endpoint `/api/conduit/alt_query`.
              It is a test fixture: it creates and writes the two tables
              above and reads them back.
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
-- Reverse: Remove QueryRef #157 - Transaction Block Stress
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
