-- fontkit/tables/head.lua -- port of fontkit/src/tables/head.js
--
-- The font header. Two fields off this one matter downstream: unitsPerEm, which
-- every metric in the font is expressed in and which pdfkit divides by to get
-- 1000-unit PDF glyph space, and indexToLocFormat, which decides whether `loca`
-- holds 16- or 32-bit offsets.
--
-- macStyle.italic feeds the /Italic flag in the PDF font descriptor.

local M = {}

-- Bit positions in macStyle, low bit first. Bits 7..15 are reserved and dropped,
-- as upstream drops them.
local MAC_STYLE = {
    "bold", "italic", "underline", "outline",
    "shadow", "condensed", "extended",
}

function M.decode(r)
    local t = {}
    t.version = r:int32()            -- 0x00010000 for version 1.0
    t.revision = r:int32()           -- set by the font manufacturer
    t.checkSumAdjustment = r:uint32()
    t.magicNumber = r:uint32()       -- 0x5F0F3CF5
    t.flags = r:uint16()
    t.unitsPerEm = r:uint16()        -- 64 to 16384

    -- LONGDATETIME, as a pair of int32s rather than one 64-bit value. Upstream
    -- splits it the same way and never reassembles it.
    t.created = { r:int32(), r:int32() }
    t.modified = { r:int32(), r:int32() }

    -- Bounding box over every glyph in the font.
    t.xMin = r:int16()
    t.yMin = r:int16()
    t.xMax = r:int16()
    t.yMax = r:int16()

    local macStyle = r:uint16()
    t.macStyle = {}
    for i, name in ipairs(MAC_STYLE) do
        t.macStyle[name] = (macStyle & (1 << (i - 1))) ~= 0
    end

    t.lowestRecPPEM = r:uint16()     -- smallest readable size in pixels
    t.fontDirectionHint = r:int16()
    t.indexToLocFormat = r:int16()   -- 0 for short loca offsets, 1 for long
    t.glyphDataFormat = r:int16()    -- 0 for the current format
    return t
end

--- 54 bytes, always: every field is fixed width.
function M.size()
    return 54
end

--- Write the table back out.
---
--- The subset writer hands this a copy of the font's own head with
--- indexToLocFormat swapped for the one its loca actually uses. Everything else
--- goes out untouched, checkSumAdjustment included -- which is stale the moment
--- the file is rebuilt, and which fontkit does not recompute. A viewer that
--- validated it would reject every subset pdfkit has ever produced, so none do.
function M.encode(w, t)
    w:int32(t.version)
    w:int32(t.revision)
    w:uint32(t.checkSumAdjustment)
    w:uint32(t.magicNumber)
    w:uint16(t.flags)
    w:uint16(t.unitsPerEm)

    w:int32(t.created[1]); w:int32(t.created[2])
    w:int32(t.modified[1]); w:int32(t.modified[2])

    w:int16(t.xMin)
    w:int16(t.yMin)
    w:int16(t.xMax)
    w:int16(t.yMax)

    local macStyle = 0
    for i, name in ipairs(MAC_STYLE) do
        if t.macStyle[name] then
            macStyle = macStyle | (1 << (i - 1))
        end
    end
    w:uint16(macStyle)

    w:uint16(t.lowestRecPPEM)
    w:int16(t.fontDirectionHint)
    w:int16(t.indexToLocFormat)
    w:int16(t.glyphDataFormat)
end

return M
