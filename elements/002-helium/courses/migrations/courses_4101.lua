-- Migration: courses_4101.lua
-- course_build_runs receipt, flag, and annotation
--
-- receipt_mail is null until the one receipt attempt, then sent or
-- failed. flagged is 0 or 1. annotation is the staff note.
-- Existing rows keep a null receipt. No diagram.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Add receipt_mail, flagged, and annotation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_build_runs"
cfg.MIGRATION = "4101"
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
            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN receipt_mail ${VARCHAR_20};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN flagged ${INTEGER_SMALL} NOT NULL DEFAULT 0;

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN annotation ${VARCHAR_500};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add ${TABLE} receipt, flag, and annotation'                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: receipt, flag, and annotation

            - **receipt_mail**: null when no attempt has been made,
              `sent` when the receipt was queued, `failed` when it
              was not. One attempt. The column is not a retry queue.
            - **flagged**: 0 or 1. Existing rows become 0.
            - **annotation**: staff note, at most 500 characters.
              The editor sends at most 250 so the bind cast fits.

            State is unchanged. No diagram.
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
            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN annotation;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN flagged;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN receipt_mail;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} receipt, flag, and annotation'                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop receipt, flag, and annotation

            Exact undo. REORG around each DROP.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
