-- Migration: acuranzo_1235.lua
-- QueryRef #105 - Insert Mail Template

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.1b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1235"
cfg.QUERY_REF = "105"
cfg.QUERY_NAME = "Insert Mail Template"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #105 - Insert Mail Template
-- ----------------------------------------------------------------------------
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
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_INTERNAL_SQL}                                                AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} template_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_templates (
                            template_id,
                            template_key,
                            name,
                            status_a64,
                            subject_template,
                            text_template,
                            html_template,
                            collection,
                            ${COMMON_FIELDS}
                        )
                        WITH next_template_id AS (
                            SELECT COALESCE(MAX(template_id), 0) + 1 AS new_template_id
                            FROM ${SCHEMA}mail_templates
                        )
                        SELECT
                            new_template_id,
                            :TEMPLATE_KEY,
                            :NAME,
                            :STATUS_A64,
                            :SUBJECT_TEMPLATE,
                            :TEXT_TEMPLATE,
                            :HTML_TEMPLATE,
                            :COLLECTION,
                            ${COMMON_VALUES}
                        FROM next_template_id
                    ${INSERT_KEY_RETURN} template_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a new reusable mail template into
                    `${SCHEMA}mail_templates`. The application generates the
                    `template_id` rather than relying on an engine-specific
                    autoincrement.

                    ## Parameters

                    - `TEMPLATE_KEY` (string, required): Unique external
                      identifier used by API/Lua callers.
                    - `NAME` (string, required): Human-readable template name.
                    - `STATUS_A64` (smallint, required): Template status from
                      Lookup 064 (0=inactive, 1=active, 2=deprecated).
                    - `SUBJECT_TEMPLATE` (string, required): Subject line
                      template with `%MACRO%` placeholders.
                    - `TEXT_TEMPLATE` (string, optional): Plain-body template.
                    - `HTML_TEMPLATE` (string, optional): HTML-body template.
                    - `COLLECTION` (json, required): Generic JSON metadata;
                      pass `'{}'` when no metadata is needed.

                    ## Returns

                    - `template_id`: The newly inserted template primary key.

                    ## Tables

                    - `${SCHEMA}mail_templates` (migration 1217).

                    ## Security Notes

                    - `query_type_a28` is `${TYPE_INTERNAL_SQL}` (0) so this query
                      is not reachable via the REST API.
                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the internal INSERT query for mail templates
            (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #105 - Insert Mail Template
-- ----------------------------------------------------------------------------
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
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
