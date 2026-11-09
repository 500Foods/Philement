-- Migration: acuranzo_1490.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 5/15
-- Keys 281-350. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 5

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1490"

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
                (80, 281, 1, 'Deep Sky Blue', 0, 281, '#00bfff', '', '{}', ${COMMON_VALUES}),
                (80, 282, 1, 'Deep Teal', 0, 282, '#00555a', '', '{}', ${COMMON_VALUES}),
                (80, 283, 1, 'Deep Turquoise', 0, 283, '#017374', '', '{}', ${COMMON_VALUES}),
                (80, 284, 1, 'Deep Violet', 0, 284, '#490648', '', '{}', ${COMMON_VALUES}),
                (80, 285, 1, 'Denim', 0, 285, '#3b638c', '', '{}', ${COMMON_VALUES}),
                (80, 286, 1, 'Denim Blue', 0, 286, '#3b5b92', '', '{}', ${COMMON_VALUES}),
                (80, 287, 1, 'Desert', 0, 287, '#ccad60', '', '{}', ${COMMON_VALUES}),
                (80, 288, 1, 'Diarrhea', 0, 288, '#9f8303', '', '{}', ${COMMON_VALUES}),
                (80, 289, 1, 'Dim Gray', 0, 289, '#696969', '', '{}', ${COMMON_VALUES}),
                (80, 290, 1, 'Dim Grey', 0, 290, '#696969', '', '{}', ${COMMON_VALUES}),
                (80, 291, 1, 'Dirt', 0, 291, '#8a6e45', '', '{}', ${COMMON_VALUES}),
                (80, 292, 1, 'Dirt Brown', 0, 292, '#836539', '', '{}', ${COMMON_VALUES}),
                (80, 293, 1, 'Dirty Blue', 0, 293, '#3f829d', '', '{}', ${COMMON_VALUES}),
                (80, 294, 1, 'Dirty Green', 0, 294, '#667e2c', '', '{}', ${COMMON_VALUES}),
                (80, 295, 1, 'Dirty Orange', 0, 295, '#c87606', '', '{}', ${COMMON_VALUES}),
                (80, 296, 1, 'Dirty Pink', 0, 296, '#ca7b80', '', '{}', ${COMMON_VALUES}),
                (80, 297, 1, 'Dirty Purple', 0, 297, '#734a65', '', '{}', ${COMMON_VALUES}),
                (80, 298, 1, 'Dirty Yellow', 0, 298, '#cdc50a', '', '{}', ${COMMON_VALUES}),
                (80, 299, 1, 'Dodger Blue', 0, 299, '#1e90ff', '', '{}', ${COMMON_VALUES}),
                (80, 300, 1, 'Drab', 0, 300, '#828344', '', '{}', ${COMMON_VALUES}),
                (80, 301, 1, 'Drab Green', 0, 301, '#749551', '', '{}', ${COMMON_VALUES}),
                (80, 302, 1, 'Dried Blood', 0, 302, '#4b0101', '', '{}', ${COMMON_VALUES}),
                (80, 303, 1, 'Duck Egg Blue', 0, 303, '#c3fbf4', '', '{}', ${COMMON_VALUES}),
                (80, 304, 1, 'Dull Blue', 0, 304, '#49759c', '', '{}', ${COMMON_VALUES}),
                (80, 305, 1, 'Dull Brown', 0, 305, '#876e4b', '', '{}', ${COMMON_VALUES}),
                (80, 306, 1, 'Dull Green', 0, 306, '#74a662', '', '{}', ${COMMON_VALUES}),
                (80, 307, 1, 'Dull Orange', 0, 307, '#d8863b', '', '{}', ${COMMON_VALUES}),
                (80, 308, 1, 'Dull Pink', 0, 308, '#d5869d', '', '{}', ${COMMON_VALUES}),
                (80, 309, 1, 'Dull Purple', 0, 309, '#84597e', '', '{}', ${COMMON_VALUES}),
                (80, 310, 1, 'Dull Red', 0, 310, '#bb3f3f', '', '{}', ${COMMON_VALUES}),
                (80, 311, 1, 'Dull Teal', 0, 311, '#5f9e8f', '', '{}', ${COMMON_VALUES}),
                (80, 312, 1, 'Dull Yellow', 0, 312, '#eedc5b', '', '{}', ${COMMON_VALUES}),
                (80, 313, 1, 'Dusk', 0, 313, '#4e5481', '', '{}', ${COMMON_VALUES}),
                (80, 314, 1, 'Dusk Blue', 0, 314, '#26538d', '', '{}', ${COMMON_VALUES}),
                (80, 315, 1, 'Dusky Blue', 0, 315, '#475f94', '', '{}', ${COMMON_VALUES}),
                (80, 316, 1, 'Dusky Pink', 0, 316, '#cc7a8b', '', '{}', ${COMMON_VALUES}),
                (80, 317, 1, 'Dusky Purple', 0, 317, '#895b7b', '', '{}', ${COMMON_VALUES}),
                (80, 318, 1, 'Dusky Rose', 0, 318, '#ba6873', '', '{}', ${COMMON_VALUES}),
                (80, 319, 1, 'Dust', 0, 319, '#b2996e', '', '{}', ${COMMON_VALUES}),
                (80, 320, 1, 'Dusty Blue', 0, 320, '#5a86ad', '', '{}', ${COMMON_VALUES}),
                (80, 321, 1, 'Dusty Green', 0, 321, '#76a973', '', '{}', ${COMMON_VALUES}),
                (80, 322, 1, 'Dusty Lavender', 0, 322, '#ac86a8', '', '{}', ${COMMON_VALUES}),
                (80, 323, 1, 'Dusty Orange', 0, 323, '#f0833a', '', '{}', ${COMMON_VALUES}),
                (80, 324, 1, 'Dusty Pink', 0, 324, '#d58a94', '', '{}', ${COMMON_VALUES}),
                (80, 325, 1, 'Dusty Purple', 0, 325, '#825f87', '', '{}', ${COMMON_VALUES}),
                (80, 326, 1, 'Dusty Red', 0, 326, '#b9484e', '', '{}', ${COMMON_VALUES}),
                (80, 327, 1, 'Dusty Rose', 0, 327, '#c0737a', '', '{}', ${COMMON_VALUES}),
                (80, 328, 1, 'Dusty Teal', 0, 328, '#4c9085', '', '{}', ${COMMON_VALUES}),
                (80, 329, 1, 'Earth', 0, 329, '#a2653e', '', '{}', ${COMMON_VALUES}),
                (80, 330, 1, 'Easter Green', 0, 330, '#8cfd7e', '', '{}', ${COMMON_VALUES}),
                (80, 331, 1, 'Easter Purple', 0, 331, '#c071fe', '', '{}', ${COMMON_VALUES}),
                (80, 332, 1, 'Ecru', 0, 332, '#feffca', '', '{}', ${COMMON_VALUES}),
                (80, 333, 1, 'Egg Shell', 0, 333, '#fffcc4', '', '{}', ${COMMON_VALUES}),
                (80, 334, 1, 'Eggplant', 0, 334, '#380835', '', '{}', ${COMMON_VALUES}),
                (80, 335, 1, 'Eggplant Purple', 0, 335, '#430541', '', '{}', ${COMMON_VALUES}),
                (80, 336, 1, 'Eggshell', 0, 336, '#ffffd4', '', '{}', ${COMMON_VALUES}),
                (80, 337, 1, 'Eggshell Blue', 0, 337, '#c4fff7', '', '{}', ${COMMON_VALUES}),
                (80, 338, 1, 'Electric Blue', 0, 338, '#0652ff', '', '{}', ${COMMON_VALUES}),
                (80, 339, 1, 'Electric Green', 0, 339, '#21fc0d', '', '{}', ${COMMON_VALUES}),
                (80, 340, 1, 'Electric Lime', 0, 340, '#a8ff04', '', '{}', ${COMMON_VALUES}),
                (80, 341, 1, 'Electric Pink', 0, 341, '#ff0490', '', '{}', ${COMMON_VALUES}),
                (80, 342, 1, 'Electric Purple', 0, 342, '#aa23ff', '', '{}', ${COMMON_VALUES}),
                (80, 343, 1, 'Emerald', 0, 343, '#01a049', '', '{}', ${COMMON_VALUES}),
                (80, 344, 1, 'Emerald Green', 0, 344, '#028f1e', '', '{}', ${COMMON_VALUES}),
                (80, 345, 1, 'Evergreen', 0, 345, '#05472a', '', '{}', ${COMMON_VALUES}),
                (80, 346, 1, 'Faded Blue', 0, 346, '#658cbb', '', '{}', ${COMMON_VALUES}),
                (80, 347, 1, 'Faded Green', 0, 347, '#7bb274', '', '{}', ${COMMON_VALUES}),
                (80, 348, 1, 'Faded Orange', 0, 348, '#f0944d', '', '{}', ${COMMON_VALUES}),
                (80, 349, 1, 'Faded Pink', 0, 349, '#de9dac', '', '{}', ${COMMON_VALUES}),
                (80, 350, 1, 'Faded Purple', 0, 350, '#916e99', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 5'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 5

            Seeds lookup 080 keys 281-350.
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
               AND key_idx >= 281
               AND key_idx <= 350;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 5'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 5
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
