-- jsstring.lua -- no counterpart in PDFKit
--
-- PDFKit leans on JavaScript string methods that Lua does not have, and the
-- differences are not cosmetic. Three of them decide where text breaks:
--
--   1. WHAT COUNTS AS WHITESPACE. JavaScript's /\s/ matches sixteen codepoints,
--      including no-break space, the en/em quad family, line separator and the
--      byte order mark. Lua's %s matches six ASCII bytes. Text containing a
--      U+00A0 wraps differently under the two definitions.
--
--   2. WHAT .length COUNTS. JavaScript counts UTF-16 code units, so an emoji is
--      two and a CJK ideograph is one. PDFKit multiplies characterSpacing by
--      (length - 1), so getting this wrong shifts every glyph after the first.
--      utf16_length below is the faithful count.
--
--   3. HOW split(/\s+/) TREATS THE EDGES. Splitting "  a b" gives a leading
--      empty string, and "a b " gives a trailing one. PDFKit counts the result
--      to divide up justified space, so a phantom element widens the gaps.
--
-- Slicing is the one place this module deliberately does NOT follow JavaScript.
-- The original slices by UTF-16 code unit, which can cut an emoji in half; the
-- source even carries a TODO saying so. Here slicing is by codepoint, so a
-- chopped-up long word is always valid UTF-8. For text in the Basic
-- Multilingual Plane -- which is everything the standard 14 fonts can render --
-- the two agree exactly.

local M = {}

-- The sixteen codepoints JavaScript's /\s/ matches. Listed rather than ranged
-- so that each one is visible and checkable against the spec.
local SPACE_CODEPOINTS = {
    [0x0009] = true, -- tab
    [0x000A] = true, -- line feed
    [0x000B] = true, -- vertical tab
    [0x000C] = true, -- form feed
    [0x000D] = true, -- carriage return
    [0x0020] = true, -- space
    [0x00A0] = true, -- no-break space
    [0x1680] = true, -- ogham space mark
    [0x2028] = true, -- line separator
    [0x2029] = true, -- paragraph separator
    [0x202F] = true, -- narrow no-break space
    [0x205F] = true, -- medium mathematical space
    [0x3000] = true, -- ideographic space
    [0xFEFF] = true, -- byte order mark
}
-- U+2000..U+200A: en quad through hair space.
for cp = 0x2000, 0x200A do
    SPACE_CODEPOINTS[cp] = true
end

function M.is_space(codepoint)
    return SPACE_CODEPOINTS[codepoint] or false
end

--------------------------------------------------------------------------------
-- Codepoint access
--------------------------------------------------------------------------------

