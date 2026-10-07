-- Migration: argent_2014.lua
-- QueryRef #2000 - Argent balance

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Install QueryRef 2000, Argent balance

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2014"
cfg.QUERY_REF = "2000"
cfg.QUERY_NAME = "Argent balance"
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
                    SELECT
                        l.ledger_id,
                        l.name,
                        l.currency,
                        COALESCE(SUM(ln.amount_cents), 0) AS balance_cents
                    FROM ${SCHEMA}ledgers l
                    LEFT JOIN ${SCHEMA}transactions t
                        ON t.organization_id = l.organization_id
                       AND t.txn_on <= :AS_OF
                       AND (
                            CASE :USE_DEFAULT
                                WHEN 1 THEN CASE WHEN t.status_a2003 IN (3, 4) THEN 1 ELSE 0 END
                                WHEN 0 THEN CASE WHEN t.status_a2003 IN (:STATUS_1, :STATUS_2, :STATUS_3, :STATUS_4, :STATUS_5) THEN 1 ELSE 0 END
                                ELSE 0
                            END
                       ) = 1
                    LEFT JOIN ${SCHEMA}lines ln
                        ON ln.txn_id = t.txn_id
                       AND ln.ledger_id = l.ledger_id
                    WHERE l.organization_id = :ORGANIZATION_ID
                      AND l.is_posting = 1
                    GROUP BY l.ledger_id, l.name, l.currency
                    ORDER BY l.ledger_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Posting-ledger sums in each ledger currency.
                    One row per posting ledger. balance_cents is 0 when no line matches.
                    The sum is lines.amount_cents only. It does not add opening_balance_cents.
                    It does not filter ledger status or valid_until.
                    It does not reject an unbalanced transaction.

                    ## Parameters

                    Every call binds all eight names. Each name appears once.

                    - `ORGANIZATION_ID` (integer, required): Organization to sum.
                    - `AS_OF` (date, required): Include transactions on this date.
                    - `USE_DEFAULT` (integer, required): 1 selects lookup 2003 keys
                      3 and 4. 0 selects only the STATUS slots below. Any other
                      value, including null, matches no status.
                    - `STATUS_1` through `STATUS_5` (integer, always bound):
                      Lookup 2003 keys. Null is an unused slot. A null slot does
                      not match. Slots are ignored when USE_DEFAULT is 1, and
                      they are still bound.

                    ## Returns

                    - `ledger_id`, `name`, `currency`, `balance_cents`.

                    ## Tables

                    - `${SCHEMA}ledgers`, `${SCHEMA}transactions`, `${SCHEMA}lines`.

                    ## Security Notes

                    - `query_type_a28` is `TYPE_SQL` so Argent tools can call
                      this row by query_ref.
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

            Installs the posting balance query. Migration number ${MIGRATION}.
            Caller-facing query_ref ${QUERY_REF}, type SQL.
            Does not touch the organizations bookkeeping rows.
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF}
              AND query_type_a28 = ${TYPE_SQL};

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

            Deletes only the type SQL row for query_ref ${QUERY_REF}.
            argent_2000.lua stores organizations bookkeeping on the same
            query_ref with migration types. Those rows stay.
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
        'JSON query definition in collection'                               AS code,
        'Diagram QueryRef #${QUERY_REF}'                                    AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            QueryRef ${QUERY_REF}. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "query",
                        "object_id": "query.${QUERY_REF}",
                        "object_ref": "${MIGRATION}",
                        "query_ref": "${QUERY_REF}",
                        "name": "${QUERY_NAME}"
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
