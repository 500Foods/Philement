-- Migration: design_1290.lua
-- Orphan ref in DB but not on disk
--
-- CHANGELOG
-- 1.0.0 - 2026-08-23 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "legacy_data"
cfg.MIGRATION = "1290"

table.insert(queries,{sql=[[
    CREATE TABLE ${SCHEMA}legacy_data (
        old_id      ${INTEGER}          NOT NULL,
        old_value   ${VARCHAR_255}
    );
]]})

return queries
end
