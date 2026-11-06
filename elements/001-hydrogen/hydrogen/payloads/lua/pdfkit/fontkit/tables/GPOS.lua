-- fontkit/tables/GPOS.lua -- port of fontkit/src/tables/GPOS.js
--
-- The Glyph Positioning table: rules for moving glyphs off their default
-- positions. Kerning lives here, and so does everything that puts an accent on
-- top of a letter or joins one Arabic glyph to the next.
--
-- Nine lookup types:
--
--   1  Single       shift one covered glyph
--   2  Pair         shift a pair -- kerning, by glyph or by class
--   3  Cursive      join glyphs entry-anchor to exit-anchor
--   4  MarkToBase   put a mark on a base glyph
--   5  MarkToLig    put a mark on one component of a ligature
--   6  MarkToMark   stack a mark on another mark
--   7  Contextual   run other lookups when a sequence appears
--   8  Chaining     the same, with context before and after
--   9  Extension    an indirection, for 32-bit offsets
--
-- Two structures do most of the work. A *value record* is a set of deltas -- x
-- and y placement, x and y advance -- of which only the ones a subtable says it
-- carries are actually stored, so its width is not fixed and has to be computed
-- from a bitfield read earlier in the subtable. An *anchor* is a point in glyph
-- space; attaching a mark means lining up its anchor with the base's.

local ot = require("pdfkit.fontkit.tables.opentype")

local M = {}

--------------------------------------------------------------------------------
-- Value records
--------------------------------------------------------------------------------

-- Which fields a value record carries, low bit first. This is also the order
-- they are stored in, which is what makes a record decodable at all.
local VALUE_FORMAT = {
    "xPlacement", "yPlacement",
    "xAdvance", "yAdvance",
    "xPlaDevice", "yPlaDevice",
    "xAdvDevice", "yAdvDevice",
}

--- How wide a value record with this format is.
---
--- Every field is two bytes -- an int16 for the four deltas, a uint16 offset for
--- the four device tables -- so it is just the number of bits set. Needed
--- because value records sit inside lazy arrays, which have to know an element's
--- width before decoding one.
local function value_record_size(fmt)
    local size = 0
    for _, name in ipairs(VALUE_FORMAT) do
        if fmt[name] then
            size = size + 2
        end
    end
    return size
end

--- Decode one value record.
---
--- @param fmt  the decoded ValueFormat bitfield from the enclosing subtable
--- @param rel  the start of that subtable, which device offsets are measured
---             from -- not the start of this record, which is why it has to be
---             carried down rather than read from the cursor
---
--- A field the format does not mention is simply absent from the result, which
--- is what the processor's `value.xAdvance ~= nil` tests are reading.
local function decode_value_record(r, fmt, rel)
    local v = {}

    if fmt.xPlacement then v.xPlacement = r:int16() end
    if fmt.yPlacement then v.yPlacement = r:int16() end
    if fmt.xAdvance then v.xAdvance = r:int16() end
    if fmt.yAdvance then v.yAdvance = r:int16() end

    -- Decoded for completeness and never read: a device table only says
    -- anything to a variation processor, and this port has none.
    if fmt.xPlaDevice then v.xPlaDevice = ot.ptr16(r, rel, ot.decode_device) end
    if fmt.yPlaDevice then v.yPlaDevice = ot.ptr16(r, rel, ot.decode_device) end
    if fmt.xAdvDevice then v.xAdvDevice = ot.ptr16(r, rel, ot.decode_device) end
    if fmt.yAdvDevice then v.yAdvDevice = ot.ptr16(r, rel, ot.decode_device) end

    return v
end

--------------------------------------------------------------------------------
-- Anchors
--------------------------------------------------------------------------------

--- A point on a glyph, in font units.
---
--- Format 2 adds a contour point number, so a hinted outline can drag the anchor
--- with it; format 3 adds device tables. fontkit reads both and uses neither --
--- see the TODO in its getAnchor -- so all three formats come out as the same
--- pair of coordinates.
local function decode_anchor(r)
    local base = r.pos
    local a = { version = r:uint16() }

    if a.version == 1 then
        a.xCoordinate = r:int16()
        a.yCoordinate = r:int16()
    elseif a.version == 2 then
        a.xCoordinate = r:int16()
        a.yCoordinate = r:int16()
        a.anchorPoint = r:uint16()
    elseif a.version == 3 then
        a.xCoordinate = r:int16()
        a.yCoordinate = r:int16()
        a.xDeviceTable = ot.ptr16(r, base, ot.decode_device)
        a.yDeviceTable = ot.ptr16(r, base, ot.decode_device)
    else
        error("Unknown version " .. a.version)
    end

    return a
