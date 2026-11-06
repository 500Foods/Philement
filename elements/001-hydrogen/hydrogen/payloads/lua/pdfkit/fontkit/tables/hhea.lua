-- fontkit/tables/hhea.lua -- port of fontkit/src/tables/hhea.js
--
-- The horizontal header. ascent, descent and lineGap are the fallbacks fontkit
-- reports when OS/2 is missing or is the short Apple version, and numberOfMetrics
-- is what tells `hmtx` how many full metric records precede the run of bare left
-- side bearings at its end.

local M = {}

function M.decode(r)
    local t = {}
    t.version = r:int32()
    t.ascent = r:int16()               -- baseline to highest ascender
    t.descent = r:int16()              -- baseline to lowest descender (negative)
    t.lineGap = r:int16()              -- typographic line gap
    t.advanceWidthMax = r:uint16()     -- widest advance in `hmtx`
    t.minLeftSideBearing = r:int16()
    t.minRightSideBearing = r:int16()
    t.xMaxExtent = r:int16()
    t.caretSlopeRise = r:int16()       -- cursor slope is rise/run; 1/0 is vertical
    t.caretSlopeRun = r:int16()
    t.caretOffset = r:int16()          -- 0 for upright fonts
    r:skip(8)                          -- four reserved int16s
    t.metricDataFormat = r:int16()     -- 0 for the current format
    t.numberOfMetrics = r:uint16()     -- number of advance widths in `hmtx`
    return t
end

--- 36 bytes, always.
function M.size()
    return 36
end

--- Write the table back out, with the reserved run zeroed.
---
--- The decoder skips those eight bytes without keeping them, so whatever the
--- source font had there is lost; restructure's Reserved field encodes as zeros
--- and so does this. They are required to be zero anyway.
function M.encode(w, t)
    w:int32(t.version)
    w:int16(t.ascent)
    w:int16(t.descent)
    w:int16(t.lineGap)
    w:uint16(t.advanceWidthMax)
    w:int16(t.minLeftSideBearing)
    w:int16(t.minRightSideBearing)
    w:int16(t.xMaxExtent)
    w:int16(t.caretSlopeRise)
    w:int16(t.caretSlopeRun)
    w:int16(t.caretOffset)
    w:fill(0, 8)                       -- four reserved int16s
    w:int16(t.metricDataFormat)
    w:uint16(t.numberOfMetrics)
end

return M
