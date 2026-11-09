-- Migration: acuranzo_1492.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 7/15
-- Keys 421-490. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 7

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1492"

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
                (80, 421, 1, 'Greyish Blue', 0, 421, '#5e819d', '', '{}', ${COMMON_VALUES}),
                (80, 422, 1, 'Greyish Brown', 0, 422, '#7a6a4f', '', '{}', ${COMMON_VALUES}),
                (80, 423, 1, 'Greyish Green', 0, 423, '#82a67d', '', '{}', ${COMMON_VALUES}),
                (80, 424, 1, 'Greyish Pink', 0, 424, '#c88d94', '', '{}', ${COMMON_VALUES}),
                (80, 425, 1, 'Greyish Purple', 0, 425, '#887191', '', '{}', ${COMMON_VALUES}),
                (80, 426, 1, 'Greyish Teal', 0, 426, '#719f91', '', '{}', ${COMMON_VALUES}),
                (80, 427, 1, 'Gross Green', 0, 427, '#a0bf16', '', '{}', ${COMMON_VALUES}),
                (80, 428, 1, 'Gunmetal', 0, 428, '#536267', '', '{}', ${COMMON_VALUES}),
                (80, 429, 1, 'Hazel', 0, 429, '#8e7618', '', '{}', ${COMMON_VALUES}),
                (80, 430, 1, 'Heather', 0, 430, '#a484ac', '', '{}', ${COMMON_VALUES}),
                (80, 431, 1, 'Heliotrope', 0, 431, '#d94ff5', '', '{}', ${COMMON_VALUES}),
                (80, 432, 1, 'Highlighter Green', 0, 432, '#1bfc06', '', '{}', ${COMMON_VALUES}),
                (80, 433, 1, 'Honeydew', 0, 433, '#f0fff0', '', '{}', ${COMMON_VALUES}),
                (80, 434, 1, 'Hospital Green', 0, 434, '#9be5aa', '', '{}', ${COMMON_VALUES}),
                (80, 435, 1, 'Hot Green', 0, 435, '#25ff29', '', '{}', ${COMMON_VALUES}),
                (80, 436, 1, 'Hot Magenta', 0, 436, '#f504c9', '', '{}', ${COMMON_VALUES}),
                (80, 437, 1, 'Hot Pink', 0, 437, '#ff69b4', '', '{}', ${COMMON_VALUES}),
                (80, 438, 1, 'Hot Purple', 0, 438, '#cb00f5', '', '{}', ${COMMON_VALUES}),
                (80, 439, 1, 'Hunter Green', 0, 439, '#0b4008', '', '{}', ${COMMON_VALUES}),
                (80, 440, 1, 'Ice', 0, 440, '#d6fffa', '', '{}', ${COMMON_VALUES}),
                (80, 441, 1, 'Ice Blue', 0, 441, '#d7fffe', '', '{}', ${COMMON_VALUES}),
                (80, 442, 1, 'Icky Green', 0, 442, '#8fae22', '', '{}', ${COMMON_VALUES}),
                (80, 443, 1, 'Indian Red', 0, 443, '#cd5c5c', '', '{}', ${COMMON_VALUES}),
                (80, 444, 1, 'Indigo', 0, 444, '#4b0082', '', '{}', ${COMMON_VALUES}),
                (80, 445, 1, 'Indigo Blue', 0, 445, '#3a18b1', '', '{}', ${COMMON_VALUES}),
                (80, 446, 1, 'Iris', 0, 446, '#6258c4', '', '{}', ${COMMON_VALUES}),
                (80, 447, 1, 'Irish Green', 0, 447, '#019529', '', '{}', ${COMMON_VALUES}),
                (80, 448, 1, 'Ivory', 0, 448, '#fffff0', '', '{}', ${COMMON_VALUES}),
                (80, 449, 1, 'Jade', 0, 449, '#1fa774', '', '{}', ${COMMON_VALUES}),
                (80, 450, 1, 'Jade Green', 0, 450, '#2baf6a', '', '{}', ${COMMON_VALUES}),
                (80, 451, 1, 'Jungle Green', 0, 451, '#048243', '', '{}', ${COMMON_VALUES}),
                (80, 452, 1, 'Kelley Green', 0, 452, '#009337', '', '{}', ${COMMON_VALUES}),
                (80, 453, 1, 'Kelly Green', 0, 453, '#02ab2e', '', '{}', ${COMMON_VALUES}),
                (80, 454, 1, 'Kermit Green', 0, 454, '#5cb200', '', '{}', ${COMMON_VALUES}),
                (80, 455, 1, 'Key Lime', 0, 455, '#aeff6e', '', '{}', ${COMMON_VALUES}),
                (80, 456, 1, 'Khaki', 0, 456, '#f0e68c', '', '{}', ${COMMON_VALUES}),
                (80, 457, 1, 'Khaki Green', 0, 457, '#728639', '', '{}', ${COMMON_VALUES}),
                (80, 458, 1, 'Kiwi', 0, 458, '#9cef43', '', '{}', ${COMMON_VALUES}),
                (80, 459, 1, 'Kiwi Green', 0, 459, '#8ee53f', '', '{}', ${COMMON_VALUES}),
                (80, 460, 1, 'Lavender', 0, 460, '#e6e6fa', '', '{}', ${COMMON_VALUES}),
                (80, 461, 1, 'Lavender Blue', 0, 461, '#8b88f8', '', '{}', ${COMMON_VALUES}),
                (80, 462, 1, 'Lavender Blush', 0, 462, '#fff0f5', '', '{}', ${COMMON_VALUES}),
                (80, 463, 1, 'Lavender Pink', 0, 463, '#dd85d7', '', '{}', ${COMMON_VALUES}),
                (80, 464, 1, 'Lawn Green', 0, 464, '#7cfc00', '', '{}', ${COMMON_VALUES}),
                (80, 465, 1, 'Leaf', 0, 465, '#71aa34', '', '{}', ${COMMON_VALUES}),
                (80, 466, 1, 'Leaf Green', 0, 466, '#5ca904', '', '{}', ${COMMON_VALUES}),
                (80, 467, 1, 'Leafy Green', 0, 467, '#51b73b', '', '{}', ${COMMON_VALUES}),
                (80, 468, 1, 'Leather', 0, 468, '#ac7434', '', '{}', ${COMMON_VALUES}),
                (80, 469, 1, 'Lemon', 0, 469, '#fdff52', '', '{}', ${COMMON_VALUES}),
                (80, 470, 1, 'Lemon Chiffon', 0, 470, '#fffacd', '', '{}', ${COMMON_VALUES}),
                (80, 471, 1, 'Lemon Green', 0, 471, '#adf802', '', '{}', ${COMMON_VALUES}),
                (80, 472, 1, 'Lemon Lime', 0, 472, '#bffe28', '', '{}', ${COMMON_VALUES}),
                (80, 473, 1, 'Lemon Yellow', 0, 473, '#fdff38', '', '{}', ${COMMON_VALUES}),
                (80, 474, 1, 'Lichen', 0, 474, '#8fb67b', '', '{}', ${COMMON_VALUES}),
                (80, 475, 1, 'Light Aqua', 0, 475, '#8cffdb', '', '{}', ${COMMON_VALUES}),
                (80, 476, 1, 'Light Aquamarine', 0, 476, '#7bfdc7', '', '{}', ${COMMON_VALUES}),
                (80, 477, 1, 'Light Beige', 0, 477, '#fffeb6', '', '{}', ${COMMON_VALUES}),
                (80, 478, 1, 'Light Blue', 0, 478, '#add8e6', '', '{}', ${COMMON_VALUES}),
                (80, 479, 1, 'Light Blue Green', 0, 479, '#7efbb3', '', '{}', ${COMMON_VALUES}),
                (80, 480, 1, 'Light Blue Grey', 0, 480, '#b7c9e2', '', '{}', ${COMMON_VALUES}),
                (80, 481, 1, 'Light Bluish Green', 0, 481, '#76fda8', '', '{}', ${COMMON_VALUES}),
                (80, 482, 1, 'Light Bright Green', 0, 482, '#53fe5c', '', '{}', ${COMMON_VALUES}),
                (80, 483, 1, 'Light Brown', 0, 483, '#ad8150', '', '{}', ${COMMON_VALUES}),
                (80, 484, 1, 'Light Burgundy', 0, 484, '#a8415b', '', '{}', ${COMMON_VALUES}),
                (80, 485, 1, 'Light Coral', 0, 485, '#f08080', '', '{}', ${COMMON_VALUES}),
                (80, 486, 1, 'Light Cyan', 0, 486, '#e0ffff', '', '{}', ${COMMON_VALUES}),
                (80, 487, 1, 'Light Eggplant', 0, 487, '#894585', '', '{}', ${COMMON_VALUES}),
                (80, 488, 1, 'Light Forest Green', 0, 488, '#4f9153', '', '{}', ${COMMON_VALUES}),
                (80, 489, 1, 'Light Gold', 0, 489, '#fddc5c', '', '{}', ${COMMON_VALUES}),
                (80, 490, 1, 'Light Goldenrod Yellow', 0, 490, '#fafad2', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 7'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 7

            Seeds lookup 080 keys 421-490.
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
               AND key_idx >= 421
               AND key_idx <= 490;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 7'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 7
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
