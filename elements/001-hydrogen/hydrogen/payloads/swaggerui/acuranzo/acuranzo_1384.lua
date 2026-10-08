-- Migration: acuranzo_1384.lua
-- UPDATE Lookup 030 - Query Dialect (key 6 Firebase → Firebird)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-09-18 - UPDATE Lookup 030 key 6: Firebase → Firebird. Do not edit 1383 in place.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1384"
cfg.LOOKUP_ID = "030"
cfg.LOOKUP_NAME = "Query Dialect"
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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Firebird',
                code = ${JIS}[==[{"icon":"<img src=\"assets/images/sql_dialect_firebird.png\" />"}]==]${JIE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = 6;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                             AS code,
        'Relabel Lookup ${LOOKUP_ID} key 6 from Firebase to Firebird in ${TABLE} table'  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Relabel Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Updates key 6 from 'Firebase' to 'Firebird' in Lookup 030 - Query Dialect.
            Key 5 (MS SQL Server) is unchanged. Does not edit 1383 in place.
            Icon changes from sql_dialect_firebase.png to sql_dialect_firebird.png.
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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Firebase',
                code = ${JIS}[==[{"icon":"<img src=\"assets/images/sql_dialect_firebase.png\" />"}]==]${JIE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = 6;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                             AS code,
        'Reverse Lookup ${LOOKUP_ID} key 6: Firebird → Firebase'             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Reverse Lookup ${LOOKUP_ID} key 6

            Reverts key 6 from 'Firebird' back to 'Firebase' in Lookup 030.
            Does not touch keys 0-5 seeded by 1055.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
