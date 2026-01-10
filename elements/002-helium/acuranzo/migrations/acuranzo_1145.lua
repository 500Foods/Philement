-- Migration: acuranzo_1145.lua
-- Test Data - Auth Test API Keys

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-01-08 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1145"
cfg.QUERY_REF = "1145"
cfg.QUERY_NAME = "Test Data - Auth Test API Keys"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            -- Insert test API keys for authentication testing
            INSERT INTO ${SCHEMA}licenses (
                api_key,
                system_id,
                app_id,
                license_expiry,
                features,
                ${COMMON_FIELDS}
            )
            VALUES
                ('${HYDROGEN_DEMO_API_KEY}', 1, 1, '2026-12-31', ${JIS} {"auth": true} ${JIE}, ${COMMON_VALUES}),
                ('demo-api-key-admin', 1, 1, '2026-12-31', ${JIS} {"auth": true, "admin": true} ${JIE}, ${COMMON_VALUES}),
                ('demo-api-key-expired', 1, 1, '2025-121-311', ${JIS} {"auth": true} ${JIE}, ${COMMON_VALUES});

            -- Insert test IP whitelist/blacklist entries
            INSERT INTO ${SCHEMA}lists (
                list_type,
                list_name,
                list_value,
                list_status,
                ${COMMON_FIELDS}
            )
            VALUES
                (1, 'Test Whitelist', '192.168.1.100', 1, ${COMMON_VALUES}),  -- Whitelisted IP
                (0, 'Test Blacklist', '192.168.1.200', 1, ${COMMON_VALUES});  -- Blocked IP

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Test Data - Auth Test API Keys and IPs'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Test Data - Auth Test API Keys and IPs

            This migration creates test API keys and IP whitelist/blacklist entries for authentication testing.
            Includes valid, admin, and expired API keys, plus test IP entries.
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
            -- Remove test API keys
            DELETE FROM ${SCHEMA}licenses
            WHERE api_key LIKE 'test-api-key-%';

            -- Remove test IP entries
            DELETE FROM ${SCHEMA}lists
            WHERE list_name LIKE 'Test %';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Test Data - Auth Test API Keys and IPs'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Test Data - Auth Test API Keys and IPs

            This migration removes the test API keys and IP entries created for authentication testing.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end