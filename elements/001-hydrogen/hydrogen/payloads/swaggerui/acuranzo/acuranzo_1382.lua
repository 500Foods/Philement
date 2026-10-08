-- Migration: acuranzo_1382.lua
-- Retarget Lookup 037 from Role Status to Role Origin

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-09-16 - Reverse restores 1062 header summary/collection and value-row icons/sort_seq. Forward clears those icons. No origin_a37 column on roles.
-- 1.0.0 - 2026-09-09 - Initial creation. Retargets Lookup 037 from duplicate Role Status to Role Origin (Seeded / Manual).

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
            -- "Role Status" (duplicate of 034, seeded by 1062) to "Role Origin".
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = '${LOOKUP_NAME}',
                value_int  = 0,
                sort_seq   = 0,
                code       = [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Origin of a role assignment: whether it was database-seeded
                    or manually granted by an administrator.
                ]==],
                summary  = [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Origin of a role assignment: Seeded or Manual.
                ]==],
                collection = ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": true,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END}
            WHERE lookup_id = 0
              AND key_idx   = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Seeded',
                value_int  = 0,
                sort_seq   = 0,
                code       = '',
                summary    = '',
                collection = ${JIS}[==[{}]==]${JIE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Manual',
                value_int  = 0,
                sort_seq   = 1,
                code       = '',
                summary    = '',
                collection = ${JIS}[==[{}]==]${JIE}
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

            Changes Lookup ${LOOKUP_ID} from "Role Status" (a duplicate of 034
            seeded by migration 1062) to "Role Origin" with values Seeded
            (key_idx 0) and Manual (key_idx 1). This is an UPDATE of those
            existing rows, not an INSERT. The `roles` table has no origin
            column yet; this lookup is the catalog for that meaning.
        ]==]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Reverse: Restore Lookup 037 to Role Status as seeded by 1062
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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Role Status',
                value_int  = 0,
                sort_seq   = 0,
                code       = '',
                summary  = [==[
                    # 037 - Role Status

                    Role status - active or inactive.
                ]==],
                collection = ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": true,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END}
            WHERE lookup_id = 0
              AND key_idx   = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Inactive',
                value_int  = 0,
                sort_seq   = 0,
                code       = '',
                summary    = '',
                collection = ${JIS}[==[{"icon":"<fa fa-xmark fa-swap-opacity style='color: #FF0000; filter: var(--ACZ-shadow-4);'></fa>"}]==]${JIE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx   = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = 'Active',
                value_int  = 0,
                sort_seq   = 1,
                code       = '',
                summary    = '',
                collection = ${JIS}[==[{"icon":"<fa fa-check fa-swap-opacity style='color: #00FF00; filter: var(--ACZ-shadow-4);'></fa>"}]==]${JIE}
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
            originally seeded by migration 1062, including header summary,
            editor collection, and value-row icons.
        ]==]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
