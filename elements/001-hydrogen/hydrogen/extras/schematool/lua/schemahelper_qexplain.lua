-- schemahelper_qexplain.lua
-- Finding-explanation text generators for SchemaHelper: produces the
-- human-readable check / apply / confirm lines shown in the review and
-- explore views. No internal module dependencies.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_queue.lua (explain cluster)

local M = {}

function M.sides_of(finding)
    local left = finding.expected or ""
    local right = finding.actual or ""
    local lname = "Migration"
    local rname = "Database"
    if finding.live and finding.live ~= "" then
        right = finding.live
        lname = "Expected"
        rname = "Live"
    end
    return left, right, lname, rname
end

function M.explain_default(finding)
    local kind = finding.kind or ""
    if kind ~= "row_missing" and kind ~= "row_diff"
        and kind ~= "row_present" and kind ~= "unkeyed" then
        return nil
    end
    local lines = {}
    lines[#lines + 1] = "  table:     " .. (finding.object or "")
    if finding.column and finding.column ~= "" and finding.column ~= "-" then
        lines[#lines + 1] = "  key:       " .. finding.column
    end
    if kind == "row_missing" then
        lines[#lines + 1] = "  apply:     [U]pdate Database — INSERT this default row"
    elseif kind == "row_diff" then
        lines[#lines + 1] = "  apply:     [U]pdate Database — UPDATE migration-owned columns"
    elseif kind == "row_present" then
        lines[#lines + 1] = "  apply:     [U]pdate Database — DELETE this key only"
    else
        lines[#lines + 1] = "  apply:     refused — run Hydrogen AutoMigration"
    end
    lines[#lines + 1] = "  confirm:   table.key"
    return lines
end

function M.explain_check(finding)
    local lines = {}
    local row_lines = M.explain_default(finding)
    if row_lines then
        return row_lines
    end
    if finding.object and finding.object ~= "" then
        lines[#lines + 1] = "  check:     catalog expected vs live object"
        lines[#lines + 1] = "  table:     " .. finding.object
        if finding.column and finding.column ~= "" and finding.column ~= "-" then
            lines[#lines + 1] = "  column:    " .. finding.column
        end
        if finding.ref then
            lines[#lines + 1] = "  migration: last fold ref "
                .. tostring(finding.ref)
                .. " (expected shape)"
        else
            lines[#lines + 1] = "  migration: (fold did not record a ref)"
        end
        if finding.expected and finding.expected ~= "" then
            lines[#lines + 1] = "  expected:  " .. finding.expected
        end
        if finding.live and finding.live ~= "" then
            lines[#lines + 1] = "  live:      " .. finding.live
        end
        if finding.kind == "nullable" then
            local want_null = (finding.expected == "true"
                or finding.expected == "YES" or finding.expected == "1")
            if want_null then
                lines[#lines + 1] = "  apply:     [U]pdate Database — ALTER COLUMN DROP NOT NULL"
            else
                lines[#lines + 1] = "  apply:     [U]pdate Database — ALTER COLUMN SET NOT NULL"
            end
        elseif finding.kind == "column" then
            lines[#lines + 1] = "  apply:     [U]pdate Database — ADD COLUMN (type from expected fold)"
        elseif finding.kind == "type" then
            lines[#lines + 1] = "  apply:     [U]pdate Database — change column type (existing values may be rejected)"
        elseif finding.kind == "table" then
            lines[#lines + 1] = "  apply:     [U]pdate Database — CREATE TABLE from the folded column list"
        elseif finding.kind == "dropped" then
            if finding.column and finding.column ~= "" and finding.column ~= "-" then
                lines[#lines + 1] = "  apply:     [U]pdate Database — DROP COLUMN (confirm token DROP object.column)"
            else
                lines[#lines + 1] = "  apply:     [U]pdate Database — DROP TABLE (confirm token DROP object)"
            end
        end
        return lines
    end
    local file = finding.file
    if not file or file == "" then
        file = "(migration)"
    end
    local ref = tostring(finding.ref or "?")
    if finding.kind == "orphan" then
        lines[#lines + 1] = "  check:     orphan — ref in DB, absent from disk"
        lines[#lines + 1] = "  migration: (no design_" .. ref .. ".lua on disk)"
        lines[#lines + 1] = "  database:  queries  ref=" .. ref
            .. "  type 1000/1003 (loaded/applied)"
        lines[#lines + 1] = "  action:    [U]pdate Database — deletes ref rows (BETWEEN 1000 AND 1003)"
        return lines
    end
    if finding.db_type == 1003 then
        lines[#lines + 1] = "  check:     APPLY — migration vs applied queries row"
        lines[#lines + 1] = "  migration: " .. file
        lines[#lines + 1] = "  database:  queries  ref=" .. ref .. "  type=1003 (applied)"
        lines[#lines + 1] = "  compared:  code, name, summary"
        lines[#lines + 1] = "  ignored:   query_type 1000→1003 is APPLY promotion, not a defect"
    elseif finding.db_type == 1000 then
        lines[#lines + 1] = "  check:     LOAD — migration vs loaded queries row"
        lines[#lines + 1] = "  migration: " .. file
        lines[#lines + 1] = "  database:  queries  ref=" .. ref .. "  type=1000 (loaded)"
        lines[#lines + 1] = "  compared:  code, name, summary"
    else
        lines[#lines + 1] = "  left:      migration / expected"
        lines[#lines + 1] = "  right:     database / actual"
    end
    if finding.field == "row" then
        lines[#lines + 1] = "  apply:     replace code, name, and summary together"
        lines[#lines + 1] = "  note:      this does not replay DDL"
    end
    return lines
end

return M
