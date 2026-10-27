-- Migration: courses_4118.lua
-- course_build_runs CB-2 pipeline columns and state index
--
-- CB-2 extends the CB-S row. Do not create a second run table.
-- template_key, price_cents, canvas_course_id, and error_text are
-- nullable. Existing rows keep nulls. price_cents is integer cents.
-- canvas_course_id is the Canvas course id once one exists.
-- error_text is the last failure note for state failed.
-- suggestion_id is already UNIQUE (4097), so only state gets an index.
-- No diagram, same as 4101.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-26 - Add CB-2 columns and state index

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "course_build_runs"
cfg.MIGRATION = "4118"
if engine == "mysql" or engine == "mariadb" then
    cfg.DROP_STATE_INDEX = "DROP INDEX course_build_runs_idx_state ON " .. (cfg.SCHEMA or "") .. "course_build_runs"
else
    cfg.DROP_STATE_INDEX = "DROP INDEX " .. (cfg.SCHEMA or "") .. "course_build_runs_idx_state"
end
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
                ADD COLUMN template_key ${VARCHAR_50};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN price_cents ${INTEGER};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN canvas_course_id ${INTEGER};

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN error_text ${VARCHAR_500};

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_state
                ON ${SCHEMA}${TABLE}(state);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add ${TABLE} CB-2 columns and state index'                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: CB-2 columns and state index

            - **template_key**: course template, at most 50 characters.
            - **price_cents**: integer cents. Null until priced.
            - **canvas_course_id**: Canvas course id. Null until a
              course is created in Canvas.
            - **error_text**: last failure note, at most 500
              characters. Set with state `failed`.
            - **${TABLE}_idx_state**: index for list by state.

            `suggestion_id` is already unique. State is unchanged.
            No diagram.
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
            ${DROP_STATE_INDEX};

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN error_text;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN canvas_course_id;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN price_cents;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN template_key;

            ${SUBQUERY_DELIMITER}

            ${REORG}

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} CB-2 columns and state index'                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop CB-2 columns and state index

            Exact undo. Drops `${TABLE}_idx_state`, then the four
            columns. REORG around each DROP.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
