-- Migration: acuranzo_1386.lua
-- QueryRef #156 - Transaction Block Smoke
--
-- First runtime QueryRef whose SQL carries transaction-block markers. The
-- marker syntax and the runtime path that executes it are documented in
-- docs/H/plans/TRANSACTION_PLAN.md (Phases 1-3).
--
-- The stored SQL is engine-neutral: there are no engine-specific branches,
-- because query_cache_lookup() matches by query_ref only. Per-engine DDL
-- semantics are asserted by the blackbox test (test 64), not by branching
-- in this file.
--
-- The fixture owns its table: TEST_TRANSACTIONS is created by this query and
-- dropped by QueryRef #158 (Transaction Fixture Cleanup). Always run #158
-- after an invocation; it tolerates the nothing-to-clean case.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
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
            VALUES (1, :RUN_LABEL, 'inserted inside the transaction block', ${COMMON_VALUES});
            -- STATEMENT
            UPDATE ${SCHEMA}TEST_TRANSACTIONS
               SET note = 'updated inside the same transaction block'
             WHERE txn_id = 1;
            -- TRANSACTION_END

            SELECT txn_id, run_label, note
              FROM ${SCHEMA}TEST_TRANSACTIONS
             ORDER BY txn_id;
        ]=]
                                                                             AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # QueryRef #${QUERY_REF} - ${QUERY_NAME}

            First runtime QueryRef whose SQL carries transaction-block markers
            (see TRANSACTION_PLAN.md for the full contract).

            ## Behaviour

            1. `CREATE TABLE` runs as a bare (autocommit) statement, exactly
               as any unmarked runtime query would run it.
            2. `-- TRANSACTION_START` opens a transaction block: the INSERT
               and the UPDATE run as one transaction and commit together.
            3. `-- TRANSACTION_END` closes the block.
            4. The trailing SELECT runs as a bare statement after the block.

            The response payload is an array with one entry per executed
            statement (`statement`, `block`, `rows`, `affected`, `success`,
            `error`); the top-level row_count and affected_rows are totals
            over all entries. Unmarked QueryRefs keep their existing
            single-object payload.

            ## Parameters

            - `RUN_LABEL` (string, required): stamped into the row so the
              caller can prove which invocation wrote it. Bound to the INSERT
              only; the validator checks the union of all names and each
              statement is bound only the names it references.

            ## Returns

            - The verify SELECT's rows as the last entry of the array.
            - On failure: everything collected so far, plus the failing
              statement and block numbers and the engine's message.

            ## Tables

            - `${SCHEMA}TEST_TRANSACTIONS` (created by this query, dropped by
              QueryRef #158). No self-drop: the table is left for #158.

            ## Security Notes

            - `query_type_a28` is `TYPE_SQL` (1), so the query is reachable
              only through the runtime endpoint `/api/conduit/alt_query`.
              It is a test fixture: it creates and writes TEST_TRANSACTIONS
              and reads it back.
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
-- Reverse: Remove QueryRef #156 - Transaction Block Smoke
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
