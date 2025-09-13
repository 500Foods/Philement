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
                        CREATE TABLE %%SCHEMA%%queries (
                            id %%INTEGER%% PRIMARY KEY,
                            engine %%TEXT%% NOT NULL,
                            status %%TEXT%% NOT NULL %%CHECK_CONSTRAINT%%,
                            query %%TEXT%% NOT NULL,
                            docs %%TEXT%%,
                            last_modified %%TEXT%% NOT NULL,
                            checksum %%TEXT%% NOT NULL
                        )
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
                            id,
                            engine,
                            status,
                            query,
                            docs,
                            last_modified,
                            checksum
                        )
                        VALUES (
                            10,
                            '%s',
                            'Utility',
                            [=[
                                SELECT COUNT(*) 
                                FROM %%SCHEMA%%queries 
                                WHERE status = 'Pending' AND engine = ?
                            ]=],
                            [=[
                                # Utility Query 10
                                Counts pending migrations in the `queries` table.
                            ]=],
                            %%TIMESTAMP%%,
                            'dummy_checksum_10'
                        )
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
                            id,
                            engine,
                            status,
                            query,
                            docs,
                            last_modified,
                            checksum
                        )
                        VALUES (
                            11,
                            '%s',
                            'Utility',
                            [=[
                                SELECT id, query 
                                FROM %%SCHEMA%%queries 
                                WHERE status = 'Pending' 
                                  AND engine = ?
                                ORDER BY id ASC 
                                LIMIT 1
                            ]=],
                            [=[
                                # Utility Query 11
                                Fetches the next pending migration from the `queries` table.
                            ]=],
                            %%TIMESTAMP%%,
                            'dummy_checksum_11'
                        )
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
    }
}