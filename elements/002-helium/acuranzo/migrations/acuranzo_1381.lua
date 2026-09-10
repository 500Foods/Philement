-- Migration: acuranzo_1381.lua
-- QueryRef #155 - Get Role Names By IDs

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-09-09 - Initial creation. QueryRef #155 resolves integer
--              role_ids to role names for Course Manager authz (L3).

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "roles"
cfg.MIGRATION = "1381"
cfg.QUERY_REF = "155"
cfg.QUERY_NAME = "Get Role Names By IDs"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #155 - Get Role Names By IDs
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
        ${QTC_FAST}                                                         AS query_queue_a58,
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
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        r.role_id,
                        r.name
                    FROM
                        ${SCHEMA}${TABLE} r
                    WHERE
                        r.role_id IN (
                            ${INTEGERS}
                        )
                    AND r.status_a34 = 1
                    ORDER BY
                        r.role_id;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Resolves a list of integer `role_id` values to their
                    human-readable `name` from the `roles` table.

                    ## Parameters

                    - `INTEGERS` (integer, repeated, required): One or more
                      `role_id` values to resolve.

                    ## Returns

                    - `role_id` (integer): The role ID.
                    - `name` (string): The role name (e.g. `staff`, `admin`).

                    ## Tables

                    - `${SCHEMA}${TABLE}` (migration 1016).

                    ## Security Notes

                    - `query_type_a28` is `TYPE_INTERNAL_SQL` (0) so the inner
                      query is not directly reachable via the REST API; the
                      outer query_ref (${QUERY_REF}) is the only public handle.
                    - Reachable via auth_query so JWT-bearing clients can
                      resolve role_ids to names for the staff/admin check (L3).
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

            This migration creates the internal SELECT query that resolves
            role_ids to role names (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #155 - Get Role Names By IDs
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

            Deletes the internal SQL query for role_id -> name resolution
            created by the forward migration.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
