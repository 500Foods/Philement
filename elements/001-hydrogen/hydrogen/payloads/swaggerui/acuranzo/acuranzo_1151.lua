-- Migration: acuranzo_1151.lua
-- QueryRef #057 - Query Params Test

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.3.0 - 2026-01-22 - Revert CAST for text types, investigate DB2 untyped parameter issue

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1151"
cfg.QUERY_REF = "057"
cfg.QUERY_NAME = "Query Params Test"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine ~= 'mysql' then table.insert(queries,{sql=[[
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
                ${TYPE_PUBLIC}                                                      AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        CAST(:INTEGER AS ${INTEGER}) as integer_test,
                        CAST(:STRING AS ${CHAR_20}) as string_test,
                        CASE WHEN :BOOLEAN THEN 1 ELSE 0 END as boolean_test,
                        CAST(:FLOAT AS ${FLOAT}) as float_test,
                        CAST(:TEXT AS ${CHAR_50}) as text_test,
                        CAST(:DATE AS ${DATE}) as date_test,
                        CAST(:TIME as ${TIME}) as time_test,
                        CAST(:DATETIME as ${DATETIME}) as datetime_test,
                        CAST(:TIMESTAMP AS ${TIMESTAMP_TZ}) as timestamp_test
                    FROM
                        ${SCHEMA}numbers
                    WHERE
                        numbers = 1;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query tests all supported parameter datatypes in the Conduit service.

                    ## Parameters

                    - INTEGER(integer) - Integer parameter test
                    - STRING(string) - String parameter test
                    - BOOLEAN(boolean) - Boolean parameter test
                    - FLOAT(float) - Float parameter test
                    - TEXT(text) - Text parameter test
                    - DATE(date) - Date parameter test
                    - TIME(time) - Time parameter test
                    - DATETIME(datetime) - Datetime parameter test
                    - TIMESTAMP(timestamp) - Timestamp parameter test

                    ## Returns

                    - `integer_test`: The provided INTEGER parameter value
                    - `string_test`: The provided STRING parameter value
                    - `boolean_test`: The provided BOOLEAN parameter value
                    - `float_test`: The provided FLOAT parameter value
                    - `text_test`: The provided TEXT parameter value
                    - `date_test`: The provided DATE parameter value
                    - `time_test`: The provided TIME parameter value
                    - `datetime_test`: The provided DATETIME parameter value
                    - `timestamp_test`: The provided TIMESTAMP parameter value

                    ## Tables

                    - `${SCHEMA}numbers`: The numbers table - see migration 1147

                    ## Notes

                    This is a public query intended to test parameter processing for all supported datatypes.
                    All parameters are required to validate proper type handling across database engines.

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
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
if engine == 'mysql' then table.insert(queries,{sql=[[
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
                ${TYPE_PUBLIC}                                                      AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        :INTEGER as integer_test,
                        :STRING as string_test,
                        CASE WHEN :BOOLEAN THEN 1 ELSE 0 END as boolean_test,
                        :FLOAT as float_test,
                        :TEXT as text_test,
                        CAST(:DATE AS ${DATE}) as date_test,
                        CAST(:TIME as ${TIME}) as time_test,
                        CAST(:DATETIME as ${DATETIME}) as datetime_test,
                        CAST(:TIMESTAMP AS ${TIMESTAMP_TZ}) as timestamp_test
                    FROM
                        ${SCHEMA}numbers
                    WHERE
                        numbers = 1;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query tests all supported parameter datatypes in the Conduit service.

                    ## Parameters

                    - INTEGER(integer) - Integer parameter test
                    - STRING(string) - String parameter test
                    - BOOLEAN(boolean) - Boolean parameter test
                    - FLOAT(float) - Float parameter test
                    - TEXT(text) - Text parameter test
                    - DATE(date) - Date parameter test
                    - TIME(time) - Time parameter test
                    - DATETIME(datetime) - Datetime parameter test
                    - TIMESTAMP(timestamp) - Timestamp parameter test

                    ## Returns

                    - `integer_test`: The provided INTEGER parameter value
                    - `string_test`: The provided STRING parameter value
                    - `boolean_test`: The provided BOOLEAN parameter value
                    - `float_test`: The provided FLOAT parameter value
                    - `text_test`: The provided TEXT parameter value
                    - `date_test`: The provided DATE parameter value
                    - `time_test`: The provided TIME parameter value
                    - `datetime_test`: The provided DATETIME parameter value
                    - `timestamp_test`: The provided TIMESTAMP parameter value

                    ## Tables

                    - `${SCHEMA}numbers`: The numbers table - see migration 1147

                    ## Notes

                    This is a public query intended to test parameter processing for all supported datatypes.
                    All parameters are required to validate proper type handling across database engines.

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
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]}) end
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

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end