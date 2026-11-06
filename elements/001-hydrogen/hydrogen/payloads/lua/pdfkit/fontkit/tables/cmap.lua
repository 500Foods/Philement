-- fontkit/tables/cmap.lua -- port of fontkit/src/tables/cmap.js
--
-- Character code to glyph id. The table is a list of (platform, encoding) pairs
-- each pointing at a subtable, and the subtables come in nine formats that have
-- accumulated since 1991. A font usually ships two or three of them describing
-- the same mapping for different consumers; CmapProcessor picks one and ignores
-- the rest.
--
-- This file only decodes. The lookup logic for each format lives in
-- CmapProcessor.lua, which is how upstream splits it too.
--
-- Two things here are load-bearing rather than decoration:
--
-- Subtables are decoded on first touch. Upstream marks the pointer lazy, and
-- fonts do ship subtables that fail to decode -- format 10 cannot decode at all,
-- see below -- so a font with a broken subtable it never selects must still
-- work. The __index metamethod below is that laziness.
--
-- The arrays inside a subtable are lazy for the same reason plus a practical
-- one: a format 4 cmap for a large font is four parallel arrays of several
-- thousand entries each, of which a lookup touches about four.

local reader = require("pdfkit.fontkit.reader")

local Reader = reader.Reader

local M = {}

--------------------------------------------------------------------------------
-- Element decoders, with the fixed widths their lazy arrays need
--------------------------------------------------------------------------------

local function uint8(r) return r:uint8() end
local function uint16(r) return r:uint16() end
local function int16(r) return r:int16() end

-- Format 2's per-high-byte record.
local function sub_header(r)
    return {
        firstCode = r:uint16(),
        entryCount = r:uint16(),
        idDelta = r:int16(),
        idRangeOffset = r:uint16(),
    }
end

-- The run record shared by formats 8, 12 and 13. In 12 glyphID is the id of the
-- first character in the run and the rest follow it; in 13 every character in the
-- run maps to that one id.
local function cmap_group(r)
    return {
        startCharCode = r:uint32(),
        endCharCode = r:uint32(),
        glyphID = r:uint32(),
    }
end

local function unicode_value_range(r)
    return { startUnicodeValue = r:uint24(), additionalCount = r:uint8() }
end

local function uvs_mapping(r)
    return { unicodeValue = r:uint24(), glyphID = r:uint16() }
end

--- A count-prefixed array at `offset`, used by both halves of a variation record.
---
--- Eager, and one-based, unlike everything else in this file. Upstream declares
--- these two as plain arrays rather than lazy ones, and CmapProcessor indexes
--- them directly -- a lazy value there would read as nothing at all. The arrays
--- are short: one entry per variation sequence the selector covers.
local function counted_array(data, offset, decode)
    local r = Reader.new(data, offset)
    local count = r:uint32()
    local out = {}
    for i = 1, count do
        out[i] = decode(r)
    end
    return out
end

--------------------------------------------------------------------------------
-- Subtables
--------------------------------------------------------------------------------

