-- Migration: argent_2053.lua
-- QueryRef #2006 - Argent income and expense
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Install QueryRef 2006
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2053"
cfg.QUERY_REF = "2006"
cfg.QUERY_NAME = "Argent income and expense"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
    cfg.CAST_BIG = "signed"
    cfg.NULL_BIG = "CAST(NULL AS signed)"
    cfg.CURRENCY_TYPE = "char(20)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
    cfg.CAST_BIG = cfg.INTEGER_BIG
    cfg.NULL_BIG = "CAST(NULL AS " .. cfg.INTEGER_BIG .. ")"
    cfg.CURRENCY_TYPE = cfg.VARCHAR_20
end
cfg.NULL_CURRENCY = "CAST(NULL AS " .. cfg.CURRENCY_TYPE .. ")"
if engine == "firebird" then
    cfg.AMOUNT_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
    cfg.TAX_SUM = "CAST(COALESCE(SUM(ln.tax_cents), 0) AS BIGINT)"
    cfg.NET_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
    cfg.RUN_SUM = "CAST(SUM(m.amount_cents) OVER (ORDER BY m.txn_on, m.txn_id, m.line_seq ROWS UNBOUNDED PRECEDING) AS BIGINT)"
else
    cfg.AMOUNT_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
    cfg.TAX_SUM = "COALESCE(SUM(ln.tax_cents), 0)"
    cfg.NET_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
    cfg.RUN_SUM = "SUM(m.amount_cents) OVER (ORDER BY m.txn_on, m.txn_id, m.line_seq ROWS UNBOUNDED PRECEDING)"
end
if engine == "postgresql" or engine == "firebird" then
    cfg.FILE_TEXT = "LOWER(SUBSTRING(a.file_text FROM 1 FOR 240))"
elseif engine == "db2" then
    cfg.FILE_TEXT = "LOWER(SUBSTR(a.file_text, 1, 240))"
else
    cfg.FILE_TEXT = "LOWER(SUBSTRING(a.file_text, 1, 240))"
