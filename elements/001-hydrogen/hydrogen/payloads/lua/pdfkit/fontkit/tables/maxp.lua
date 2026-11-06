-- fontkit/tables/maxp.lua -- port of fontkit/src/tables/maxp.js
--
-- The maximum profile: numGlyphs, plus a list of ceilings a rasteriser would use
-- to size its buffers. Only numGlyphs is read anywhere else, but the subset
-- writer emits the table whole, so all of it is kept.
--
-- Version 0.5 -- which CFF-flavoured fonts use -- stops after numGlyphs. Upstream
-- reads the later fields anyway, off whatever bytes follow the table, and so does
-- this. They are garbage in that case and nothing looks at them.

local M = {}

function M.decode(r)
    local t = {}
    t.version = r:int32()
    t.numGlyphs = r:uint16()              -- glyph count
    t.maxPoints = r:uint16()              -- most points in a simple glyph
    t.maxContours = r:uint16()            -- most contours in a simple glyph
    t.maxComponentPoints = r:uint16()     -- most points in a composite glyph
    t.maxComponentContours = r:uint16()   -- most contours in a composite glyph
    t.maxZones = r:uint16()               -- 1 if the twilight zone is unused, else 2
    t.maxTwilightPoints = r:uint16()      -- most points used in Z0
    t.maxStorage = r:uint16()             -- storage area locations
    t.maxFunctionDefs = r:uint16()        -- FDEFs
    t.maxInstructionDefs = r:uint16()     -- IDEFs
    t.maxStackElements = r:uint16()       -- deepest interpreter stack
    t.maxSizeOfInstructions = r:uint16()  -- longest glyph instruction stream
    t.maxComponentElements = r:uint16()   -- most top-level components in a composite
    t.maxComponentDepth = r:uint16()      -- deepest composite nesting; 1 is flat
    return t
end

--- 32 bytes, always.
function M.size()
    return 32
end

--- Write the table back out.
---
--- The subset writer copies the font's own maxp with numGlyphs replaced. The
--- ceilings are therefore the whole font's, not the subset's -- too generous
--- rather than too small, which is the safe direction and saves walking every
--- glyph to recompute them.
function M.encode(w, t)
    w:int32(t.version)
    w:uint16(t.numGlyphs)
    w:uint16(t.maxPoints)
    w:uint16(t.maxContours)
    w:uint16(t.maxComponentPoints)
    w:uint16(t.maxComponentContours)
    w:uint16(t.maxZones)
    w:uint16(t.maxTwilightPoints)
    w:uint16(t.maxStorage)
    w:uint16(t.maxFunctionDefs)
    w:uint16(t.maxInstructionDefs)
    w:uint16(t.maxStackElements)
    w:uint16(t.maxSizeOfInstructions)
    w:uint16(t.maxComponentElements)
    w:uint16(t.maxComponentDepth)
end

return M
