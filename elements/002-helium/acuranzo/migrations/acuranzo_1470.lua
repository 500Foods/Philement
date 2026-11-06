-- Migration: acuranzo_1470.lua
-- Lookup 079 (Report Barcode Symbologies) (Reporting Phase 54)
--
-- Seeds QR Code, Code 39, Code 128, UPC-A, and GS1-128.
-- The code column is the token the report stores.
-- Adds lookup 054 key 52 (Symbology) with no inline list.
-- Points lookup 053 keys 43 and 45 at that key.
-- Field Barcode also lists the existing Field key 7.
-- No lookup 075 section. No caller-facing QueryRef. QueryRef 175 stays unused.
-- Reverse deletes lookup 079 and key 52, and restores the 1086 lists.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-05 - Report barcode symbologies

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1470"
cfg.LOOKUP_ID = "079"
cfg.LOOKUP_NAME = "Report Barcode Symbologies"
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
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (0, ${LOOKUP_ID}, 1, 'Report Barcode Symbologies', 0, 0, '', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 1, 1, 'QR Code', 1, 1, 'qr', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 2, 1, 'Code 39', 2, 2, 'code39', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 3, 1, 'Code 128', 3, 3, 'code128', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 4, 1, 'UPC-A', 4, 4, 'upca', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 5, 1, 'GS1-128', 5, 5, 'gs1-128', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 52, 1, 'Symbology', 1, 221, '', '', ${JSON_INGEST_START}[===[{"Group":"Object","section":4,"Label":"Symbology","Editor":"List"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-A", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 52]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 43;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-B", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 7, 14, 24, 29, 52]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 45;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Adds lookup 079, lookup 054 key 52, and the Static Barcode and Field Barcode attribute lists.
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
            WHERE lookup_id = 0
              AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 54
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-A", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 43;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-B", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 45;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} barcode rows'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID}

            Deletes lookup 079 and lookup 054 key 52.
            Restores lookup 053 keys 43 and 45 to the 1086 collections.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

return queries end
