-- Migration: acuranzo_1306.lua
-- Phase 56 follow-up: rename Mail.Notices / CourseExpiration to first-dot split
--
-- Hydrogen client invoke splits Group.Name on the FIRST dot
-- (scripting_invoke.c). 1305 seeded group_name='Mail.Notices' +
-- script_name='CourseExpiration', so POST script=Mail.Notices.CourseExpiration
-- looks up Mail / Notices.CourseExpiration and 404s script_not_found.
-- Data-only UPDATE (no diagram). Do not restamp 1305 (already APPLIED).
--
-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Rename 1305 row to Mail / Notices.CourseExpiration (FL-56b)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1306"
cfg.GROUP_NAME = "Mail"
cfg.SCRIPT_NAME = "Notices.CourseExpiration"
-- ----------------------------------------------------------------------------
-- Forward: rename 1305 row to match first-dot invoke
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}scripts
               SET group_name = 'Mail',
                   script_name = 'Notices.CourseExpiration'
             WHERE group_name = 'Mail.Notices'
               AND script_name = 'CourseExpiration';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Rename Mail.Notices.CourseExpiration to first-dot split'           AS name,
        [=[
            # Forward Migration ${MIGRATION}: First-dot invoke rename

            `POST /api/conduit/script` splits `script` on the first `.`.
            Client name stays `Mail.Notices.CourseExpiration`; the
            `scripts` row must be `group_name=Mail` +
            `script_name=Notices.CourseExpiration`. 1305 used the
            opposite split. No code change; columns only. No diagram.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}scripts
               SET group_name = 'Mail.Notices',
                   script_name = 'CourseExpiration'
             WHERE group_name = 'Mail'
               AND script_name = 'Notices.CourseExpiration';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Mail.Notices / CourseExpiration script name'               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore 1305 script name
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
