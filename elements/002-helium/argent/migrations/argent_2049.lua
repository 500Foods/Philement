-- Migration: argent_2049.lua
-- QueryRef #2002 - Argent due
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Install QueryRef 2002
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2049"
cfg.QUERY_REF = "2002"
cfg.QUERY_NAME = "Argent due"
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
        CAST(:FROM_ON AS ${DATE}) AS from_on,
        CAST(:TO_ON AS ${DATE}) AS to_on,
        CAST(:USE_ORG AS ${CAST_INTEGER}) AS use_org,
        CAST(:ORGANIZATION_ID AS ${CAST_INTEGER}) AS organization_id,
        CAST(:USE_DEFAULT AS ${CAST_INTEGER}) AS use_default,
        CAST(:STATUS_1 AS ${CAST_INTEGER}) AS status_1,
        CAST(:STATUS_2 AS ${CAST_INTEGER}) AS status_2,
        CAST(:STATUS_3 AS ${CAST_INTEGER}) AS status_3,
        CAST(:STATUS_4 AS ${CAST_INTEGER}) AS status_4,
        CAST(:STATUS_5 AS ${CAST_INTEGER}) AS status_5
    ${DUMMY_TABLE}
)
SELECT
    due_rows.row_kind,
    due_rows.organization_id,
    due_rows.entity_id,
    due_rows.name,
    due_rows.on_date,
    due_rows.end_on,
    due_rows.amount_cents,
    due_rows.currency,
    due_rows.status_key,
    due_rows.from_ledger_id,
    due_rows.to_ledger_id,
    due_rows.schedule_id,
    due_rows.kind_a2004
FROM (
    SELECT
        'schedule' AS row_kind,
        s.organization_id AS organization_id,
        s.schedule_id AS entity_id,
        s.name AS name,
        s.anchor_on AS on_date,
        s.end_on AS end_on,
        s.amount_cents AS amount_cents,
        CAST(s.currency AS ${CURRENCY_TYPE}) AS currency,
        s.status_a2007 AS status_key,
        s.from_ledger_id AS from_ledger_id,
        s.to_ledger_id AS to_ledger_id,
        s.schedule_id AS schedule_id,
        CAST(NULL AS ${CAST_INTEGER}) AS kind_a2004
    FROM ${SCHEMA}schedules s
    INNER JOIN req
        ON s.anchor_on <= req.to_on
       AND (s.end_on IS NULL OR s.end_on >= req.from_on)
       AND s.status_a2007 = 1
       AND (
            CASE req.use_org
                WHEN 0 THEN 1
                WHEN 1 THEN CASE WHEN s.organization_id = req.organization_id THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
    UNION ALL
    SELECT
        'transaction' AS row_kind,
        t.organization_id AS organization_id,
        t.txn_id AS entity_id,
        t.description AS name,
        t.txn_on AS on_date,
        CAST(NULL AS ${DATE}) AS end_on,
        ${NULL_BIG} AS amount_cents,
        ${NULL_CURRENCY} AS currency,
        t.status_a2003 AS status_key,
        CAST(NULL AS ${CAST_INTEGER}) AS from_ledger_id,
        CAST(NULL AS ${CAST_INTEGER}) AS to_ledger_id,
        t.schedule_id AS schedule_id,
        t.kind_a2004 AS kind_a2004
    FROM ${SCHEMA}transactions t
    INNER JOIN req
        ON t.txn_on >= req.from_on
       AND t.txn_on <= req.to_on
       AND (
            CASE req.use_default
                WHEN 1 THEN CASE WHEN t.status_a2003 = 1 THEN 1 ELSE 0 END
                WHEN 0 THEN CASE WHEN t.status_a2003 IN (req.status_1, req.status_2, req.status_3, req.status_4, req.status_5) THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
       AND (
            CASE req.use_org
                WHEN 0 THEN 1
                WHEN 1 THEN CASE WHEN t.organization_id = req.organization_id THEN 1 ELSE 0 END
                ELSE 0
            END
       ) = 1
) AS due_rows
ORDER BY due_rows.row_kind, due_rows.on_date, due_rows.entity_id
;

                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
# QueryRef #2002 - Argent due

Schedule rows and Reserved transactions.
Each parameter name appears once. An unused integer slot is 0.
Reverse deletes only the type SQL row. Bookkeeping rows that
already use query_ref 2002 stay.
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
