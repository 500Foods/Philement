-- Migration: acuranzo_1287.lua
-- QueryRef #145 - Canvas: Link Account to Canvas User (Phase 30)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Initial creation for Phase 30 Canvas user create/link

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1287"
cfg.QUERY_REF = "145"
cfg.QUERY_NAME = "Canvas: Link Account to Canvas User"
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
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} link_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}account_canvas_links (
                            link_id,
                            account_id,
                            canvas_user_id,
                            canvas_email,
                            last_seen_at,
                            ${COMMON_FIELDS}
                        )
                        WITH
                            next_link_id AS (
                                SELECT COALESCE(MAX(link_id), 0) + 1 AS new_link_id
                                FROM ${SCHEMA}account_canvas_links
                            ),
                            account_check AS (
                                SELECT account_id
                                FROM ${SCHEMA}accounts
                                WHERE account_id = :ACCOUNTID
                                  AND NOT EXISTS (
                                      SELECT 1
                                      FROM ${SCHEMA}account_canvas_links acl
                                      WHERE acl.account_id = :ACCOUNTID
                                  )
                            )
                        SELECT
                            next_link_id.new_link_id,
                            account_check.account_id,
                            :CANVASUSERID,
                            :CANVASEMAIL,
                            ${NOW},
                            '2025-01-01 00:00:00',
                            '2035-01-01 00:00:00',
                            0,
                            ${NOW},
                            0,
                            ${NOW}
                        FROM next_link_id, account_check
                    ${INSERT_KEY_RETURN} link_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Writes the durable Hydrogen ↔ Canvas link after
                    `ensure_canvas_user` resolves a Canvas `user_id` (Phase 30).
                    Idempotent on `account_id`: if a link already exists the
                    INSERT returns zero rows (caller should have used #144
                    first; this guard covers races / retries).

                    UNIQUE(`canvas_user_id`) still raises if two different
                    Hydrogen accounts try to claim the same Canvas user — that
                    is a real conflict the caller must surface, not swallow.

                    ## Parameters

                    - `ACCOUNTID` (integer): Hydrogen account PK.
                    - `CANVASUSERID` (integer): Canvas LMS numeric user id from
                      the Admin API create-or-lookup response.
                    - `CANVASEMAIL` (string|null): Email used for the Canvas
                      match/create (login_attribute=email).

                    ## Returns

                    - `link_id` (integer): PK of the new link row.
                      Zero rows if `account_id` is missing or already linked.

                    ## Tables

                    - `${SCHEMA}account_canvas_links` (acuranzo_1283).
                    - `${SCHEMA}accounts`: existence check only.

                    ## Notes

                    - Call only after Canvas API succeeded and
                      `email_ambiguous` was ruled out (FL-23 fail-closed).
                    - Does not create the Canvas user — that is the Hydrogen
                      Canvas client / Lua job. This query only persists the
                      mapping.

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

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
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

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
