-- fontkit/TrueTypeCollection.lua
--   -- port of fontkit/src/TrueTypeCollection.js
--
-- A .ttc file: several fonts in one, sharing whatever tables happen to be
-- identical between them. Chalkboard and Chalkboard Bold are one file on macOS
-- because they have the same `cmap` and the same hinting programs, and only the
-- outlines differ.
--
-- Almost nothing happens here. The header is a list of byte offsets, and each
-- offset is the start of an ordinary SFNT table directory -- which is exactly
-- what TTFFont already reads, and exactly why TTFFont remembers where its
-- directory began rather than assuming the file starts there. Opening a font out
-- of a collection is therefore: seek, and hand the same bytes to TTFFont.
--
-- The tables the fonts share are shared by *pointing at the same offsets*, so
-- two fonts out of one collection may return the identical bytes for `cmap`
-- while differing in `glyf`. Nothing here has to know that; each font's
-- directory says where its own tables are and the sharing falls out.

local reader = require("pdfkit.fontkit.reader")
local TTFFont = require("pdfkit.fontkit.TTFFont")

local TrueTypeCollection = {}
TrueTypeCollection.__index = TrueTypeCollection

function TrueTypeCollection.probe(buffer)
    return buffer:sub(1, 4) == "ttcf"
end

--- Decode the header.
---
--- Version 2 adds three fields describing a digital signature covering the whole
--- collection. They are read and never used -- as upstream -- because a PDF
--- carries a subset rather than the file, and the signature would not survive
--- subsetting even if anything checked it.
function TrueTypeCollection.new(stream)
    local self = setmetatable({}, TrueTypeCollection)

    self.type = "TTC"
    self.stream = stream

    if stream:tag() ~= "ttcf" then
        error("Not a TrueType collection")
    end

    local header = { version = stream:uint32() }
    header.numFonts = stream:uint32()
    header.offsets = {}
    for i = 1, header.numFonts do
        header.offsets[i] = stream:uint32()
    end

    if header.version == 0x00020000 then
        header.dsigTag = stream:uint32()
        header.dsigLength = stream:uint32()
        header.dsigOffset = stream:uint32()
    elseif header.version ~= 0x00010000 then
        -- A VersionedStruct upstream, which throws on a version it has no entry
        -- for rather than guessing at the layout.
        error("Unknown TTC version " .. string.format("0x%08x", header.version))
    end

    self.header = header
    return self
end

--- The font in this collection with the given PostScript name, or nil.
---
--- A linear scan that decodes each font's `name` table until one matches. That
--- is upstream's approach and it is fine: a collection holds a handful of fonts,
--- and the `name` table is small and lazily decoded, so the fonts that do not
--- match cost one table read each.
function TrueTypeCollection:getFont(name)
    for _, offset in ipairs(self.header.offsets) do
        local stream = reader.Reader.new(self.stream.data, offset)
        local font = TTFFont.new(stream)

        if font:postscriptName() == name then
            return font
        end
    end

    return nil
end

--- Every font in the collection, in the order the header lists them.
---
--- A getter upstream, a method here. Each call builds fresh TTFFont objects --
--- upstream does too -- so two calls give two sets of objects over the same
--- bytes, and any glyph caching on them is not shared.
function TrueTypeCollection:fonts()
    local fonts = {}

    for i, offset in ipairs(self.header.offsets) do
        fonts[i] = TTFFont.new(reader.Reader.new(self.stream.data, offset))
    end

    return fonts
end

return TrueTypeCollection
