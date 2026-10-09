-- schemahelper_apply_logic.lua
-- Finding classification and policy logic: confirm tokens, refuse reasons,
-- and can_apply gate. Depends on schemahelper_apply_sqlutil and
-- schemahelper_apply_engine.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (finding logic)

local U = require("schemahelper_apply_sqlutil")
local E = require("schemahelper_apply_engine")

local M = {}

function M.confirm_token(finding)
    if not finding then
        return nil
    end
    if finding.kind == "orphan" then
        if not finding.ref then
            return nil
        end
        return tostring(finding.ref)
    end
    if E.is_default_kind(finding.kind) then
        if not finding.object or finding.object == "" then
            return nil
        end
        if not finding.column or finding.column == ""
            or finding.column == "-" then
            return nil
        end
        return finding.object .. "." .. finding.column
    end
    local class = finding.class or ""
    if class:find("^catalog") then
        if finding.kind == "dropped" and finding.object then
            if not E.column_is_table(finding.column) then
                return "DROP " .. finding.object .. "." .. finding.column
            end
            return "DROP " .. finding.object
        end
        if finding.object then
            if not E.column_is_table(finding.column) then
                return finding.object .. "." .. finding.column
            end
            return finding.object
        end
        return nil
    end
    if not finding.ref or not finding.field then
        return nil
    end
    if finding.field == "row" then
        return tostring(finding.ref)
    end
    return string.format("%d.%s", finding.ref, finding.field)
end

local function sqlite_rebuild_reason(kind, column)
    if kind == "nullable" then
        return "SQLite nullability needs a table rebuild"
    end
    if kind == "type" then
        return "SQLite type change needs a table rebuild"
    end
    if kind == "dropped" and not E.column_is_table(column) then
        return "SQLite DROP COLUMN needs a table rebuild"
    end
    return nil
end

function M.refuse_reason(finding, allow_write, engine)
    if not allow_write then
        return "need --allow-write"
    end
    if not finding then
        return "no finding"
    end
    if finding.view == "decoded" then
        return "decoded view — apply the encoded field"
    end
    local kind = finding.kind or ""
    local class = finding.class or ""
    if kind == "missing_load" or kind == "missing_apply" or kind == "unkeyed" then
        return "run Hydrogen AutoMigration"
    end
    if E.is_default_kind(kind) then
        return nil
    end
    if kind == "anomaly" or class:find("anomaly", 1, true) then
        return "anomaly — do not auto-delete"
    end
    if class:find("^catalog") or kind:find("^cat") then
        if kind == "extra_table" or kind == "extra_column"
            or class == "catalog live extra" then
            return "live extra — not applicable"
        end
        if engine == "sqlite" then
            local why = sqlite_rebuild_reason(kind, finding.column)
            if why then
                return why
            end
        end
        if kind == "nullable" or kind == "column" or kind == "type"
            or kind == "dropped" or kind == "table" then
            return nil
        end
        return "catalog / no live DDL"
    end
    if kind == "orphan" then
        if type(finding.ref) ~= "number" or finding.ref < 1 then
            return "missing ref"
        end
        return nil
    end
    local field = finding.field
    if not field or not U.APPLY_FIELDS[field] then
        return "not a metadata field"
    end
    if class ~= "metadata content drift" then
        return "not a metadata field UPDATE"
    end
    if type(finding.ref) ~= "number" or finding.ref < 1 then
        return "missing ref"
    end
    if type(finding.db_type) ~= "number" then
        return "missing query type"
    end
    return nil
end

function M.can_apply(finding, allow_write)
    return M.refuse_reason(finding, allow_write) == nil
end

return M
