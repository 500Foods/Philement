-- fontkit/tables/hmtx.lua -- port of fontkit/src/tables/hmtx.js
--
-- Horizontal metrics. The table is two runs back to back: hhea.numberOfMetrics
-- {advance, bearing} pairs, then a bare left side bearing for every remaining
-- glyph. The tail exists so a font whose last few hundred glyphs all share one
-- advance width need not repeat it -- they take the advance of the final full
-- record. Glyph.advanceWidth does that fallback; this file only reads.
--
-- Both runs stay lazy. pdfkit asks for the advance of each glyph it actually
-- uses, which for a subset is a few dozen out of a few thousand.

local M = {}

local function hmtx_entry(r)
    return { advance = r:uint16(), bearing = r:int16() }
end

local function int16(r)
    return r:int16()
end

function M.decode(r, font)
    local t = {}
    t.metrics = r:lazy(font.hhea.numberOfMetrics, 4, hmtx_entry)
    t.bearings = r:lazy(font.maxp.numGlyphs - font.hhea.numberOfMetrics, 2, int16)
    return t
end

--- A lazy array as a plain one, and a plain one as itself.
---
--- The subset writer builds its metrics by hand, so both encode paths see real
--- arrays; a lazy one would only arrive if some caller re-emitted a font's own
--- table. restructure has the same two cases and resolves them the same way.
local function materialise(a)
    return a.toArray and a:toArray() or a
end

--- Four bytes per full metric, two per trailing bearing.
function M.size(t)
    return #materialise(t.metrics) * 4 + #materialise(t.bearings) * 2
end

--- Write the metrics, then the bare bearings.
---
--- A subset writes one full record per glyph and no bearings at all: the tail
--- only saves space when a long run of glyphs shares an advance, and a few dozen
--- subsetted glyphs never do.
function M.encode(w, t)
    for _, m in ipairs(materialise(t.metrics)) do
        w:uint16(m.advance)
        w:int16(m.bearing)
    end
    w:array(materialise(t.bearings), w.int16)
end

return M
