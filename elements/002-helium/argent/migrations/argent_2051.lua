-- Migration: argent_2051.lua
-- QueryRef #2004 - Argent ledger history
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Install QueryRef 2004
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2051"
cfg.QUERY_REF = "2004"
cfg.QUERY_NAME = "Argent ledger history"
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
        CAST(:LEDGER_ID AS ${CAST_INTEGER}) AS ledger_id,
        CAST(:FROM_ON AS ${DATE}) AS from_on,
        CAST(:TO_ON AS ${DATE}) AS to_on,
        CAST(:USE_DEFAULT AS ${CAST_INTEGER}) AS use_default,
        CAST(:STATUS_1 AS ${CAST_INTEGER}) AS status_1,
        CAST(:STATUS_2 AS ${CAST_INTEGER}) AS status_2,
        CAST(:STATUS_3 AS ${CAST_INTEGER}) AS status_3,
        CAST(:STATUS_4 AS ${CAST_INTEGER}) AS status_4,
        CAST(:STATUS_5 AS ${CAST_INTEGER}) AS status_5
    ${DUMMY_TABLE}
),
matched AS (
    SELECT
        t.txn_id,
        t.txn_on,
        t.description,
        t.status_a2003,
        t.kind_a2004,
        ln.line_id,
        ln.line_seq,
        ln.amount_cents,
        ln.memo,
        req.from_on,
        req.to_on
    FROM req
    INNER JOIN ${SCHEMA}lines ln
        ON ln.ledger_id = req.ledger_id
    INNER JOIN ${SCHEMA}transactions t
        ON t.txn_id = ln.txn_id
       AND (
            CASE req.use_default
                WHEN 1 THEN CASE WHEN t.status_a2003 IN (3, 4) THEN 1 ELSE 0 END
                WHEN 0 THEN CASE WHEN t.status_a2003 IN (req.status_1, req.status_2, req.status_3, req.status_4, req.status_5) THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
),
ranked AS (
    SELECT
        m.txn_id,
        m.txn_on,
        m.description,
        m.status_a2003,
        m.kind_a2004,
        m.line_id,
        m.line_seq,
        m.amount_cents,
        m.memo,
        m.from_on,
        m.to_on,
        ${RUN_SUM} AS running_cents
    FROM matched m
)
SELECT
    ranked.txn_id,
    ranked.txn_on,
    ranked.description,
    ranked.status_a2003,
    ranked.kind_a2004,
    ranked.line_id,
    ranked.line_seq,
    ranked.amount_cents,
    ranked.memo,
    ranked.running_cents
FROM ranked
WHERE ranked.txn_on >= ranked.from_on
  AND ranked.txn_on <= ranked.to_on
ORDER BY ranked.txn_on, ranked.txn_id, ranked.line_seq
;

                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
# QueryRef #2004 - Argent ledger history

Lines in the window and a running balance. The default status list is lookup 2003 keys 3 and 4.
Each parameter name appears once. An unused integer slot is 0.
Reverse deletes only the type SQL row. Bookkeeping rows that
already use query_ref 2004 stay.
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
