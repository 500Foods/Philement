-- Migration: acuranzo_1075.lua
-- Defaults for Lookup 042 - Modules

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.2 - 2026-03-18 - Updated to use lower-case JSON keys, <fa>-style icon definitions
-- 1.0.1 - 2026-03-16 - Changed icons to use <fa> conveintion
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
                (${LOOKUP_ID},  1, 1, 'Login',              0,  1, '', '', ${JIS}[==[{"index": 1, "icon":"<fa fa-right-to-bracket></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Menu',               0,  2, '', '', ${JIS}[==[{"index": 2, "icon":"<fa fa-bars-staggered></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Profile',            0,  3, '', '', ${JIS}[==[{"index": 3, "icon":"<fa fa-user-gear></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Session',            0,  4, '', '', ${JIS}[==[{"index": 4, "icon":"<fa fa-receipt></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'Crimson',            0,  5, '', '', ${JIS}[==[{"index": 5, "icon":"<fa fa-fire></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'Tour',               0,  6, '', '', ${JIS}[==[{"index": 6, "icon":"<fa fa-signs-post></fa>"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID},  7, 1, 'Dashboard',          1,  7, '', '', ${JIS}[==[{"index":  7, "icon":"<fa fa-chart-mixed></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  8, 1, 'Mail',               1,  8, '', '', ${JIS}[==[{"index":  8, "icon":"<fa fa-envelope></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  9, 1, 'Profile',            1,  9, '', '', ${JIS}[==[{"index":  9, "icon":"<fa fa-users></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 10, 1, 'Sessions',           1, 10, '', '', ${JIS}[==[{"index": 10, "icon":"<fa fa-receipt></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 11, 1, 'Versions',           1, 11, '', '', ${JIS}[==[{"index": 11, "icon":"<fa fa-flag></fa>"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 12, 1, 'Calendars',          2, 12, '', '', ${JIS}[==[{"index": 12, "icon":"<fa fa-calendar-days></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 13, 1, 'Contacts',           2, 13, '', '', ${JIS}[==[{"index": 13, "icon":"<fa fa-address-book></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 14, 1, 'Files',              2, 14, '', '', ${JIS}[==[{"index": 14, "icon":"<fa fa-cabinet-filing></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 15, 1, 'Documents',          2, 15, '', '', ${JIS}[==[{"index": 15, "icon":"<fa fa-books></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 16, 1, 'Media',              2, 16, '', '', ${JIS}[==[{"index": 16, "icon":"<fa fa-photo-film-music></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 17, 1, 'Diagrams',           2, 17, '', '', ${JIS}[==[{"index": 17, "icon":"<fa fa-chart-diagram></fa>"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 18, 1, 'Chats',              3, 18, '', '', ${JIS}[==[{"index": 18, "icon":"<fa fa-messages></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 19, 1, 'Notifications',      3, 19, '', '', ${JIS}[==[{"index": 19, "icon":"<fa fa-bell></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 20, 1, 'Annotations',        3, 20, '', '', ${JIS}[==[{"index": 20, "icon":"<fa fa-bookmark></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 21, 1, 'Tickets',            3, 21, '', '', ${JIS}[==[{"index": 21, "icon":"<fa fa-bell-concierge></fa>"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 22, 1, 'Styles',             4, 22, '', '', ${JIS}[==[{"index": 22, "icon":"<fa fa-flask></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 23, 1, 'Lookups',            4, 23, '', '', ${JIS}[==[{"index": 23, "icon":"<fa fa-clipboard-list></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 24, 1, 'Reports',            4, 24, '', '', ${JIS}[==[{"index": 24, "icon":"<fa fa-file-invoice></fa>"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 25, 1, 'Roles',              5, 25, '', '', ${JIS}[==[{"index": 25, "icon":"<fa fa-helmet-safety></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 26, 1, 'Security',           5, 26, '', '', ${JIS}[==[{"index": 26, "icon":"<fa fa-user-secret></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 27, 1, 'AI Auditor',         5, 27, '', '', ${JIS}[==[{"index": 27, "icon":"<fa fa-robot></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 28, 1, 'Jobs',               5, 28, '', '', ${JIS}[==[{"index": 28, "icon":"<fa fa-hammer-brush></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 29, 1, 'Queries',            5, 29, '', '', ${JIS}[==[{"index": 29, "icon":"<fa fa-clipboard-question></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 30, 1, 'Sync',               5, 30, '', '', ${JIS}[==[{"index": 30, "icon":"<fa fa-arrows-spin></fa>"}]==]${JIE}, ${COMMON_VALUES});

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
            AND key_idx IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29.30);

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
