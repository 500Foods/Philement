-- Migration: acuranzo_1075.lua
-- Defaults for Lookup 042 - Modules

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-20 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1075"
cfg.LOOKUP_ID = "042"
cfg.LOOKUP_NAME = "Modules"
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

                    Modules.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": false,
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
                (${LOOKUP_ID},  0, 1, 'Main Menu',          0, 0, '', '', ${JIS}[==[{"Index": -2, "Icon":"<i class='fa fa-fw fa-xl fa-circle'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  1, 1, 'Login',              0, 0, '', '', ${JIS}[==[{"Index": -1, "Icon":"<i class='fa fa-fw fa-xl fa-circle'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Session Logs',       0, 0, '', '', ${JIS}[==[{"Index":  0, "Icon":"<i class='fa fa-fw fa-xl fa-receipt'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Version History',    0, 0, '', '', ${JIS}[==[{"Index":  1, "Icon":"<i class='fa fa-fw fa-xl fa-circle'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Queries',            2, 1, '', '', ${JIS}[==[{"Index":  2, "Icon":"<i class='fa fa-fw fa-xl fa-clipboard-question'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'Lookups',            2, 2, '', '', ${JIS}[==[{"Index":  3, "Icon":"<i class='fa fa-fw fa-xl fa-clipboard-list'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'Chats',              3, 1, '', '', ${JIS}[==[{"Index":  4, "Icon":"<i class='fa fa-fw fa-xl fa-robot'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  7, 1, 'AI Auditor',         3, 2, '', '', ${JIS}[==[{"Index":  5, "Icon":"<i class='fa fa-fw fa-xl fa-scale-balanced'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  8, 1, 'Dashboard',          1, 1, '', '', ${JIS}[==[{"Index":  6, "Icon":"<i class='fa fa-fw fa-xl fa-bullseye-arrow'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  9, 1, 'Document Library',   1, 2, '', '', ${JIS}[==[{"Index":  7, "Icon":"<i class='fa fa-fw fa-xl fa-books'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 10, 1, 'Media Library',      1, 3, '', '', ${JIS}[==[{"Index":  8, "Icon":"<i class='fa fa-fw fa-xl fa-photo-film-music'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 11, 1, 'Diagram Library',    1, 4, '', '', ${JIS}[==[{"Index": 10, "Icon":"<i class='fa fa-fw fa-xl fa-shapes'></i>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 12, 1, 'Reports',            2, 5, '', '', ${JIS}[==[{"Index":  9, "Icon":"<i class='fa fa-fw fa-xl fa-clipboard-check'></i>"}]==]${JIE}, ${COMMON_VALUES});

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
            AND key_idx IN (0,1,2,3,4,5,6,7,8,9,10,11,12);

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
