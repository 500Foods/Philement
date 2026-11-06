-- fontkit/subset/Subset.lua -- port of fontkit/src/subset/Subset.js
--
-- The bookkeeping half of subsetting, shared by the TrueType and PostScript
-- writers: which glyphs of the source font are wanted, and what each is
-- renumbered to.
--
-- A subset renumbers because a PDF font is indexed by its own glyph ids, and
-- keeping the source font's would mean keeping its glyph count too -- a `loca`
-- with forty thousand entries for the two hundred glyphs the document uses. So
-- the first glyph asked for becomes glyph 1, the second glyph 2, and the caller
-- gets that number back to write into its content stream.
--
-- Glyph 0 is included by the constructor and so is always glyph 0 of the subset.
-- That is the .notdef box, and the format requires it to be first.

local Subset = {}
Subset.__index = Subset

--- Set up the two lists on `self`, which a subclass has already given its own
--- metatable. Split out from a constructor because Lua has no `super()`.
function Subset.init(self, font)
    self.font = font

    -- Subset glyph id (zero-based, as an index into this array minus one) to the
    -- source font's glyph id.
    self.glyphs = {}

    -- The inverse: source glyph id to subset glyph id.
    self.mapping = {}

    -- Always include the missing glyph.
    self:includeGlyph(0)

    return self
end

--- Add a glyph to the subset if it is not already in it, and return the id it
--- has in the subset.
---
--- Takes a glyph id or a Glyph object, as upstream does -- pdfkit passes objects
--- from its layout, and the composite-glyph walk inside TTFSubset passes ids.
---
--- Asking twice is free and gives the same answer, which is what makes it safe
--- to call from the middle of encoding: a composite glyph pulls in its
--- components, and two composites sharing an accent renumber it once.
function Subset:includeGlyph(glyph)
    if type(glyph) == "table" then
        glyph = glyph.id
    end

    if self.mapping[glyph] == nil then
        self.glyphs[#self.glyphs + 1] = glyph
        self.mapping[glyph] = #self.glyphs - 1
    end

    return self.mapping[glyph]
end

return Subset
