-- fontkit/layout/GlyphRun.lua -- port of fontkit/src/layout/GlyphRun.js
--
-- What font:layout() hands back: a list of glyphs, a matching list of
-- positions, and the shaping parameters that produced them.
--
-- The two lists are parallel and stay that way. Substitution rewrites glyphs
-- before positions exist; positioning fills positions in and only ever adjusts
-- them in place. So `#glyphs == #positions` holds from the moment positioning
-- starts, and every consumer relies on it.

local BBox = require("pdfkit.fontkit.glyph.BBox")
local Script = require("pdfkit.fontkit.layout.Script")

local GlyphRun = {}
GlyphRun.__index = GlyphRun

--- @param glyphs    Glyph objects, in visual order once shaping is done
--- @param features  either an array of tags to switch on, or a map of tag to
---                  boolean; a map is the only way to switch one *off*
--- @param script    ISO 15924 tag, detected from the text when not given
--- @param language  language tag, or nil for the script's default
--- @param direction "ltr" or "rtl", or nil for the script's own direction
function GlyphRun.new(glyphs, features, script, language, direction)
    local self = setmetatable({
        glyphs = glyphs,
        positions = nil,        -- filled in by LayoutEngine:position
        script = script,
        language = language or nil,
        direction = direction or Script.direction(script),
        features = {},
    }, GlyphRun)

    -- Upstream accepts the features either way and normalises to an object.
    -- A sequence is a list of tags to enable; anything else is already the map.
    -- The two are told apart by the length operator, and the one case it cannot
    -- distinguish -- an empty table -- means the same thing either way.
    if type(features) == "table" then
        if #features > 0 then
            for _, tag in ipairs(features) do
                self.features[tag] = true
            end
        else
            self.features = features
        end
    end

    return self
end

--- The width of the whole run: every advance, added up.
---
--- A getter upstream, a method here. Note it sums the *positions*, not the
--- glyphs' own advance widths -- kerning and GPOS have had their say by now.
function GlyphRun:advanceWidth()
    local width = 0
    for _, position in ipairs(self.positions) do
        width = width + position.xAdvance
    end
    return width
end

function GlyphRun:advanceHeight()
    local height = 0
    for _, position in ipairs(self.positions) do
        height = height + position.yAdvance
    end
    return height
end

--- A box containing every glyph in the run, in run coordinates.
---
--- The pen walks along accumulating advances; each glyph's own box is offset by
--- where the pen has got to plus that glyph's positioning offset.
function GlyphRun:bbox()
    local bbox = BBox.new()
    local x, y = 0, 0

    for index = 1, #self.glyphs do
        local p = self.positions[index]
        local b = self.glyphs[index]:bbox()

        bbox:addPoint(b.minX + x + p.xOffset, b.minY + y + p.yOffset)
        bbox:addPoint(b.maxX + x + p.xOffset, b.maxY + y + p.yOffset)

        x = x + p.xAdvance
        y = y + p.yAdvance
    end

    return bbox
end

return GlyphRun