end

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
WITH req AS (
    SELECT
        CAST(:ORGANIZATION_ID AS ${CAST_INTEGER}) AS organization_id,
        CAST(:FROM_ON AS ${DATE}) AS from_on,
        CAST(:TO_ON AS ${DATE}) AS to_on,
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
sums AS (
    SELECT
        l.ledger_id,
        l.name,
        l.ledger_type_a2001,
        l.currency,
        l.organization_id,
        org.default_currency,
        ${AMOUNT_SUM} AS amount_cents
    FROM ${SCHEMA}ledgers l
    INNER JOIN req
        ON l.organization_id = req.organization_id
    INNER JOIN ${SCHEMA}organizations org
        ON org.organization_id = l.organization_id
    LEFT JOIN ${SCHEMA}transactions t
        ON t.organization_id = l.organization_id
       AND t.txn_on >= req.from_on
       AND t.txn_on <= req.to_on
       AND (
            CASE req.use_default
                WHEN 1 THEN CASE WHEN t.status_a2003 IN (3, 4) THEN 1 ELSE 0 END
                WHEN 0 THEN CASE WHEN t.status_a2003 IN (req.status_1, req.status_2, req.status_3, req.status_4, req.status_5) THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
    LEFT JOIN ${SCHEMA}lines ln
        ON ln.txn_id = t.txn_id
       AND ln.ledger_id = l.ledger_id
    WHERE l.is_posting = 1
      AND l.ledger_type_a2001 IN (4, 5)
    GROUP BY
        l.ledger_id,
        l.name,
        l.ledger_type_a2001,
        l.currency,
        l.organization_id,
        org.default_currency
),
direct_rates AS (
    SELECT
        s.ledger_id,
        r.rate_n,
        r.rate_d,
        r.as_of,
        ROW_NUMBER() OVER (
            PARTITION BY s.ledger_id
            ORDER BY r.as_of DESC, r.rate_id DESC
        ) AS rn
    FROM sums s
    INNER JOIN req
        ON req.organization_id = s.organization_id
    INNER JOIN ${SCHEMA}rates r
        ON r.base_currency = s.currency
       AND r.quote_currency = s.default_currency
       AND r.as_of <= req.to_on
       AND (
            CASE req.use_rate_default
                WHEN 1 THEN CASE WHEN r.source_a2012 = 1 THEN 1 ELSE 0 END
                WHEN 0 THEN CASE WHEN r.source_a2012 = req.rate_source THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
    WHERE s.currency <> s.default_currency
),
inverse_rates AS (
    SELECT
        s.ledger_id,
        r.rate_n,
        r.rate_d,
        r.as_of,
        ROW_NUMBER() OVER (
            PARTITION BY s.ledger_id
            ORDER BY r.as_of DESC, r.rate_id DESC
        ) AS rn
    FROM sums s
    INNER JOIN req
        ON req.organization_id = s.organization_id
    INNER JOIN ${SCHEMA}rates r
        ON r.base_currency = s.default_currency
       AND r.quote_currency = s.currency
       AND r.as_of <= req.to_on
       AND (
            CASE req.use_rate_default
                WHEN 1 THEN CASE WHEN r.source_a2012 = 1 THEN 1 ELSE 0 END
                WHEN 0 THEN CASE WHEN r.source_a2012 = req.rate_source THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
    WHERE s.currency <> s.default_currency
)
SELECT
    s.ledger_id,
    s.name,
    s.ledger_type_a2001,
    s.currency,
    s.default_currency,
    s.amount_cents,
    CASE
        WHEN s.currency = s.default_currency THEN 1
        WHEN s.amount_cents = 0 THEN NULL
        WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.rate_n
        WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.rate_d
        ELSE NULL
    END AS rate_n,
    CASE
        WHEN s.currency = s.default_currency THEN 1
        WHEN s.amount_cents = 0 THEN NULL
        WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.rate_d
        WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.rate_n
        ELSE NULL
    END AS rate_d,
    CASE
        WHEN s.currency = s.default_currency THEN NULL
        WHEN s.amount_cents = 0 THEN NULL
        WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN dir.as_of
        WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN inv.as_of
        ELSE NULL
    END AS rate_as_of,
    CASE
        WHEN s.currency = s.default_currency THEN s.amount_cents
        WHEN s.amount_cents = 0 THEN 0
        WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN s.amount_cents * dir.rate_n / NULLIF(dir.rate_d, 0)
        WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN s.amount_cents * inv.rate_d / NULLIF(inv.rate_n, 0)
        ELSE NULL
    END AS converted_cents,
    CASE
        WHEN s.currency = s.default_currency THEN NULL
        WHEN s.amount_cents = 0 THEN NULL
        WHEN dir.rate_n IS NOT NULL AND dir.rate_n <> 0 AND dir.rate_d <> 0 THEN NULL
        WHEN inv.rate_n IS NOT NULL AND inv.rate_n <> 0 AND inv.rate_d <> 0 THEN NULL
        ELSE 'missing'
    END AS rate_warning
FROM sums s
LEFT JOIN direct_rates dir
    ON dir.ledger_id = s.ledger_id
   AND dir.rn = 1
LEFT JOIN inverse_rates inv
    ON inv.ledger_id = s.ledger_id
   AND inv.rn = 1
ORDER BY s.ledger_type_a2001, s.ledger_id
;

                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
# QueryRef #2006 - Argent income and expense

Income and expense totals. A missing rate leaves converted_cents null and rate_warning missing.
Each parameter name appears once. An unused integer slot is 0.
Reverse deletes only the type SQL row. Bookkeeping rows that
already use query_ref 2006 stay.
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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF}

            Installs the report query. Caller-facing query_ref ${QUERY_REF},
            type SQL. Does not change an earlier bookkeeping row on that
            query_ref. Does not add a column.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
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
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF}

            Deletes only the type SQL row for query_ref ${QUERY_REF}.
            Bookkeeping rows on that query_ref stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
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
return queries end
