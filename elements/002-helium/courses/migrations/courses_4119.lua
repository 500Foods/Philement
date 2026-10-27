-- Migration: courses_4119.lua
-- course_build_artifacts: outline, materials, modules
--
-- One row per artifact version for a course_build_runs row. kind is
-- outline, material, module_html, checklist, or a later kind.
-- module_index is null when the artifact is not per module.
-- body is TEXT or JSON text. model_id is null for human work.
-- created_at is from COMMON_CREATE. Rows are appended, not updated;
-- a new version is a new row. There is no SQL foreign key.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-26 - Create course_build_artifacts

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_build_artifacts"
cfg.MIGRATION = "4119"
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
                artifact_id         ${INTEGER}          NOT NULL,
                run_id              ${INTEGER}          NOT NULL,
                kind                ${VARCHAR_50}       NOT NULL,
                module_index        ${INTEGER}                  ,
                body                ${TEXT_BIG}                 ,
                model_id            ${VARCHAR_100}              ,
                ${COMMON_CREATE}
                ${PRIMARY}(artifact_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_run
                ON ${SCHEMA}${TABLE}(run_id, kind, module_index);

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

            Artifacts for one `course_build_runs` row. `run_id` is the
            draft. There is no SQL foreign key. Materials and modules
            are artifacts, not states.

            - **artifact_id**: Surrogate primary key.
            - **run_id**: The draft this artifact belongs to.
            - **kind**: `outline`, `material`, `module_html`,
              `checklist`, or a later kind. At most 50 characters.
            - **module_index**: Module number. Null when not per module.
            - **body**: Artifact text or JSON text.
            - **model_id**: Model that produced it. Null for human work.

            Rows are appended. A new version is a new row; the
            latest is the highest `artifact_id` for run, kind, and
            module. `created_at` comes from `COMMON_CREATE`.
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

            Drops the artifacts, including `${TABLE}_idx_run`.
            `course_build_runs` and `course_build_actions` stay.
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
                            {"name": "artifact_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "run_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "kind", "datatype": "${VARCHAR_50}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "module_index", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "body", "datatype": "${TEXT_BIG}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "model_id", "datatype": "${VARCHAR_100}", "nullable": true, "primary_key": false, "unique": false},
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
