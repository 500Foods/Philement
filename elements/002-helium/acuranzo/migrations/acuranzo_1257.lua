-- Migration: acuranzo_1257.lua
-- Create Mail Relay send role

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-08 - Initial creation for MAILRELAY_PLAN Phase 7.5b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "roles"
cfg.MIGRATION = "1257"
-- ----------------------------------------------------------------------------
-- Forward: Insert the mail_send role
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
            INSERT INTO ${SCHEMA}${TABLE} (
                role_id,
                status_a34,
                scope_a35,
                type_a36,
                name,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                1,                              -- role_id
                1,                              -- Active (Role Status lookup 034)
                1,                              -- System (Role Scope lookup 035)
                1,                              -- Auditor (Role Type lookup 036)
                'mail_send',                    -- name
                'Can send and preview Mail Relay messages.', -- summary
                '{}',                           -- collection
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create mail_send Role'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create mail_send Role

            Inserts the `mail_send` role required by the Mail Relay REST API
            (`POST /api/mailrelay/send` and `POST /api/mailrelay/preview`).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove the mail_send role
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
            WHERE role_id = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove mail_send Role'                                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove mail_send Role

            Deletes the `mail_send` role created by the forward migration.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
