-- Migration: acuranzo_1437.lua
-- Lookup 054 (Report Object Attributes) fonts for Reporting Phase 12
--
-- Does not edit 1087 in place. Key 15 Font Family gains Vanadium Sans
-- and Vanadium Mono ahead of the existing faces, which stay so an old
-- row still resolves; the hint default becomes Vanadium Sans. Key 4
-- Align lists Left, Center, Right. Key 17 Font Style lists the v1 set
-- Normal, Bold, Italic, Bold + Italic. No caller-facing QueryRef.
-- Reverse restores the 1087 JSON for keys 4, 15, and 17.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-11-02 - Initial creation (Reporting Phase 12)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1437"
cfg.LOOKUP_ID = "054"
cfg.LOOKUP_NAME = "Report Object Attributes"
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
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right"], "Group": "Position", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Default: Vanadium Sans"}, "List": ["Vanadium Sans", "Vanadium Mono", "Helvetica", "Open Sans", "Cairo", "Courier New", "Lexend"], "Group": "Fonts", "Label": "Font Family", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 15;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Normal", "Bold", "Italic", "Bold + Italic"], "Group": "Fonts", "Label": "Font Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 17;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Update Lookup ${LOOKUP_ID} fonts in ${TABLE} table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} fonts

            Adds Vanadium Sans and Vanadium Mono to key 15, sets key 4 Align and
            key 17 Font Style to the v1 captions.
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
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Default: Helvetica"}, "List": ["Helvetica", "Open Sans", "Cairo", "Courier New", "Lexend"], "Group": "Fonts", "Label": "Font Family", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 15;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Normal", "Bold", "Underline", "Italic", "Bold + Underlilne", "Bold + Italic", "Underline + Italic", "Bold + Underline + Italic", "Superscript", "Subscript", "Strikethrough"], "Group": "Fonts", "Label": "Font Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 17;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Undo Lookup ${LOOKUP_ID} fonts in ${TABLE} table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Undo Lookup ${LOOKUP_ID} fonts

            Restores the acuranzo_1087 JSON on keys 4, 15, and 17.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
