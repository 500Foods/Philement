-- Migration: acuranzo_1488.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 3/15
-- Keys 141-210. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 3

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1488"

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
                (80, 141, 1, 'Butterscotch', 0, 141, '#fdb147', '', '{}', ${COMMON_VALUES}),
                (80, 142, 1, 'Cadet Blue', 0, 142, '#5f9ea0', '', '{}', ${COMMON_VALUES}),
                (80, 143, 1, 'Camel', 0, 143, '#c69f59', '', '{}', ${COMMON_VALUES}),
                (80, 144, 1, 'Camo', 0, 144, '#7f8f4e', '', '{}', ${COMMON_VALUES}),
                (80, 145, 1, 'Camo Green', 0, 145, '#526525', '', '{}', ${COMMON_VALUES}),
                (80, 146, 1, 'Camouflage Green', 0, 146, '#4b6113', '', '{}', ${COMMON_VALUES}),
                (80, 147, 1, 'Canary', 0, 147, '#fdff63', '', '{}', ${COMMON_VALUES}),
                (80, 148, 1, 'Canary Yellow', 0, 148, '#fffe40', '', '{}', ${COMMON_VALUES}),
                (80, 149, 1, 'Candy Pink', 0, 149, '#ff63e9', '', '{}', ${COMMON_VALUES}),
                (80, 150, 1, 'Caramel', 0, 150, '#af6f09', '', '{}', ${COMMON_VALUES}),
                (80, 151, 1, 'Carmine', 0, 151, '#9d0216', '', '{}', ${COMMON_VALUES}),
                (80, 152, 1, 'Carnation', 0, 152, '#fd798f', '', '{}', ${COMMON_VALUES}),
                (80, 153, 1, 'Carnation Pink', 0, 153, '#ff7fa7', '', '{}', ${COMMON_VALUES}),
                (80, 154, 1, 'Carolina Blue', 0, 154, '#8ab8fe', '', '{}', ${COMMON_VALUES}),
                (80, 155, 1, 'Celadon', 0, 155, '#befdb7', '', '{}', ${COMMON_VALUES}),
                (80, 156, 1, 'Celery', 0, 156, '#c1fd95', '', '{}', ${COMMON_VALUES}),
                (80, 157, 1, 'Cement', 0, 157, '#a5a391', '', '{}', ${COMMON_VALUES}),
                (80, 158, 1, 'Cerise', 0, 158, '#de0c62', '', '{}', ${COMMON_VALUES}),
                (80, 159, 1, 'Cerulean', 0, 159, '#0485d1', '', '{}', ${COMMON_VALUES}),
                (80, 160, 1, 'Cerulean Blue', 0, 160, '#056eee', '', '{}', ${COMMON_VALUES}),
                (80, 161, 1, 'Charcoal', 0, 161, '#343837', '', '{}', ${COMMON_VALUES}),
                (80, 162, 1, 'Charcoal Grey', 0, 162, '#3c4142', '', '{}', ${COMMON_VALUES}),
                (80, 163, 1, 'Chartreuse', 0, 163, '#7fff00', '', '{}', ${COMMON_VALUES}),
                (80, 164, 1, 'Cherry', 0, 164, '#cf0234', '', '{}', ${COMMON_VALUES}),
                (80, 165, 1, 'Cherry Red', 0, 165, '#f7022a', '', '{}', ${COMMON_VALUES}),
                (80, 166, 1, 'Chestnut', 0, 166, '#742802', '', '{}', ${COMMON_VALUES}),
                (80, 167, 1, 'Chocolate', 0, 167, '#d2691e', '', '{}', ${COMMON_VALUES}),
                (80, 168, 1, 'Chocolate Brown', 0, 168, '#411900', '', '{}', ${COMMON_VALUES}),
                (80, 169, 1, 'Cinnamon', 0, 169, '#ac4f06', '', '{}', ${COMMON_VALUES}),
                (80, 170, 1, 'Claret', 0, 170, '#680018', '', '{}', ${COMMON_VALUES}),
                (80, 171, 1, 'Clay', 0, 171, '#b66a50', '', '{}', ${COMMON_VALUES}),
                (80, 172, 1, 'Clay Brown', 0, 172, '#b2713d', '', '{}', ${COMMON_VALUES}),
                (80, 173, 1, 'Clear Blue', 0, 173, '#247afd', '', '{}', ${COMMON_VALUES}),
                (80, 174, 1, 'Cloudy Blue', 0, 174, '#acc2d9', '', '{}', ${COMMON_VALUES}),
                (80, 175, 1, 'Cobalt', 0, 175, '#1e488f', '', '{}', ${COMMON_VALUES}),
                (80, 176, 1, 'Cobalt Blue', 0, 176, '#030aa7', '', '{}', ${COMMON_VALUES}),
                (80, 177, 1, 'Cocoa', 0, 177, '#875f42', '', '{}', ${COMMON_VALUES}),
                (80, 178, 1, 'Coffee', 0, 178, '#a6814c', '', '{}', ${COMMON_VALUES}),
                (80, 179, 1, 'Cool Blue', 0, 179, '#4984b8', '', '{}', ${COMMON_VALUES}),
                (80, 180, 1, 'Cool Green', 0, 180, '#33b864', '', '{}', ${COMMON_VALUES}),
                (80, 181, 1, 'Cool Grey', 0, 181, '#95a3a6', '', '{}', ${COMMON_VALUES}),
                (80, 182, 1, 'Copper', 0, 182, '#b66325', '', '{}', ${COMMON_VALUES}),
                (80, 183, 1, 'Coral', 0, 183, '#ff7f50', '', '{}', ${COMMON_VALUES}),
                (80, 184, 1, 'Coral Pink', 0, 184, '#ff6163', '', '{}', ${COMMON_VALUES}),
                (80, 185, 1, 'Cornflower', 0, 185, '#6a79f7', '', '{}', ${COMMON_VALUES}),
                (80, 186, 1, 'Cornflower Blue', 0, 186, '#6495ed', '', '{}', ${COMMON_VALUES}),
                (80, 187, 1, 'Cornsilk', 0, 187, '#fff8dc', '', '{}', ${COMMON_VALUES}),
                (80, 188, 1, 'Cranberry', 0, 188, '#9e003a', '', '{}', ${COMMON_VALUES}),
                (80, 189, 1, 'Cream', 0, 189, '#ffffc2', '', '{}', ${COMMON_VALUES}),
                (80, 190, 1, 'Creme', 0, 190, '#ffffb6', '', '{}', ${COMMON_VALUES}),
                (80, 191, 1, 'Crimson', 0, 191, '#dc143c', '', '{}', ${COMMON_VALUES}),
                (80, 192, 1, 'Custard', 0, 192, '#fffd78', '', '{}', ${COMMON_VALUES}),
                (80, 193, 1, 'Cyan', 0, 193, '#00ffff', '', '{}', ${COMMON_VALUES}),
                (80, 194, 1, 'Dandelion', 0, 194, '#fedf08', '', '{}', ${COMMON_VALUES}),
                (80, 195, 1, 'Dark', 0, 195, '#1b2431', '', '{}', ${COMMON_VALUES}),
                (80, 196, 1, 'Dark Aqua', 0, 196, '#05696b', '', '{}', ${COMMON_VALUES}),
                (80, 197, 1, 'Dark Aquamarine', 0, 197, '#017371', '', '{}', ${COMMON_VALUES}),
                (80, 198, 1, 'Dark Beige', 0, 198, '#ac9362', '', '{}', ${COMMON_VALUES}),
                (80, 199, 1, 'Dark Blue', 0, 199, '#00008b', '', '{}', ${COMMON_VALUES}),
                (80, 200, 1, 'Dark Blue Green', 0, 200, '#005249', '', '{}', ${COMMON_VALUES}),
                (80, 201, 1, 'Dark Blue Grey', 0, 201, '#1f3b4d', '', '{}', ${COMMON_VALUES}),
                (80, 202, 1, 'Dark Brown', 0, 202, '#341c02', '', '{}', ${COMMON_VALUES}),
                (80, 203, 1, 'Dark Coral', 0, 203, '#cf524e', '', '{}', ${COMMON_VALUES}),
                (80, 204, 1, 'Dark Cream', 0, 204, '#fff39a', '', '{}', ${COMMON_VALUES}),
                (80, 205, 1, 'Dark Cyan', 0, 205, '#008b8b', '', '{}', ${COMMON_VALUES}),
                (80, 206, 1, 'Dark Forest Green', 0, 206, '#002d04', '', '{}', ${COMMON_VALUES}),
                (80, 207, 1, 'Dark Fuchsia', 0, 207, '#9d0759', '', '{}', ${COMMON_VALUES}),
                (80, 208, 1, 'Dark Gold', 0, 208, '#b59410', '', '{}', ${COMMON_VALUES}),
                (80, 209, 1, 'Dark Goldenrod', 0, 209, '#b8860b', '', '{}', ${COMMON_VALUES}),
                (80, 210, 1, 'Dark Grass Green', 0, 210, '#388004', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 3'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 3

            Seeds lookup 080 keys 141-210.
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
               AND key_idx >= 141
               AND key_idx <= 210;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 3'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 3
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
