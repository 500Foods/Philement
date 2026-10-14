-- Migration: acuranzo_1386.lua
-- QueryRef #156 - Transaction Block Smoke
--
-- Installs the first runtime QueryRef whose SQL carries transaction-block
-- markers. APPLY inserts that QueryRef (type 1). It does not execute the
-- marker SQL. The marker syntax and the runtime path are documented in
-- docs/H/plans/TRANSACTION_PLAN.md.
--
-- The stored SQL is one text per engine, expanded by macros at generation.
-- There is no engine branch: query_cache_lookup() matches by query_ref only.
-- Per-engine DDL semantics are asserted by blackbox test 64.
--
-- The fixture owns its table. TEST_TRANSACTIONS is created when the QueryRef
-- runs and dropped by QueryRef #158. This migration does not create or drop
-- that table. Always run #158 after an invocation.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-10-14 - Nest the QueryRef insert inside the forward code, matching acuranzo_1381. The 1.0.0 file stored the marker SQL as the migration's own code.
-- 1.0.0 - 2026-10-14 - Initial creation for TRANSACTION_PLAN Phase 4.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1386"
cfg.QUERY_REF = "156"
cfg.QUERY_NAME = "Transaction Block Smoke"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #156 - Transaction Block Smoke
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
                    VALUES (1, :RUN_LABEL, 'inserted inside the transaction block', ${COMMON_VALUES});
                    -- STATEMENT
                    UPDATE ${SCHEMA}TEST_TRANSACTIONS
                       SET note = 'updated inside the same transaction block'
                     WHERE txn_id = 1;
                    -- TRANSACTION_END

                    SELECT txn_id, run_label, note
                      FROM ${SCHEMA}TEST_TRANSACTIONS
                     ORDER BY txn_id;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    First runtime QueryRef whose SQL carries transaction-block
                    markers. See TRANSACTION_PLAN.md for the contract.

                    ## Behaviour

                    1. `CREATE TABLE TEST_TRANSACTIONS` runs as a bare
                       (autocommit) statement.
                    2. `-- TRANSACTION_START` opens one transaction: the INSERT
                       and the UPDATE commit together.
                    3. `-- TRANSACTION_END` closes the block.
                    4. The trailing SELECT runs as a bare statement.

                    The response `data_json` is an array with one entry per
                    executed statement. Each entry has `statement`, `block`,
                    `rows`, `affected`, `success`, and `error`. `statement`
                    restarts at 1 inside each block. Top-level `row_count` and
                    `affected_rows` are totals. On failure the top-level
                    message names the block and statement and says the open
                    segment rolled back. The engine's message is `error` on
                    the failing entry.

                    ## Parameters

                    - `RUN_LABEL` (string, required): stamped into the row.
                      Bound to the INSERT only.

                    ## Returns

                    - The verify SELECT's row as the last entry.
                    - On failure: entries collected so far, plus the failing
                      block and statement.

                    ## Tables

                    - `${SCHEMA}TEST_TRANSACTIONS` (created by this query,
                      dropped by QueryRef #158). No self-drop.

                    ## Security Notes

                    - `query_type_a28` is `TYPE_SQL` (1), so the query is
                      reachable through `/api/conduit/alt_query`. It is a
                      test fixture: it creates TEST_TRANSACTIONS, writes one
                      row, and reads it back.
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

            Inserts the transaction-block smoke QueryRef. Running that
            QueryRef creates TEST_TRANSACTIONS. QueryRef #158 drops it.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #156 - Transaction Block Smoke
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

            Deletes the runtime QueryRef. It does not drop TEST_TRANSACTIONS.
            That table is created only when the QueryRef runs, and QueryRef
            #158 drops it.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
