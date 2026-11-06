-- fontkit/tables/init.lua -- port of fontkit/src/tables/index.js
--
-- Tag to decoder. TTFFont walks the directory and, for every tag it finds a
-- decoder for here, exposes a lazily-decoded table under that name -- so this
-- registry is also the list of tables the port understands. A tag missing from
-- it is left as a directory entry with an offset and a length, which is all a
-- subset needs to copy it.
--
-- Upstream registers about forty tables. The ones here are those a TrueType font
-- needs to be measured, mapped and subsetted:
--
--   cmap head hhea hmtx maxp name OS/2 post   -- required in every SFNT font
--   loca cvt  fpgm prep glyf                  -- TrueType outlines and hinting
--   kern                                      -- pre-OpenType kerning
--   GSUB GPOS GDEF                            -- OpenType layout
--
-- `CFF ` is the PostScript outline table. CFF2, the bitmap and colour glyph
-- tables, the Apple layout tables (`morx`, `kerx`), and the variation tables
-- (`fvar`, `gvar`, `HVAR` and the rest) are absent.

local M = {
    -- Required tables
    cmap = require("pdfkit.fontkit.tables.cmap"),
    head = require("pdfkit.fontkit.tables.head"),
    hhea = require("pdfkit.fontkit.tables.hhea"),
    hmtx = require("pdfkit.fontkit.tables.hmtx"),
    maxp = require("pdfkit.fontkit.tables.maxp"),
    name = require("pdfkit.fontkit.tables.name"),
    ["OS/2"] = require("pdfkit.fontkit.tables.OS2"),
    post = require("pdfkit.fontkit.tables.post"),

    -- TrueType outlines
    loca = require("pdfkit.fontkit.tables.loca"),
    ["cvt "] = require("pdfkit.fontkit.tables.cvt"),
    fpgm = require("pdfkit.fontkit.tables.fpgm"),
    prep = require("pdfkit.fontkit.tables.prep"),

    -- Layout
    kern = require("pdfkit.fontkit.tables.kern"),

    -- OpenType layout. Registering these is what switches shaping on: the
    -- layout engine picks an OpenType engine when `font.GSUB` or `font.GPOS`
    -- is present, and a font with neither still lays out through the kern
    -- table as before.
    GSUB = require("pdfkit.fontkit.tables.GSUB"),
    GPOS = require("pdfkit.fontkit.tables.GPOS"),
    GDEF = require("pdfkit.fontkit.tables.GDEF"),

    -- Outlines. Write-only: the module has an `encode` and no `decode`, so the
    -- subset writer can name it in a directory while TTFFont leaves it alone.
    -- Reading a glyph goes through loca and glyph/TTFGlyph.lua, never through
    -- font.glyf; decoding it here would only slice most of the font into a list
    -- of buffers that nothing reads.
    glyf = require("pdfkit.fontkit.tables.glyf"),

    -- PostScript outlines. The decoder slices the table so its internal offsets
    -- start at zero, which is what the charstring interpreter and the subsetter
    -- both index by.
    ["CFF "] = require("pdfkit.fontkit.cff.font"),
}

return M
