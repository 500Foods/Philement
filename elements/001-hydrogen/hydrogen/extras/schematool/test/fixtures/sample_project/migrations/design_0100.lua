-- Migration: design_0100.lua
-- Adds the accounts table
--
-- CHANGELOG
-- 1.0.0 - 2026-08-23 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "accounts"
cfg.MIGRATION = "0100"

table.insert(queries,{sql=[[
    CREATE TABLE ${SCHEMA}accounts (
        id                  ${INTEGER}          NOT NULL,
        password_hash       ${VARCHAR_255}      NOT NULL,
        name                ${VARCHAR_255}
    );
]]})

return queries
end
