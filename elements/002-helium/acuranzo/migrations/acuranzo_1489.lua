-- Migration: acuranzo_1489.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 4/15
-- Keys 211-280. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 4

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1489"

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
                (80, 211, 1, 'Dark Gray', 0, 211, '#a9a9a9', '', '{}', ${COMMON_VALUES}),
                (80, 212, 1, 'Dark Green', 0, 212, '#006400', '', '{}', ${COMMON_VALUES}),
                (80, 213, 1, 'Dark Green Blue', 0, 213, '#1f6357', '', '{}', ${COMMON_VALUES}),
                (80, 214, 1, 'Dark Grey', 0, 214, '#a9a9a9', '', '{}', ${COMMON_VALUES}),
                (80, 215, 1, 'Dark Grey Blue', 0, 215, '#29465b', '', '{}', ${COMMON_VALUES}),
                (80, 216, 1, 'Dark Hot Pink', 0, 216, '#d90166', '', '{}', ${COMMON_VALUES}),
                (80, 217, 1, 'Dark Indigo', 0, 217, '#1f0954', '', '{}', ${COMMON_VALUES}),
                (80, 218, 1, 'Dark Khaki', 0, 218, '#bdb76b', '', '{}', ${COMMON_VALUES}),
                (80, 219, 1, 'Dark Lavender', 0, 219, '#856798', '', '{}', ${COMMON_VALUES}),
                (80, 220, 1, 'Dark Lilac', 0, 220, '#9c6da5', '', '{}', ${COMMON_VALUES}),
                (80, 221, 1, 'Dark Lime', 0, 221, '#84b701', '', '{}', ${COMMON_VALUES}),
                (80, 222, 1, 'Dark Lime Green', 0, 222, '#7ebd01', '', '{}', ${COMMON_VALUES}),
                (80, 223, 1, 'Dark Magenta', 0, 223, '#8b008b', '', '{}', ${COMMON_VALUES}),
                (80, 224, 1, 'Dark Maroon', 0, 224, '#3c0008', '', '{}', ${COMMON_VALUES}),
                (80, 225, 1, 'Dark Mauve', 0, 225, '#874c62', '', '{}', ${COMMON_VALUES}),
                (80, 226, 1, 'Dark Mint', 0, 226, '#48c072', '', '{}', ${COMMON_VALUES}),
                (80, 227, 1, 'Dark Mint Green', 0, 227, '#20c073', '', '{}', ${COMMON_VALUES}),
                (80, 228, 1, 'Dark Mustard', 0, 228, '#a88905', '', '{}', ${COMMON_VALUES}),
                (80, 229, 1, 'Dark Navy', 0, 229, '#000435', '', '{}', ${COMMON_VALUES}),
                (80, 230, 1, 'Dark Navy Blue', 0, 230, '#00022e', '', '{}', ${COMMON_VALUES}),
                (80, 231, 1, 'Dark Olive', 0, 231, '#373e02', '', '{}', ${COMMON_VALUES}),
                (80, 232, 1, 'Dark Olive Green', 0, 232, '#556b2f', '', '{}', ${COMMON_VALUES}),
                (80, 233, 1, 'Dark Orange', 0, 233, '#ff8c00', '', '{}', ${COMMON_VALUES}),
                (80, 234, 1, 'Dark Orchid', 0, 234, '#9932cc', '', '{}', ${COMMON_VALUES}),
                (80, 235, 1, 'Dark Pastel Green', 0, 235, '#56ae57', '', '{}', ${COMMON_VALUES}),
                (80, 236, 1, 'Dark Peach', 0, 236, '#de7e5d', '', '{}', ${COMMON_VALUES}),
                (80, 237, 1, 'Dark Periwinkle', 0, 237, '#665fd1', '', '{}', ${COMMON_VALUES}),
                (80, 238, 1, 'Dark Pink', 0, 238, '#cb416b', '', '{}', ${COMMON_VALUES}),
                (80, 239, 1, 'Dark Plum', 0, 239, '#3f012c', '', '{}', ${COMMON_VALUES}),
                (80, 240, 1, 'Dark Purple', 0, 240, '#35063e', '', '{}', ${COMMON_VALUES}),
                (80, 241, 1, 'Dark Red', 0, 241, '#8b0000', '', '{}', ${COMMON_VALUES}),
                (80, 242, 1, 'Dark Rose', 0, 242, '#b5485d', '', '{}', ${COMMON_VALUES}),
                (80, 243, 1, 'Dark Royal Blue', 0, 243, '#02066f', '', '{}', ${COMMON_VALUES}),
                (80, 244, 1, 'Dark Sage', 0, 244, '#598556', '', '{}', ${COMMON_VALUES}),
                (80, 245, 1, 'Dark Salmon', 0, 245, '#e9967a', '', '{}', ${COMMON_VALUES}),
                (80, 246, 1, 'Dark Sand', 0, 246, '#a88f59', '', '{}', ${COMMON_VALUES}),
                (80, 247, 1, 'Dark Sea Green', 0, 247, '#8fbc8f', '', '{}', ${COMMON_VALUES}),
                (80, 248, 1, 'Dark Seafoam', 0, 248, '#1fb57a', '', '{}', ${COMMON_VALUES}),
                (80, 249, 1, 'Dark Seafoam Green', 0, 249, '#3eaf76', '', '{}', ${COMMON_VALUES}),
                (80, 250, 1, 'Dark Sky Blue', 0, 250, '#448ee4', '', '{}', ${COMMON_VALUES}),
                (80, 251, 1, 'Dark Slate Blue', 0, 251, '#483d8b', '', '{}', ${COMMON_VALUES}),
                (80, 252, 1, 'Dark Slate Gray', 0, 252, '#2f4f4f', '', '{}', ${COMMON_VALUES}),
                (80, 253, 1, 'Dark Slate Grey', 0, 253, '#2f4f4f', '', '{}', ${COMMON_VALUES}),
                (80, 254, 1, 'Dark Tan', 0, 254, '#af884a', '', '{}', ${COMMON_VALUES}),
                (80, 255, 1, 'Dark Taupe', 0, 255, '#7f684e', '', '{}', ${COMMON_VALUES}),
                (80, 256, 1, 'Dark Teal', 0, 256, '#014d4e', '', '{}', ${COMMON_VALUES}),
                (80, 257, 1, 'Dark Turquoise', 0, 257, '#00ced1', '', '{}', ${COMMON_VALUES}),
                (80, 258, 1, 'Dark Violet', 0, 258, '#9400d3', '', '{}', ${COMMON_VALUES}),
                (80, 259, 1, 'Dark Yellow', 0, 259, '#d5b60a', '', '{}', ${COMMON_VALUES}),
                (80, 260, 1, 'Dark Yellow Green', 0, 260, '#728f02', '', '{}', ${COMMON_VALUES}),
                (80, 261, 1, 'Darkblue', 0, 261, '#030764', '', '{}', ${COMMON_VALUES}),
                (80, 262, 1, 'Darkgreen', 0, 262, '#054907', '', '{}', ${COMMON_VALUES}),
                (80, 263, 1, 'Darkish Blue', 0, 263, '#014182', '', '{}', ${COMMON_VALUES}),
                (80, 264, 1, 'Darkish Green', 0, 264, '#287c37', '', '{}', ${COMMON_VALUES}),
                (80, 265, 1, 'Darkish Pink', 0, 265, '#da467d', '', '{}', ${COMMON_VALUES}),
                (80, 266, 1, 'Darkish Purple', 0, 266, '#751973', '', '{}', ${COMMON_VALUES}),
                (80, 267, 1, 'Darkish Red', 0, 267, '#a90308', '', '{}', ${COMMON_VALUES}),
                (80, 268, 1, 'Deep Aqua', 0, 268, '#08787f', '', '{}', ${COMMON_VALUES}),
                (80, 269, 1, 'Deep Blue', 0, 269, '#040273', '', '{}', ${COMMON_VALUES}),
                (80, 270, 1, 'Deep Brown', 0, 270, '#410200', '', '{}', ${COMMON_VALUES}),
                (80, 271, 1, 'Deep Green', 0, 271, '#02590f', '', '{}', ${COMMON_VALUES}),
                (80, 272, 1, 'Deep Lavender', 0, 272, '#8d5eb7', '', '{}', ${COMMON_VALUES}),
                (80, 273, 1, 'Deep Lilac', 0, 273, '#966ebd', '', '{}', ${COMMON_VALUES}),
                (80, 274, 1, 'Deep Magenta', 0, 274, '#a0025c', '', '{}', ${COMMON_VALUES}),
                (80, 275, 1, 'Deep Orange', 0, 275, '#dc4d01', '', '{}', ${COMMON_VALUES}),
                (80, 276, 1, 'Deep Pink', 0, 276, '#ff1493', '', '{}', ${COMMON_VALUES}),
                (80, 277, 1, 'Deep Purple', 0, 277, '#36013f', '', '{}', ${COMMON_VALUES}),
                (80, 278, 1, 'Deep Red', 0, 278, '#9a0200', '', '{}', ${COMMON_VALUES}),
                (80, 279, 1, 'Deep Rose', 0, 279, '#c74767', '', '{}', ${COMMON_VALUES}),
                (80, 280, 1, 'Deep Sea Blue', 0, 280, '#015482', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 4'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 4

            Seeds lookup 080 keys 211-280.
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
               AND key_idx >= 211
               AND key_idx <= 280;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 4'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 4
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