--- Byte offsets of every codepoint in `s`, one-based, plus a sentinel one past
--- the end. So #offsets - 1 is the number of codepoints, and codepoint i spans
--- bytes offsets[i] .. offsets[i + 1] - 1.
---
--- Bytes that do not begin a well-formed sequence are treated as one codepoint
--- each. Nothing here should ever raise on malformed input: this module is on
--- the path of every string a caller hands to text().
local function offsets_of(s)
    local offsets = {}
    local i, n = 1, #s
    while i <= n do
        offsets[#offsets + 1] = i
        local b = s:byte(i)
        local width
        if b < 0x80 then
            width = 1
        elseif b >= 0xF0 and b <= 0xF4 then
            width = 4
        elseif b >= 0xE0 and b <= 0xEF then
            width = 3
        elseif b >= 0xC2 and b <= 0xDF then
            width = 2
        else
            width = 1 -- stray continuation byte
        end

        -- Verify the continuation bytes are actually there; a truncated
        -- sequence advances by one so the walk cannot run off the end.
        for k = 1, width - 1 do
            local c = s:byte(i + k)
            if not c or c < 0x80 or c > 0xBF then
                width = 1
                break
            end
        end

        i = i + width
    end
    offsets[#offsets + 1] = n + 1
    return offsets
end

M.offsets_of = offsets_of

--- Number of codepoints in `s`.
function M.length(s)
    return #offsets_of(s) - 1
end

--- Number of UTF-16 code units in `s` -- what JavaScript's .length returns.
--- Everything outside the Basic Multilingual Plane counts as two.
function M.utf16_length(s)
    local units = 0
    local i, n = 1, #s
    while i <= n do
        local b = s:byte(i)
        if b < 0x80 then
            units = units + 1
            i = i + 1
        elseif b >= 0xF0 and b <= 0xF4 and (s:byte(i + 3) or 0) >= 0x80 then
            units = units + 2 -- surrogate pair
            i = i + 4
        elseif b >= 0xE0 and b <= 0xEF and (s:byte(i + 2) or 0) >= 0x80 then
            units = units + 1
            i = i + 3
        elseif b >= 0xC2 and b <= 0xDF and (s:byte(i + 1) or 0) >= 0x80 then
            units = units + 1
            i = i + 2
        else
            units = units + 1
            i = i + 1
        end
    end
    return units
end

--- Decode the codepoint at one-based byte offset `i`. Returns the codepoint
--- only; the caller is expected to know the width from offsets_of.
local function decode_at(s, i)
    local b = s:byte(i)
    if not b then return nil end
    if b < 0x80 then return b end

    local need, cp
    if b >= 0xF0 and b <= 0xF4 then
        need, cp = 3, b & 0x07
    elseif b >= 0xE0 and b <= 0xEF then
        need, cp = 2, b & 0x0F
    elseif b >= 0xC2 and b <= 0xDF then
        need, cp = 1, b & 0x1F
    else
        return b
    end

    for k = 1, need do
        local c = s:byte(i + k)
        if not c or c < 0x80 or c > 0xBF then
            return b
        end
        cp = (cp << 6) | (c & 0x3F)
    end
    return cp
end

M.decode_at = decode_at

--- The first `n` codepoints of `s`. Equivalent to JavaScript's slice(0, n),
--- except measured in codepoints.
function M.prefix(s, n)
    if n <= 0 then return "" end
    local offsets = offsets_of(s)
    if n >= #offsets - 1 then return s end
    return s:sub(1, offsets[n + 1] - 1)
end

--- Everything after the first `n` codepoints. JavaScript's slice(n).
function M.suffix(s, n)
    if n <= 0 then return s end
    local offsets = offsets_of(s)
    if n >= #offsets - 1 then return "" end
    return s:sub(offsets[n + 1])
end

--- `s` without its last codepoint. JavaScript's slice(0, -1).
function M.drop_last(s)
    if s == "" then return "" end
    local offsets = offsets_of(s)
    return s:sub(1, offsets[#offsets - 1] - 1)
end

--- The last codepoint of `s`, or nil when empty. JavaScript's s[s.length - 1],
--- give or take the surrogate the original would have handed back.
function M.last_codepoint(s)
    if s == "" then return nil end
    local offsets = offsets_of(s)
    return decode_at(s, offsets[#offsets - 1])
end

--------------------------------------------------------------------------------
-- Whitespace operations
--------------------------------------------------------------------------------

--- Iterate the codepoints of `s` as (byte_start, byte_after, codepoint).
local function each_codepoint(s)
    local offsets = offsets_of(s)
    local i = 0
    return function()
        i = i + 1
        if i >= #offsets then return nil end
        return offsets[i], offsets[i + 1], decode_at(s, offsets[i])
    end
end

M.each_codepoint = each_codepoint

--- JavaScript's String.prototype.trim.
function M.trim(s)
    local offsets = offsets_of(s)
    local first, last = 1, #offsets - 1

    while first <= last and M.is_space(decode_at(s, offsets[first])) do
        first = first + 1
    end
    while last >= first and M.is_space(decode_at(s, offsets[last])) do
        last = last - 1
    end

    if first > last then return "" end
    return s:sub(offsets[first], offsets[last + 1] - 1)
end

--- s.replace(/\s+$/, '') -- strip trailing whitespace only.
function M.rtrim(s)
    local offsets = offsets_of(s)
    local last = #offsets - 1

    while last >= 1 and M.is_space(decode_at(s, offsets[last])) do
        last = last - 1
    end

    if last < 1 then return "" end
    return s:sub(1, offsets[last + 1] - 1)
end

--- s.replace(/\s+/g, '') -- remove all whitespace.
function M.strip_all_space(s)
    local out = {}
    for from, to, cp in each_codepoint(s) do
        if not M.is_space(cp) then
            out[#out + 1] = s:sub(from, to - 1)
        end
    end
    return table.concat(out)
end

--- s.replace(/\s{2,}/g, ' ') -- collapse runs of two or more into one space.
--- A single whitespace character is left exactly as it was, which matters:
--- a lone tab or no-break space survives, a pair of them becomes one space.
function M.collapse_space_runs(s)
    local out = {}
    local run_start, run_count = nil, 0

    local function flush()
        if run_count == 0 then return end
        if run_count >= 2 then
            out[#out + 1] = " "
        else
            out[#out + 1] = run_start
        end
        run_start, run_count = nil, 0
    end

    for from, to, cp in each_codepoint(s) do
        local chunk = s:sub(from, to - 1)
        if M.is_space(cp) then
            if run_count == 0 then run_start = chunk end
            run_count = run_count + 1
        else
            flush()
            out[#out + 1] = chunk
        end
    end
    flush()

    return table.concat(out)
end

--- s.split(/\s+/) -- with JavaScript's edge behaviour intact.
---
--- A leading run of whitespace produces a leading "" and a trailing run
--- produces a trailing "". PDFKit divides justified space by (count - 1), so
--- those phantom elements are load-bearing: dropping them would make justified
--- lines too wide.
function M.split_space(s)
    if s == "" then
        return { "" } -- "".split(/\s+/) is [""], not []
    end

    local parts = {}
    local buf = {}
    local in_run = false

    for from, to, cp in each_codepoint(s) do
        if M.is_space(cp) then
            if not in_run then
                parts[#parts + 1] = table.concat(buf)
                buf = {}
                in_run = true
            end
        else
            in_run = false
            buf[#buf + 1] = s:sub(from, to - 1)
        end
    end

    parts[#parts + 1] = table.concat(buf)
    return parts
end

--- s.split('\n') -- a plain literal split, no regex, no whitespace class.
function M.split_lines(s)
    local parts = {}
    local start = 1
    while true do
        local i = s:find("\n", start, true)
        if not i then
            parts[#parts + 1] = s:sub(start)
            return parts
        end
        parts[#parts + 1] = s:sub(start, i - 1)
        start = i + 1
    end
end

--- s.replace(/\n/g, '') -- drop newlines, keep everything else.
function M.strip_newlines(s)
    return (s:gsub("\n", ""))
end

return M
