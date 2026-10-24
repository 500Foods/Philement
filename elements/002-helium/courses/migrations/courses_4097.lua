-- Migration: courses_4097.lua
-- course_build_runs: one draft row per suggestion
--
-- Identity for Course Builder. suggestion_id is unique. working_title
-- starts equal to course_name and can diverge. state starts as
-- suggested. course_suggestions is not altered. Later pipeline
-- columns extend this table. Do not create a second run table.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Create course_build_runs

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_build_runs"
cfg.MIGRATION = "4097"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                run_id              ${INTEGER}          NOT NULL,
                suggestion_id       ${INTEGER}          NOT NULL,
                working_title       ${VARCHAR_100}      NOT NULL,
                state               ${VARCHAR_50}       NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(run_id),
                ${UNIQUE}(suggestion_id)
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            One row per course suggestion once a working title is saved.
            `suggestion_id` matches `course_suggestions.suggestion_id`
            and is unique. There is no SQL foreign key.

            - **run_id**: Surrogate primary key.
            - **suggestion_id**: The suggestion this draft belongs to.
            - **working_title**: Operator title, at most 100 characters.
              The submitted `course_name` stays on `course_suggestions`.
            - **state**: `suggested` on the first save. Later values
              extend this column.

            `created_at` and `updated_at` come from `COMMON_CREATE`.
            The list uses `updated_at` as last activity when this row
            exists.
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Drops the draft identity. Reverse the action ledger
            (migration 4098) first when that table is present.
            `course_suggestions` stays.
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
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {"name": "run_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "suggestion_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": true},
                            {"name": "working_title", "datatype": "${VARCHAR_100}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "state", "datatype": "${VARCHAR_50}", "nullable": false, "primary_key": false, "unique": false},
                            ${COMMON_DIAGRAM}
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
