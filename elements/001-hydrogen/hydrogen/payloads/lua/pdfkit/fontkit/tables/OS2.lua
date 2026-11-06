-- fontkit/tables/OS2.lua -- port of fontkit/src/tables/OS2.js
--
-- Metrics and classification that the OS asks for rather than the rasteriser.
-- Four fields reach the PDF: sFamilyClass, whose high byte pdfkit turns into the
-- /Serif and /Script descriptor flags, and xHeight, capHeight and the typo
-- metrics, which fontkit prefers over hhea when they are present.
--
-- Versions here are a plain uint16 and strictly additive. Version 0 is where the
-- Apple original ends; Microsoft appended the typo metrics in 1, the heights in
-- 2, and the optical size range in 5. Versions 3 and 4 add nothing over 2, which
-- upstream expresses by aliasing them onto version 2's field list.

local encodings = require("pdfkit.fontkit.encodings")

local M = {}

-- Embedding permissions. Bits 0, 4, 5, 6 and 7 are reserved; the gaps are real.
local FS_TYPE = {
    [2] = "noEmbedding", [3] = "viewOnly", [4] = "editable",
    [9] = "noSubsetting", [10] = "bitmapOnly",
}

local FS_SELECTION = {
    "italic", "underscore", "negative", "outlined", "strikeout",
    "bold", "regular", "useTypoMetrics", "wws", "oblique",
}

local function bitfield(val, names)
    local out = {}
    for i, name in pairs(names) do
        out[name] = (val & (1 << (i - 1))) ~= 0
    end
    return out
end

-- Versions upstream has a field list for. Anything else raises, and the caller
-- leaves the table undefined rather than guessing at the layout.
local KNOWN = { [0] = true, [1] = true, [2] = true, [3] = true, [4] = true, [5] = true }

function M.decode(r)
    local t = {}
    t.version = r:uint16()
    if not KNOWN[t.version] then
        error("Unknown version " .. tostring(t.version))
    end

    -- Header, present in every version.
    t.xAvgCharWidth = r:int16()        -- weighted mean advance of lowercase and space
    t.usWeightClass = r:uint16()       -- stroke weight, 100..900
    t.usWidthClass = r:uint16()        -- width relative to normal for the family
    t.fsType = bitfield(r:uint16(), FS_TYPE)   -- embedding licence
    t.ySubscriptXSize = r:int16()
    t.ySubscriptYSize = r:int16()
    t.ySubscriptXOffset = r:int16()
    t.ySubscriptYOffset = r:int16()
    t.ySuperscriptXSize = r:int16()
    t.ySuperscriptYSize = r:int16()
    t.ySuperscriptXOffset = r:int16()
    t.ySuperscriptYOffset = r:int16()
    t.yStrikeoutSize = r:int16()
    t.yStrikeoutPosition = r:int16()
    t.sFamilyClass = r:int16()         -- IBM family classification; class is the high byte
    t.panose = r:array(10, r.uint8)    -- ten-axis visual description
    t.ulCharRange = { r:uint32(), r:uint32(), r:uint32(), r:uint32() }
    t.vendorID = (encodings.decode(r:bytes(4), "ascii"))
    t.fsSelection = bitfield(r:uint16(), FS_SELECTION)
    t.usFirstCharIndex = r:uint16()    -- lowest Unicode index covered
    t.usLastCharIndex = r:uint16()     -- highest Unicode index covered

    if t.version == 0 then
        return t
    end

    -- Version 1 and up.
    t.typoAscender = r:int16()
    t.typoDescender = r:int16()
    t.typoLineGap = r:int16()
    t.winAscent = r:uint16()
    t.winDescent = r:uint16()
    t.codePageRange = { r:uint32(), r:uint32() }

    if t.version == 1 then
        return t
    end

    -- Version 2 and up, which is where 3 and 4 also stop.
    t.xHeight = r:int16()
    t.capHeight = r:int16()
    t.defaultChar = r:uint16()
    t.breakChar = r:uint16()
    t.maxContent = r:uint16()

    if t.version == 5 then
        t.usLowerOpticalPointSize = r:uint16()
        t.usUpperOpticalPointSize = r:uint16()
    end

    return t
end

return M
