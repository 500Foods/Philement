-- Migration: helium_0000.lua
-- Bootstraps the migration system by creating the queries table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for queries table with SQLite, PostgreSQL, MySQL, DB2 support.

local config = require 'database'

return {
    queries = {
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            id = 1000,
            title = "Create queries table",
            checksum = "dummy_checksum_1000",
            docs =  [[
                        # Helium Migration 1000
                        Creates the `queries` table to store migration metadata and utility queries.
                    ]],
            sql =   [[
                        %%CREATE_TABLE%% %%SCHEMA%%queries (
                            query_id %%SERIAL%%,
                            query_ref %%INTEGER%% NOT NULL,
                            query_type_lua_28 %%INTEGER%% NOT NULL,
                            query_dialect_lua_30 %%INTEGER%% NOT NULL,
                            name %%VARCHAR_100%% NOT NULL,
                            summary %%TEXT%%,
                            query_code %%TEXT%% NOT NULL,
                            query_status_lua_27 %%INTEGER%% NOT NULL,
                            collection %%JSONB%%,
                            valid_after %%TIMESTAMP_TZ%%,
                            valid_until %%TIMESTAMP_TZ%%,
                            created_id %%INTEGER%% NOT NULL,
                            created_at %%TIMESTAMP_TZ%% NOT NULL,
                            updated_id %%INTEGER%% NOT NULL,
                            updated_at %%TIMESTAMP_TZ%% NOT NULL
                        );
                    ]]
        },
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            id = 10,
            title = "Utility: Count pending migrations",
            checksum = "dummy_checksum_10",
            docs =  [[
                        # Utility Query 10
                        Counts pending migrations in the `queries` table for a given engine.
                    ]],
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            query_id,
                            query_ref,
                            query_type_lua_28,
                            query_dialect_lua_30,
                            name,
                            summary,
                            query_code,
                            query_status_lua_27,
                            collection,
                            valid_after,
                            valid_until,
                            created_id,
                            created_at,
                            updated_id,
                            updated_at
                        )
                        VALUES (
                            10,
                            0,
                            1,
                            %%DIALECT%%,
                            'Utility: Count pending migrations',
                            [=[
                                # Utility Query 10
                                Counts pending migrations in the `queries` table for a given engine.
                            ]=],
                            [=[
                                SELECT COUNT(*)
                                FROM %%SCHEMA%%queries
                                WHERE query_status_lua_27 = %%STATUS_PENDING%% AND query_dialect_lua_30 = ?
                            ]=],
                            %%STATUS_UTILITY%%,
                            NULL,
                            NULL,
                            NULL,
                            0,
                            %%NOW%%,
                            0,
                            %%NOW%%
                        );
                    ]]
        },
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            id = 11,
            title = "Utility: Fetch next migration",
            docs =  [[
                        # Utility Query 11
                        Fetches the next pending migration from the `queries` table for a given engine.
                    ]],
            checksum = "dummy_checksum_11",
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            query_id,
                            query_ref,
                            query_type_lua_28,
                            query_dialect_lua_30,
                            name,
                            summary,
                            query_code,
                            query_status_lua_27,
                            collection,
                            valid_after,
                            valid_until,
                            created_id,
                            created_at,
                            updated_id,
                            updated_at
                        )
                        VALUES (
                            11,
                            0,
                            1,
                            %%DIALECT%%,
                            'Utility: Fetch next migration',
                            [=[
                                # Utility Query 11
                                Fetches the next pending migration from the `queries` table for a given engine.
                            ]=],
                            [=[
                                SELECT query_id, query_code
                                FROM %%SCHEMA%%queries
                                WHERE query_status_lua_27 = %%STATUS_PENDING%%
                                  AND query_dialect_lua_30 = ?
                                ORDER BY query_id ASC
                                LIMIT 1
                            ]=],
                            %%STATUS_UTILITY%%,
                            NULL,
                            NULL,
                            NULL,
                            0,
                            %%NOW%%,
                            0,
                            %%NOW%%
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
    }
}
