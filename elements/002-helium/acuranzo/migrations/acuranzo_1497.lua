-- Migration: acuranzo_1497.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 12/15
-- Keys 771-840. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 12

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1497"

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
                (80, 771, 1, 'Purpleish', 0, 771, '#98568d', '', '{}', ${COMMON_VALUES}),
                (80, 772, 1, 'Purpleish Blue', 0, 772, '#6140ef', '', '{}', ${COMMON_VALUES}),
                (80, 773, 1, 'Purpleish Pink', 0, 773, '#df4ec8', '', '{}', ${COMMON_VALUES}),
                (80, 774, 1, 'Purpley', 0, 774, '#8756e4', '', '{}', ${COMMON_VALUES}),
                (80, 775, 1, 'Purpley Blue', 0, 775, '#5f34e7', '', '{}', ${COMMON_VALUES}),
                (80, 776, 1, 'Purpley Grey', 0, 776, '#947e94', '', '{}', ${COMMON_VALUES}),
                (80, 777, 1, 'Purpley Pink', 0, 777, '#c83cb9', '', '{}', ${COMMON_VALUES}),
                (80, 778, 1, 'Purplish', 0, 778, '#94568c', '', '{}', ${COMMON_VALUES}),
                (80, 779, 1, 'Purplish Blue', 0, 779, '#601ef9', '', '{}', ${COMMON_VALUES}),
                (80, 780, 1, 'Purplish Brown', 0, 780, '#6b4247', '', '{}', ${COMMON_VALUES}),
                (80, 781, 1, 'Purplish Grey', 0, 781, '#7a687f', '', '{}', ${COMMON_VALUES}),
                (80, 782, 1, 'Purplish Pink', 0, 782, '#ce5dae', '', '{}', ${COMMON_VALUES}),
                (80, 783, 1, 'Purplish Red', 0, 783, '#b0054b', '', '{}', ${COMMON_VALUES}),
                (80, 784, 1, 'Purply', 0, 784, '#983fb2', '', '{}', ${COMMON_VALUES}),
                (80, 785, 1, 'Purply Blue', 0, 785, '#661aee', '', '{}', ${COMMON_VALUES}),
                (80, 786, 1, 'Purply Pink', 0, 786, '#f075e6', '', '{}', ${COMMON_VALUES}),
                (80, 787, 1, 'Putty', 0, 787, '#beae8a', '', '{}', ${COMMON_VALUES}),
                (80, 788, 1, 'Racing Green', 0, 788, '#014600', '', '{}', ${COMMON_VALUES}),
                (80, 789, 1, 'Radioactive Green', 0, 789, '#2cfa1f', '', '{}', ${COMMON_VALUES}),
                (80, 790, 1, 'Raspberry', 0, 790, '#b00149', '', '{}', ${COMMON_VALUES}),
                (80, 791, 1, 'Raw Sienna', 0, 791, '#9a6200', '', '{}', ${COMMON_VALUES}),
                (80, 792, 1, 'Raw Umber', 0, 792, '#a75e09', '', '{}', ${COMMON_VALUES}),
                (80, 793, 1, 'Really Light Blue', 0, 793, '#d4ffff', '', '{}', ${COMMON_VALUES}),
                (80, 794, 1, 'Rebecca Purple', 0, 794, '#663399', '', '{}', ${COMMON_VALUES}),
                (80, 795, 1, 'Red', 0, 795, '#ff0000', '', '{}', ${COMMON_VALUES}),
                (80, 796, 1, 'Red Brown', 0, 796, '#8b2e16', '', '{}', ${COMMON_VALUES}),
                (80, 797, 1, 'Red Orange', 0, 797, '#fd3c06', '', '{}', ${COMMON_VALUES}),
                (80, 798, 1, 'Red Pink', 0, 798, '#fa2a55', '', '{}', ${COMMON_VALUES}),
                (80, 799, 1, 'Red Purple', 0, 799, '#820747', '', '{}', ${COMMON_VALUES}),
                (80, 800, 1, 'Red Violet', 0, 800, '#9e0168', '', '{}', ${COMMON_VALUES}),
                (80, 801, 1, 'Red Wine', 0, 801, '#8c0034', '', '{}', ${COMMON_VALUES}),
                (80, 802, 1, 'Reddish', 0, 802, '#c44240', '', '{}', ${COMMON_VALUES}),
                (80, 803, 1, 'Reddish Brown', 0, 803, '#7f2b0a', '', '{}', ${COMMON_VALUES}),
                (80, 804, 1, 'Reddish Grey', 0, 804, '#997570', '', '{}', ${COMMON_VALUES}),
                (80, 805, 1, 'Reddish Orange', 0, 805, '#f8481c', '', '{}', ${COMMON_VALUES}),
                (80, 806, 1, 'Reddish Pink', 0, 806, '#fe2c54', '', '{}', ${COMMON_VALUES}),
                (80, 807, 1, 'Reddish Purple', 0, 807, '#910951', '', '{}', ${COMMON_VALUES}),
                (80, 808, 1, 'Reddy Brown', 0, 808, '#6e1005', '', '{}', ${COMMON_VALUES}),
                (80, 809, 1, 'Rich Blue', 0, 809, '#021bf9', '', '{}', ${COMMON_VALUES}),
                (80, 810, 1, 'Rich Purple', 0, 810, '#720058', '', '{}', ${COMMON_VALUES}),
                (80, 811, 1, 'Robin Egg Blue', 0, 811, '#8af1fe', '', '{}', ${COMMON_VALUES}),
                (80, 812, 1, 'Robin''s Egg', 0, 812, '#6dedfd', '', '{}', ${COMMON_VALUES}),
                (80, 813, 1, 'Robin''s Egg Blue', 0, 813, '#98eff9', '', '{}', ${COMMON_VALUES}),
                (80, 814, 1, 'Rosa', 0, 814, '#fe86a4', '', '{}', ${COMMON_VALUES}),
                (80, 815, 1, 'Rose', 0, 815, '#cf6275', '', '{}', ${COMMON_VALUES}),
                (80, 816, 1, 'Rose Pink', 0, 816, '#f7879a', '', '{}', ${COMMON_VALUES}),
                (80, 817, 1, 'Rose Red', 0, 817, '#be013c', '', '{}', ${COMMON_VALUES}),
                (80, 818, 1, 'Rosy Brown', 0, 818, '#bc8f8f', '', '{}', ${COMMON_VALUES}),
                (80, 819, 1, 'Rosy Pink', 0, 819, '#f6688e', '', '{}', ${COMMON_VALUES}),
                (80, 820, 1, 'Rouge', 0, 820, '#ab1239', '', '{}', ${COMMON_VALUES}),
                (80, 821, 1, 'Royal', 0, 821, '#0c1793', '', '{}', ${COMMON_VALUES}),
                (80, 822, 1, 'Royal Blue', 0, 822, '#4169e1', '', '{}', ${COMMON_VALUES}),
                (80, 823, 1, 'Royal Purple', 0, 823, '#4b006e', '', '{}', ${COMMON_VALUES}),
                (80, 824, 1, 'Ruby', 0, 824, '#ca0147', '', '{}', ${COMMON_VALUES}),
                (80, 825, 1, 'Russet', 0, 825, '#a13905', '', '{}', ${COMMON_VALUES}),
                (80, 826, 1, 'Rust', 0, 826, '#a83c09', '', '{}', ${COMMON_VALUES}),
                (80, 827, 1, 'Rust Brown', 0, 827, '#8b3103', '', '{}', ${COMMON_VALUES}),
                (80, 828, 1, 'Rust Orange', 0, 828, '#c45508', '', '{}', ${COMMON_VALUES}),
                (80, 829, 1, 'Rust Red', 0, 829, '#aa2704', '', '{}', ${COMMON_VALUES}),
                (80, 830, 1, 'Rusty Orange', 0, 830, '#cd5909', '', '{}', ${COMMON_VALUES}),
                (80, 831, 1, 'Rusty Red', 0, 831, '#af2f0d', '', '{}', ${COMMON_VALUES}),
                (80, 832, 1, 'Saddle Brown', 0, 832, '#8b4513', '', '{}', ${COMMON_VALUES}),
                (80, 833, 1, 'Saffron', 0, 833, '#feb209', '', '{}', ${COMMON_VALUES}),
                (80, 834, 1, 'Sage', 0, 834, '#87ae73', '', '{}', ${COMMON_VALUES}),
                (80, 835, 1, 'Sage Green', 0, 835, '#88b378', '', '{}', ${COMMON_VALUES}),
                (80, 836, 1, 'Salmon', 0, 836, '#fa8072', '', '{}', ${COMMON_VALUES}),
                (80, 837, 1, 'Salmon Pink', 0, 837, '#fe7b7c', '', '{}', ${COMMON_VALUES}),
                (80, 838, 1, 'Sand', 0, 838, '#e2ca76', '', '{}', ${COMMON_VALUES}),
                (80, 839, 1, 'Sand Brown', 0, 839, '#cba560', '', '{}', ${COMMON_VALUES}),
                (80, 840, 1, 'Sand Yellow', 0, 840, '#fce166', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 12'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 12

            Seeds lookup 080 keys 771-840.
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
               AND key_idx >= 771
               AND key_idx <= 840;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 12'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 12
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
