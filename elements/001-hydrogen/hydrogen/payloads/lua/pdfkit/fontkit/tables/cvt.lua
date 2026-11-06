-- fontkit/tables/cvt.lua -- port of fontkit/src/tables/cvt.js
--
-- The control value table: numbers the hinting instructions index into, mostly
-- stem widths and alignment zones. Nothing here interprets them -- they are read
-- so the subset writer can copy them across unchanged, since dropping them would
-- leave the hinting programs referring to values that no longer exist.
--
-- The table has no count of its own; it is int16s to the end.

local M = {}

function M.decode(r, font, length)
    local finish = r.pos + length
    local values = {}
    while r.pos + 2 <= finish do
        values[#values + 1] = r:int16()
    end
    return { controlValues = values }
end

function M.size(t)
    return #t.controlValues * 2
end

--- Copied through unchanged: the subset writer hands back the font's own table.
function M.encode(w, t)
    w:array(t.controlValues, w.int16)
end

return M
