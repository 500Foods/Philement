-- Migration: acuranzo_1385.lua
-- QueryRef #154 claim: derived pick so MySQL and SQL Server accept it
--
-- 1377 stored one UPDATE that names mail_queue in the SET target and again
-- in the WHERE subquery. MySQL rejects that (error 1093: can't specify
-- target table for update in FROM clause). MariaDB accepts it, which is
-- why only the MySQL variants of Test 58 failed. SQL Server has no LIMIT;
-- FreeTDS prepare reports native 8180 ("Statement(s) could not be prepared").
--
-- The pick is wrapped in a derived table so MySQL materializes it before
-- the UPDATE. SQL Server stores OFFSET/FETCH instead of LIMIT. Every other
-- engine keeps LIMIT 1; Firebird still rewrites that token at execute time.
-- affected_rows stays the claim result (1 won, 0 lost). 1377 is not edited.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-01 - QueryRef #154 claim uses a derived pick; MSSQL uses OFFSET/FETCH

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1385"
cfg.QUERY_REF = "154"
cfg.QUERY_NAME = "Atomic Claim Next Pending Mail Queue Row"

local row_gate = "LIMIT 1"
if engine == "mssql" then
    row_gate = "OFFSET 0 ROWS FETCH NEXT 1 ROWS ONLY"
end

local claim_new = [[
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
                            SELECT queue_id FROM (
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
                                ]] .. row_gate .. "\n" .. [[                            ) AS picked
                        )
                        AND status_a63 = 0
]]

local claim_previous = [[
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
]]

-- ----------------------------------------------------------------------------
-- Forward: Replace QueryRef #154 claim text
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. claim_new .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_INTERNAL_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                             AS code,
        'Fix QueryRef #${QUERY_REF} claim for MySQL and SQL Server'               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Fix QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Replaces the claim text installed by migration 1377. The pending row
            is chosen in a derived table so MySQL does not update `mail_queue`
            from itself. SQL Server stores `OFFSET 0 ROWS FETCH NEXT 1 ROWS ONLY`
            in place of `LIMIT 1`. The caller still treats affected_rows 1 as a
            won claim and 0 as a lost race.
        ]=]
                                                                                      AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Restore the 1377 claim text
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. claim_previous .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_INTERNAL_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            AND query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                             AS code,
        'Restore QueryRef #${QUERY_REF} claim text from migration 1377'           AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore QueryRef #${QUERY_REF}

            Puts the migration 1377 claim text back. Provided so the migration
            system can reverse this update.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
