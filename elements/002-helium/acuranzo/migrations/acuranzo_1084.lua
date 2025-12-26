-- Migration: acuranzo_1084.lua
-- Defaults for Lookup 051 - Document Types

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-25 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1084"
cfg.LOOKUP_ID = "051"
cfg.LOOKUP_NAME = "Document Types"
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

                    Used mostly to provide an icon for the documents.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": true,
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
                (${LOOKUP_ID},  0, 1, 'Unspecified',            0, 99, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file fa-fw'></i>", "IconLarge": "<i class='fa fa-file fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  1, 1, 'HTML Page',              0,  1, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file-shield fa-fw'></i>", "IconLarge": "<i class='fa fa-file-shield fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'PDF',                    0,  2, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file-pdf fa-fw'></i>", "IconLarge": "<i class='fa fa-file-pdf fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Word Document',          0,  3, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file-doc fa-fw'></i>", "IconLarge": "<i class='fa fa-file-doc fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Excel Document',         0,  4, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file-xls fa-fw'></i>", "IconLarge": "<i class='fa fa-file-xls fa-fw fa-8x'></i> "}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'Powerpoint Document',    0,  5, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-file-ppt fa-fw'></i>", "IconLarge": "<i class='fa fa-file-ppt fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'HTML Link',              0,  6, '', '', ${JIS}[==[{"Icon": "<i class='fa fa-link fa-fw'></i>", "IconLarge": "<i class='fa fa-link fa-fw fa-8x'></i>"}]==]${JIE}, ${COMMON_VALUES});

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
            AND key_idx IN (0,1,2,3,4,5,6);

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
