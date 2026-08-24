-- Migration: design_1200.lua
-- Migration that is missing LOAD (expected=null in findings)
--
-- CHANGELOG
-- 1.0.0 - 2026-08-23 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookup_types"
cfg.MIGRATION = "1200"

table.insert(queries,{sql=[[
    CREATE TABLE ${SCHEMA}lookup_types (
        type_id     ${INTEGER}          NOT NULL,
        type_name   ${VARCHAR_64}       NOT NULL
    );
]]})

return queries
end
