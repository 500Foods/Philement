-- Migration: acuranzo_1491.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 6/15
-- Keys 351-420. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 6

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1491"

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
                (80, 351, 1, 'Faded Red', 0, 351, '#d3494e', '', '{}', ${COMMON_VALUES}),
                (80, 352, 1, 'Faded Yellow', 0, 352, '#feff7f', '', '{}', ${COMMON_VALUES}),
                (80, 353, 1, 'Fawn', 0, 353, '#cfaf7b', '', '{}', ${COMMON_VALUES}),
                (80, 354, 1, 'Fern', 0, 354, '#63a950', '', '{}', ${COMMON_VALUES}),
                (80, 355, 1, 'Fern Green', 0, 355, '#548d44', '', '{}', ${COMMON_VALUES}),
                (80, 356, 1, 'Fire Engine Red', 0, 356, '#fe0002', '', '{}', ${COMMON_VALUES}),
                (80, 357, 1, 'Firebrick', 0, 357, '#b22222', '', '{}', ${COMMON_VALUES}),
                (80, 358, 1, 'Flat Blue', 0, 358, '#3c73a8', '', '{}', ${COMMON_VALUES}),
                (80, 359, 1, 'Flat Green', 0, 359, '#699d4c', '', '{}', ${COMMON_VALUES}),
                (80, 360, 1, 'Floral White', 0, 360, '#fffaf0', '', '{}', ${COMMON_VALUES}),
                (80, 361, 1, 'Fluorescent Green', 0, 361, '#08ff08', '', '{}', ${COMMON_VALUES}),
                (80, 362, 1, 'Fluro Green', 0, 362, '#0aff02', '', '{}', ${COMMON_VALUES}),
                (80, 363, 1, 'Foam Green', 0, 363, '#90fda9', '', '{}', ${COMMON_VALUES}),
                (80, 364, 1, 'Forest', 0, 364, '#0b5509', '', '{}', ${COMMON_VALUES}),
                (80, 365, 1, 'Forest Green', 0, 365, '#228b22', '', '{}', ${COMMON_VALUES}),
                (80, 366, 1, 'Forrest Green', 0, 366, '#154406', '', '{}', ${COMMON_VALUES}),
                (80, 367, 1, 'French Blue', 0, 367, '#436bad', '', '{}', ${COMMON_VALUES}),
                (80, 368, 1, 'Fresh Green', 0, 368, '#69d84f', '', '{}', ${COMMON_VALUES}),
                (80, 369, 1, 'Frog Green', 0, 369, '#58bc08', '', '{}', ${COMMON_VALUES}),
                (80, 370, 1, 'Fuchsia', 0, 370, '#ff00ff', '', '{}', ${COMMON_VALUES}),
                (80, 371, 1, 'Gainsboro', 0, 371, '#dcdcdc', '', '{}', ${COMMON_VALUES}),
                (80, 372, 1, 'Ghost White', 0, 372, '#f8f8ff', '', '{}', ${COMMON_VALUES}),
                (80, 373, 1, 'Gold', 0, 373, '#ffd700', '', '{}', ${COMMON_VALUES}),
                (80, 374, 1, 'Golden', 0, 374, '#f5bf03', '', '{}', ${COMMON_VALUES}),
                (80, 375, 1, 'Golden Brown', 0, 375, '#b27a01', '', '{}', ${COMMON_VALUES}),
                (80, 376, 1, 'Golden Rod', 0, 376, '#f9bc08', '', '{}', ${COMMON_VALUES}),
                (80, 377, 1, 'Golden Yellow', 0, 377, '#fec615', '', '{}', ${COMMON_VALUES}),
                (80, 378, 1, 'Goldenrod', 0, 378, '#daa520', '', '{}', ${COMMON_VALUES}),
                (80, 379, 1, 'Grape', 0, 379, '#6c3461', '', '{}', ${COMMON_VALUES}),
                (80, 380, 1, 'Grape Purple', 0, 380, '#5d1451', '', '{}', ${COMMON_VALUES}),
                (80, 381, 1, 'Grapefruit', 0, 381, '#fd5956', '', '{}', ${COMMON_VALUES}),
                (80, 382, 1, 'Grass', 0, 382, '#5cac2d', '', '{}', ${COMMON_VALUES}),
                (80, 383, 1, 'Grass Green', 0, 383, '#3f9b0b', '', '{}', ${COMMON_VALUES}),
                (80, 384, 1, 'Grassy Green', 0, 384, '#419c03', '', '{}', ${COMMON_VALUES}),
                (80, 385, 1, 'Gray', 0, 385, '#808080', '', '{}', ${COMMON_VALUES}),
                (80, 386, 1, 'Green', 0, 386, '#008000', '', '{}', ${COMMON_VALUES}),
                (80, 387, 1, 'Green Apple', 0, 387, '#5edc1f', '', '{}', ${COMMON_VALUES}),
                (80, 388, 1, 'Green Blue', 0, 388, '#06b48b', '', '{}', ${COMMON_VALUES}),
                (80, 389, 1, 'Green Brown', 0, 389, '#544e03', '', '{}', ${COMMON_VALUES}),
                (80, 390, 1, 'Green Grey', 0, 390, '#77926f', '', '{}', ${COMMON_VALUES}),
                (80, 391, 1, 'Green Teal', 0, 391, '#0cb577', '', '{}', ${COMMON_VALUES}),
                (80, 392, 1, 'Green Yellow', 0, 392, '#adff2f', '', '{}', ${COMMON_VALUES}),
                (80, 393, 1, 'Green/blue', 0, 393, '#01c08d', '', '{}', ${COMMON_VALUES}),
                (80, 394, 1, 'Green/yellow', 0, 394, '#b5ce08', '', '{}', ${COMMON_VALUES}),
                (80, 395, 1, 'Greenblue', 0, 395, '#23c48b', '', '{}', ${COMMON_VALUES}),
                (80, 396, 1, 'Greenish', 0, 396, '#40a368', '', '{}', ${COMMON_VALUES}),
                (80, 397, 1, 'Greenish Beige', 0, 397, '#c9d179', '', '{}', ${COMMON_VALUES}),
                (80, 398, 1, 'Greenish Blue', 0, 398, '#0b8b87', '', '{}', ${COMMON_VALUES}),
                (80, 399, 1, 'Greenish Brown', 0, 399, '#696112', '', '{}', ${COMMON_VALUES}),
                (80, 400, 1, 'Greenish Cyan', 0, 400, '#2afeb7', '', '{}', ${COMMON_VALUES}),
                (80, 401, 1, 'Greenish Grey', 0, 401, '#96ae8d', '', '{}', ${COMMON_VALUES}),
                (80, 402, 1, 'Greenish Tan', 0, 402, '#bccb7a', '', '{}', ${COMMON_VALUES}),
                (80, 403, 1, 'Greenish Teal', 0, 403, '#32bf84', '', '{}', ${COMMON_VALUES}),
                (80, 404, 1, 'Greenish Turquoise', 0, 404, '#00fbb0', '', '{}', ${COMMON_VALUES}),
                (80, 405, 1, 'Greenish Yellow', 0, 405, '#cdfd02', '', '{}', ${COMMON_VALUES}),
                (80, 406, 1, 'Greeny Blue', 0, 406, '#42b395', '', '{}', ${COMMON_VALUES}),
                (80, 407, 1, 'Greeny Brown', 0, 407, '#696006', '', '{}', ${COMMON_VALUES}),
                (80, 408, 1, 'Greeny Grey', 0, 408, '#7ea07a', '', '{}', ${COMMON_VALUES}),
                (80, 409, 1, 'Greeny Yellow', 0, 409, '#c6f808', '', '{}', ${COMMON_VALUES}),
                (80, 410, 1, 'Grey', 0, 410, '#808080', '', '{}', ${COMMON_VALUES}),
                (80, 411, 1, 'Grey Blue', 0, 411, '#6b8ba4', '', '{}', ${COMMON_VALUES}),
                (80, 412, 1, 'Grey Brown', 0, 412, '#7f7053', '', '{}', ${COMMON_VALUES}),
                (80, 413, 1, 'Grey Green', 0, 413, '#789b73', '', '{}', ${COMMON_VALUES}),
                (80, 414, 1, 'Grey Pink', 0, 414, '#c3909b', '', '{}', ${COMMON_VALUES}),
                (80, 415, 1, 'Grey Purple', 0, 415, '#826d8c', '', '{}', ${COMMON_VALUES}),
                (80, 416, 1, 'Grey Teal', 0, 416, '#5e9b8a', '', '{}', ${COMMON_VALUES}),
                (80, 417, 1, 'Grey/blue', 0, 417, '#647d8e', '', '{}', ${COMMON_VALUES}),
                (80, 418, 1, 'Grey/green', 0, 418, '#86a17d', '', '{}', ${COMMON_VALUES}),
                (80, 419, 1, 'Greyblue', 0, 419, '#77a1b5', '', '{}', ${COMMON_VALUES}),
                (80, 420, 1, 'Greyish', 0, 420, '#a8a495', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 6'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 6

            Seeds lookup 080 keys 351-420.
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
               AND key_idx >= 351
               AND key_idx <= 420;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 6'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 6
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
