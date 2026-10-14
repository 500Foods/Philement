-- Migration: acuranzo_1387.lua
-- QueryRef #157 - Transaction Block Stress
--
-- Installs the stress QueryRef: several transaction blocks, DDL before,
-- between, and inside blocks, and a parameter-driven mid-block failure.
-- APPLY inserts that QueryRef (type 1). It does not execute the marker SQL.
-- See docs/H/plans/TRANSACTION_PLAN.md.
--
-- The stored SQL is one text per engine, expanded by macros at generation.
-- There is no engine branch: query_cache_lookup() matches by query_ref only.
-- Engines differ only in how they treat DDL inside a block. QueryRef #158
-- makes that difference observable.
--
-- TEST_TRANSACTIONS and TEST_TXN_STAGING are created when this QueryRef runs
-- and dropped by QueryRef #158. This migration does not create or drop them.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-10-14 - Nest the QueryRef insert inside the forward code, matching acuranzo_1381. Index name is unqualified, matching other Acuranzo CREATE INDEX statements. The 1.0.0 file stored the marker SQL as the migration's own code.
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

                    CREATE INDEX TEST_TRANSACTIONS_idx_label
                        ON ${SCHEMA}TEST_TRANSACTIONS (run_label);

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
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Proves the transaction-block behaviour QueryRef #156 cannot:
                    several blocks, DDL before, between, and inside blocks, and
                    a forced mid-block failure.

                    ## Behaviour

                    1. `CREATE TABLE TEST_TRANSACTIONS` — bare statement, DDL
                       before the blocks.
                    2. Block one inserts the first row and commits.
                    3. `CREATE INDEX TEST_TRANSACTIONS_idx_label` — bare
                       statement, DDL between the blocks. The index name is
                       unqualified.
                    4. Block two creates `TEST_TXN_STAGING`, inserts one staged
                       row, then inserts the second primary row.
                    5. The trailing SELECT reads the primary table. It does not
                       run when block two fails.

                    `statement` restarts at 1 inside each block. A failure's
                    top-level message names the block and statement and says
                    the open segment rolled back. The engine's message is
                    `error` on the failing entry.

                    ## The forced failure

                    Block two's last statement inserts
                    `CASE WHEN :SHOULD_FAIL = 1 THEN 1 ELSE 2 END` as `txn_id`.
                    Row 1 already exists, so `SHOULD_FAIL = 1` violates the
                    primary key and block two rolls back. `SHOULD_FAIL = 0`
                    inserts row 2 and every block commits.

                    Block one has already committed. After the forced failure
                    the primary table still holds row 1 and does not hold row 2.

                    ## Per-engine DDL semantics

                    `TEST_TXN_STAGING` is created inside block two:

                    - PostgreSQL, YugabyteDB, SQLite, DB2, and MS SQL Server
                      keep DDL in the transaction, so the staging table is
                      rolled back with the block.
                    - Firebird, MySQL, and MariaDB commit at the DDL, so the
                      empty staging table survives the rollback. The staged
                      row does not: it was inserted after the DDL, in the
                      segment that rolls back.

                    QueryRef #158's drop of `TEST_TXN_STAGING` is the
                    observable. It fails with a missing-table error where DDL
                    rolled back, and succeeds where DDL committed.

                    ## Parameters

                    - `RUN_LABEL` (string, required): stamped into the rows.
                    - `SHOULD_FAIL` (integer, required): 0 commits block two;
                      1 forces the duplicate-key failure.

                    ## Tables

                    - `${SCHEMA}TEST_TRANSACTIONS` (created here, dropped by #158).
                    - `${SCHEMA}TEST_TXN_STAGING` (created inside block two,
                      dropped by #158). No self-drop on either path.

                    ## Security Notes

                    - `query_type_a28` is `TYPE_SQL` (1), so the query is
                      reachable through `/api/conduit/alt_query`. It is a
                      test fixture.
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

            Inserts the transaction-block stress QueryRef. Running that
            QueryRef creates TEST_TRANSACTIONS and TEST_TXN_STAGING.
            QueryRef #158 drops both.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #157 - Transaction Block Stress
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

            Deletes the runtime QueryRef. It does not drop TEST_TRANSACTIONS
            or TEST_TXN_STAGING. Those tables are created only when the
            QueryRef runs, and QueryRef #158 drops them.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
