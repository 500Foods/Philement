-- Migration: acuranzo_1494.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 9/15
-- Keys 561-630. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 9

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1494"

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
                (80, 561, 1, 'Linen', 0, 561, '#faf0e6', '', '{}', ${COMMON_VALUES}),
                (80, 562, 1, 'Lipstick', 0, 562, '#d5174e', '', '{}', ${COMMON_VALUES}),
                (80, 563, 1, 'Lipstick Red', 0, 563, '#c0022f', '', '{}', ${COMMON_VALUES}),
                (80, 564, 1, 'Macaroni And Cheese', 0, 564, '#efb435', '', '{}', ${COMMON_VALUES}),
                (80, 565, 1, 'Magenta', 0, 565, '#ff00ff', '', '{}', ${COMMON_VALUES}),
                (80, 566, 1, 'Mahogany', 0, 566, '#4a0100', '', '{}', ${COMMON_VALUES}),
                (80, 567, 1, 'Maize', 0, 567, '#f4d054', '', '{}', ${COMMON_VALUES}),
                (80, 568, 1, 'Mango', 0, 568, '#ffa62b', '', '{}', ${COMMON_VALUES}),
                (80, 569, 1, 'Manilla', 0, 569, '#fffa86', '', '{}', ${COMMON_VALUES}),
                (80, 570, 1, 'Marigold', 0, 570, '#fcc006', '', '{}', ${COMMON_VALUES}),
                (80, 571, 1, 'Marine', 0, 571, '#042e60', '', '{}', ${COMMON_VALUES}),
                (80, 572, 1, 'Marine Blue', 0, 572, '#01386a', '', '{}', ${COMMON_VALUES}),
                (80, 573, 1, 'Maroon', 0, 573, '#800000', '', '{}', ${COMMON_VALUES}),
                (80, 574, 1, 'Mauve', 0, 574, '#ae7181', '', '{}', ${COMMON_VALUES}),
                (80, 575, 1, 'Medium Aquamarine', 0, 575, '#66cdaa', '', '{}', ${COMMON_VALUES}),
                (80, 576, 1, 'Medium Blue', 0, 576, '#0000cd', '', '{}', ${COMMON_VALUES}),
                (80, 577, 1, 'Medium Brown', 0, 577, '#7f5112', '', '{}', ${COMMON_VALUES}),
                (80, 578, 1, 'Medium Green', 0, 578, '#39ad48', '', '{}', ${COMMON_VALUES}),
                (80, 579, 1, 'Medium Grey', 0, 579, '#7d7f7c', '', '{}', ${COMMON_VALUES}),
                (80, 580, 1, 'Medium Orchid', 0, 580, '#ba55d3', '', '{}', ${COMMON_VALUES}),
                (80, 581, 1, 'Medium Pink', 0, 581, '#f36196', '', '{}', ${COMMON_VALUES}),
                (80, 582, 1, 'Medium Purple', 0, 582, '#9370db', '', '{}', ${COMMON_VALUES}),
                (80, 583, 1, 'Medium Sea Green', 0, 583, '#3cb371', '', '{}', ${COMMON_VALUES}),
                (80, 584, 1, 'Medium Slate Blue', 0, 584, '#7b68ee', '', '{}', ${COMMON_VALUES}),
                (80, 585, 1, 'Medium Spring Green', 0, 585, '#00fa9a', '', '{}', ${COMMON_VALUES}),
                (80, 586, 1, 'Medium Turquoise', 0, 586, '#48d1cc', '', '{}', ${COMMON_VALUES}),
                (80, 587, 1, 'Medium Violet Red', 0, 587, '#c71585', '', '{}', ${COMMON_VALUES}),
                (80, 588, 1, 'Melon', 0, 588, '#ff7855', '', '{}', ${COMMON_VALUES}),
                (80, 589, 1, 'Merlot', 0, 589, '#730039', '', '{}', ${COMMON_VALUES}),
                (80, 590, 1, 'Metallic Blue', 0, 590, '#4f738e', '', '{}', ${COMMON_VALUES}),
                (80, 591, 1, 'Mid Blue', 0, 591, '#276ab3', '', '{}', ${COMMON_VALUES}),
                (80, 592, 1, 'Mid Green', 0, 592, '#50a747', '', '{}', ${COMMON_VALUES}),
                (80, 593, 1, 'Midnight', 0, 593, '#03012d', '', '{}', ${COMMON_VALUES}),
                (80, 594, 1, 'Midnight Blue', 0, 594, '#191970', '', '{}', ${COMMON_VALUES}),
                (80, 595, 1, 'Midnight Purple', 0, 595, '#280137', '', '{}', ${COMMON_VALUES}),
                (80, 596, 1, 'Military Green', 0, 596, '#667c3e', '', '{}', ${COMMON_VALUES}),
                (80, 597, 1, 'Milk Chocolate', 0, 597, '#7f4e1e', '', '{}', ${COMMON_VALUES}),
                (80, 598, 1, 'Mint', 0, 598, '#9ffeb0', '', '{}', ${COMMON_VALUES}),
                (80, 599, 1, 'Mint Cream', 0, 599, '#f5fffa', '', '{}', ${COMMON_VALUES}),
                (80, 600, 1, 'Mint Green', 0, 600, '#8fff9f', '', '{}', ${COMMON_VALUES}),
                (80, 601, 1, 'Minty Green', 0, 601, '#0bf77d', '', '{}', ${COMMON_VALUES}),
                (80, 602, 1, 'Misty Rose', 0, 602, '#ffe4e1', '', '{}', ${COMMON_VALUES}),
                (80, 603, 1, 'Moccasin', 0, 603, '#ffe4b5', '', '{}', ${COMMON_VALUES}),
                (80, 604, 1, 'Mocha', 0, 604, '#9d7651', '', '{}', ${COMMON_VALUES}),
                (80, 605, 1, 'Moss', 0, 605, '#769958', '', '{}', ${COMMON_VALUES}),
                (80, 606, 1, 'Moss Green', 0, 606, '#658b38', '', '{}', ${COMMON_VALUES}),
                (80, 607, 1, 'Mossy Green', 0, 607, '#638b27', '', '{}', ${COMMON_VALUES}),
                (80, 608, 1, 'Mud', 0, 608, '#735c12', '', '{}', ${COMMON_VALUES}),
                (80, 609, 1, 'Mud Brown', 0, 609, '#60460f', '', '{}', ${COMMON_VALUES}),
                (80, 610, 1, 'Mud Green', 0, 610, '#606602', '', '{}', ${COMMON_VALUES}),
                (80, 611, 1, 'Muddy Brown', 0, 611, '#886806', '', '{}', ${COMMON_VALUES}),
                (80, 612, 1, 'Muddy Green', 0, 612, '#657432', '', '{}', ${COMMON_VALUES}),
                (80, 613, 1, 'Muddy Yellow', 0, 613, '#bfac05', '', '{}', ${COMMON_VALUES}),
                (80, 614, 1, 'Mulberry', 0, 614, '#920a4e', '', '{}', ${COMMON_VALUES}),
                (80, 615, 1, 'Murky Green', 0, 615, '#6c7a0e', '', '{}', ${COMMON_VALUES}),
                (80, 616, 1, 'Mushroom', 0, 616, '#ba9e88', '', '{}', ${COMMON_VALUES}),
                (80, 617, 1, 'Mustard', 0, 617, '#ceb301', '', '{}', ${COMMON_VALUES}),
                (80, 618, 1, 'Mustard Brown', 0, 618, '#ac7e04', '', '{}', ${COMMON_VALUES}),
                (80, 619, 1, 'Mustard Green', 0, 619, '#a8b504', '', '{}', ${COMMON_VALUES}),
                (80, 620, 1, 'Mustard Yellow', 0, 620, '#d2bd0a', '', '{}', ${COMMON_VALUES}),
                (80, 621, 1, 'Muted Blue', 0, 621, '#3b719f', '', '{}', ${COMMON_VALUES}),
                (80, 622, 1, 'Muted Green', 0, 622, '#5fa052', '', '{}', ${COMMON_VALUES}),
                (80, 623, 1, 'Muted Pink', 0, 623, '#d1768f', '', '{}', ${COMMON_VALUES}),
                (80, 624, 1, 'Muted Purple', 0, 624, '#805b87', '', '{}', ${COMMON_VALUES}),
                (80, 625, 1, 'Nasty Green', 0, 625, '#70b23f', '', '{}', ${COMMON_VALUES}),
                (80, 626, 1, 'Navajo White', 0, 626, '#ffdead', '', '{}', ${COMMON_VALUES}),
                (80, 627, 1, 'Navy', 0, 627, '#000080', '', '{}', ${COMMON_VALUES}),
                (80, 628, 1, 'Navy Blue', 0, 628, '#001146', '', '{}', ${COMMON_VALUES}),
                (80, 629, 1, 'Navy Green', 0, 629, '#35530a', '', '{}', ${COMMON_VALUES}),
                (80, 630, 1, 'Neon Blue', 0, 630, '#04d9ff', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 9'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 9

            Seeds lookup 080 keys 561-630.
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
               AND key_idx >= 561
               AND key_idx <= 630;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 9'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 9
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
