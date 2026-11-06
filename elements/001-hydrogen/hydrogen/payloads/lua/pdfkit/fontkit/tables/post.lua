-- fontkit/tables/post.lua -- port of fontkit/src/tables/post.js
--
-- PostScript information: italic angle, underline placement, and -- in version 2
-- -- a name for every glyph. pdfkit reads isFixedPitch for the /FixedPitch
-- descriptor flag and italicAngle for /ItalicAngle; the names are here because
-- getGlyphName is part of the API being ported.
--
-- The version is a 16.16 fixed-point number, which is why 2.5 is a version. It
-- selects the trailing fields; the eight header fields are common to all of them.
-- An unrecognised version raises, matching upstream, where the caller catches it
-- and leaves the table undefined.

local encodings = require("pdfkit.fontkit.encodings")

local M = {}

function M.decode(r, font, length)
    local finish = r.pos + length
    local t = {}
    t.version = r:fixed32()

    -- Header, present in every version.
    t.italicAngle = r:fixed32()        -- counter-clockwise degrees from vertical
    t.underlinePosition = r:int16()    -- top of the underline, from the baseline
    t.underlineThickness = r:int16()
    t.isFixedPitch = r:uint32()        -- nonzero if monospaced
    t.minMemType42 = r:uint32()        -- memory use as a downloaded Type 42 font
    t.maxMemType42 = r:uint32()
    t.minMemType1 = r:uint32()         -- memory use as a downloaded Type 1 font
    t.maxMemType1 = r:uint32()

    if t.version == 1 or t.version == 3 then
        -- Version 1 is the standard Macintosh glyph order, version 3 is no names
        -- at all. Neither adds a field.
    elseif t.version == 2 then
        t.numberOfGlyphs = r:uint16()
        t.glyphNameIndex = r:array(t.numberOfGlyphs, r.uint16)

        -- Indices below 258 are the standard Macintosh order; the rest index
        -- into this list. It has no count -- it runs to the end of the table.
        local names = {}
        while r.pos < finish do
            local n = r:uint8()
            names[#names + 1] = (encodings.decode(r:bytes(n), "ascii"))
        end
        t.names = names
    elseif t.version == 2.5 then
        -- Deprecated: each glyph's name is the standard name of the glyph an
        -- offset away. The spec calls the offset signed; upstream reads it
        -- unsigned, so a backwards reference wraps to a large index and the
        -- name comes out nil. Kept as-is -- version 2.5 was withdrawn in 2000
        -- and there is no font to check a correction against.
        t.numberOfGlyphs = r:uint16()
        t.offsets = r:array(t.numberOfGlyphs, r.uint8)
    elseif t.version == 4 then
        -- Used by some Apple fonts: a character code per glyph.
        t.map = r:array(font.maxp.numGlyphs, r.uint32)
    else
        error("Unknown version " .. tostring(t.version))
    end

    return t
end

return M
