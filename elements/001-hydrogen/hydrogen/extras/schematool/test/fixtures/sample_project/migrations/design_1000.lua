-- Migration: design_1000.lua
-- Bootstraps the migration system by creating the queries table
--
-- CHANGELOG
-- 1.0.0 - 2026-08-23 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1000"

table.insert(queries,{sql=[[
    CREATE TABLE ${SCHEMA}${QUERIES} (
        query_ref               ${INTEGER}          NOT NULL,
        query_code              ${VARCHAR_8192}     NOT NULL
    );
]]})

return queries
end
