-- Migration: courses_4098.lua
-- course_build_actions: append-only action ledger
--
-- One row per operator or model action on a draft. This slice writes
-- action_name working_title and leaves the measures null.
--
-- Units, confirm before apply:
--   human_cost_cents  integer cents of operator cost. Null when none.
--   token_cost_cents  integer cents of model spend. Null when no model ran.
--   elapsed_ms        integer milliseconds. Null when not measured.
--   prompt_count      integer count of prompts. Null when no model ran.
--   prompt            prompt text. Null when no model ran.
--   result_ref        short artifact reference, not the model body.
--                     Null when no model ran.
-- created_at is when the action was recorded. actor_account_id is who.
-- A missing measure stays SQL NULL. Rows are not updated.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Create course_build_actions

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_build_actions"
cfg.MIGRATION = "4098"
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
                action_id           ${INTEGER}          NOT NULL,
                run_id              ${INTEGER}          NOT NULL,
                actor_account_id    ${INTEGER}                  ,
                action_name         ${VARCHAR_50}       NOT NULL,
                human_cost_cents    ${INTEGER}                  ,
                token_cost_cents    ${INTEGER}                  ,
                elapsed_ms          ${INTEGER}                  ,
                prompt_count        ${INTEGER}                  ,
                prompt              ${TEXT_BIG}                 ,
                result_ref          ${VARCHAR_100}              ,
                ${COMMON_CREATE}
                ${PRIMARY}(action_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_run
                ON ${SCHEMA}${TABLE}(run_id);

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

            Append-only ledger for one `course_build_runs` row. `run_id`
            is the draft. There is no SQL foreign key. This screen does
            not list the ledger.

            ## Units

            Confirm these before apply. A measure that does not apply
            is NULL.

            - **human_cost_cents**: integer cents.
            - **token_cost_cents**: integer cents of model spend.
            - **elapsed_ms**: integer milliseconds.
            - **prompt_count**: integer count of prompts.
            - **prompt**: prompt text when a model ran.
            - **result_ref**: short reference to a later artifact.
            - **created_at**: when the row was written.
            - **actor_account_id**: account that performed the action.
              Null for a later system action.

            The first writer stores `action_name` `working_title` and
            leaves the six measure columns null.
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

            Drops the ledger, including `${TABLE}_idx_run`. Draft rows
            in `course_build_runs` stay.
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
                            {"name": "action_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "run_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "actor_account_id", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "action_name", "datatype": "${VARCHAR_50}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "human_cost_cents", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "token_cost_cents", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "elapsed_ms", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "prompt_count", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "prompt", "datatype": "${TEXT_BIG}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "result_ref", "datatype": "${VARCHAR_100}", "nullable": true, "primary_key": false, "unique": false},
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
