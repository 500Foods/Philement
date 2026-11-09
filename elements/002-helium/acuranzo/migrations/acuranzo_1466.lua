-- Migration: acuranzo_1466.lua
-- Report barcode symbologies (Reporting)
--
-- Lookup 079 keys 6-26. Keys 1-5 stay as acuranzo_1462.lua wrote them.
-- sort_seq follows the symbology name, digits as numbers, so keys 1, 4,
-- and 5 move. Compact PDF417 is pdf417comp. pdf417trunc is that same symbol.
-- UPC-A and UPC-E with the check digit supplied stay the Check digit row.
-- Reverse deletes keys 6-26 and restores those three sort values.
-- No new lookup id. No caller-facing QueryRef.
-- QueryRef 175 stays unused.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-05 - Lookup 079 lists the wider barcode set
-- 1.0.1 - 2026-11-05 - Sort by name and add Compact PDF417
-- 1.0.2 - 2026-11-09 - Renumbered from acuranzo_1478.lua (Render split); content unchanged

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1466"

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[==[

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
              SET sort_seq = 23
            WHERE lookup_id = 79
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
              SET sort_seq = 24
            WHERE lookup_id = 79
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
              SET sort_seq = 16
            WHERE lookup_id = 79
              AND key_idx = 5;

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 6, 1, 'Code 128 (suppress C)', 6, 4, 'code128ab', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 7, 1, 'Flattermarken', 7, 11, 'flat', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 8, 1, 'EAN-8', 8, 9, 'ean8', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 9, 1, 'EAN-13', 9, 10, 'ean13', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 10, 1, 'UPC-E', 10, 25, 'upce', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 11, 1, 'Interleaved 2 of 5', 11, 17, 'c25inter', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 12, 1, 'ITF-14', 12, 19, 'itf14', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 13, 1, 'NVE-18 (SSCC)', 13, 21, 'nve18', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 14, 1, 'GS1 DataBar Omnidirectional', 14, 14, 'dbaromn', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 15, 1, 'GS1 DataBar Stacked Omnidirectional', 15, 15, 'dbaromnstk', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 16, 1, 'GS1 DataBar Expanded', 16, 12, 'dbarexp', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 17, 1, 'GS1 DataBar Expanded Stacked', 17, 13, 'dbarexpstk', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 18, 1, 'Data Matrix', 18, 6, 'datamatrix', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 19, 1, 'EAN-2 add-on', 19, 7, 'ean2', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 20, 1, 'EAN-5 add-on', 20, 8, 'ean5', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 21, 1, 'ISBN', 21, 18, 'isbn', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 22, 1, 'PDF417', 22, 22, 'pdf417', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 23, 1, 'Aztec', 23, 1, 'aztec', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 24, 1, 'MaxiCode', 24, 20, 'maxicode', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 25, 1, 'USPS Intelligent Mail', 25, 26, 'usps', '', '{}', ${COMMON_VALUES});
            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (79, 26, 1, 'Compact PDF417', 26, 5, 'pdf417comp', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 079 barcode symbologies'                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Lookup 079 barcode symbologies

            Adds lookup 079 keys 6 through 26.
            sort_seq follows the symbology name.
            Keys 1, 4, and 5 keep their codes and take the new sort.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[==[

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
            WHERE lookup_id = 79
              AND key_idx BETWEEN 6 AND 26;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
              SET sort_seq = key_idx
            WHERE lookup_id = 79
              AND key_idx IN (1, 4, 5);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove lookup 079 keys 6-26'                                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: remove lookup 079 keys 6-26

            Deletes the symbology rows added by this file.
            Keys 1, 4, and 5 return to their original sort.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

return queries end
