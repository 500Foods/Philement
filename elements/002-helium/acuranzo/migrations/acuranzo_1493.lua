-- Migration: acuranzo_1493.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 8/15
-- Keys 491-560. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 8

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1493"

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
                (80, 491, 1, 'Light Grass Green', 0, 491, '#9af764', '', '{}', ${COMMON_VALUES}),
                (80, 492, 1, 'Light Gray', 0, 492, '#d3d3d3', '', '{}', ${COMMON_VALUES}),
                (80, 493, 1, 'Light Green', 0, 493, '#90ee90', '', '{}', ${COMMON_VALUES}),
                (80, 494, 1, 'Light Green Blue', 0, 494, '#56fca2', '', '{}', ${COMMON_VALUES}),
                (80, 495, 1, 'Light Greenish Blue', 0, 495, '#63f7b4', '', '{}', ${COMMON_VALUES}),
                (80, 496, 1, 'Light Grey', 0, 496, '#d3d3d3', '', '{}', ${COMMON_VALUES}),
                (80, 497, 1, 'Light Grey Blue', 0, 497, '#9dbcd4', '', '{}', ${COMMON_VALUES}),
                (80, 498, 1, 'Light Grey Green', 0, 498, '#b7e1a1', '', '{}', ${COMMON_VALUES}),
                (80, 499, 1, 'Light Indigo', 0, 499, '#6d5acf', '', '{}', ${COMMON_VALUES}),
                (80, 500, 1, 'Light Khaki', 0, 500, '#e6f2a2', '', '{}', ${COMMON_VALUES}),
                (80, 501, 1, 'Light Lavendar', 0, 501, '#efc0fe', '', '{}', ${COMMON_VALUES}),
                (80, 502, 1, 'Light Lavender', 0, 502, '#dfc5fe', '', '{}', ${COMMON_VALUES}),
                (80, 503, 1, 'Light Light Blue', 0, 503, '#cafffb', '', '{}', ${COMMON_VALUES}),
                (80, 504, 1, 'Light Light Green', 0, 504, '#c8ffb0', '', '{}', ${COMMON_VALUES}),
                (80, 505, 1, 'Light Lilac', 0, 505, '#edc8ff', '', '{}', ${COMMON_VALUES}),
                (80, 506, 1, 'Light Lime', 0, 506, '#aefd6c', '', '{}', ${COMMON_VALUES}),
                (80, 507, 1, 'Light Lime Green', 0, 507, '#b9ff66', '', '{}', ${COMMON_VALUES}),
                (80, 508, 1, 'Light Magenta', 0, 508, '#fa5ff7', '', '{}', ${COMMON_VALUES}),
                (80, 509, 1, 'Light Maroon', 0, 509, '#a24857', '', '{}', ${COMMON_VALUES}),
                (80, 510, 1, 'Light Mauve', 0, 510, '#c292a1', '', '{}', ${COMMON_VALUES}),
                (80, 511, 1, 'Light Mint', 0, 511, '#b6ffbb', '', '{}', ${COMMON_VALUES}),
                (80, 512, 1, 'Light Mint Green', 0, 512, '#a6fbb2', '', '{}', ${COMMON_VALUES}),
                (80, 513, 1, 'Light Moss Green', 0, 513, '#a6c875', '', '{}', ${COMMON_VALUES}),
                (80, 514, 1, 'Light Mustard', 0, 514, '#f7d560', '', '{}', ${COMMON_VALUES}),
                (80, 515, 1, 'Light Navy', 0, 515, '#155084', '', '{}', ${COMMON_VALUES}),
                (80, 516, 1, 'Light Navy Blue', 0, 516, '#2e5a88', '', '{}', ${COMMON_VALUES}),
                (80, 517, 1, 'Light Neon Green', 0, 517, '#4efd54', '', '{}', ${COMMON_VALUES}),
                (80, 518, 1, 'Light Olive', 0, 518, '#acbf69', '', '{}', ${COMMON_VALUES}),
                (80, 519, 1, 'Light Olive Green', 0, 519, '#a4be5c', '', '{}', ${COMMON_VALUES}),
                (80, 520, 1, 'Light Orange', 0, 520, '#fdaa48', '', '{}', ${COMMON_VALUES}),
                (80, 521, 1, 'Light Pastel Green', 0, 521, '#b2fba5', '', '{}', ${COMMON_VALUES}),
                (80, 522, 1, 'Light Pea Green', 0, 522, '#c4fe82', '', '{}', ${COMMON_VALUES}),
                (80, 523, 1, 'Light Peach', 0, 523, '#ffd8b1', '', '{}', ${COMMON_VALUES}),
                (80, 524, 1, 'Light Periwinkle', 0, 524, '#c1c6fc', '', '{}', ${COMMON_VALUES}),
                (80, 525, 1, 'Light Pink', 0, 525, '#ffb6c1', '', '{}', ${COMMON_VALUES}),
                (80, 526, 1, 'Light Plum', 0, 526, '#9d5783', '', '{}', ${COMMON_VALUES}),
                (80, 527, 1, 'Light Purple', 0, 527, '#bf77f6', '', '{}', ${COMMON_VALUES}),
                (80, 528, 1, 'Light Red', 0, 528, '#ff474c', '', '{}', ${COMMON_VALUES}),
                (80, 529, 1, 'Light Rose', 0, 529, '#ffc5cb', '', '{}', ${COMMON_VALUES}),
                (80, 530, 1, 'Light Royal Blue', 0, 530, '#3a2efe', '', '{}', ${COMMON_VALUES}),
                (80, 531, 1, 'Light Sage', 0, 531, '#bcecac', '', '{}', ${COMMON_VALUES}),
                (80, 532, 1, 'Light Salmon', 0, 532, '#ffa07a', '', '{}', ${COMMON_VALUES}),
                (80, 533, 1, 'Light Sea Green', 0, 533, '#20b2aa', '', '{}', ${COMMON_VALUES}),
                (80, 534, 1, 'Light Seafoam', 0, 534, '#a0febf', '', '{}', ${COMMON_VALUES}),
                (80, 535, 1, 'Light Seafoam Green', 0, 535, '#a7ffb5', '', '{}', ${COMMON_VALUES}),
                (80, 536, 1, 'Light Sky Blue', 0, 536, '#87cefa', '', '{}', ${COMMON_VALUES}),
                (80, 537, 1, 'Light Slate Gray', 0, 537, '#778899', '', '{}', ${COMMON_VALUES}),
                (80, 538, 1, 'Light Slate Grey', 0, 538, '#778899', '', '{}', ${COMMON_VALUES}),
                (80, 539, 1, 'Light Steel Blue', 0, 539, '#b0c4de', '', '{}', ${COMMON_VALUES}),
                (80, 540, 1, 'Light Tan', 0, 540, '#fbeeac', '', '{}', ${COMMON_VALUES}),
                (80, 541, 1, 'Light Teal', 0, 541, '#90e4c1', '', '{}', ${COMMON_VALUES}),
                (80, 542, 1, 'Light Turquoise', 0, 542, '#7ef4cc', '', '{}', ${COMMON_VALUES}),
                (80, 543, 1, 'Light Urple', 0, 543, '#b36ff6', '', '{}', ${COMMON_VALUES}),
                (80, 544, 1, 'Light Violet', 0, 544, '#d6b4fc', '', '{}', ${COMMON_VALUES}),
                (80, 545, 1, 'Light Yellow', 0, 545, '#ffffe0', '', '{}', ${COMMON_VALUES}),
                (80, 546, 1, 'Light Yellow Green', 0, 546, '#ccfd7f', '', '{}', ${COMMON_VALUES}),
                (80, 547, 1, 'Light Yellowish Green', 0, 547, '#c2ff89', '', '{}', ${COMMON_VALUES}),
                (80, 548, 1, 'Lightblue', 0, 548, '#7bc8f6', '', '{}', ${COMMON_VALUES}),
                (80, 549, 1, 'Lighter Green', 0, 549, '#75fd63', '', '{}', ${COMMON_VALUES}),
                (80, 550, 1, 'Lighter Purple', 0, 550, '#a55af4', '', '{}', ${COMMON_VALUES}),
                (80, 551, 1, 'Lightgreen', 0, 551, '#76ff7b', '', '{}', ${COMMON_VALUES}),
                (80, 552, 1, 'Lightish Blue', 0, 552, '#3d7afd', '', '{}', ${COMMON_VALUES}),
                (80, 553, 1, 'Lightish Green', 0, 553, '#61e160', '', '{}', ${COMMON_VALUES}),
                (80, 554, 1, 'Lightish Purple', 0, 554, '#a552e6', '', '{}', ${COMMON_VALUES}),
                (80, 555, 1, 'Lightish Red', 0, 555, '#fe2f4a', '', '{}', ${COMMON_VALUES}),
                (80, 556, 1, 'Lilac', 0, 556, '#cea2fd', '', '{}', ${COMMON_VALUES}),
                (80, 557, 1, 'Liliac', 0, 557, '#c48efd', '', '{}', ${COMMON_VALUES}),
                (80, 558, 1, 'Lime', 0, 558, '#00ff00', '', '{}', ${COMMON_VALUES}),
                (80, 559, 1, 'Lime Green', 0, 559, '#32cd32', '', '{}', ${COMMON_VALUES}),
                (80, 560, 1, 'Lime Yellow', 0, 560, '#d0fe1d', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 8'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 8

            Seeds lookup 080 keys 491-560.
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
               AND key_idx >= 491
               AND key_idx <= 560;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 8'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 8
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
