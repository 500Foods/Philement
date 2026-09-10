-- Migration: acuranzo_1382.lua
-- Retarget Lookup 037 from Role Status to Role Origin

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-09-09 - Initial creation. Retargets Lookup 037 from duplicate
--              "Role Status" to "Role Origin" (Seeded / Manual) for the
--              roles table origin_a37 column introduced in migration 1016.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1382"
cfg.LOOKUP_ID = "037"
cfg.LOOKUP_NAME = "Role Origin"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Forward: Retarget Lookup 037 header + value rows to Role Origin
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            -- Retarget the header row (lookup_id 0, key_idx 037) from
            -- "Role Status" (duplicate of 034) to "Role Origin".
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = '${LOOKUP_NAME}',
                value_int  = 0,
                sort_seq   = 0,
                code       = [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Origin of a role assignment: whether it was database-seeded
                    or manually granted by an administrator.
                ]==],
                summary  = 'Origin of a role assignment: Seeded or Manual.',
                collection = ${JIS}[==[{}]==]${JIE}
            WHERE lookup_id = 0
              AND key_idx   = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            -- Retarget the two existing value rows (Inactive/Active) to Origin (Seeded/Manual)
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Seeded',
                value_int  = 0
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Manual',
                value_int  = 0
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Retarget Lookup ${LOOKUP_ID} to ${LOOKUP_NAME}'                    AS name,
        [==[
            # Forward Migration ${MIGRATION}: Retarget Lookup ${LOOKUP_ID}

            Changes Lookup ${LOOKUP_ID} from "Role Status" (a duplicate of 034)
            to "Role Origin" with values Seeded (key_idx 0) and Manual
            (key_idx 1). This distinguishes roles that were database-seeded
            (e.g. mail_send, staff, admin) from manually granted roles.
        ]==]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Reverse: Restore Lookup 037 to Role Status
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            -- Restore Lookup 037 header back to "Role Status"
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Role Status',
                value_int  = 0,
                sort_seq   = 0,
                code       = '',
                summary  = '',
                collection = ${JIS}[==[{}]==]${JIE}
            WHERE lookup_id = 0
              AND key_idx   = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            -- Restore value rows back to Inactive/Active
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Inactive',
                value_int  = 0
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Active',
                value_int  = 0
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
            SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
            and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Lookup ${LOOKUP_ID} to Role Status'                        AS name,
        [==[
            # Reverse Migration ${MIGRATION}: Restore Lookup ${LOOKUP_ID}

            Restores Lookup ${LOOKUP_ID} back to "Role Status" (duplicate of 034)
            with values Inactive (key_idx 0) and Active (key_idx 1), as
            originally seeded by migration 1062.
        ]==]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
