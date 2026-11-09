-- Migration: acuranzo_1496.lua
-- Lookup 080 Named Colours (Reporting Phase 77) batch 11/15
-- Keys 701-770. CSS wins on duplicate names over xkcd (CC0).
-- value_txt = name, code = #rrggbb, value_int = swatch flag (0 seeded default).
-- Reverse deletes this batch.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 77 Named Colours batch 11

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1496"

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
                (80, 701, 1, 'Papaya Whip', 0, 701, '#ffefd5', '', '{}', ${COMMON_VALUES}),
                (80, 702, 1, 'Parchment', 0, 702, '#fefcaf', '', '{}', ${COMMON_VALUES}),
                (80, 703, 1, 'Pastel Blue', 0, 703, '#a2bffe', '', '{}', ${COMMON_VALUES}),
                (80, 704, 1, 'Pastel Green', 0, 704, '#b0ff9d', '', '{}', ${COMMON_VALUES}),
                (80, 705, 1, 'Pastel Orange', 0, 705, '#ff964f', '', '{}', ${COMMON_VALUES}),
                (80, 706, 1, 'Pastel Pink', 0, 706, '#ffbacd', '', '{}', ${COMMON_VALUES}),
                (80, 707, 1, 'Pastel Purple', 0, 707, '#caa0ff', '', '{}', ${COMMON_VALUES}),
                (80, 708, 1, 'Pastel Red', 0, 708, '#db5856', '', '{}', ${COMMON_VALUES}),
                (80, 709, 1, 'Pastel Yellow', 0, 709, '#fffe71', '', '{}', ${COMMON_VALUES}),
                (80, 710, 1, 'Pea', 0, 710, '#a4bf20', '', '{}', ${COMMON_VALUES}),
                (80, 711, 1, 'Pea Green', 0, 711, '#8eab12', '', '{}', ${COMMON_VALUES}),
                (80, 712, 1, 'Pea Soup', 0, 712, '#929901', '', '{}', ${COMMON_VALUES}),
                (80, 713, 1, 'Pea Soup Green', 0, 713, '#94a617', '', '{}', ${COMMON_VALUES}),
                (80, 714, 1, 'Peach', 0, 714, '#ffb07c', '', '{}', ${COMMON_VALUES}),
                (80, 715, 1, 'Peach Puff', 0, 715, '#ffdab9', '', '{}', ${COMMON_VALUES}),
                (80, 716, 1, 'Peachy Pink', 0, 716, '#ff9a8a', '', '{}', ${COMMON_VALUES}),
                (80, 717, 1, 'Peacock Blue', 0, 717, '#016795', '', '{}', ${COMMON_VALUES}),
                (80, 718, 1, 'Pear', 0, 718, '#cbf85f', '', '{}', ${COMMON_VALUES}),
                (80, 719, 1, 'Periwinkle', 0, 719, '#8e82fe', '', '{}', ${COMMON_VALUES}),
                (80, 720, 1, 'Periwinkle Blue', 0, 720, '#8f99fb', '', '{}', ${COMMON_VALUES}),
                (80, 721, 1, 'Perrywinkle', 0, 721, '#8f8ce7', '', '{}', ${COMMON_VALUES}),
                (80, 722, 1, 'Peru', 0, 722, '#cd853f', '', '{}', ${COMMON_VALUES}),
                (80, 723, 1, 'Petrol', 0, 723, '#005f6a', '', '{}', ${COMMON_VALUES}),
                (80, 724, 1, 'Pig Pink', 0, 724, '#e78ea5', '', '{}', ${COMMON_VALUES}),
                (80, 725, 1, 'Pine', 0, 725, '#2b5d34', '', '{}', ${COMMON_VALUES}),
                (80, 726, 1, 'Pine Green', 0, 726, '#0a481e', '', '{}', ${COMMON_VALUES}),
                (80, 727, 1, 'Pink', 0, 727, '#ffc0cb', '', '{}', ${COMMON_VALUES}),
                (80, 728, 1, 'Pink Purple', 0, 728, '#db4bda', '', '{}', ${COMMON_VALUES}),
                (80, 729, 1, 'Pink Red', 0, 729, '#f5054f', '', '{}', ${COMMON_VALUES}),
                (80, 730, 1, 'Pink/purple', 0, 730, '#ef1de7', '', '{}', ${COMMON_VALUES}),
                (80, 731, 1, 'Pinkish', 0, 731, '#d46a7e', '', '{}', ${COMMON_VALUES}),
                (80, 732, 1, 'Pinkish Brown', 0, 732, '#b17261', '', '{}', ${COMMON_VALUES}),
                (80, 733, 1, 'Pinkish Grey', 0, 733, '#c8aca9', '', '{}', ${COMMON_VALUES}),
                (80, 734, 1, 'Pinkish Orange', 0, 734, '#ff724c', '', '{}', ${COMMON_VALUES}),
                (80, 735, 1, 'Pinkish Purple', 0, 735, '#d648d7', '', '{}', ${COMMON_VALUES}),
                (80, 736, 1, 'Pinkish Red', 0, 736, '#f10c45', '', '{}', ${COMMON_VALUES}),
                (80, 737, 1, 'Pinkish Tan', 0, 737, '#d99b82', '', '{}', ${COMMON_VALUES}),
                (80, 738, 1, 'Pinky', 0, 738, '#fc86aa', '', '{}', ${COMMON_VALUES}),
                (80, 739, 1, 'Pinky Purple', 0, 739, '#c94cbe', '', '{}', ${COMMON_VALUES}),
                (80, 740, 1, 'Pinky Red', 0, 740, '#fc2647', '', '{}', ${COMMON_VALUES}),
                (80, 741, 1, 'Piss Yellow', 0, 741, '#ddd618', '', '{}', ${COMMON_VALUES}),
                (80, 742, 1, 'Pistachio', 0, 742, '#c0fa8b', '', '{}', ${COMMON_VALUES}),
                (80, 743, 1, 'Plum', 0, 743, '#dda0dd', '', '{}', ${COMMON_VALUES}),
                (80, 744, 1, 'Plum Purple', 0, 744, '#4e0550', '', '{}', ${COMMON_VALUES}),
                (80, 745, 1, 'Poison Green', 0, 745, '#40fd14', '', '{}', ${COMMON_VALUES}),
                (80, 746, 1, 'Poo', 0, 746, '#8f7303', '', '{}', ${COMMON_VALUES}),
                (80, 747, 1, 'Poo Brown', 0, 747, '#885f01', '', '{}', ${COMMON_VALUES}),
                (80, 748, 1, 'Poop', 0, 748, '#7f5e00', '', '{}', ${COMMON_VALUES}),
                (80, 749, 1, 'Poop Brown', 0, 749, '#7a5901', '', '{}', ${COMMON_VALUES}),
                (80, 750, 1, 'Poop Green', 0, 750, '#6f7c00', '', '{}', ${COMMON_VALUES}),
                (80, 751, 1, 'Powder Blue', 0, 751, '#b0e0e6', '', '{}', ${COMMON_VALUES}),
                (80, 752, 1, 'Powder Pink', 0, 752, '#ffb2d0', '', '{}', ${COMMON_VALUES}),
                (80, 753, 1, 'Primary Blue', 0, 753, '#0804f9', '', '{}', ${COMMON_VALUES}),
                (80, 754, 1, 'Prussian Blue', 0, 754, '#004577', '', '{}', ${COMMON_VALUES}),
                (80, 755, 1, 'Puce', 0, 755, '#a57e52', '', '{}', ${COMMON_VALUES}),
                (80, 756, 1, 'Puke', 0, 756, '#a5a502', '', '{}', ${COMMON_VALUES}),
                (80, 757, 1, 'Puke Brown', 0, 757, '#947706', '', '{}', ${COMMON_VALUES}),
                (80, 758, 1, 'Puke Green', 0, 758, '#9aae07', '', '{}', ${COMMON_VALUES}),
                (80, 759, 1, 'Puke Yellow', 0, 759, '#c2be0e', '', '{}', ${COMMON_VALUES}),
                (80, 760, 1, 'Pumpkin', 0, 760, '#e17701', '', '{}', ${COMMON_VALUES}),
                (80, 761, 1, 'Pumpkin Orange', 0, 761, '#fb7d07', '', '{}', ${COMMON_VALUES}),
                (80, 762, 1, 'Pure Blue', 0, 762, '#0203e2', '', '{}', ${COMMON_VALUES}),
                (80, 763, 1, 'Purple', 0, 763, '#800080', '', '{}', ${COMMON_VALUES}),
                (80, 764, 1, 'Purple Blue', 0, 764, '#632de9', '', '{}', ${COMMON_VALUES}),
                (80, 765, 1, 'Purple Brown', 0, 765, '#673a3f', '', '{}', ${COMMON_VALUES}),
                (80, 766, 1, 'Purple Grey', 0, 766, '#866f85', '', '{}', ${COMMON_VALUES}),
                (80, 767, 1, 'Purple Pink', 0, 767, '#e03fd8', '', '{}', ${COMMON_VALUES}),
                (80, 768, 1, 'Purple Red', 0, 768, '#990147', '', '{}', ${COMMON_VALUES}),
                (80, 769, 1, 'Purple/blue', 0, 769, '#5d21d0', '', '{}', ${COMMON_VALUES}),
                (80, 770, 1, 'Purple/pink', 0, 770, '#d725de', '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Lookup 080 Named Colours batch 11'                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Named Colours batch 11

            Seeds lookup 080 keys 701-770.
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
               AND key_idx >= 701
               AND key_idx <= 770;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Named Colours batch 11'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Named Colours batch 11
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
