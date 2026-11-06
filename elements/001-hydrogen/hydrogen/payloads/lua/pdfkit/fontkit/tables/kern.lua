-- fontkit/tables/kern.lua -- port of fontkit/src/tables/kern.js
--
-- The old kerning table: how much closer or further apart to set one glyph when
-- a particular other glyph follows it. AV, To, Ye -- pairs whose shapes leave a
-- visible gap at the nominal advance width.
--
-- There are two incompatible tables with this tag, and which one you have is not
-- announced anywhere but in the first two bytes. Microsoft's starts with a
-- uint16 version 0; Apple's starts with a uint32 version 0x00010000, whose high
-- half reads as 1. Everything after that differs -- header size, the width of
-- the length field, and the meaning of the coverage bits, which Apple numbers
-- from the other end. This is why the version is threaded down into every
-- subtable: a coverage flag cannot be read without knowing whose table it is.
--
-- Three subtable formats are defined. Format 0 is a sorted list of pairs and is
-- what essentially every font in the wild uses. Format 2 is a pair of class
-- tables indexing a two-dimensional array of values, stored as raw byte offsets
-- so that a lookup is two additions. Format 3 is the same idea packed smaller.
-- All three are ported; only format 0 has ever been seen here.
--
-- Superseded by GPOS, which every OpenType font has -- but plenty of fonts ship
-- both, and a font with only `kern` still has to kern.

local M = {}

--------------------------------------------------------------------------------
-- Subtable formats
--------------------------------------------------------------------------------

--- Format 0: a sorted array of {left, right, value} triples.
---
--- The three fields after nPairs are a precomputed binary search -- the largest
--- power of two below the entry count, its log, and the remainder. They are read
--- because the format has them, not because anything uses them: the search here
--- computes its own bounds.
local function decode_format0(r)
    local s = {}
    s.nPairs = r:uint16()
    s.searchRange = r:uint16()
    s.entrySelector = r:uint16()
    s.rangeShift = r:uint16()
    s.pairs = r:array(s.nPairs, function(r)
        return { left = r:uint16(), right = r:uint16(), value = r:int16() }
    end)
    return s
end

--- A class table: a first glyph, a count, and one byte offset per glyph.
---
--- The offsets are into the format 2 value array, already multiplied by the row
--- width for a left table and by two for a right table, so that a lookup adds
--- them and is done.
local function decode_class_table(r)
    local t = {}
    t.firstGlyph = r:uint16()
    t.nGlyphs = r:uint16()
    t.offsets = r:array(t.nGlyphs, r.uint16)

    -- Upstream writes `t.offsets.length && Math.max(...)`, which is the largest
    -- offset, or 0 for an empty table -- JavaScript's `&&` yielding the falsy
    -- left operand rather than a boolean. It bounds the value array below.
    t.max = 0
    for _, off in ipairs(t.offsets) do
        if off > t.max then t.max = off end
    end

    return t
end

--- Follow a uint16 offset relative to `parentStart` and decode what is there.
---
--- restructure's Pointer with `{type: 'parent'}`, which for these subtables
--- means relative to the start of the enclosing kern subtable *header*, not to
--- the start of the format-specific body. An offset of zero is its nullValue and
--- comes back as nothing -- unambiguous here, since offset zero is the header.
---
--- The cursor is left where the offset was read from, as Pointer does.
local function pointer(r, parentStart, decode)
    local offset = r:uint16()
    if offset == 0 then
        return nil
    end

    local pos = r.pos
    r:seek(parentStart + offset)
    local val = decode(r)
    r:seek(pos)
    return val
end

--- Format 2: a two-dimensional array of values indexed by glyph class.
---
--- The value array is lazy and its length is worked out backwards, since the
--- format does not store one: the largest left offset is the start of the last
--- row, so that offset over the row width is the row index, plus one for the row
--- itself, times the number of int16s in a row.
---
--- `off`, the array's own displacement from the subtable header, is kept because
--- the lookup needs it -- the left and right offsets are absolute within the
--- subtable and have to be brought back to array-relative before use.
local function decode_format2(r, parentStart)
    local s = {}
    s.rowWidth = r:uint16()
    s.leftTable = pointer(r, parentStart, decode_class_table)
    s.rightTable = pointer(r, parentStart, decode_class_table)
    s.array = pointer(r, parentStart, function(r)
        local a = {}
        a.off = r.pos - parentStart
        a.len = (((s.leftTable.max - a.off) / s.rowWidth) + 1) * (s.rowWidth / 2)
        a.values = r:lazy(a.len, 2, r.int16)
        return a
    end)
    return s
