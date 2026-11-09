-- Migration: acuranzo_1500.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 15/15
-- Keys 981-1002. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 15

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1500"

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
                (80, 981, 1, 'White', 0, 981, '#ffffff', '', '{}', ${COMMON_VALUES}),
                (80, 982, 1, 'White Smoke', 0, 982, '#f5f5f5', '', '{}', ${COMMON_VALUES}),
                (80, 983, 1, 'Windows Blue', 0, 983, '#3778bf', '', '{}', ${COMMON_VALUES}),
                (80, 984, 1, 'Wine', 0, 984, '#80013f', '', '{}', ${COMMON_VALUES}),
                (80, 985, 1, 'Wine Red', 0, 985, '#7b0323', '', '{}', ${COMMON_VALUES}),
                (80, 986, 1, 'Wintergreen', 0, 986, '#20f986', '', '{}', ${COMMON_VALUES}),
                (80, 987, 1, 'Wisteria', 0, 987, '#a87dc2', '', '{}', ${COMMON_VALUES}),
                (80, 988, 1, 'Yellow', 0, 988, '#ffff00', '', '{}', ${COMMON_VALUES}),
                (80, 989, 1, 'Yellow Brown', 0, 989, '#b79400', '', '{}', ${COMMON_VALUES}),
                (80, 990, 1, 'Yellow Green', 0, 990, '#9acd32', '', '{}', ${COMMON_VALUES}),
                (80, 991, 1, 'Yellow Ochre', 0, 991, '#cb9d06', '', '{}', ${COMMON_VALUES}),
                (80, 992, 1, 'Yellow Orange', 0, 992, '#fcb001', '', '{}', ${COMMON_VALUES}),
                (80, 993, 1, 'Yellow Tan', 0, 993, '#ffe36e', '', '{}', ${COMMON_VALUES}),
                (80, 994, 1, 'Yellow/green', 0, 994, '#c8fd3d', '', '{}', ${COMMON_VALUES}),
                (80, 995, 1, 'Yellowgreen', 0, 995, '#bbf90f', '', '{}', ${COMMON_VALUES}),
                (80, 996, 1, 'Yellowish', 0, 996, '#faee66', '', '{}', ${COMMON_VALUES}),
                (80, 997, 1, 'Yellowish Brown', 0, 997, '#9b7a01', '', '{}', ${COMMON_VALUES}),
                (80, 998, 1, 'Yellowish Green', 0, 998, '#b0dd16', '', '{}', ${COMMON_VALUES}),
                (80, 999, 1, 'Yellowish Orange', 0, 999, '#ffab0f', '', '{}', ${COMMON_VALUES}),
                (80, 1000, 1, 'Yellowish Tan', 0, 1000, '#fcfc81', '', '{}', ${COMMON_VALUES}),
                (80, 1001, 1, 'Yellowy Brown', 0, 1001, '#ae8b0c', '', '{}', ${COMMON_VALUES}),
                (80, 1002, 1, 'Yellowy Green', 0, 1002, '#bff128', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 15'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 15

            Seeds lookup 080 keys 981-1002.
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
               AND key_idx >= 981
               AND key_idx <= 1002;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 15'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 15
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
