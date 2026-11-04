-- Migration: acuranzo_1448.lua
-- QueryRef #174 - Reporting Demo (Reporting Phase 28)
--
-- Synthetic data for the report designer. No tables are read: the
-- rows are literals (integer, float, string, memo, datetime, boolean)
-- shaped by three typed parameters with defaults. query_type_a28 = 3
-- (SQL - Reporting) so it appears in QueryRef 173. collection.reporting
-- lists the returned fields and the parameters (types and defaults).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-11-03 - Timestamps built via REPLACE: Hydrogen read ':30' etc. in literals as binds
-- 1.0.0 - 2026-11-03 - Initial creation (Reporting Phase 28)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1448"
cfg.QUERY_REF = "174"
cfg.QUERY_NAME = "Reporting Demo"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_FLOAT = "double"
    cfg.CAST_TEXT = "char(100)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_FLOAT = cfg.FLOAT_BIG
    cfg.CAST_TEXT = cfg.VARCHAR_100
end
-- Engines that need a FROM on a literal SELECT
if engine == "db2" then
    cfg.DUMMY_FROM = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.DUMMY_FROM = "FROM RDB$DATABASE"
else
    cfg.DUMMY_FROM = ""
end

local sql_new = [[
                    SELECT d.row_id,
                           d.name,
                           d.amount * CAST(:SCALE AS ${CAST_FLOAT}) AS amount,
                           d.memo,
                           d.created_at,
                           d.active,
                           CAST(:TITLE AS ${CAST_TEXT}) AS title
                      FROM (
                            SELECT 1 AS row_id, 'Alpha' AS name, 1250.50000 AS amount, 'Short memo.' AS memo, REPLACE('2026-01-15 09|30|00', '|', ':') AS created_at, 1 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 2 AS row_id, 'Bravo' AS name, 89.99000 AS amount, 'A longer memo that wraps across more than one line when the box is narrow, to exercise memo layout.' AS memo, REPLACE('2026-02-28 14|05|10', '|', ':') AS created_at, 0 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 3 AS row_id, 'Charlie' AS name, 4300.00000 AS amount, 'Memo with punctuation: commas, periods; and quotes.' AS memo, REPLACE('2026-03-31 23|59|59', '|', ':') AS created_at, 1 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 4 AS row_id, 'Delta' AS name, -17.25000 AS amount, 'Negative amount.' AS memo, REPLACE('2026-06-01 00|00|00', '|', ':') AS created_at, 0 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 5 AS row_id, 'Echo' AS name, 0.00000 AS amount, 'Zero amount.' AS memo, REPLACE('2026-07-04 12|00|00', '|', ':') AS created_at, 1 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 6 AS row_id, 'Foxtrot' AS name, 999999.99000 AS amount, 'Large amount.' AS memo, REPLACE('2026-09-15 08|15|30', '|', ':') AS created_at, 1 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 7 AS row_id, 'Golf' AS name, 3.14159 AS amount, 'Many decimals.' AS memo, REPLACE('2026-10-31 18|45|00', '|', ':') AS created_at, 0 AS active ${DUMMY_FROM}
                            UNION ALL
                            SELECT 8 AS row_id, 'Hotel' AS name, 42.00000 AS amount, '' AS memo, REPLACE('2026-11-03 06|33|00', '|', ':') AS created_at, 1 AS active ${DUMMY_FROM}
                           ) d
                     WHERE d.row_id <= CAST(:ROW_LIMIT AS ${CAST_INTEGER})
                     ORDER BY d.row_id]]

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
                ${TYPE_REPORTING_SQL}                                               AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Synthetic rows for the report designer; reads no
                    table. Up to 8 rows: row_id integer, name string,
                    amount float, memo, created_at datetime (ISO text),
                    active boolean (1/0), title string.
                    INTEGER ROW_LIMIT (default 5) caps the rows.
                    FLOAT SCALE (default 1.0) multiplies amount.
                    STRING TITLE (default "Reporting Demo") is echoed
                    on every row. Each bind is used once.
                ]==]
                                                                                     AS summary,
                ${JSON_INGEST_START}[===[{"reporting": {"version": 1, "group": "Demo", "fields": [{"name": "row_id", "type": "integer", "kind": "field"}, {"name": "name", "type": "string", "kind": "field"}, {"name": "amount", "type": "float", "kind": "field"}, {"name": "memo", "type": "memo", "kind": "field"}, {"name": "created_at", "type": "datetime", "kind": "field"}, {"name": "active", "type": "boolean", "kind": "field"}, {"name": "title", "type": "string", "kind": "field"}], "parameters": [{"name": "ROW_LIMIT", "type": "integer", "default": 5, "kind": "field"}, {"name": "TITLE", "type": "string", "default": "Reporting Demo", "kind": "field"}, {"name": "SCALE", "type": "float", "default": 1.0, "kind": "field"}]}}]===]${JSON_INGEST_END}
                                                                                    AS collection,
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

            Inserts the synthetic Reporting demo query (SQL - Reporting).
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
        ${QTC_FAST}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF}
              AND query_type_a28 = ${TYPE_REPORTING_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF}'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF}

            Deletes the SQL - Reporting row only.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
