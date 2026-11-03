-- Migration: acuranzo_1438.lua
-- Lookup 057 (Report Page Sizes) for Reporting Phase 13
--
-- Does not edit 1090 in place. Key 1 (Letter landscape JSON) is
-- relabeled US Letter Landscape. Inserts keys 2-7: US Legal portrait
-- and landscape, Tabloid portrait and landscape (inches, 0.25 in
-- margins), A4 and A3 portrait (mm, 6 mm margins). Page Size strings
-- Tabloid and A3 are not yet in the lookup 054 key 30 list. No
-- caller-facing QueryRef. Reverse deletes keys 2-7 and restores the
-- key 1 label US Letter Legal.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-11-02 - Initial creation (Reporting Phase 13)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1438"
cfg.LOOKUP_ID = "057"
cfg.LOOKUP_NAME = "Report Page Sizes"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'US Letter Landscape'
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 2, 1, 'US Legal Portrait', 0, 2, '', '', ${JSON_INGEST_START}[===[{"Page Size": "US Legal", "Page Units": "in", "Page Width": "8.5 in", "Page Height": "14 in", "Page Margin Top": "0.25 in", "Page Margin Left": "0.25 in", "Page Orientation": "Portrait", "Page Margin Right": "0.25 in", "Page Margin Bottom": "0.25 in"}]===]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 3, 1, 'US Legal Landscape', 0, 3, '', '', ${JSON_INGEST_START}[===[{"Page Size": "US Legal", "Page Units": "in", "Page Width": "14 in", "Page Height": "8.5 in", "Page Margin Top": "0.25 in", "Page Margin Left": "0.25 in", "Page Orientation": "Landscape", "Page Margin Right": "0.25 in", "Page Margin Bottom": "0.25 in"}]===]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 4, 1, 'Tabloid Portrait', 0, 4, '', '', ${JSON_INGEST_START}[===[{"Page Size": "Tabloid", "Page Units": "in", "Page Width": "11 in", "Page Height": "17 in", "Page Margin Top": "0.25 in", "Page Margin Left": "0.25 in", "Page Orientation": "Portrait", "Page Margin Right": "0.25 in", "Page Margin Bottom": "0.25 in"}]===]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 5, 1, 'Tabloid Landscape', 0, 5, '', '', ${JSON_INGEST_START}[===[{"Page Size": "Tabloid", "Page Units": "in", "Page Width": "17 in", "Page Height": "11 in", "Page Margin Top": "0.25 in", "Page Margin Left": "0.25 in", "Page Orientation": "Landscape", "Page Margin Right": "0.25 in", "Page Margin Bottom": "0.25 in"}]===]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 6, 1, 'A4 Portrait', 0, 6, '', '', ${JSON_INGEST_START}[===[{"Page Size": "A4", "Page Units": "mm", "Page Width": "210 mm", "Page Height": "297 mm", "Page Margin Top": "6 mm", "Page Margin Left": "6 mm", "Page Orientation": "Portrait", "Page Margin Right": "6 mm", "Page Margin Bottom": "6 mm"}]===]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 7, 1, 'A3 Portrait', 0, 7, '', '', ${JSON_INGEST_START}[===[{"Page Size": "A3", "Page Units": "mm", "Page Width": "297 mm", "Page Height": "420 mm", "Page Margin Top": "6 mm", "Page Margin Left": "6 mm", "Page Orientation": "Portrait", "Page Margin Right": "6 mm", "Page Margin Bottom": "6 mm"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Update Lookup ${LOOKUP_ID} page sizes in ${TABLE} table'          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} page sizes

            Relabels key 1 as US Letter Landscape and adds keys 2-7: US Legal and
            Tabloid in portrait and landscape, A4 and A3 portrait.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx IN (2, 3, 4, 5, 6, 7);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'US Letter Legal'
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Undo Lookup ${LOOKUP_ID} page sizes in ${TABLE} table'            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Undo Lookup ${LOOKUP_ID} page sizes

            Deletes keys 2-7 and restores the acuranzo_1090 label on key 1.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
