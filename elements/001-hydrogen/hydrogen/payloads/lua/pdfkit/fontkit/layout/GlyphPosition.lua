-- fontkit/layout/GlyphPosition.lua -- port of fontkit/src/layout/GlyphPosition.js
--
-- Where one glyph sits and how far the pen moves afterwards. Four numbers, all
-- in font units:
--
--   xAdvance, yAdvance   how far to move the pen once this glyph is drawn
--   xOffset,  yOffset    where to draw it relative to the pen, without moving it
--
-- The distinction is what lets a combining accent sit over the letter before it
-- while contributing nothing to the width of the line: offsets place it, and
-- both advances stay at zero.
--
-- A run starts with every advance set from the glyph's own metrics and every
-- offset at zero; GPOS, the kern table and the Unicode fallback then adjust
-- these in place.

local GlyphPosition = {}
GlyphPosition.__index = GlyphPosition

function GlyphPosition.new(xAdvance, yAdvance, xOffset, yOffset)
    return setmetatable({
        xAdvance = xAdvance or 0,
        yAdvance = yAdvance or 0,
        xOffset = xOffset or 0,
        yOffset = yOffset or 0,
    }, GlyphPosition)
end

return GlyphPosition
