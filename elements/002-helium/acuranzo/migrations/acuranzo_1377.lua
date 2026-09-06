-- Migration: acuranzo_1377.lua
-- QueryRef #154 - Atomic Claim Next Pending Mail Queue Row
--
-- Phase 11.1-11.3: HA atomic claim to prevent duplicate sends across instances.
-- Replaces the SELECT-then-UPDATE TOCTOU pattern (QueryRef #096 + #097) with a
-- single atomic UPDATE that selects and claims one row in one statement.
--
-- This migration uses a single QueryRef (#154) with a dialect-agnostic SQL
-- template. The SQL uses UPDATE ... WHERE queue_id = (scalar subquery with
-- ORDER BY ... LIMIT 1) AND status_a63 = 0, WITHOUT RETURNING. The caller
-- (worker_claim_cb) detects whether the claim succeeded by checking
-- affected_rows (1 = claimed, 0 = lost race). LIMIT 1 in scalar subqueries
-- is supported by PostgreSQL, MySQL/MariaDB, SQLite, and DB2 11.1+.
--
-- No engine-specific branches: query_cache_lookup() matches by query_ref only,
-- so a single query row under ref 154 is looked up regardless of engine.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-09-06 - Initial creation for MAILRELAY_PLAN Phase 11.1-11.3.
-- 1.0.1 - 2026-09-06 - Consolidated to single QueryRef #154, no engine branches.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1377"
cfg.QUERY_REF = "154"
cfg.QUERY_NAME = "Atomic Claim Next Pending Mail Queue Row"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #154 - Atomic Claim Next Pending Mail Queue Row
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
                    UPDATE ${SCHEMA}mail_queue
                    SET
                        status_a63 = 1,
                        instance_id = :INSTANCE_ID,
                        claim_token = :CLAIM_TOKEN,
                        attempts = attempts + 1,
                        last_attempt_at = ${NOW},
                        updated_at = ${NOW}
                    WHERE
                        queue_id = (
                            SELECT queue_id FROM ${SCHEMA}mail_queue
                            WHERE
                                status_a63 = 0
                                AND (
                                    next_attempt_at IS NULL
                                    OR next_attempt_at <= ${NOW}
                                )
                            ORDER BY
                                priority DESC,
                                next_attempt_at ASC,
                                queue_id ASC
                            LIMIT 1
                        )
                        AND status_a63 = 0
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Atomically claims the next pending outbound mail queue row for
                    exclusive processing by a single instance. Uses
                    `UPDATE ... WHERE queue_id = (subquery with LIMIT 1) AND status_a63 = 0`
                    for a single-statement atomic compare-and-swap. The caller checks
                    affected_rows (1 = claimed, 0 = lost race) to detect success.
                    No `RETURNING` clause is used so the SQL is dialect-agnostic
                    across PostgreSQL, MySQL/MariaDB, SQLite, and DB2 11.1+.

                    ## Parameters

                    - `INSTANCE_ID` (string, required): Owning Hydrogen instance.
                    - `CLAIM_TOKEN` (string, required): Random token held during the
                      send attempt; used for stale recovery.

                    ## Returns

                    - No result rows. The caller checks affected-row count to detect
                      a lost race.

                    ## Tables

                    - `${SCHEMA}mail_queue` (migration 1218).

                    ## Security Notes

                    - `query_type_a28` is `TYPE_INTERNAL_SQL` (0) so this query is
                      not reachable via the REST API.
                ]==]
                                                                                      AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                             AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the internal UPDATE query that atomically claims
            the next pending mail queue row (QueryRef #${QUERY_REF}). Uses a single
            dialect-agnostic SQL template (no engine-specific branches) since
            query_cache_lookup() matches by query_ref only.
        ]=]
                                                                                      AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #154
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
            AND query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                             AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                           AS name,
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
