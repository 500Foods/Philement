-- Migration: acuranzo_1145.lua
-- Test Data - Auth Test API Keys

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.4.0 - 2026-01-11 - Fixed lists table columns (list_key, list_type_a31, status_a32, list_note vs wrong names)
-- 1.3.0 - 2026-01-11 - Added SUBQUERY_DELIMITER between INSERT/DELETE statements for MySQL compatibility
-- 1.2.0 - 2026-01-11 - Fixed column names to match licenses table schema (app_key not api_key)
-- 1.1.0 - 2026-01-10 - Fixed date typo, aligned reverse migration with forward migration
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
            -- Uses licenses table with columns: license_id, status_a13, app_key, system_id, name, summary, collection
            INSERT INTO ${SCHEMA}licenses (
                license_id,
                status_a13,
                app_key,
                system_id,
                name,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES
                (9001, 1, '${HYDROGEN_DEMO_API_KEY}', 1, 'Demo API Key',         'Valid API key for testing',  ${JIS}[==[ {"auth": true}                   ]==]${JIE}, ${COMMON_VALUES}),
                (9002, 1, 'demo-api-key-admin',       1, 'Demo Admin API Key',   'Admin API key for testing',  ${JIS}[==[ {"auth": true, "admin": true}    ]==]${JIE}, ${COMMON_VALUES}),
                (9003, 0, 'demo-api-key-expired',     1, 'Expired API Key',      'Expired API key for testing', ${JIS}[==[ {"auth": true}                   ]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            -- Insert test IP whitelist/blacklist entries
            -- Uses lists table with columns: list_key, list_type_a31, status_a32, list_value, list_note, collection
            INSERT INTO ${SCHEMA}lists (
                list_key,
                list_type_a31,
                status_a32,
                list_value,
                list_note,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES
                (9001, 1, 1, '192.168.1.100', 'Test Whitelist', '{}', ${COMMON_VALUES}),  -- Whitelisted IP
                (9002, 0, 1, '192.168.1.200', 'Test Blacklist', '{}', ${COMMON_VALUES});  -- Blocked IP

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
            -- Remove test API keys (demo keys inserted in forward migration)
            -- Uses fixed license_ids (9001, 9002, 9003) for reliable deletion
            DELETE FROM ${SCHEMA}licenses
            WHERE license_id IN (9001, 9002, 9003);

            ${SUBQUERY_DELIMITER}

            -- Remove test IP entries (uses fixed list_keys 9001, 9002 for reliable deletion)
            DELETE FROM ${SCHEMA}lists
            WHERE list_key IN (9001, 9002);

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