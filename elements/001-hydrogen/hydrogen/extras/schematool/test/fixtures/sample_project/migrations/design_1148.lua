-- Migration: design_1148.lua
-- Migration with content drift (code field mismatch)
--
-- CHANGELOG
-- 1.0.0 - 2026-08-23 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "drift_test"
cfg.MIGRATION = "1148"

table.insert(queries,{sql=[[
    CREATE TABLE ${SCHEMA}drift_test (
        id      ${INTEGER}          NOT NULL,
        code    ${VARCHAR_50}       NOT NULL
    );
]]})

return queries
end
