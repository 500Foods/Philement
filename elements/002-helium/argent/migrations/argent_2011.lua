-- Migration: argent_2011.lua
-- Seeds lookup 2011, calendar state

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Seed lookup 2011 (not set, pending, set, failed)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "2011"
cfg.LOOKUP_ID = "2011"
cfg.LOOKUP_NAME = "Calendar State"
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
                'calendar_state',
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Calendar sync state for Argent. Keys 1 through 4 are not set,
                    pending, set, and failed.
                    Column: transactions.calendar_state_a2011.
                    Inserts write key 1 until a tool sets another.
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
                (${LOOKUP_ID}, 1, 1, 'Not set',  0, 1, 'not_set', '', ${JIS}[==[{"icon":"<fa fa-calendar></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 2, 1, 'Pending',  0, 2, 'pending', '', ${JIS}[==[{"icon":"<fa fa-hourglass></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 3, 1, 'Set',      0, 3, 'set',     '', ${JIS}[==[{"icon":"<fa fa-calendar-check></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 4, 1, 'Failed',   0, 4, 'failed',  '', ${JIS}[==[{"icon":"<fa fa-calendar-xmark></fa>"}]==]${JIE}, ${COMMON_VALUES});

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
            Value keys are 1 through 4. No new table. No caller-facing QueryRef.
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
              AND key_idx IN (1, 2, 3, 4);

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

            Deletes the directory row and keys 1 through 4. No other lookup_id.
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
                            {"key_idx": 1, "code": "not_set", "value_txt": "Not set"},
                            {"key_idx": 2, "code": "pending", "value_txt": "Pending"},
                            {"key_idx": 3, "code": "set", "value_txt": "Set"},
                            {"key_idx": 4, "code": "failed", "value_txt": "Failed"}
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
