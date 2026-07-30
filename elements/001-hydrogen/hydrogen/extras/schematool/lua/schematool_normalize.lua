-- schematool_normalize.lua
-- Normalize migration text for loose/strict comparison.
--
-- CHANGELOG
-- 1.0.0 - 2026-07-29 - Phase 4 normalizer

local M = {}

--- Normalize a string for comparison.
-- strict: unify newlines only, preserve other bytes
-- loose: unify newlines, trim trailing WS per line, collapse blank runs, strip trailing newlines
function M.normalize(s, mode)
    if s == nil then
        return ""
    end
    s = tostring(s)
    -- Unify CRLF / CR to LF
    s = s:gsub("\r\n", "\n"):gsub("\r", "\n")

    if mode == "strict" then
        return s
    end

    -- loose
    local lines = {}
    for line in (s .. "\n"):gmatch("(.-)\n") do
        -- trim trailing whitespace
        line = line:gsub("[ \t]+$", "")
        lines[#lines + 1] = line
    end

    -- Collapse runs of blank lines to a single blank line
    local out = {}
    local blank_run = false
    for _, line in ipairs(lines) do
        if line == "" then
            if not blank_run then
                out[#out + 1] = ""
                blank_run = true
            end
        else
            out[#out + 1] = line
            blank_run = false
        end
    end

    -- Drop leading/trailing blank lines (trailing newline normalize)
    while #out > 0 and out[1] == "" do
        table.remove(out, 1)
    end
    while #out > 0 and out[#out] == "" do
        table.remove(out)
    end

    return table.concat(out, "\n")
end

function M.equal(a, b, mode)
    return M.normalize(a, mode) == M.normalize(b, mode)
end

--- Compare code + name + summary; returns ok, list of mismatched field names
function M.match_payload(expected, actual, mode)
    local bad = {}
    if not M.equal(expected.code, actual.code, mode) then
        bad[#bad + 1] = "code"
    end
    if not M.equal(expected.name, actual.name, mode) then
        bad[#bad + 1] = "name"
    end
    if not M.equal(expected.summary, actual.summary, mode) then
        bad[#bad + 1] = "summary"
    end
    return #bad == 0, bad
end

return M
