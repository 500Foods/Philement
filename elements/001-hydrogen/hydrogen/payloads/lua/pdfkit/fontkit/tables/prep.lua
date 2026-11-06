-- fontkit/tables/prep.lua -- port of fontkit/src/tables/prep.js
--
-- The control value program: instructions re-run whenever the point size or the
-- transformation changes, which is where the values in `cvt ` get scaled for the
-- current size. Like `fpgm`, opaque bytes that a subset copies through.

local M = {}

function M.decode(r, font, length)
    local finish = r.pos + length
    local program = {}
    while r.pos < finish do
        program[#program + 1] = r:uint8()
    end
    return { controlValueProgram = program }
end

function M.size(t)
    return #t.controlValueProgram
end

--- Copied through unchanged, like `cvt `.
function M.encode(w, t)
    w:array(t.controlValueProgram, w.uint8)
end

return M
