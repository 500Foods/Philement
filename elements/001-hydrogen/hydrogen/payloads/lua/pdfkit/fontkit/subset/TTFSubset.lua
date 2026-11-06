-- fontkit/subset/TTFSubset.lua -- port of fontkit/src/subset/TTFSubset.js
--
-- Builds a new TrueType font out of the glyphs a document actually used. This is
-- what ends up embedded in the PDF, and it is why a document setting one word in
-- a 700KB CJK font does not carry 700KB.
--
-- The trick is that outlines are not re-encoded. Each glyph's bytes are copied
-- verbatim out of the source font's `glyf`, so the hinting, the point flags and
-- the padding all survive untouched. Only two things are rebuilt: `loca`, which
-- has to point at the new positions, and the glyph ids inside composite glyphs,
-- which have to name the components by their subset numbers.
--
-- Nine tables go out -- the ones the PDF spec requires of an embedded TrueType
-- font. Absent are `cmap`, `name`, `OS/2` and `post`: a PDF font is addressed by
-- glyph id through the encoding in the PDF itself, so nothing ever asks the
-- embedded font what character a glyph is for. That also means the result is not
-- a standalone font and will not install; it is only meaningful inside the PDF
-- that carries it.

local Subset = require("pdfkit.fontkit.subset.Subset")
local Directory = require("pdfkit.fontkit.tables.directory")
local TTFGlyphEncoder = require("pdfkit.fontkit.glyph.TTFGlyphEncoder")

local TTFSubset = setmetatable({}, { __index = Subset })
TTFSubset.__index = TTFSubset

--- Copy a decoded table so the subset's edits do not reach the source font.
---
--- Upstream calls the `clone` package on head, hhea and maxp before changing a
--- field in each. It matters: a document that embeds the same font twice -- two
--- subsets of one shared TTFFont -- would otherwise have the first subset's
--- numberOfMetrics written into the second.
---
--- Recursive because head holds three nested values (the two date pairs and the
--- macStyle flags); everything else in these three tables is a number.
local function clone(value)
    if type(value) ~= "table" then
        return value
    end

    local out = {}
    for k, v in pairs(value) do
        out[k] = clone(v)
    end
    return out
end

function TTFSubset.new(font)
    local self = Subset.init(setmetatable({}, TTFSubset), font)
    self.glyphEncoder = TTFGlyphEncoder.new()
    return self
end

--- Copy one glyph's outline into the subset and record its metrics.
---
--- Returns the subset id, though only the recursive call below reads it.
function TTFSubset:_addGlyph(gid)
    local glyph = self.font:getGlyph(gid)
    local glyf = glyph:_decode()

    -- Where the outline lives in the source font. The offsets array has one
    -- entry per glyph plus a closing one, so a glyph's bytes run from its own
    -- offset up to the next -- and an empty glyph, a space, has the two equal.
    local curOffset = self.font.loca.offsets[gid + 1]
    local nextOffset = self.font.loca.offsets[gid + 2]

    local stream = self.font:_getTableStream("glyf")
    stream:skip(curOffset)

    local buffer = stream:bytes(nextOffset - curOffset)

    -- A composite glyph names its parts by the source font's glyph ids, which
    -- the subset is about to renumber. Each has to be pulled in -- recursively,
    -- since a component can itself be composite -- and its new id written back
    -- over the old one, in place, at the offset the decoder recorded.
    --
    -- Upstream patches the bytes through a DataView. Lua strings do not change,
    -- so the buffer is rebuilt around the two-byte holes instead. The components
    -- come out of the decoder in file order, so the pieces concatenate in order.
    if glyf and glyf.numberOfContours < 0 then
        local parts = {}
        local prev = 0

        for _, component in ipairs(glyf.components) do
            local subsetGid = self:includeGlyph(component.glyphID)
            parts[#parts + 1] = buffer:sub(prev + 1, component.pos)
            parts[#parts + 1] = string.pack(">I2", subsetGid & 0xFFFF)
            prev = component.pos + 2
        end

        parts[#parts + 1] = buffer:sub(prev + 1)
        buffer = table.concat(parts)

    elseif glyf and self.font._variationProcessor then
        -- A variable font's stored outline is the default instance; the points
        -- the document was laid out with are somewhere between the masters, so
        -- the outline has to be written back out from the moved points.
        --
        -- Unreachable until the variation processor is ported, at which point
        -- this is the line that makes a variable font embed at the right weight.
        buffer = self.glyphEncoder:encodeSimple(glyph:path(), glyf.instructions)
    end

    self.glyf[#self.glyf + 1] = buffer
    self.loca.offsets[#self.loca.offsets + 1] = self.offset

    self.hmtx.metrics[#self.hmtx.metrics + 1] = {
        advance = glyph:advanceWidth(),
        bearing = glyph:_getMetrics().leftBearing,
    }

    self.offset = self.offset + #buffer
    return #self.glyf - 1
end

--- Build the font file. Returns it as a string.
function TTFSubset:encode()
    -- Tables required by the PDF spec:
    --   head, hhea, loca, maxp, cvt , prep, glyf, hmtx, fpgm
    --
    -- Additional tables required for standalone fonts:
    --   name, cmap, OS/2, post

    self.glyf = {}
    self.offset = 0
    self.loca = {
        offsets = {},
        version = self.font.loca.version,
    }

    self.hmtx = {
        metrics = {},
        bearings = {},
    }

    -- Include all the glyphs. Not a `for` loop, because _addGlyph appends to
    -- self.glyphs when it meets a composite and the new entries have to be
    -- visited too -- the list grows under the iteration on purpose.
    local i = 1
    while i <= #self.glyphs do
        self:_addGlyph(self.glyphs[i])
        i = i + 1
    end

    local maxp = clone(self.font.maxp)
    maxp.numGlyphs = #self.glyf

    -- The closing offset, which is what gives the last glyph its length.
    self.loca.offsets[#self.loca.offsets + 1] = self.offset

    local head = clone(self.font.head)
    head.indexToLocFormat = self.loca.version

    local hhea = clone(self.font.hhea)
    hhea.numberOfMetrics = #self.hmtx.metrics

    -- The order below is the directory order, and it is the source font's
    -- declaration order rather than the alphabetical one the spec asks for.
    -- Kept as upstream has it: every PDF pdfkit has produced carries this order
    -- and every reader accepts it.
    --
    -- `cvt `, `prep` and `fpgm` go across whole and unedited -- they are hinting
    -- programs that index each other's tables, so subsetting them would mean
    -- interpreting them. A font without one simply omits the entry.
    return Directory.toBuffer({
        tables = {
            { tag = "head",  value = head },
            { tag = "hhea",  value = hhea },
            { tag = "loca",  value = self.loca },
            { tag = "maxp",  value = maxp },
            { tag = "cvt ",  value = self.font["cvt "] },
            { tag = "prep",  value = self.font.prep },
            { tag = "glyf",  value = self.glyf },
            { tag = "hmtx",  value = self.hmtx },
            { tag = "fpgm",  value = self.font.fpgm },
        },
    })
end

return TTFSubset
