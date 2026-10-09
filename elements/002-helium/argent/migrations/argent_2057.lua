-- Migration: argent_2057.lua
-- QueryRef #2010 - Argent search
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Install QueryRef 2010
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "2057"
cfg.QUERY_REF = "2010"
cfg.QUERY_NAME = "Argent search"
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
        CAST(:Q AS ${CAST_TEXT}) AS q,
        CAST(:USE_TYPES AS ${CAST_INTEGER}) AS use_types,
        CAST(:TYPE_1 AS ${CAST_TEXT}) AS type_1,
        CAST(:TYPE_2 AS ${CAST_TEXT}) AS type_2,
        CAST(:TYPE_3 AS ${CAST_TEXT}) AS type_3,
        CAST(:TYPE_4 AS ${CAST_TEXT}) AS type_4,
        CAST(:TYPE_5 AS ${CAST_TEXT}) AS type_5,
        CAST(:TYPE_6 AS ${CAST_TEXT}) AS type_6,
        CAST(:TYPE_7 AS ${CAST_TEXT}) AS type_7
    ${DUMMY_TABLE}
),
hits AS (
    SELECT
        'organization' AS entity_type,
        o.organization_id AS entity_id,
        o.name AS label,
        o.organization_id AS organization_id
    FROM ${SCHEMA}organizations o, req
    WHERE LOWER(CAST(o.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'ledger' AS entity_type,
        l.ledger_id AS entity_id,
        l.name AS label,
        l.organization_id AS organization_id
    FROM ${SCHEMA}ledgers l, req
    WHERE LOWER(CAST(l.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'transaction' AS entity_type,
        t.txn_id AS entity_id,
        t.description AS label,
        t.organization_id AS organization_id
    FROM ${SCHEMA}transactions t, req
    WHERE LOWER(CAST(t.description AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'schedule' AS entity_type,
        s.schedule_id AS entity_id,
        s.name AS label,
        s.organization_id AS organization_id
    FROM ${SCHEMA}schedules s, req
    WHERE LOWER(CAST(s.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'contact' AS entity_type,
        c.contact_id AS entity_id,
        c.name AS label,
        l.organization_id AS organization_id
    FROM ${SCHEMA}contacts c, ${SCHEMA}ledgers l, req
    WHERE c.ledger_id = l.ledger_id
      AND LOWER(CAST(c.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'tag' AS entity_type,
        g.tag_id AS entity_id,
        g.name AS label,
        g.organization_id AS organization_id
    FROM ${SCHEMA}tags g, req
    WHERE LOWER(CAST(g.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
    UNION ALL
    SELECT
        'attachment' AS entity_type,
        a.attachment_id AS entity_id,
        a.name AS label,
        CAST(NULL AS ${CAST_INTEGER}) AS organization_id
    FROM ${SCHEMA}attachments a, req
    WHERE LOWER(CAST(a.name AS ${CAST_TEXT})) LIKE LOWER(req.q)
       OR CAST(${FILE_TEXT} AS ${CAST_TEXT}) LIKE LOWER(req.q)
)
SELECT
    hits.entity_type,
    hits.entity_id,
    hits.label,
    hits.organization_id
FROM hits
INNER JOIN req
    ON (
        CASE req.use_types
            WHEN 1 THEN 1
            WHEN 0 THEN CASE WHEN hits.entity_type IN (req.type_1, req.type_2, req.type_3, req.type_4, req.type_5, req.type_6, req.type_7) THEN 1 ELSE 0 END
            ELSE 0
        END
    ) = 1
ORDER BY hits.entity_type, hits.entity_id
;

                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
# QueryRef #2010 - Argent search

Names, descriptions, and the first 240 characters of file_text.
Each parameter name appears once. An unused integer slot is 0.
Reverse deletes only the type SQL row. Bookkeeping rows that
already use query_ref 2010 stay.
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
