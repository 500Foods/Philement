-- Migration: acuranzo_1465.lua
-- Lookup 078 (Report Shape Kinds) and shape attributes (Reporting Phase 44a)
--
-- Seeds line, rectangle, roundrect, ellipse, x, trapezoid, star, pentagon,
-- and hexagon. Path rows store unit-square commands. Polygons point up.
-- The trapezoid's short edge is on top. X is two open strokes.
-- Adds lookup 054 keys 41-51 and points lookup 053 key 25 at them.
-- Shape and Side stay in Object. Stroke, Fill, and Corners are
-- lookup 075 keys 10-12. Keys 26, 33, 39, and 53 stay unchanged.
-- No caller-facing QueryRef.
-- Reverse deletes lookup 078, keys 41-51, lookup 075 keys 10-12,
-- and restores the 1086 key 25 row.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-04 - Report shape kinds and shape attributes
-- 1.0.1 - 2026-11-04 - Shape sections Stroke, Fill, and Corners

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1465"
cfg.LOOKUP_ID = "078"
cfg.LOOKUP_NAME = "Report Shape Kinds"
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
                (0, ${LOOKUP_ID}, 1, 'Report Shape Kinds', 0, 0, '', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 1, 1, 'Line', 1, 1, 'line', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 2, 1, 'Rectangle', 2, 2, 'rectangle', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 3, 1, 'Rounded Rectangle', 3, 3, 'roundrect', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 4, 1, 'Ellipse', 4, 4, 'ellipse', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 5, 1, 'X', 5, 5, 'x', '', ${JSON_INGEST_START}[===[[{"closed":false,"commands":[["M",0,0],["L",1,1],["M",1,0],["L",0,1]]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 6, 1, 'Trapezoid', 6, 6, 'trapezoid', '', ${JSON_INGEST_START}[===[[{"closed":true,"commands":[["M",0.25,0],["L",0.75,0],["L",1,1],["L",0,1],["Z"]]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 7, 1, 'Star', 7, 7, 'star', '', ${JSON_INGEST_START}[===[[{"closed":true,"commands":[["M",0.5,0],["L",0.618034,0.381966],["L",1,0.381966],["L",0.690983,0.618034],["L",0.809017,1],["L",0.5,0.763932],["L",0.190983,1],["L",0.309017,0.618034],["L",0,0.381966],["L",0.381966,0.381966],["Z"]]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 8, 1, 'Pentagon', 8, 8, 'pentagon', '', ${JSON_INGEST_START}[===[[{"closed":true,"commands":[["M",0.5,0],["L",1,0.381966],["L",0.809017,1],["L",0.190983,1],["L",0,0.381966],["Z"]]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 9, 1, 'Hexagon', 9, 9, 'hexagon', '', ${JSON_INGEST_START}[===[[{"closed":true,"commands":[["M",0.5,0],["L",1,0.25],["L",1,0.75],["L",0.5,1],["L",0,0.75],["L",0,0.25],["Z"]]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 41, 1, 'Shape', 1, 210, '', '', ${JSON_INGEST_START}[===[[{"Group":"Object","section":4,"Label":"Shape","Editor":"List"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 42, 1, 'Side', 1, 211, '', '', ${JSON_INGEST_START}[===[[{"Group":"Object","section":4,"Label":"Side","Editor":"List","List":["top","right","bottom","left"]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 43, 1, 'Stroke Width', 1, 212, '', '', ${JSON_INGEST_START}[===[[{"Group":"Stroke","section":10,"Label":"Stroke Width","Editor":"Number"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 44, 1, 'Stroke Style', 1, 213, '', '', ${JSON_INGEST_START}[===[[{"Group":"Stroke","section":10,"Label":"Stroke Style","Editor":"List","List":["solid","dashed","dotted","none"]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 45, 1, 'Stroke Color', 1, 214, '', '', ${JSON_INGEST_START}[===[[{"Group":"Stroke","section":10,"Label":"Stroke Color","Editor":"Color"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 46, 1, 'Fill Style', 1, 215, '', '', ${JSON_INGEST_START}[===[[{"Group":"Fill","section":11,"Label":"Fill Style","Editor":"List","List":["none","solid"]}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 47, 1, 'Fill Color', 1, 216, '', '', ${JSON_INGEST_START}[===[[{"Group":"Fill","section":11,"Label":"Fill Color","Editor":"Color"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 48, 1, 'Corner Top Left', 1, 217, '', '', ${JSON_INGEST_START}[===[[{"Group":"Corners","section":12,"Label":"Corner Top Left","Editor":"Number"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 49, 1, 'Corner Top Right', 1, 218, '', '', ${JSON_INGEST_START}[===[[{"Group":"Corners","section":12,"Label":"Corner Top Right","Editor":"Number"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 50, 1, 'Corner Bottom Right', 1, 219, '', '', ${JSON_INGEST_START}[===[[{"Group":"Corners","section":12,"Label":"Corner Bottom Right","Editor":"Number"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 51, 1, 'Corner Bottom Left', 1, 220, '', '', ${JSON_INGEST_START}[===[[{"Group":"Corners","section":12,"Label":"Corner Bottom Left","Editor":"Number"}]]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (75, 10, 1, 'Stroke', 10, 10, '', '', ${JSON_INGEST_START}[===[{"icon": "<fa fa-pen></fa>"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (75, 11, 1, 'Fill', 11, 11, '', '', ${JSON_INGEST_START}[===[{"icon": "<fa fa-fill-drip></fa>"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (75, 12, 1, 'Corners', 12, 12, '', '', ${JSON_INGEST_START}[===[{"icon": "<fa fa-vector-square></fa>"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[[{"icon":"<fa fa-shapes></fa>","color":"Color-A","group":"Shape","attributes":[0,1,2,3,4,14,29,41,42,43,44,45,46,47,48,49,50,51]}]]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 25;

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

            Adds lookup 078, lookup 054 keys 41-51, lookup 075 keys 10-12, and the Static Shape attribute list.
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
              AND key_idx IN (41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51);

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 75
              AND key_idx IN (10, 11, 12);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[[{"icon":"<fa fa-shapes></fa>","color":"Color-A","group":"Shape","attributes":[0,1,2,3,4,14,24,29]}]]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 25;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} shape rows'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID}

            Deletes lookup 078, lookup 054 keys 41-51, and lookup 075 keys 10-12.
            Restores lookup 053 key 25 to the 1086 collection.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

return queries end
