-- Migration: acuranzo_1486.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 1/15
-- Keys 1-70. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch and the lookup header.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 1

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1486"

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
                (0, 80, 1, 'Named Colours', 0, 0, '', 'CSS + xkcd named colours for the colour picker', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}lookups
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (80, 1, 1, 'Acid Green', 0, 1, '#8ffe09', '', '{}', ${COMMON_VALUES}),
                (80, 2, 1, 'Adobe', 0, 2, '#bd6c48', '', '{}', ${COMMON_VALUES}),
                (80, 3, 1, 'Algae', 0, 3, '#54ac68', '', '{}', ${COMMON_VALUES}),
                (80, 4, 1, 'Algae Green', 0, 4, '#21c36f', '', '{}', ${COMMON_VALUES}),
                (80, 5, 1, 'Alice Blue', 0, 5, '#f0f8ff', '', '{}', ${COMMON_VALUES}),
                (80, 6, 1, 'Almost Black', 0, 6, '#070d0d', '', '{}', ${COMMON_VALUES}),
                (80, 7, 1, 'Amber', 0, 7, '#feb308', '', '{}', ${COMMON_VALUES}),
                (80, 8, 1, 'Amethyst', 0, 8, '#9b5fc0', '', '{}', ${COMMON_VALUES}),
                (80, 9, 1, 'Antique White', 0, 9, '#faebd7', '', '{}', ${COMMON_VALUES}),
                (80, 10, 1, 'Apple', 0, 10, '#6ecb3c', '', '{}', ${COMMON_VALUES}),
                (80, 11, 1, 'Apple Green', 0, 11, '#76cd26', '', '{}', ${COMMON_VALUES}),
                (80, 12, 1, 'Apricot', 0, 12, '#ffb16d', '', '{}', ${COMMON_VALUES}),
                (80, 13, 1, 'Aqua', 0, 13, '#00ffff', '', '{}', ${COMMON_VALUES}),
                (80, 14, 1, 'Aqua Blue', 0, 14, '#02d8e9', '', '{}', ${COMMON_VALUES}),
                (80, 15, 1, 'Aqua Green', 0, 15, '#12e193', '', '{}', ${COMMON_VALUES}),
                (80, 16, 1, 'Aqua Marine', 0, 16, '#2ee8bb', '', '{}', ${COMMON_VALUES}),
                (80, 17, 1, 'Aquamarine', 0, 17, '#7fffd4', '', '{}', ${COMMON_VALUES}),
                (80, 18, 1, 'Army Green', 0, 18, '#4b5d16', '', '{}', ${COMMON_VALUES}),
                (80, 19, 1, 'Asparagus', 0, 19, '#77ab56', '', '{}', ${COMMON_VALUES}),
                (80, 20, 1, 'Aubergine', 0, 20, '#3d0734', '', '{}', ${COMMON_VALUES}),
                (80, 21, 1, 'Auburn', 0, 21, '#9a3001', '', '{}', ${COMMON_VALUES}),
                (80, 22, 1, 'Avocado', 0, 22, '#90b134', '', '{}', ${COMMON_VALUES}),
                (80, 23, 1, 'Avocado Green', 0, 23, '#87a922', '', '{}', ${COMMON_VALUES}),
                (80, 24, 1, 'Azul', 0, 24, '#1d5dec', '', '{}', ${COMMON_VALUES}),
                (80, 25, 1, 'Azure', 0, 25, '#f0ffff', '', '{}', ${COMMON_VALUES}),
                (80, 26, 1, 'Baby Blue', 0, 26, '#a2cffe', '', '{}', ${COMMON_VALUES}),
                (80, 27, 1, 'Baby Green', 0, 27, '#8cff9e', '', '{}', ${COMMON_VALUES}),
                (80, 28, 1, 'Baby Pink', 0, 28, '#ffb7ce', '', '{}', ${COMMON_VALUES}),
                (80, 29, 1, 'Baby Poo', 0, 29, '#ab9004', '', '{}', ${COMMON_VALUES}),
                (80, 30, 1, 'Baby Poop', 0, 30, '#937c00', '', '{}', ${COMMON_VALUES}),
                (80, 31, 1, 'Baby Poop Green', 0, 31, '#8f9805', '', '{}', ${COMMON_VALUES}),
                (80, 32, 1, 'Baby Puke Green', 0, 32, '#b6c406', '', '{}', ${COMMON_VALUES}),
                (80, 33, 1, 'Baby Purple', 0, 33, '#ca9bf7', '', '{}', ${COMMON_VALUES}),
                (80, 34, 1, 'Baby Shit Brown', 0, 34, '#ad900d', '', '{}', ${COMMON_VALUES}),
                (80, 35, 1, 'Baby Shit Green', 0, 35, '#889717', '', '{}', ${COMMON_VALUES}),
                (80, 36, 1, 'Banana', 0, 36, '#ffff7e', '', '{}', ${COMMON_VALUES}),
                (80, 37, 1, 'Banana Yellow', 0, 37, '#fafe4b', '', '{}', ${COMMON_VALUES}),
                (80, 38, 1, 'Barbie Pink', 0, 38, '#fe46a5', '', '{}', ${COMMON_VALUES}),
                (80, 39, 1, 'Barf Green', 0, 39, '#94ac02', '', '{}', ${COMMON_VALUES}),
                (80, 40, 1, 'Barney', 0, 40, '#ac1db8', '', '{}', ${COMMON_VALUES}),
                (80, 41, 1, 'Barney Purple', 0, 41, '#a00498', '', '{}', ${COMMON_VALUES}),
                (80, 42, 1, 'Battleship Grey', 0, 42, '#6b7c85', '', '{}', ${COMMON_VALUES}),
                (80, 43, 1, 'Beige', 0, 43, '#f5f5dc', '', '{}', ${COMMON_VALUES}),
                (80, 44, 1, 'Berry', 0, 44, '#990f4b', '', '{}', ${COMMON_VALUES}),
                (80, 45, 1, 'Bile', 0, 45, '#b5c306', '', '{}', ${COMMON_VALUES}),
                (80, 46, 1, 'Bisque', 0, 46, '#ffe4c4', '', '{}', ${COMMON_VALUES}),
                (80, 47, 1, 'Black', 0, 47, '#000000', '', '{}', ${COMMON_VALUES}),
                (80, 48, 1, 'Blanched Almond', 0, 48, '#ffebcd', '', '{}', ${COMMON_VALUES}),
                (80, 49, 1, 'Bland', 0, 49, '#afa88b', '', '{}', ${COMMON_VALUES}),
                (80, 50, 1, 'Blood', 0, 50, '#770001', '', '{}', ${COMMON_VALUES}),
                (80, 51, 1, 'Blood Orange', 0, 51, '#fe4b03', '', '{}', ${COMMON_VALUES}),
                (80, 52, 1, 'Blood Red', 0, 52, '#980002', '', '{}', ${COMMON_VALUES}),
                (80, 53, 1, 'Blue', 0, 53, '#0000ff', '', '{}', ${COMMON_VALUES}),
                (80, 54, 1, 'Blue Blue', 0, 54, '#2242c7', '', '{}', ${COMMON_VALUES}),
                (80, 55, 1, 'Blue Green', 0, 55, '#137e6d', '', '{}', ${COMMON_VALUES}),
                (80, 56, 1, 'Blue Grey', 0, 56, '#607c8e', '', '{}', ${COMMON_VALUES}),
                (80, 57, 1, 'Blue Purple', 0, 57, '#5729ce', '', '{}', ${COMMON_VALUES}),
                (80, 58, 1, 'Blue Violet', 0, 58, '#8a2be2', '', '{}', ${COMMON_VALUES}),
                (80, 59, 1, 'Blue With A Hint Of Purple', 0, 59, '#533cc6', '', '{}', ${COMMON_VALUES}),
                (80, 60, 1, 'Blue/green', 0, 60, '#0f9b8e', '', '{}', ${COMMON_VALUES}),
                (80, 61, 1, 'Blue/grey', 0, 61, '#758da3', '', '{}', ${COMMON_VALUES}),
                (80, 62, 1, 'Blue/purple', 0, 62, '#5a06ef', '', '{}', ${COMMON_VALUES}),
                (80, 63, 1, 'Blueberry', 0, 63, '#464196', '', '{}', ${COMMON_VALUES}),
                (80, 64, 1, 'Bluegreen', 0, 64, '#017a79', '', '{}', ${COMMON_VALUES}),
                (80, 65, 1, 'Bluegrey', 0, 65, '#85a3b2', '', '{}', ${COMMON_VALUES}),
                (80, 66, 1, 'Bluey Green', 0, 66, '#2bb179', '', '{}', ${COMMON_VALUES}),
                (80, 67, 1, 'Bluey Grey', 0, 67, '#89a0b0', '', '{}', ${COMMON_VALUES}),
                (80, 68, 1, 'Bluey Purple', 0, 68, '#6241c7', '', '{}', ${COMMON_VALUES}),
                (80, 69, 1, 'Bluish', 0, 69, '#2976bb', '', '{}', ${COMMON_VALUES}),
                (80, 70, 1, 'Bluish Green', 0, 70, '#10a674', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 1'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 1

            Seeds lookup 080 keys 1-70.
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
               AND key_idx >= 1
               AND key_idx <= 70;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}lookups
             WHERE lookup_id = 0
               AND key_idx = 80;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 1'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 1
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
