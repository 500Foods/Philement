-- fontkit/tables/loca.lua -- port of fontkit/src/tables/loca.js
--
-- Where each glyph lives inside `glyf`. One offset per glyph plus a closing one,
-- so glyph i occupies offsets[i] up to offsets[i+1] and an empty range means an
-- empty glyph -- a space, typically.
--
-- head.indexToLocFormat picks the width. The short form stores offsets halved,
-- which caps `glyf` at 128KB and forces every glyph to start on an even byte;
-- upstream doubles them on the way in and halves them again on the way out, so
-- the rest of the library never sees the distinction. The `_processed` flag
-- guards against doubling twice, which matters because the subset writer hands
-- the same decoded table back to the encoder.
--
-- The count is not stored anywhere. Both forms read to the end of the table, per
-- the length in the directory.

local M = {}

function M.decode(r, font, length)
    local t = {}
    t.version = font.head.indexToLocFormat
    local finish = r.pos + length

    local offsets = {}
    if t.version == 0 then
        while r.pos + 2 <= finish do
            offsets[#offsets + 1] = r:uint16() << 1
        end
    else
        while r.pos + 4 <= finish do
            offsets[#offsets + 1] = r:uint32()
        end
    end

    t.offsets = offsets
    t._processed = (t.version == 0) or nil
    return t
end

--- Halve the offsets again, once, for a short-format table.
---
--- The inverse of the doubling in decode. Both `size` and `encode` call it, and
--- the directory calls `size` two or three times over while it lays the file
--- out, so it has to be safe to call repeatedly -- hence the flag rather than a
--- straight loop. The unsigned mask before the shift is what JavaScript's `>>>`
--- does to its operand, and is the difference between an offset past 2GB coming
--- out halved and coming out enormous.
local function preEncode(t)
    if t.version == 0 and t._processed ~= false then
        for i = 1, #t.offsets do
            t.offsets[i] = (t.offsets[i] & 0xFFFFFFFF) >> 1
        end
        t._processed = false
    end
end

--- Two or four bytes per offset, depending on the format.
function M.size(t)
    preEncode(t)
    return #t.offsets * (t.version == 0 and 2 or 4)
end

--- Write the offsets back out in the format `version` names.
---
--- The subset builds its own loca rather than copying the font's: one offset per
--- glyph it kept, in the order it kept them, plus the closing one.
function M.encode(w, t)
    preEncode(t)
    local write = t.version == 0 and w.uint16 or w.uint32
    for i = 1, #t.offsets do
        write(w, t.offsets[i])
    end
end

return M