--- Decode one subtable. `r` is positioned at its start.
local function decode_subtable(r)
    local start = r.pos
    local t = {}
    t.version = r:uint16() -- the format number, called `version` upstream

    if t.version == 0 then
        -- Byte encoding: 256 codes, one byte each.
        t.length = r:uint16()
        t.language = r:uint16()
        t.codeMap = r:lazy(256, 1, uint8)
    elseif t.version == 2 then
        -- High-byte mapping, for the pre-Unicode CJK encodings: the first byte
        -- selects a sub-header, which then interprets the second.
        t.length = r:uint16()
        t.language = r:uint16()
        t.subHeaderKeys = r:array(256, r.uint16)

        -- Upstream takes the largest key as the count. The spec says the keys
        -- are byte offsets and the count is max/8 + 1, so this reads more
        -- sub-headers than exist -- harmless, since they are lazy and only the
        -- ones a lookup names are ever decoded.
        local maxKey = t.subHeaderKeys[1]
        for i = 2, 256 do
            if t.subHeaderKeys[i] > maxKey then maxKey = t.subHeaderKeys[i] end
        end
        t.subHeaderCount = maxKey

        t.subHeaders = r:lazy(t.subHeaderCount, 8, sub_header)
        t.glyphIndexArray = r:lazy(t.subHeaderCount, 2, uint16)
    elseif t.version == 4 then
        -- Segment mapping to delta values -- the format nearly every font uses
        -- for the Basic Multilingual Plane.
        t.length = r:uint16()
        t.language = r:uint16()
        t.segCountX2 = r:uint16()
        t.segCount = t.segCountX2 >> 1
        t.searchRange = r:uint16()
        t.entrySelector = r:uint16()
        t.rangeShift = r:uint16()
        t.endCode = r:lazy(t.segCount, 2, uint16)
        r:skip(2) -- reservedPad, always zero
        t.startCode = r:lazy(t.segCount, 2, uint16)
        t.idDelta = r:lazy(t.segCount, 2, int16)
        t.idRangeOffset = r:lazy(t.segCount, 2, uint16)

        -- Whatever is left of the subtable is the glyph index array that
        -- idRangeOffset points into.
        t.glyphIndexArray = r:lazy((t.length - (r.pos - start)) // 2, 2, uint16)
    elseif t.version == 6 then
        -- Trimmed table: one contiguous run of codes.
        t.length = r:uint16()
        t.language = r:uint16()
        t.firstCode = r:uint16()
        t.entryCount = r:uint16()
        t.glyphIndices = r:lazy(t.entryCount, 2, uint16)
    elseif t.version == 8 then
        -- Mixed 16- and 32-bit coverage. The is32 bitmap says which lead bytes
        -- start a two-byte code.
        r:skip(2) -- reserved
        t.length = r:uint32()
        t.language = r:uint16() -- uint32 per the spec; upstream reads a uint16
        t.is32 = r:lazy(8192, 1, uint8)
        t.nGroups = r:uint32()
        t.groups = r:lazy(t.nGroups, 12, cmap_group)
    elseif t.version == 10 then
        -- Trimmed array. Upstream declares the glyph array's length as
        -- 'numChars', a field that does not exist in the struct, so restructure
        -- fails to resolve it and throws. Reproduced rather than repaired: the
        -- format is not used by any font in the wild, and quietly deciding what
        -- the length should be would be a guess with nothing to check it against.
        r:skip(2) -- reserved
        t.length = r:uint32()
        t.language = r:uint32()
        t.firstCode = r:uint32()
        t.entryCount = r:uint32()
        error("Not a fixed size")
    elseif t.version == 12 or t.version == 13 then
        -- Segmented coverage (12) and many-to-one ranges (13). Same layout; the
        -- difference is how glyphID is read, which is CmapProcessor's problem.
        r:skip(2) -- reserved
        t.length = r:uint32()
        t.language = r:uint32()
        t.nGroups = r:uint32()
        t.groups = r:lazy(t.nGroups, 12, cmap_group)
    elseif t.version == 14 then
        -- Unicode variation sequences: which glyph to use when a base character
        -- is followed by a variation selector.
        t.length = r:uint32()
        t.numRecords = r:uint32()

        local data = r.data
        t.varSelectors = r:lazy(t.numRecords, 11, function(vr)
            local rec = { varSelector = vr:uint24() }

            -- Both offsets are from the start of this subtable, and either may
            -- be zero, meaning absent.
            local defaultUVS = vr:uint32()
            local nonDefaultUVS = vr:uint32()

            if defaultUVS ~= 0 then
                rec.defaultUVS = counted_array(data, start + defaultUVS, unicode_value_range)
            end
            if nonDefaultUVS ~= 0 then
                rec.nonDefaultUVS = counted_array(data, start + nonDefaultUVS, uvs_mapping)
            end

            return rec
        end)
    else
        error("Unknown version " .. tostring(t.version))
    end

    return t
end

M.decode_subtable = decode_subtable

--------------------------------------------------------------------------------
-- The table itself
--------------------------------------------------------------------------------

-- Decodes `entry.table` the first time it is read, and caches it. Anything else
-- reads as nil, which is what an absent field should do.
local ENTRY = {}
ENTRY.__index = function(entry, key)
    if key ~= "table" then
        return nil
    end
    local t = decode_subtable(Reader.new(entry._data, entry._offset))
    rawset(entry, "table", t)
    return t
end

function M.decode(r)
    local start = r.pos
    local t = {}
    t.version = r:uint16()
    t.numSubtables = r:uint16()

    local tables = {}
    for i = 1, t.numSubtables do
        local entry = {}
        entry.platformID = r:uint16()
        entry.encodingID = r:uint16()

        -- Offsets are from the start of the cmap table, not the file.
        entry._data = r.data
        entry._offset = start + r:uint32()

        tables[i] = setmetatable(entry, ENTRY)
    end

    t.tables = tables
    return t
end

return M
