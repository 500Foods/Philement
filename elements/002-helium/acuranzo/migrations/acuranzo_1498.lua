-- Migration: acuranzo_1498.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 13/15
-- Keys 841-910. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 13

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1498"

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
                (80, 841, 1, 'Sandstone', 0, 841, '#c9ae74', '', '{}', ${COMMON_VALUES}),
                (80, 842, 1, 'Sandy', 0, 842, '#f1da7a', '', '{}', ${COMMON_VALUES}),
                (80, 843, 1, 'Sandy Brown', 0, 843, '#f4a460', '', '{}', ${COMMON_VALUES}),
                (80, 844, 1, 'Sandy Yellow', 0, 844, '#fdee73', '', '{}', ${COMMON_VALUES}),
                (80, 845, 1, 'Sap Green', 0, 845, '#5c8b15', '', '{}', ${COMMON_VALUES}),
                (80, 846, 1, 'Sapphire', 0, 846, '#2138ab', '', '{}', ${COMMON_VALUES}),
                (80, 847, 1, 'Scarlet', 0, 847, '#be0119', '', '{}', ${COMMON_VALUES}),
                (80, 848, 1, 'Sea', 0, 848, '#3c9992', '', '{}', ${COMMON_VALUES}),
                (80, 849, 1, 'Sea Blue', 0, 849, '#047495', '', '{}', ${COMMON_VALUES}),
                (80, 850, 1, 'Sea Green', 0, 850, '#2e8b57', '', '{}', ${COMMON_VALUES}),
                (80, 851, 1, 'Seafoam', 0, 851, '#80f9ad', '', '{}', ${COMMON_VALUES}),
                (80, 852, 1, 'Seafoam Blue', 0, 852, '#78d1b6', '', '{}', ${COMMON_VALUES}),
                (80, 853, 1, 'Seafoam Green', 0, 853, '#7af9ab', '', '{}', ${COMMON_VALUES}),
                (80, 854, 1, 'Seashell', 0, 854, '#fff5ee', '', '{}', ${COMMON_VALUES}),
                (80, 855, 1, 'Seaweed', 0, 855, '#18d17b', '', '{}', ${COMMON_VALUES}),
                (80, 856, 1, 'Seaweed Green', 0, 856, '#35ad6b', '', '{}', ${COMMON_VALUES}),
                (80, 857, 1, 'Sepia', 0, 857, '#985e2b', '', '{}', ${COMMON_VALUES}),
                (80, 858, 1, 'Shamrock', 0, 858, '#01b44c', '', '{}', ${COMMON_VALUES}),
                (80, 859, 1, 'Shamrock Green', 0, 859, '#02c14d', '', '{}', ${COMMON_VALUES}),
                (80, 860, 1, 'Shit', 0, 860, '#7f5f00', '', '{}', ${COMMON_VALUES}),
                (80, 861, 1, 'Shit Brown', 0, 861, '#7b5804', '', '{}', ${COMMON_VALUES}),
                (80, 862, 1, 'Shit Green', 0, 862, '#758000', '', '{}', ${COMMON_VALUES}),
                (80, 863, 1, 'Shocking Pink', 0, 863, '#fe02a2', '', '{}', ${COMMON_VALUES}),
                (80, 864, 1, 'Sick Green', 0, 864, '#9db92c', '', '{}', ${COMMON_VALUES}),
                (80, 865, 1, 'Sickly Green', 0, 865, '#94b21c', '', '{}', ${COMMON_VALUES}),
                (80, 866, 1, 'Sickly Yellow', 0, 866, '#d0e429', '', '{}', ${COMMON_VALUES}),
                (80, 867, 1, 'Sienna', 0, 867, '#a0522d', '', '{}', ${COMMON_VALUES}),
                (80, 868, 1, 'Silver', 0, 868, '#c0c0c0', '', '{}', ${COMMON_VALUES}),
                (80, 869, 1, 'Sky', 0, 869, '#82cafc', '', '{}', ${COMMON_VALUES}),
                (80, 870, 1, 'Sky Blue', 0, 870, '#87ceeb', '', '{}', ${COMMON_VALUES}),
                (80, 871, 1, 'Slate', 0, 871, '#516572', '', '{}', ${COMMON_VALUES}),
                (80, 872, 1, 'Slate Blue', 0, 872, '#6a5acd', '', '{}', ${COMMON_VALUES}),
                (80, 873, 1, 'Slate Gray', 0, 873, '#708090', '', '{}', ${COMMON_VALUES}),
                (80, 874, 1, 'Slate Green', 0, 874, '#658d6d', '', '{}', ${COMMON_VALUES}),
                (80, 875, 1, 'Slate Grey', 0, 875, '#708090', '', '{}', ${COMMON_VALUES}),
                (80, 876, 1, 'Slime Green', 0, 876, '#99cc04', '', '{}', ${COMMON_VALUES}),
                (80, 877, 1, 'Snot', 0, 877, '#acbb0d', '', '{}', ${COMMON_VALUES}),
                (80, 878, 1, 'Snot Green', 0, 878, '#9dc100', '', '{}', ${COMMON_VALUES}),
                (80, 879, 1, 'Snow', 0, 879, '#fffafa', '', '{}', ${COMMON_VALUES}),
                (80, 880, 1, 'Soft Blue', 0, 880, '#6488ea', '', '{}', ${COMMON_VALUES}),
                (80, 881, 1, 'Soft Green', 0, 881, '#6fc276', '', '{}', ${COMMON_VALUES}),
                (80, 882, 1, 'Soft Pink', 0, 882, '#fdb0c0', '', '{}', ${COMMON_VALUES}),
                (80, 883, 1, 'Soft Purple', 0, 883, '#a66fb5', '', '{}', ${COMMON_VALUES}),
                (80, 884, 1, 'Spearmint', 0, 884, '#1ef876', '', '{}', ${COMMON_VALUES}),
                (80, 885, 1, 'Spring Green', 0, 885, '#00ff7f', '', '{}', ${COMMON_VALUES}),
                (80, 886, 1, 'Spruce', 0, 886, '#0a5f38', '', '{}', ${COMMON_VALUES}),
                (80, 887, 1, 'Squash', 0, 887, '#f2ab15', '', '{}', ${COMMON_VALUES}),
                (80, 888, 1, 'Steel', 0, 888, '#738595', '', '{}', ${COMMON_VALUES}),
                (80, 889, 1, 'Steel Blue', 0, 889, '#4682b4', '', '{}', ${COMMON_VALUES}),
                (80, 890, 1, 'Steel Grey', 0, 890, '#6f828a', '', '{}', ${COMMON_VALUES}),
                (80, 891, 1, 'Stone', 0, 891, '#ada587', '', '{}', ${COMMON_VALUES}),
                (80, 892, 1, 'Stormy Blue', 0, 892, '#507b9c', '', '{}', ${COMMON_VALUES}),
                (80, 893, 1, 'Straw', 0, 893, '#fcf679', '', '{}', ${COMMON_VALUES}),
                (80, 894, 1, 'Strawberry', 0, 894, '#fb2943', '', '{}', ${COMMON_VALUES}),
                (80, 895, 1, 'Strong Blue', 0, 895, '#0c06f7', '', '{}', ${COMMON_VALUES}),
                (80, 896, 1, 'Strong Pink', 0, 896, '#ff0789', '', '{}', ${COMMON_VALUES}),
                (80, 897, 1, 'Sun Yellow', 0, 897, '#ffdf22', '', '{}', ${COMMON_VALUES}),
                (80, 898, 1, 'Sunflower', 0, 898, '#ffc512', '', '{}', ${COMMON_VALUES}),
                (80, 899, 1, 'Sunflower Yellow', 0, 899, '#ffda03', '', '{}', ${COMMON_VALUES}),
                (80, 900, 1, 'Sunny Yellow', 0, 900, '#fff917', '', '{}', ${COMMON_VALUES}),
                (80, 901, 1, 'Sunshine Yellow', 0, 901, '#fffd37', '', '{}', ${COMMON_VALUES}),
                (80, 902, 1, 'Swamp', 0, 902, '#698339', '', '{}', ${COMMON_VALUES}),
                (80, 903, 1, 'Swamp Green', 0, 903, '#748500', '', '{}', ${COMMON_VALUES}),
                (80, 904, 1, 'Tan', 0, 904, '#d2b48c', '', '{}', ${COMMON_VALUES}),
                (80, 905, 1, 'Tan Brown', 0, 905, '#ab7e4c', '', '{}', ${COMMON_VALUES}),
                (80, 906, 1, 'Tan Green', 0, 906, '#a9be70', '', '{}', ${COMMON_VALUES}),
                (80, 907, 1, 'Tangerine', 0, 907, '#ff9408', '', '{}', ${COMMON_VALUES}),
                (80, 908, 1, 'Taupe', 0, 908, '#b9a281', '', '{}', ${COMMON_VALUES}),
                (80, 909, 1, 'Tea', 0, 909, '#65ab7c', '', '{}', ${COMMON_VALUES}),
                (80, 910, 1, 'Tea Green', 0, 910, '#bdf8a3', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 13'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 13

            Seeds lookup 080 keys 841-910.
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
               AND key_idx >= 841
               AND key_idx <= 910;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 13'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 13
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
