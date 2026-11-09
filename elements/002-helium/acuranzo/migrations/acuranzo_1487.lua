-- Migration: acuranzo_1487.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 2/15
-- Keys 71-140. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 2

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1487"

table.insert(queries,{sql=[====[

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

            INSERT INTO ${SCHEMA}lookups
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (80, 71, 1, 'Bluish Grey', 0, 71, '#748b97', '', '{}', ${COMMON_VALUES}),
                (80, 72, 1, 'Bluish Purple', 0, 72, '#703be7', '', '{}', ${COMMON_VALUES}),
                (80, 73, 1, 'Blurple', 0, 73, '#5539cc', '', '{}', ${COMMON_VALUES}),
                (80, 74, 1, 'Blush', 0, 74, '#f29e8e', '', '{}', ${COMMON_VALUES}),
                (80, 75, 1, 'Blush Pink', 0, 75, '#fe828c', '', '{}', ${COMMON_VALUES}),
                (80, 76, 1, 'Booger', 0, 76, '#9bb53c', '', '{}', ${COMMON_VALUES}),
                (80, 77, 1, 'Booger Green', 0, 77, '#96b403', '', '{}', ${COMMON_VALUES}),
                (80, 78, 1, 'Bordeaux', 0, 78, '#7b002c', '', '{}', ${COMMON_VALUES}),
                (80, 79, 1, 'Boring Green', 0, 79, '#63b365', '', '{}', ${COMMON_VALUES}),
                (80, 80, 1, 'Bottle Green', 0, 80, '#044a05', '', '{}', ${COMMON_VALUES}),
                (80, 81, 1, 'Brick', 0, 81, '#a03623', '', '{}', ${COMMON_VALUES}),
                (80, 82, 1, 'Brick Orange', 0, 82, '#c14a09', '', '{}', ${COMMON_VALUES}),
                (80, 83, 1, 'Brick Red', 0, 83, '#8f1402', '', '{}', ${COMMON_VALUES}),
                (80, 84, 1, 'Bright Aqua', 0, 84, '#0bf9ea', '', '{}', ${COMMON_VALUES}),
                (80, 85, 1, 'Bright Blue', 0, 85, '#0165fc', '', '{}', ${COMMON_VALUES}),
                (80, 86, 1, 'Bright Cyan', 0, 86, '#41fdfe', '', '{}', ${COMMON_VALUES}),
                (80, 87, 1, 'Bright Green', 0, 87, '#01ff07', '', '{}', ${COMMON_VALUES}),
                (80, 88, 1, 'Bright Lavender', 0, 88, '#c760ff', '', '{}', ${COMMON_VALUES}),
                (80, 89, 1, 'Bright Light Blue', 0, 89, '#26f7fd', '', '{}', ${COMMON_VALUES}),
                (80, 90, 1, 'Bright Light Green', 0, 90, '#2dfe54', '', '{}', ${COMMON_VALUES}),
                (80, 91, 1, 'Bright Lilac', 0, 91, '#c95efb', '', '{}', ${COMMON_VALUES}),
                (80, 92, 1, 'Bright Lime', 0, 92, '#87fd05', '', '{}', ${COMMON_VALUES}),
                (80, 93, 1, 'Bright Lime Green', 0, 93, '#65fe08', '', '{}', ${COMMON_VALUES}),
                (80, 94, 1, 'Bright Magenta', 0, 94, '#ff08e8', '', '{}', ${COMMON_VALUES}),
                (80, 95, 1, 'Bright Olive', 0, 95, '#9cbb04', '', '{}', ${COMMON_VALUES}),
                (80, 96, 1, 'Bright Orange', 0, 96, '#ff5b00', '', '{}', ${COMMON_VALUES}),
                (80, 97, 1, 'Bright Pink', 0, 97, '#fe01b1', '', '{}', ${COMMON_VALUES}),
                (80, 98, 1, 'Bright Purple', 0, 98, '#be03fd', '', '{}', ${COMMON_VALUES}),
                (80, 99, 1, 'Bright Red', 0, 99, '#ff000d', '', '{}', ${COMMON_VALUES}),
                (80, 100, 1, 'Bright Sea Green', 0, 100, '#05ffa6', '', '{}', ${COMMON_VALUES}),
                (80, 101, 1, 'Bright Sky Blue', 0, 101, '#02ccfe', '', '{}', ${COMMON_VALUES}),
                (80, 102, 1, 'Bright Teal', 0, 102, '#01f9c6', '', '{}', ${COMMON_VALUES}),
                (80, 103, 1, 'Bright Turquoise', 0, 103, '#0ffef9', '', '{}', ${COMMON_VALUES}),
                (80, 104, 1, 'Bright Violet', 0, 104, '#ad0afd', '', '{}', ${COMMON_VALUES}),
                (80, 105, 1, 'Bright Yellow', 0, 105, '#fffd01', '', '{}', ${COMMON_VALUES}),
                (80, 106, 1, 'Bright Yellow Green', 0, 106, '#9dff00', '', '{}', ${COMMON_VALUES}),
                (80, 107, 1, 'British Racing Green', 0, 107, '#05480d', '', '{}', ${COMMON_VALUES}),
                (80, 108, 1, 'Bronze', 0, 108, '#a87900', '', '{}', ${COMMON_VALUES}),
                (80, 109, 1, 'Brown', 0, 109, '#a52a2a', '', '{}', ${COMMON_VALUES}),
                (80, 110, 1, 'Brown Green', 0, 110, '#706c11', '', '{}', ${COMMON_VALUES}),
                (80, 111, 1, 'Brown Grey', 0, 111, '#8d8468', '', '{}', ${COMMON_VALUES}),
                (80, 112, 1, 'Brown Orange', 0, 112, '#b96902', '', '{}', ${COMMON_VALUES}),
                (80, 113, 1, 'Brown Red', 0, 113, '#922b05', '', '{}', ${COMMON_VALUES}),
                (80, 114, 1, 'Brown Yellow', 0, 114, '#b29705', '', '{}', ${COMMON_VALUES}),
                (80, 115, 1, 'Brownish', 0, 115, '#9c6d57', '', '{}', ${COMMON_VALUES}),
                (80, 116, 1, 'Brownish Green', 0, 116, '#6a6e09', '', '{}', ${COMMON_VALUES}),
                (80, 117, 1, 'Brownish Grey', 0, 117, '#86775f', '', '{}', ${COMMON_VALUES}),
                (80, 118, 1, 'Brownish Orange', 0, 118, '#cb7723', '', '{}', ${COMMON_VALUES}),
                (80, 119, 1, 'Brownish Pink', 0, 119, '#c27e79', '', '{}', ${COMMON_VALUES}),
                (80, 120, 1, 'Brownish Purple', 0, 120, '#76424e', '', '{}', ${COMMON_VALUES}),
                (80, 121, 1, 'Brownish Red', 0, 121, '#9e3623', '', '{}', ${COMMON_VALUES}),
                (80, 122, 1, 'Brownish Yellow', 0, 122, '#c9b003', '', '{}', ${COMMON_VALUES}),
                (80, 123, 1, 'Browny Green', 0, 123, '#6f6c0a', '', '{}', ${COMMON_VALUES}),
                (80, 124, 1, 'Browny Orange', 0, 124, '#ca6b02', '', '{}', ${COMMON_VALUES}),
                (80, 125, 1, 'Bruise', 0, 125, '#7e4071', '', '{}', ${COMMON_VALUES}),
                (80, 126, 1, 'Bubble Gum Pink', 0, 126, '#ff69af', '', '{}', ${COMMON_VALUES}),
                (80, 127, 1, 'Bubblegum', 0, 127, '#ff6cb5', '', '{}', ${COMMON_VALUES}),
                (80, 128, 1, 'Bubblegum Pink', 0, 128, '#fe83cc', '', '{}', ${COMMON_VALUES}),
                (80, 129, 1, 'Buff', 0, 129, '#fef69e', '', '{}', ${COMMON_VALUES}),
                (80, 130, 1, 'Burgundy', 0, 130, '#610023', '', '{}', ${COMMON_VALUES}),
                (80, 131, 1, 'Burly Wood', 0, 131, '#deb887', '', '{}', ${COMMON_VALUES}),
                (80, 132, 1, 'Burnt Orange', 0, 132, '#c04e01', '', '{}', ${COMMON_VALUES}),
                (80, 133, 1, 'Burnt Red', 0, 133, '#9f2305', '', '{}', ${COMMON_VALUES}),
                (80, 134, 1, 'Burnt Siena', 0, 134, '#b75203', '', '{}', ${COMMON_VALUES}),
                (80, 135, 1, 'Burnt Sienna', 0, 135, '#b04e0f', '', '{}', ${COMMON_VALUES}),
                (80, 136, 1, 'Burnt Umber', 0, 136, '#a0450e', '', '{}', ${COMMON_VALUES}),
                (80, 137, 1, 'Burnt Yellow', 0, 137, '#d5ab09', '', '{}', ${COMMON_VALUES}),
                (80, 138, 1, 'Burple', 0, 138, '#6832e3', '', '{}', ${COMMON_VALUES}),
                (80, 139, 1, 'Butter', 0, 139, '#ffff81', '', '{}', ${COMMON_VALUES}),
                (80, 140, 1, 'Butter Yellow', 0, 140, '#fffd74', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 2'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 2

            Seeds lookup 080 keys 71-140.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

table.insert(queries,{sql=[====[

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

            DELETE FROM ${SCHEMA}lookups
             WHERE lookup_id = 80
               AND key_idx >= 71
               AND key_idx <= 140;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 2'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 2
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
