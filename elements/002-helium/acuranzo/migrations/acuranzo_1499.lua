-- Migration: acuranzo_1499.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 14/15
-- Keys 911-980. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 14

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1499"

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
                (80, 911, 1, 'Teal', 0, 911, '#008080', '', '{}', ${COMMON_VALUES}),
                (80, 912, 1, 'Teal Blue', 0, 912, '#01889f', '', '{}', ${COMMON_VALUES}),
                (80, 913, 1, 'Teal Green', 0, 913, '#25a36f', '', '{}', ${COMMON_VALUES}),
                (80, 914, 1, 'Tealish', 0, 914, '#24bca8', '', '{}', ${COMMON_VALUES}),
                (80, 915, 1, 'Tealish Green', 0, 915, '#0cdc73', '', '{}', ${COMMON_VALUES}),
                (80, 916, 1, 'Terra Cotta', 0, 916, '#c9643b', '', '{}', ${COMMON_VALUES}),
                (80, 917, 1, 'Terracota', 0, 917, '#cb6843', '', '{}', ${COMMON_VALUES}),
                (80, 918, 1, 'Terracotta', 0, 918, '#ca6641', '', '{}', ${COMMON_VALUES}),
                (80, 919, 1, 'Thistle', 0, 919, '#d8bfd8', '', '{}', ${COMMON_VALUES}),
                (80, 920, 1, 'Tiffany Blue', 0, 920, '#7bf2da', '', '{}', ${COMMON_VALUES}),
                (80, 921, 1, 'Tomato', 0, 921, '#ff6347', '', '{}', ${COMMON_VALUES}),
                (80, 922, 1, 'Tomato Red', 0, 922, '#ec2d01', '', '{}', ${COMMON_VALUES}),
                (80, 923, 1, 'Topaz', 0, 923, '#13bbaf', '', '{}', ${COMMON_VALUES}),
                (80, 924, 1, 'Toupe', 0, 924, '#c7ac7d', '', '{}', ${COMMON_VALUES}),
                (80, 925, 1, 'Toxic Green', 0, 925, '#61de2a', '', '{}', ${COMMON_VALUES}),
                (80, 926, 1, 'Tree Green', 0, 926, '#2a7e19', '', '{}', ${COMMON_VALUES}),
                (80, 927, 1, 'True Blue', 0, 927, '#010fcc', '', '{}', ${COMMON_VALUES}),
                (80, 928, 1, 'True Green', 0, 928, '#089404', '', '{}', ${COMMON_VALUES}),
                (80, 929, 1, 'Turquoise', 0, 929, '#40e0d0', '', '{}', ${COMMON_VALUES}),
                (80, 930, 1, 'Turquoise Blue', 0, 930, '#06b1c4', '', '{}', ${COMMON_VALUES}),
                (80, 931, 1, 'Turquoise Green', 0, 931, '#04f489', '', '{}', ${COMMON_VALUES}),
                (80, 932, 1, 'Turtle Green', 0, 932, '#75b84f', '', '{}', ${COMMON_VALUES}),
                (80, 933, 1, 'Twilight', 0, 933, '#4e518b', '', '{}', ${COMMON_VALUES}),
                (80, 934, 1, 'Twilight Blue', 0, 934, '#0a437a', '', '{}', ${COMMON_VALUES}),
                (80, 935, 1, 'Ugly Blue', 0, 935, '#31668a', '', '{}', ${COMMON_VALUES}),
                (80, 936, 1, 'Ugly Brown', 0, 936, '#7d7103', '', '{}', ${COMMON_VALUES}),
                (80, 937, 1, 'Ugly Green', 0, 937, '#7a9703', '', '{}', ${COMMON_VALUES}),
                (80, 938, 1, 'Ugly Pink', 0, 938, '#cd7584', '', '{}', ${COMMON_VALUES}),
                (80, 939, 1, 'Ugly Purple', 0, 939, '#a442a0', '', '{}', ${COMMON_VALUES}),
                (80, 940, 1, 'Ugly Yellow', 0, 940, '#d0c101', '', '{}', ${COMMON_VALUES}),
                (80, 941, 1, 'Ultramarine', 0, 941, '#2000b1', '', '{}', ${COMMON_VALUES}),
                (80, 942, 1, 'Ultramarine Blue', 0, 942, '#1805db', '', '{}', ${COMMON_VALUES}),
                (80, 943, 1, 'Umber', 0, 943, '#b26400', '', '{}', ${COMMON_VALUES}),
                (80, 944, 1, 'Velvet', 0, 944, '#750851', '', '{}', ${COMMON_VALUES}),
                (80, 945, 1, 'Vermillion', 0, 945, '#f4320c', '', '{}', ${COMMON_VALUES}),
                (80, 946, 1, 'Very Dark Blue', 0, 946, '#000133', '', '{}', ${COMMON_VALUES}),
                (80, 947, 1, 'Very Dark Brown', 0, 947, '#1d0200', '', '{}', ${COMMON_VALUES}),
                (80, 948, 1, 'Very Dark Green', 0, 948, '#062e03', '', '{}', ${COMMON_VALUES}),
                (80, 949, 1, 'Very Dark Purple', 0, 949, '#2a0134', '', '{}', ${COMMON_VALUES}),
                (80, 950, 1, 'Very Light Blue', 0, 950, '#d5ffff', '', '{}', ${COMMON_VALUES}),
                (80, 951, 1, 'Very Light Brown', 0, 951, '#d3b683', '', '{}', ${COMMON_VALUES}),
                (80, 952, 1, 'Very Light Green', 0, 952, '#d1ffbd', '', '{}', ${COMMON_VALUES}),
                (80, 953, 1, 'Very Light Pink', 0, 953, '#fff4f2', '', '{}', ${COMMON_VALUES}),
                (80, 954, 1, 'Very Light Purple', 0, 954, '#f6cefc', '', '{}', ${COMMON_VALUES}),
                (80, 955, 1, 'Very Pale Blue', 0, 955, '#d6fffe', '', '{}', ${COMMON_VALUES}),
                (80, 956, 1, 'Very Pale Green', 0, 956, '#cffdbc', '', '{}', ${COMMON_VALUES}),
                (80, 957, 1, 'Vibrant Blue', 0, 957, '#0339f8', '', '{}', ${COMMON_VALUES}),
                (80, 958, 1, 'Vibrant Green', 0, 958, '#0add08', '', '{}', ${COMMON_VALUES}),
                (80, 959, 1, 'Vibrant Purple', 0, 959, '#ad03de', '', '{}', ${COMMON_VALUES}),
                (80, 960, 1, 'Violet', 0, 960, '#ee82ee', '', '{}', ${COMMON_VALUES}),
                (80, 961, 1, 'Violet Blue', 0, 961, '#510ac9', '', '{}', ${COMMON_VALUES}),
                (80, 962, 1, 'Violet Pink', 0, 962, '#fb5ffc', '', '{}', ${COMMON_VALUES}),
                (80, 963, 1, 'Violet Red', 0, 963, '#a50055', '', '{}', ${COMMON_VALUES}),
                (80, 964, 1, 'Viridian', 0, 964, '#1e9167', '', '{}', ${COMMON_VALUES}),
                (80, 965, 1, 'Vivid Blue', 0, 965, '#152eff', '', '{}', ${COMMON_VALUES}),
                (80, 966, 1, 'Vivid Green', 0, 966, '#2fef10', '', '{}', ${COMMON_VALUES}),
                (80, 967, 1, 'Vivid Purple', 0, 967, '#9900fa', '', '{}', ${COMMON_VALUES}),
                (80, 968, 1, 'Vomit', 0, 968, '#a2a415', '', '{}', ${COMMON_VALUES}),
                (80, 969, 1, 'Vomit Green', 0, 969, '#89a203', '', '{}', ${COMMON_VALUES}),
                (80, 970, 1, 'Vomit Yellow', 0, 970, '#c7c10c', '', '{}', ${COMMON_VALUES}),
                (80, 971, 1, 'Warm Blue', 0, 971, '#4b57db', '', '{}', ${COMMON_VALUES}),
                (80, 972, 1, 'Warm Brown', 0, 972, '#964e02', '', '{}', ${COMMON_VALUES}),
                (80, 973, 1, 'Warm Grey', 0, 973, '#978a84', '', '{}', ${COMMON_VALUES}),
                (80, 974, 1, 'Warm Pink', 0, 974, '#fb5581', '', '{}', ${COMMON_VALUES}),
                (80, 975, 1, 'Warm Purple', 0, 975, '#952e8f', '', '{}', ${COMMON_VALUES}),
                (80, 976, 1, 'Washed Out Green', 0, 976, '#bcf5a6', '', '{}', ${COMMON_VALUES}),
                (80, 977, 1, 'Water Blue', 0, 977, '#0e87cc', '', '{}', ${COMMON_VALUES}),
                (80, 978, 1, 'Watermelon', 0, 978, '#fd4659', '', '{}', ${COMMON_VALUES}),
                (80, 979, 1, 'Weird Green', 0, 979, '#3ae57f', '', '{}', ${COMMON_VALUES}),
                (80, 980, 1, 'Wheat', 0, 980, '#f5deb3', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 14'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 14

            Seeds lookup 080 keys 911-980.
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
               AND key_idx >= 911
               AND key_idx <= 980;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 14'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 14
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
