-- Migration: acuranzo_1051.lua
-- Defaults for Lookup 028 - Query Type

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-11-22 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1053"
cfg.LOOKUP_ID = "028"
cfg.LOOKUP_NAME = "Query Type"
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
            INSERT INTO ${SCHEMA}${TABLE}
            (
                lookup_id,
                key_idx,
                status_a1,
                value_txt,
                value_int,
                sort_seq,
                code,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                0,                              -- lookup_id
                ${LOOKUP_ID},                   -- key_idx
                1,                              -- status_a1
                '${LOOKUP_NAME}',               -- value_txt
                0,                              -- value_int
                0,                              -- sort_seq
                '',                             -- code
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Query types.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "HTMLEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": false,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END},             -- collection
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID},    0, 1, 'SQL - Internal',               0,  0, '', '', ${JIS}[==[{"icon":"<i class='fa fa-code fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},    1, 1, 'SQL - System',                 0,  1, '', '', ${JIS}[==[{"icon":"<i class='fa fa-code fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},    2, 1, 'DDL - APP',                    0,  2, '', '', ${JIS}[==[{"icon":"<i class='fa fa-code fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},    3, 1, 'SQL - Reporting',              0,  3, '', '', ${JIS}[==[{"icon":"<i class='fa fa-code fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},   10, 1, 'SQL - Public',                 0,  3, '', '', ${JIS}[==[{"icon":"<i class='fa fa-code fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  101, 1, 'AI Prompts - Auditor',         0,  4, '', '', ${JIS}[==[{"icon":"<i class='fa fa-robot fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  102, 1, 'AI Prompts - Validation',      0,  5, '', '', ${JIS}[==[{"icon":"<i class='fa fa-robot fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  103, 1, 'AI Prompts - Playground',      0,  6, '', '', ${JIS}[==[{"icon":"<i class='fa fa-robot fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  104, 1, 'AI Prompts - Categorization',  0,  7, '', '', ${JIS}[==[{"icon":"<i class='fa fa-robot fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  105, 1, 'AI Prompts - System',          0,  8, '', '', ${JIS}[==[{"icon":"<i class='fa fa-robot fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 1000, 1, 'Forward Migration',            0,  9, '', '', ${JIS}[==[{"icon":"<i class='fa fa-check fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 1001, 1, 'Reverse Migration',            0, 10, '', '', ${JIS}[==[{"icon":"<i class='fa fa-check fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 1002, 1, 'Diagram Migration',            0, 11, '', '', ${JIS}[==[{"icon":"<i class='fa fa-check fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 1003, 1, 'Applied Migration',            0, 12, '', '', ${JIS}[==[{"icon":"<i class='fa fa-check fa-fw'></i>"}]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration creates the lookup values for Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}
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
            WHERE lookup_id = 0
            AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx IN (0, 1, 2, 3, 101, 102, 103, 104, 105, 1000, 1001, 1002, 1003);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} from ${TABLE} Table

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
