-- Migration: acuranzo_1495.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 10/15
-- Keys 631-700. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 10

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1495"

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
                (80, 631, 1, 'Neon Green', 0, 631, '#0cff0c', '', '{}', ${COMMON_VALUES}),
                (80, 632, 1, 'Neon Pink', 0, 632, '#fe019a', '', '{}', ${COMMON_VALUES}),
                (80, 633, 1, 'Neon Purple', 0, 633, '#bc13fe', '', '{}', ${COMMON_VALUES}),
                (80, 634, 1, 'Neon Red', 0, 634, '#ff073a', '', '{}', ${COMMON_VALUES}),
                (80, 635, 1, 'Neon Yellow', 0, 635, '#cfff04', '', '{}', ${COMMON_VALUES}),
                (80, 636, 1, 'Nice Blue', 0, 636, '#107ab0', '', '{}', ${COMMON_VALUES}),
                (80, 637, 1, 'Night Blue', 0, 637, '#040348', '', '{}', ${COMMON_VALUES}),
                (80, 638, 1, 'Ocean', 0, 638, '#017b92', '', '{}', ${COMMON_VALUES}),
                (80, 639, 1, 'Ocean Blue', 0, 639, '#03719c', '', '{}', ${COMMON_VALUES}),
                (80, 640, 1, 'Ocean Green', 0, 640, '#3d9973', '', '{}', ${COMMON_VALUES}),
                (80, 641, 1, 'Ocher', 0, 641, '#bf9b0c', '', '{}', ${COMMON_VALUES}),
                (80, 642, 1, 'Ochre', 0, 642, '#bf9005', '', '{}', ${COMMON_VALUES}),
                (80, 643, 1, 'Ocre', 0, 643, '#c69c04', '', '{}', ${COMMON_VALUES}),
                (80, 644, 1, 'Off Blue', 0, 644, '#5684ae', '', '{}', ${COMMON_VALUES}),
                (80, 645, 1, 'Off Green', 0, 645, '#6ba353', '', '{}', ${COMMON_VALUES}),
                (80, 646, 1, 'Off White', 0, 646, '#ffffe4', '', '{}', ${COMMON_VALUES}),
                (80, 647, 1, 'Off Yellow', 0, 647, '#f1f33f', '', '{}', ${COMMON_VALUES}),
                (80, 648, 1, 'Old Lace', 0, 648, '#fdf5e6', '', '{}', ${COMMON_VALUES}),
                (80, 649, 1, 'Old Pink', 0, 649, '#c77986', '', '{}', ${COMMON_VALUES}),
                (80, 650, 1, 'Old Rose', 0, 650, '#c87f89', '', '{}', ${COMMON_VALUES}),
                (80, 651, 1, 'Olive', 0, 651, '#808000', '', '{}', ${COMMON_VALUES}),
                (80, 652, 1, 'Olive Brown', 0, 652, '#645403', '', '{}', ${COMMON_VALUES}),
                (80, 653, 1, 'Olive Drab', 0, 653, '#6b8e23', '', '{}', ${COMMON_VALUES}),
                (80, 654, 1, 'Olive Green', 0, 654, '#677a04', '', '{}', ${COMMON_VALUES}),
                (80, 655, 1, 'Olive Yellow', 0, 655, '#c2b709', '', '{}', ${COMMON_VALUES}),
                (80, 656, 1, 'Orange', 0, 656, '#ffa500', '', '{}', ${COMMON_VALUES}),
                (80, 657, 1, 'Orange Brown', 0, 657, '#be6400', '', '{}', ${COMMON_VALUES}),
                (80, 658, 1, 'Orange Pink', 0, 658, '#ff6f52', '', '{}', ${COMMON_VALUES}),
                (80, 659, 1, 'Orange Red', 0, 659, '#ff4500', '', '{}', ${COMMON_VALUES}),
                (80, 660, 1, 'Orange Yellow', 0, 660, '#ffad01', '', '{}', ${COMMON_VALUES}),
                (80, 661, 1, 'Orangeish', 0, 661, '#fd8d49', '', '{}', ${COMMON_VALUES}),
                (80, 662, 1, 'Orangered', 0, 662, '#fe420f', '', '{}', ${COMMON_VALUES}),
                (80, 663, 1, 'Orangey Brown', 0, 663, '#b16002', '', '{}', ${COMMON_VALUES}),
                (80, 664, 1, 'Orangey Red', 0, 664, '#fa4224', '', '{}', ${COMMON_VALUES}),
                (80, 665, 1, 'Orangey Yellow', 0, 665, '#fdb915', '', '{}', ${COMMON_VALUES}),
                (80, 666, 1, 'Orangish', 0, 666, '#fc824a', '', '{}', ${COMMON_VALUES}),
                (80, 667, 1, 'Orangish Brown', 0, 667, '#b25f03', '', '{}', ${COMMON_VALUES}),
                (80, 668, 1, 'Orangish Red', 0, 668, '#f43605', '', '{}', ${COMMON_VALUES}),
                (80, 669, 1, 'Orchid', 0, 669, '#da70d6', '', '{}', ${COMMON_VALUES}),
                (80, 670, 1, 'Pale', 0, 670, '#fff9d0', '', '{}', ${COMMON_VALUES}),
                (80, 671, 1, 'Pale Aqua', 0, 671, '#b8ffeb', '', '{}', ${COMMON_VALUES}),
                (80, 672, 1, 'Pale Blue', 0, 672, '#d0fefe', '', '{}', ${COMMON_VALUES}),
                (80, 673, 1, 'Pale Brown', 0, 673, '#b1916e', '', '{}', ${COMMON_VALUES}),
                (80, 674, 1, 'Pale Cyan', 0, 674, '#b7fffa', '', '{}', ${COMMON_VALUES}),
                (80, 675, 1, 'Pale Gold', 0, 675, '#fdde6c', '', '{}', ${COMMON_VALUES}),
                (80, 676, 1, 'Pale Goldenrod', 0, 676, '#eee8aa', '', '{}', ${COMMON_VALUES}),
                (80, 677, 1, 'Pale Green', 0, 677, '#98fb98', '', '{}', ${COMMON_VALUES}),
                (80, 678, 1, 'Pale Grey', 0, 678, '#fdfdfe', '', '{}', ${COMMON_VALUES}),
                (80, 679, 1, 'Pale Lavender', 0, 679, '#eecffe', '', '{}', ${COMMON_VALUES}),
                (80, 680, 1, 'Pale Light Green', 0, 680, '#b1fc99', '', '{}', ${COMMON_VALUES}),
                (80, 681, 1, 'Pale Lilac', 0, 681, '#e4cbff', '', '{}', ${COMMON_VALUES}),
                (80, 682, 1, 'Pale Lime', 0, 682, '#befd73', '', '{}', ${COMMON_VALUES}),
                (80, 683, 1, 'Pale Lime Green', 0, 683, '#b1ff65', '', '{}', ${COMMON_VALUES}),
                (80, 684, 1, 'Pale Magenta', 0, 684, '#d767ad', '', '{}', ${COMMON_VALUES}),
                (80, 685, 1, 'Pale Mauve', 0, 685, '#fed0fc', '', '{}', ${COMMON_VALUES}),
                (80, 686, 1, 'Pale Olive', 0, 686, '#b9cc81', '', '{}', ${COMMON_VALUES}),
                (80, 687, 1, 'Pale Olive Green', 0, 687, '#b1d27b', '', '{}', ${COMMON_VALUES}),
                (80, 688, 1, 'Pale Orange', 0, 688, '#ffa756', '', '{}', ${COMMON_VALUES}),
                (80, 689, 1, 'Pale Peach', 0, 689, '#ffe5ad', '', '{}', ${COMMON_VALUES}),
                (80, 690, 1, 'Pale Pink', 0, 690, '#ffcfdc', '', '{}', ${COMMON_VALUES}),
                (80, 691, 1, 'Pale Purple', 0, 691, '#b790d4', '', '{}', ${COMMON_VALUES}),
                (80, 692, 1, 'Pale Red', 0, 692, '#d9544d', '', '{}', ${COMMON_VALUES}),
                (80, 693, 1, 'Pale Rose', 0, 693, '#fdc1c5', '', '{}', ${COMMON_VALUES}),
                (80, 694, 1, 'Pale Salmon', 0, 694, '#ffb19a', '', '{}', ${COMMON_VALUES}),
                (80, 695, 1, 'Pale Sky Blue', 0, 695, '#bdf6fe', '', '{}', ${COMMON_VALUES}),
                (80, 696, 1, 'Pale Teal', 0, 696, '#82cbb2', '', '{}', ${COMMON_VALUES}),
                (80, 697, 1, 'Pale Turquoise', 0, 697, '#afeeee', '', '{}', ${COMMON_VALUES}),
                (80, 698, 1, 'Pale Violet', 0, 698, '#ceaefa', '', '{}', ${COMMON_VALUES}),
                (80, 699, 1, 'Pale Violet Red', 0, 699, '#db7093', '', '{}', ${COMMON_VALUES}),
                (80, 700, 1, 'Pale Yellow', 0, 700, '#ffff84', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 10'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 10

            Seeds lookup 080 keys 631-700.
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
               AND key_idx >= 631
               AND key_idx <= 700;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 10'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 10
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
