-- Migration: argent_2026.lua
-- Seeds lookup 2010, attachment type

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Seed lookup 2010 (note, pdf, image, report, other)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "2026"
cfg.LOOKUP_ID = "2010"
cfg.LOOKUP_NAME = "Attachment Type"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            INSERT INTO ${SCHEMA}${TABLE}
            (
                lookup_id,
                key_idx,
                status_a1,
                value_txt,
                value_int,
                sort_seq,
                code,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                0,
                ${LOOKUP_ID},
                1,
                '${LOOKUP_NAME}',
                0,
                0,
                'att_type',
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Attachment type for Argent. Keys 1 through 5 are note,
                    pdf, image, report, and other.
                    Column: attachments.att_type_a2010.
                    A note uses key 1. file_data stays null and file_text
                    holds the body. There is no attachment-status lookup.
                ]==],
                ${JSON_INGEST_START}
                [==[
                    {}
                ]==]
                ${JSON_INGEST_END},
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 1, 1, 'Note',   0, 1, 'note',   '', ${JIS}[==[{"icon":"<fa fa-note-sticky></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 2, 1, 'PDF',    0, 2, 'pdf',    '', ${JIS}[==[{"icon":"<fa fa-file-pdf></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 3, 1, 'Image',  0, 3, 'image',  '', ${JIS}[==[{"icon":"<fa fa-image></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 4, 1, 'Report', 0, 4, 'report', '', ${JIS}[==[{"icon":"<fa fa-chart-column></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 5, 1, 'Other',  0, 5, 'other',  '', ${JIS}[==[{"icon":"<fa fa-ellipsis></fa>"}]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            One family. The directory row is lookup_id 0, key_idx ${LOOKUP_ID}.
            Value keys are 1 through 5. No new table. No caller-facing QueryRef.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            WHERE lookup_id = 0
              AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx IN (1, 2, 3, 4, 5);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Deletes the directory row and keys 1 through 5. No other lookup_id.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON lookup definition in collection'                              AS code,
        'Diagram Lookup ${LOOKUP_ID}'                                       AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Lookup ${LOOKUP_ID} keys. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "lookup",
                        "object_id": "lookup.${LOOKUP_ID}",
                        "object_ref": "${MIGRATION}",
                        "lookup_id": "${LOOKUP_ID}",
                        "name": "${LOOKUP_NAME}",
                        "keys": [
                            {"key_idx": 1, "code": "note", "value_txt": "Note"},
                            {"key_idx": 2, "code": "pdf", "value_txt": "PDF"},
                            {"key_idx": 3, "code": "image", "value_txt": "Image"},
                            {"key_idx": 4, "code": "report", "value_txt": "Report"},
                            {"key_idx": 5, "code": "other", "value_txt": "Other"}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
