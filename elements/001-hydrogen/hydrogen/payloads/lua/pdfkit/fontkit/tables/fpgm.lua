-- fontkit/tables/fpgm.lua -- port of fontkit/src/tables/fpgm.js
--
-- The font program: instructions run once when the font is first used, almost
-- always to define the functions the individual glyph programs then call. Copied
-- through to a subset verbatim, for the same reason `cvt ` is.
--
-- Opaque bytes to the end of the table.

local M = {}

function M.decode(r, font, length)
    local finish = r.pos + length
    local instructions = {}
    while r.pos < finish do
        instructions[#instructions + 1] = r:uint8()
    end
    return { instructions = instructions }
end

function M.size(t)
    return #t.instructions
end

--- Copied through unchanged, like `cvt ` and `prep`.
function M.encode(w, t)
    w:array(t.instructions, w.uint8)
end

return M
