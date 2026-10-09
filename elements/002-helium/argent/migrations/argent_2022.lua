-- Migration: argent_2022.lua
-- QueryRef #2001 - Argent rollup

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Install QueryRef 2001, Argent rollup
-- 1.0.1 - 2026-10-07 - Cast the req parameters; PostgreSQL sends them as text
-- 1.0.2 - 2026-10-07 - DB2 stores WITH, the same keyword MSSQL uses
-- 1.0.3 - 2026-10-07 - DB2 recursive member uses comma joins; Firebird SUM is BIGINT

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2022"
cfg.QUERY_REF = "2001"
cfg.QUERY_NAME = "Argent rollup"
if engine == "mssql" or engine == "db2" then
    cfg.WITH_RECURSIVE = "WITH"
else
    cfg.WITH_RECURSIVE = "WITH RECURSIVE"
end
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end
if engine == "firebird" then
    cfg.BALANCE_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
else
    cfg.BALANCE_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
end
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
                    ${WITH_RECURSIVE}
                    req AS (
                        SELECT
                            CAST(:ORGANIZATION_ID AS ${CAST_INTEGER}) AS organization_id,
                            CAST(:AS_OF AS ${DATE}) AS as_of,
                            CAST(:USE_DEFAULT AS ${CAST_INTEGER}) AS use_default,
                            CAST(:STATUS_1 AS ${CAST_INTEGER}) AS status_1,
                            CAST(:STATUS_2 AS ${CAST_INTEGER}) AS status_2,
                            CAST(:STATUS_3 AS ${CAST_INTEGER}) AS status_3,
                            CAST(:STATUS_4 AS ${CAST_INTEGER}) AS status_4,
                            CAST(:STATUS_5 AS ${CAST_INTEGER}) AS status_5,
                            CAST(:USE_RATE_DEFAULT AS ${CAST_INTEGER}) AS use_rate_default,
                            CAST(:RATE_SOURCE AS ${CAST_INTEGER}) AS rate_source
                        ${DUMMY_TABLE}
                    ),
                    descendants (
                        parent_ledger_id,
                        parent_name,
                        parent_currency,
                        child_ledger_id,
                        child_name,
                        child_currency,
                        child_is_posting,
                        organization_id,
                        walk_depth
                    ) AS (
                        SELECT
                            p.ledger_id,
                            p.name,
                            p.currency,
                            c.ledger_id,
                            c.name,
                            c.currency,
                            c.is_posting,
                            p.organization_id,
                            1
                        FROM ${SCHEMA}ledgers p, ${SCHEMA}ledgers c, req
                        WHERE c.parent_id = p.ledger_id
                          AND c.organization_id = p.organization_id
                          AND p.organization_id = req.organization_id
                          AND p.is_posting = 0
                        UNION ALL
                        SELECT
                            d.parent_ledger_id,
                            d.parent_name,
                            d.parent_currency,
                            c.ledger_id,
                            c.name,
                            c.currency,
                            c.is_posting,
                            d.organization_id,
                            d.walk_depth + 1
                        FROM descendants d, ${SCHEMA}ledgers c
                        WHERE c.parent_id = d.child_ledger_id
                          AND c.organization_id = d.organization_id
                          AND d.child_is_posting = 0
                          AND d.walk_depth < 16
                    ),
                    posting_pairs AS (
                        SELECT DISTINCT
                            parent_ledger_id,
                            parent_name,
                            parent_currency,
                            child_ledger_id,
                            child_name,
                            child_currency,
                            organization_id
                        FROM descendants
                        WHERE child_is_posting = 1
                    ),
                    posting AS (
                        SELECT
                            d.parent_ledger_id,
                            d.parent_name,
                            d.parent_currency,
                            d.child_ledger_id,
                            d.child_name,
                            d.child_currency,
                            d.organization_id,
                            ${BALANCE_SUM} AS balance_cents
                        FROM posting_pairs d
                        INNER JOIN req
                            ON req.organization_id = d.organization_id
                        LEFT JOIN ${SCHEMA}transactions t
                            ON t.organization_id = d.organization_id
                           AND t.txn_on <= req.as_of
                           AND (
                                CASE req.use_default
                                    WHEN 1 THEN CASE WHEN t.status_a2003 IN (3, 4) THEN 1 ELSE 0 END
                                    WHEN 0 THEN CASE WHEN t.status_a2003 IN (req.status_1, req.status_2, req.status_3, req.status_4, req.status_5) THEN 1 ELSE 0 END
                                    ELSE 0
                                END
                           ) = 1
                        LEFT JOIN ${SCHEMA}lines ln
                            ON ln.txn_id = t.txn_id
                           AND ln.ledger_id = d.child_ledger_id
                        GROUP BY
                            d.parent_ledger_id,
                            d.parent_name,
                            d.parent_currency,
                            d.child_ledger_id,
                            d.child_name,
                            d.child_currency,
                            d.organization_id
                    ),
                    direct_rates AS (
                        SELECT
                            p.parent_ledger_id,
                            p.child_ledger_id,
                            r.rate_n,
                            r.rate_d,
                            r.as_of,
                            ROW_NUMBER() OVER (
                                PARTITION BY p.parent_ledger_id, p.child_ledger_id
                                ORDER BY r.as_of DESC, r.rate_id DESC
                            ) AS rn
                        FROM posting p
                        INNER JOIN req
                            ON req.organization_id = p.organization_id
                        INNER JOIN ${SCHEMA}rates r
                            ON r.base_currency = p.child_currency
                           AND r.quote_currency = p.parent_currency
                           AND r.as_of <= req.as_of
                           AND (
                                CASE req.use_rate_default
                                    WHEN 1 THEN CASE WHEN r.source_a2012 = 1 THEN 1 ELSE 0 END
                                    WHEN 0 THEN CASE WHEN r.source_a2012 = req.rate_source THEN 1 ELSE 0 END
                                    ELSE 0
                                END
                           ) = 1
                        WHERE p.child_currency <> p.parent_currency
                    ),
                    inverse_rates AS (
                        SELECT
                            p.parent_ledger_id,
                            p.child_ledger_id,
                            r.rate_n,
                            r.rate_d,
                            r.as_of,
                            ROW_NUMBER() OVER (
                                PARTITION BY p.parent_ledger_id, p.child_ledger_id
                                ORDER BY r.as_of DESC, r.rate_id DESC
                            ) AS rn
                        FROM posting p
                        INNER JOIN req
                            ON req.organization_id = p.organization_id
                        INNER JOIN ${SCHEMA}rates r
                            ON r.base_currency = p.parent_currency
                           AND r.quote_currency = p.child_currency
                           AND r.as_of <= req.as_of
                           AND (
                                CASE req.use_rate_default
                                    WHEN 1 THEN CASE WHEN r.source_a2012 = 1 THEN 1 ELSE 0 END
                                    WHEN 0 THEN CASE WHEN r.source_a2012 = req.rate_source THEN 1 ELSE 0 END
                                    ELSE 0
                                END
                           ) = 1
                        WHERE p.child_currency <> p.parent_currency
                    )
                    SELECT
                        p.parent_ledger_id,
                        p.parent_name,
                        p.parent_currency,
                        p.child_ledger_id,
                        p.child_name,
                        p.child_currency,
                        p.balance_cents,
                        CASE
                            WHEN p.child_currency = p.parent_currency THEN 1
                            WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.rate_n
                            WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.rate_n
                            ELSE COALESCE(dir.rate_n, inv.rate_n)
                        END AS rate_n,
                        CASE
                            WHEN p.child_currency = p.parent_currency THEN 1
                            WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.rate_d
                            WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.rate_d
                            ELSE COALESCE(dir.rate_d, inv.rate_d)
                        END AS rate_d,
                        CASE
                            WHEN p.child_currency = p.parent_currency THEN NULL
                            WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.as_of
                            WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.as_of
                            ELSE COALESCE(dir.as_of, inv.as_of)
                        END AS rate_as_of,
                        CASE
                            WHEN p.child_currency = p.parent_currency THEN p.balance_cents
                            WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN p.balance_cents * dir.rate_n / NULLIF(dir.rate_d, 0)
                            WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN p.balance_cents * inv.rate_d / NULLIF(inv.rate_n, 0)
                            ELSE NULL
                        END AS converted_cents,
                        CASE
                            WHEN p.child_currency = p.parent_currency THEN 0
                            WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN 0
                            WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN 0
                            ELSE 1
                        END AS rate_warning
                    FROM posting p
                    LEFT JOIN direct_rates dir
                        ON dir.parent_ledger_id = p.parent_ledger_id
                       AND dir.child_ledger_id = p.child_ledger_id
                       AND dir.rn = 1
                    LEFT JOIN inverse_rates inv
                        ON inv.parent_ledger_id = p.parent_ledger_id
                       AND inv.child_ledger_id = p.child_ledger_id
                       AND inv.rn = 1
                    ORDER BY p.parent_ledger_id, p.child_ledger_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    One row per posting descendant under each non-posting ancestor.
                    A leaf appears under the intermediate parent and under the root.
                    A parent with no posting descendant returns no row.
                    balance_cents follows QueryRef 2000: lines.amount_cents only,
                    0 when no line matches. It does not add opening_balance_cents.
                    It does not filter ledger status or valid_until.
                    It does not filter rate valid_until.
                    It does not fetch a rate. Fetch is a later tool.

                    Same currency: rate_n 1, rate_d 1, rate_as_of null,
                    converted_cents = balance_cents, rate_warning 0.
                    No rates row is required.
                    Different currency: newest rates row for the selected source
                    with as_of on or before the requested date. Direct is
                    base = child currency and quote = parent currency, and
                    converted_cents = balance_cents * rate_n / rate_d.
                    Inverse is the swapped pair, and converted_cents =
                    balance_cents * rate_d / rate_n. Direct wins when both exist.
                    Integer division truncates toward zero.
                    A missing rate, or a rate_n or rate_d of 0, leaves
                    converted_cents null and sets rate_warning to 1.
                    The walk stops at depth 16. One row per parent and child
                    is kept, so a parent_id cycle does not multiply the sum.

                    ## Parameters

                    Every call binds all ten names. Each name appears once.

                    - `ORGANIZATION_ID` (integer, required): Organization to roll up.
                    - `AS_OF` (date, required): Include transactions on this date,
                      and rates on or before this date.
                    - `USE_DEFAULT` (integer, required): 1 selects lookup 2003 keys
                      3 and 4. 0 selects only the STATUS slots below. Any other
                      value, including null, matches no status.
                    - `STATUS_1` through `STATUS_5` (integer, always bound):
                      Lookup 2003 keys. Null is an unused slot. A null slot does
                      not match. Slots are ignored when USE_DEFAULT is 1, and
                      they are still bound.
                    - `USE_RATE_DEFAULT` (integer, required): 1 selects lookup 2012
                      key 1 (boc). 0 selects RATE_SOURCE. Any other value,
                      including null, matches no source.
                    - `RATE_SOURCE` (integer, always bound): Lookup 2012 key.
                      Ignored when USE_RATE_DEFAULT is 1, and still bound.

                    ## Returns

                    - `parent_ledger_id`, `parent_name`, `parent_currency`.
                    - `child_ledger_id`, `child_name`, `child_currency`.
                    - `balance_cents`, `rate_n`, `rate_d`, `rate_as_of`.
                    - `converted_cents`, `rate_warning`.

                    ## Tables

                    - `${SCHEMA}ledgers`, `${SCHEMA}transactions`, `${SCHEMA}lines`,
                      `${SCHEMA}rates`.

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

            Installs the parent rollup query. Migration number ${MIGRATION}.
            Caller-facing query_ref ${QUERY_REF}, type SQL.
            Does not touch the ledgers bookkeeping rows.
            Does not fetch BoC. DB2 and MSSQL store WITH. Other engines
            store WITH RECURSIVE. The descendant walk uses comma joins
            because DB2 rejects JOIN ON inside a recursive CTE.
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
            argent_2001.lua stores ledgers bookkeeping on the same
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