end

--- Format 3: classes in single bytes, values in a shared pool.
---
--- Same lookup as format 2 with the indirection made explicit -- a class pair
--- picks an index into kernIndex, which picks a value out of kernValue. Capped
--- at 256 of each by the byte-wide counts, which is why it is smaller and why
--- large fonts cannot use it.
local function decode_format3(r)
    local s = {}
    s.glyphCount = r:uint16()
    s.kernValueCount = r:uint8()
    s.leftClassCount = r:uint8()
    s.rightClassCount = r:uint8()
    s.flags = r:uint8()
    s.kernValue = r:array(s.kernValueCount, r.int16)
    s.leftClass = r:array(s.glyphCount, r.uint8)
    s.rightClass = r:array(s.glyphCount, r.uint8)
    s.kernIndex = r:array(s.leftClassCount * s.rightClassCount, r.uint8)
    return s
end

local function decode_subtable(r, format, parentStart)
    if format == 0 then
        return decode_format0(r)
    elseif format == 2 then
        return decode_format2(r, parentStart)
    elseif format == 3 then
        return decode_format3(r)
    end

    -- Formats 1 and 4 are Apple's state-machine kerning and are not read.
    -- Upstream's VersionedStruct would throw here too; a subtable of an unknown
    -- format cannot be skipped over, since its length is the only thing known
    -- about it and KernProcessor would still be handed it.
    error("Unknown kern subtable format " .. tostring(format))
end

--------------------------------------------------------------------------------
-- Subtable headers
--------------------------------------------------------------------------------

--- Expand a byte of coverage flags into named booleans, least significant first.
local function bitfield(byte, names)
    local out = {}
    for i, name in ipairs(names) do
        if name ~= false then
            out[name] = (byte & (1 << (i - 1))) ~= 0
        end
    end
    return out
end

-- Microsoft's coverage byte, bit 0 upwards.
local COVERAGE_V0 = {
    "horizontal",   -- set for horizontal kerning data, clear for vertical
    "minimum",      -- set if the values are minimum spacings, not adjustments
    "crossStream",  -- set if the kerning is perpendicular to the text flow
    "override",     -- set if this table replaces the running total
}

-- Apple's coverage byte. Same width, different bits, and the low five are
-- unused -- `false` rather than a name, since Lua's ipairs stops at a nil.
local COVERAGE_V1 = {
    false, false, false, false, false,
    "variation",    -- set if the table holds variation kerning values
    "crossStream",  -- set if the table holds cross-stream kerning values
    "vertical",     -- set if the table holds vertical kerning values
}

--- One subtable: a header, a body, and padding out to the declared length.
local function decode_table(r, version)
    local startOffset = r.pos
    local t = { version = version }

    if version == 0 then
        t.subVersion = r:uint16()
        t.length = r:uint16()
        t.format = r:uint8()
        t.coverage = bitfield(r:uint8(), COVERAGE_V0)
    elseif version == 1 then
        t.length = r:uint32()
        t.coverage = bitfield(r:uint8(), COVERAGE_V1)
        t.format = r:uint8()
        t.tupleIndex = r:uint16()
    else
        error("Unknown kern table version " .. tostring(version))
    end

    t.subtable = decode_subtable(r, t.format, startOffset)

    -- The trailing Reserved field, whose count is `length - _currentOffset`:
    -- whatever is left of the declared length once the body has been read. The
    -- length covers the header too, so this is a seek to the next subtable.
    -- Fonts do exist whose subtable length is wrong; seeking rather than
    -- skipping keeps them recoverable in the same places upstream does.
    r:seek(startOffset + t.length)

    return t
end

--------------------------------------------------------------------------------

function M.decode(r)
    local t = {}
    t.version = r:uint16()

    if t.version == 0 then
        t.nTables = r:uint16()
    elseif t.version == 1 then
        -- The low half of Apple's uint32 version number, which restructure reads
        -- as a Reserved field after taking the high half for the version.
        r:uint16()
        t.nTables = r:uint32()
    else
        error("Unknown kern table version " .. tostring(t.version))
    end

    t.tables = r:array(t.nTables, function(r)
        return decode_table(r, t.version)
    end)

    return t
end

return M