end

--- The marks a lookup handles: each one's attachment class and its anchor.
---
--- Anchor offsets here are measured from the start of the mark array, so this
--- captures its own base.
local function decode_mark_array(r)
    local base = r.pos
    local count = r:uint16()
    local marks = {}
    for i = 1, count do
        marks[i] = {
            class = r:uint16(),
            markAnchor = ot.ptr16(r, base, decode_anchor),
        }
    end
    return marks
end

--- One anchor per mark class, for one base glyph.
---
--- A base needs a separate attachment point for each class of mark that can go
--- on it -- one above for accents, one below for cedillas, and so on.
local function decode_base_array(r, classCount)
    local base = r.pos
    local count = r:uint16()
    local rows = {}
    for i = 1, count do
        local row = {}
        for j = 1, classCount do
            row[j] = ot.ptr16(r, base, decode_anchor)
        end
        rows[i] = row
    end
    return rows
end

--- The same, but a row per ligature component rather than per glyph.
---
--- A mark on a ligature has to know which of the letters it was written over, so
--- an `fi` ligature carries one set of anchors for the `f` and another for the
--- `i`. Offsets inside a LigatureAttach are relative to that attach table, and
--- offsets to the attach tables are relative to the ligature array.
local function decode_ligature_array(r, classCount)
    local arrayBase = r.pos
    local count = r:uint16()
    local ligatures = {}

    for i = 1, count do
        ligatures[i] = ot.ptr16(r, arrayBase, function(rr)
            local attachBase = rr.pos
            local componentCount = rr:uint16()
            local components = {}
            for j = 1, componentCount do
                local row = {}
                for k = 1, classCount do
                    row[k] = ot.ptr16(rr, attachBase, decode_anchor)
                end
                components[j] = row
            end
            return components
        end)
    end

    return ligatures
end

--------------------------------------------------------------------------------
-- The lookup types
--------------------------------------------------------------------------------

local decode_subtable

--- Type 1: shift every covered glyph, either all by one value or each by its
--- own.
local function decode_single(r)
    local base = r.pos
    local t = { version = r:uint16() }

    if t.version == 1 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.valueFormat = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.value = decode_value_record(r, t.valueFormat, base)
    elseif t.version == 2 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.valueFormat = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.valueCount = r:uint16()

        local size = value_record_size(t.valueFormat)
        t.values = r:lazy(t.valueCount, size, function(rr)
            return decode_value_record(rr, t.valueFormat, base)
        end)
    else
        error("Unknown version " .. t.version)
    end

    return t
end

--- Type 2: pair positioning, which is what kerning actually is.
---
--- Format 1 lists the pairs one by one, which suits a font with a few hundred
--- of them. Format 2 puts both glyphs into classes and stores a matrix indexed
--- by the two class numbers, which suits a font with tens of thousands -- most
--- text fonts, since kerning is largely a property of glyph shape rather than
--- of the specific letter.
local function decode_pair(r)
    local base = r.pos
    local t = { version = r:uint16() }

    if t.version == 1 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.valueFormat1 = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.valueFormat2 = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.pairSetCount = r:uint16()

        t.pairSets = r:lazy(t.pairSetCount, 2, function(rr)
            local offset = rr:uint16()
            if offset == 0 then
                return false
            end

            rr:seek(base + offset)
            local count = rr:uint16()
            local set = {}
            for i = 1, count do
                set[i] = {
                    secondGlyph = rr:uint16(),
                    value1 = decode_value_record(rr, t.valueFormat1, base),
                    value2 = decode_value_record(rr, t.valueFormat2, base),
                }
            end
            return set
        end)
    elseif t.version == 2 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.valueFormat1 = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.valueFormat2 = ot.bitfield(r:uint16(), VALUE_FORMAT)
        t.classDef1 = ot.ptr16(r, base, ot.decode_class_def)
        t.classDef2 = ot.ptr16(r, base, ot.decode_class_def)
        t.class1Count = r:uint16()
        t.class2Count = r:uint16()

        -- Lazy in both dimensions, and this is the one place in the port where
        -- that is not an optimisation but a requirement: a 100 x 100 class
        -- matrix is ten thousand value records, and a font can have several such
        -- subtables in lookups no text on the page will ever reach.
        local recordSize = value_record_size(t.valueFormat1) + value_record_size(t.valueFormat2)
        local rowSize = recordSize * t.class2Count

        t.classRecords = r:lazy(t.class1Count, rowSize, function(row)
            return row:lazy(t.class2Count, recordSize, function(rr)
                return {
                    value1 = decode_value_record(rr, t.valueFormat1, base),
                    value2 = decode_value_record(rr, t.valueFormat2, base),
                }
            end)
        end)
    else
        error("Unknown version " .. t.version)
    end

    return t
end

--- Type 3: cursive attachment, which joins glyphs the way a signature joins
--- letters -- each glyph's exit point lands on the next one's entry point.
local function decode_cursive(r)
    local base = r.pos
    local t = {}
    t.format = r:uint16()
    t.coverage = ot.ptr16(r, base, ot.decode_coverage)
    t.entryExitCount = r:uint16()
    t.entryExitRecords = {}
    for i = 1, t.entryExitCount do
        t.entryExitRecords[i] = {
            entryAnchor = ot.ptr16(r, base, decode_anchor),
            exitAnchor = ot.ptr16(r, base, decode_anchor),
        }
    end
    return t
end

--- Types 4 and 6: a set of marks, and a set of things to hang them on. The only
--- difference between attaching to a base and attaching to another mark is which
--- coverage tables are consulted, so the layout is identical and the field names
--- are all that change.
local function decode_mark_to_base(r, markField, baseField, markArrayField, baseArrayField)
    local base = r.pos
    local t = {}
    t.format = r:uint16()
    t[markField] = ot.ptr16(r, base, ot.decode_coverage)
    t[baseField] = ot.ptr16(r, base, ot.decode_coverage)
    t.classCount = r:uint16()
    t[markArrayField] = ot.ptr16(r, base, decode_mark_array)
    t[baseArrayField] = ot.ptr16(r, base, function(rr)
        return decode_base_array(rr, t.classCount)
    end)
    return t
end

--- Type 5: marks onto ligature components.
local function decode_mark_to_ligature(r)
    local base = r.pos
    local t = {}
    t.format = r:uint16()
    t.markCoverage = ot.ptr16(r, base, ot.decode_coverage)
    t.ligatureCoverage = ot.ptr16(r, base, ot.decode_coverage)
    t.classCount = r:uint16()
    t.markArray = ot.ptr16(r, base, decode_mark_array)
    t.ligatureArray = ot.ptr16(r, base, function(rr)
        return decode_ligature_array(rr, t.classCount)
    end)
    return t
end

--- Type 9: extension positioning. As in GSUB -- a type and a 32-bit offset.
local function decode_extension(r)
    local base = r.pos
    local t = {}
    t.posFormat = r:uint16()
    t.lookupType = r:uint16()
    t.extension = ot.ptr32(r, base, function(rr)
        return decode_subtable(rr, t.lookupType)
    end)
    return t
end

function decode_subtable(r, lookupType)
    if lookupType == 1 then
        return decode_single(r)
    elseif lookupType == 2 then
        return decode_pair(r)
    elseif lookupType == 3 then
        return decode_cursive(r)
    elseif lookupType == 4 then
        return decode_mark_to_base(r, "markCoverage", "baseCoverage", "markArray", "baseArray")
    elseif lookupType == 5 then
        return decode_mark_to_ligature(r)
    elseif lookupType == 6 then
        return decode_mark_to_base(r, "mark1Coverage", "mark2Coverage", "mark1Array", "mark2Array")
    elseif lookupType == 7 then
        return ot.decode_context(r)
    elseif lookupType == 8 then
        return ot.decode_chaining_context(r)
    elseif lookupType == 9 then
        return decode_extension(r)
    end

    error("Unknown version " .. lookupType)
end

M.decode_subtable = decode_subtable

--------------------------------------------------------------------------------
-- The table itself
--------------------------------------------------------------------------------

function M.decode(stream)
    return ot.decode_layout_header(stream, decode_subtable)
end

return M
