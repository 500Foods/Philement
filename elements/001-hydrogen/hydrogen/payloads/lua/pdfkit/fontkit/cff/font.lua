-- fontkit/cff/font.lua -- port of fontkit/src/cff/CFFFont.js plus the DICT
-- and INDEX machinery it decodes with (CFFTop, CFFDict, CFFIndex, CFFPrivateDict).
--
-- A CFF table is a header, four indexes (names, top dict, strings, global
-- subroutines), and the charstrings and private dict those offsets point at.
-- Subsetting does not reuse those bytes. It decodes them, keeps the charstrings
-- and subroutines the document used, and encodes a new CID-keyed CFF. The
-- encoder below writes that shape: the same operators, the same five-byte
-- pointer integers, and the same index offsets fontkit's restructure encoder
-- produces.

local Reader = require("pdfkit.fontkit.reader").Reader
local Writer = require("pdfkit.fontkit.writer").Writer
local Operand = require("pdfkit.fontkit.cff.operand")
local StandardStrings = require("pdfkit.fontkit.cff.standard_strings")

local M = {}

local function opid(bytes)
    if type(bytes) == "table" then
        return (bytes[1] << 8) | bytes[2]
    end
    return bytes
end

local function define(list)
    local by_id = {}
    for _, op in ipairs(list) do
        op.id = opid(op.key)
        by_id[op.id] = op
    end
    return list, by_id
end

-- kind: number, bool, array. Pointers are filled in by the decoder, not by kind.
local TOP_OPS, TOP_BY_ID = define({
    { key = { 12, 30 }, name = "ROS",               kind = "array",  default = nil },
    { key = 0,          name = "version",           kind = "number", default = nil },
    { key = 1,          name = "Notice",            kind = "number", default = nil },
    { key = { 12, 0 },  name = "Copyright",         kind = "number", default = nil },
    { key = 2,          name = "FullName",          kind = "number", default = nil },
    { key = 3,          name = "FamilyName",        kind = "number", default = nil },
    { key = 4,          name = "Weight",            kind = "number", default = nil },
    { key = { 12, 1 },  name = "isFixedPitch",      kind = "bool",   default = false },
    { key = { 12, 2 },  name = "ItalicAngle",       kind = "number", default = 0 },
    { key = { 12, 3 },  name = "UnderlinePosition", kind = "number", default = -100 },
    { key = { 12, 4 },  name = "UnderlineThickness", kind = "number", default = 50 },
    { key = { 12, 5 },  name = "PaintType",         kind = "number", default = 0 },
    { key = { 12, 6 },  name = "CharstringType",    kind = "number", default = 2 },
    { key = { 12, 7 },  name = "FontMatrix",        kind = "array",  default = { 0.001, 0, 0, 0.001, 0, 0 } },
    { key = 13,         name = "UniqueID",          kind = "number", default = nil },
    { key = 5,          name = "FontBBox",          kind = "array",  default = { 0, 0, 0, 0 } },
    { key = { 12, 8 },  name = "StrokeWidth",       kind = "number", default = 0 },
    { key = 14,         name = "XUID",              kind = "array",  default = nil },
    { key = 15,         name = "charset",           kind = "skip",   default = nil },
    { key = 16,         name = "Encoding",          kind = "skip",   default = nil },
    { key = 17,         name = "CharStrings",       kind = "charstrings", default = nil },
    { key = 18,         name = "Private",           kind = "private", default = nil },
    { key = { 12, 20 }, name = "SyntheticBase",     kind = "number", default = nil },
    { key = { 12, 21 }, name = "PostScript",        kind = "number", default = nil },
    { key = { 12, 22 }, name = "BaseFontName",      kind = "number", default = nil },
    { key = { 12, 23 }, name = "BaseFontBlend",     kind = "array",  default = nil },
    { key = { 12, 31 }, name = "CIDFontVersion",    kind = "number", default = 0 },
    { key = { 12, 32 }, name = "CIDFontRevision",   kind = "number", default = 0 },
    { key = { 12, 33 }, name = "CIDFontType",       kind = "number", default = 0 },
    { key = { 12, 34 }, name = "CIDCount",          kind = "number", default = 8720 },
    { key = { 12, 35 }, name = "UIDBase",           kind = "number", default = nil },
    { key = { 12, 37 }, name = "FDSelect",          kind = "fdselect", default = nil },
    { key = { 12, 36 }, name = "FDArray",           kind = "fdarray", default = nil },
    { key = { 12, 38 }, name = "FontName",          kind = "number", default = nil },
})

local PRIVATE_OPS, PRIVATE_BY_ID = define({
    { key = 6,          name = "BlueValues",        kind = "array",  default = nil },
    { key = 7,          name = "OtherBlues",        kind = "array",  default = nil },
    { key = 8,          name = "FamilyBlues",       kind = "array",  default = nil },
    { key = 9,          name = "FamilyOtherBlues",  kind = "array",  default = nil },
    { key = { 12, 9 },  name = "BlueScale",         kind = "number", default = 0.039625 },
    { key = { 12, 10 }, name = "BlueShift",         kind = "number", default = 7 },
    { key = { 12, 11 }, name = "BlueFuzz",          kind = "number", default = 1 },
    { key = 10,         name = "StdHW",             kind = "number", default = nil },
    { key = 11,         name = "StdVW",             kind = "number", default = nil },
    { key = { 12, 12 }, name = "StemSnapH",         kind = "array",  default = nil },
    { key = { 12, 13 }, name = "StemSnapV",         kind = "array",  default = nil },
    { key = { 12, 14 }, name = "ForceBold",         kind = "bool",   default = false },
    { key = { 12, 17 }, name = "LanguageGroup",     kind = "number", default = 0 },
    { key = { 12, 18 }, name = "ExpansionFactor",   kind = "number", default = 0.06 },
    { key = { 12, 19 }, name = "initialRandomSeed", kind = "number", default = 0 },
    { key = 20,         name = "defaultWidthX",     kind = "number", default = 0 },
    { key = 21,         name = "nominalWidthX",     kind = "number", default = 0 },
    { key = 22,         name = "vsindex",           kind = "number", default = 0 },
    { key = 19,         name = "Subrs",             kind = "subrs",  default = nil },
})

local FONT_OPS, FONT_BY_ID = define({
    { key = 18,         name = "Private",           kind = "private", default = nil },
    { key = { 12, 38 }, name = "FontName",          kind = "number", default = nil },
    { key = { 12, 7 },  name = "FontMatrix",        kind = "array",  default = { 0.001, 0, 0, 0.001, 0, 0 } },
    { key = { 12, 5 },  name = "PaintType",         kind = "number", default = 0 },
})

local function read_off(r, offSize)
    if offSize == 1 then return r:uint8() end
    if offSize == 2 then return r:uint16() end
    if offSize == 3 then return r:uint24() end
    if offSize == 4 then return r:uint32() end
    error("Bad offset size in CFFIndex: " .. tostring(offSize) .. " " .. r.pos)
end

--- An INDEX of raw blobs, as `{offset, length}` pairs. Offsets are from the
--- start of this CFF buffer. `decode_item`, when given, replaces each blob
--- with whatever it returns and still leaves the cursor after the INDEX.
local function read_index(r, decode_item)
    local count = r:uint16()
    if count == 0 then
        return {}
    end
    local offSize = r:uint8()
    local startPos = r.pos + ((count + 1) * offSize) - 1
    local start = read_off(r, offSize)
    local out = {}
    for i = 1, count do
        local finish = read_off(r, offSize)
        local offset = startPos + start
        local length = finish - start
        if decode_item then
            local saved = r.pos
            r:seek(offset)
            out[i] = decode_item(r, length)
            r:seek(saved)
        else
            out[i] = { offset = offset, length = length }
        end
        start = finish
    end
    r:seek(startPos + start)
    return out
end

local function at(r, pos, fn)
    local saved = r.pos
    r:seek(pos)
    local value = fn()
    r:seek(saved)
    return value
end

local decode_dict

local function apply_op(op, r, operands, fields, ctx)
    if op.kind == "number" then
        return operands[1]
    elseif op.kind == "bool" then
        return operands[1] ~= nil and operands[1] ~= 0
    elseif op.kind == "array" then
        local copy = {}
        for i = 1, #operands do
            copy[i] = operands[i]
        end
        return copy
    elseif op.kind == "skip" then
        return nil
    elseif op.kind == "charstrings" then
        return at(r, operands[1], function()
            return read_index(r, nil)
        end)
    elseif op.kind == "subrs" then
        return at(r, ctx.privateBase + operands[1], function()
            return read_index(r, nil)
        end)
    elseif op.kind == "private" then
        local length = operands[1]
        local offset = operands[2]
        return at(r, offset, function()
            return decode_dict(r, length, PRIVATE_OPS, PRIVATE_BY_ID, { privateBase = r.pos })
        end)
    elseif op.kind == "fdselect" then
        return at(r, operands[1], function()
            local version = r:uint8()
            if version == 0 then
                local n = fields.CharStrings and #fields.CharStrings or 0
                local fds = {}
                for i = 1, n do
                    fds[i] = r:uint8()
                end
                return { version = 0, fds = fds }
            elseif version == 3 then
                local n = r:uint16()
                local ranges = {}
                for i = 1, n do
                    ranges[i] = { first = r:uint16(), fd = r:uint8() }
                end
                return { version = 3, nRanges = n, ranges = ranges, sentinel = r:uint16() }
            elseif version == 4 then
                local n = r:uint32()
                local ranges = {}
                for i = 1, n do
                    ranges[i] = { first = r:uint32(), fd = r:uint16() }
                end
                return { version = 4, nRanges = n, ranges = ranges, sentinel = r:uint32() }
            else
                error("Unknown FDSelect version: " .. tostring(version))
            end
        end)
    elseif op.kind == "fdarray" then
        return at(r, operands[1], function()
            return read_index(r, function(rr, length)
                return decode_dict(rr, length, FONT_OPS, FONT_BY_ID, {})
            end)
        end)
    end
    error("unhandled CFF operator kind " .. tostring(op.kind))
end

decode_dict = function(r, length, ops, by_id, ctx)
    local fields = {}
    for _, op in ipairs(ops) do
        fields[op.name] = op.default
    end
    -- FontMatrix's default is a shared table. Copy it so a later write cannot
    -- change the default every font is compared against.
    if type(fields.FontMatrix) == "table" then
        local m = fields.FontMatrix
        fields.FontMatrix = { m[1], m[2], m[3], m[4], m[5], m[6] }
    end
    if type(fields.FontBBox) == "table" then
        local b = fields.FontBBox
        fields.FontBBox = { b[1], b[2], b[3], b[4] }
    end

    local endpos = r.pos + length
    local operands = {}
    while r.pos < endpos do
        local b = r:uint8()
        if b < 28 then
            if b == 12 then
                b = (b << 8) | r:uint8()
            end
            local op = by_id[b]
            if not op then
                error("Unknown operator " .. b)
            end
            local val = apply_op(op, r, operands, fields, ctx)
            if val ~= nil then
                fields[op.name] = val
            end
            operands = {}
        else
            operands[#operands + 1] = Operand.decode(r, b)
        end
    end
    return fields
end

local Font = {}
Font.__index = Font

function Font.decode(r)
    local self = setmetatable({}, Font)
    self.stream = r
    self.version = r:int16() / 256
    self.hdrSize = r:uint8()
    self.offSize = r:uint8()
    if self.version >= 2 then
        error("CFF2 is not supported by this port")
    end
    self.nameIndex = read_index(r, function(rr, length)
        return rr:bytes(length)
    end)
    self.topDictIndex = read_index(r, function(rr, length)
        return decode_dict(rr, length, TOP_OPS, TOP_BY_ID, {})
    end)
    self.stringIndex = read_index(r, function(rr, length)
        return rr:bytes(length)
    end)
    self.globalSubrIndex = read_index(r, nil)
    if #self.topDictIndex ~= 1 then
        error("Only a single font is allowed in CFF")
    end
    self.topDict = self.topDictIndex[1]
    self.isCIDFont = self.topDict.ROS ~= nil
    return self
end

function Font:string(sid)
    if self.version >= 2 then
        return nil
    end
    -- A missing SID is null. `null < 391` is true, but the index that follows
    -- is `standardStrings[null]`, and a null property key is the string "null",
    -- not 0. The lookup misses and the subsetter stores nothing for that name.
    -- SID 0 is a real index and still resolves to ".notdef".
    if type(sid) ~= "number" then
        return nil
    end
    if sid < #StandardStrings then
        return StandardStrings[sid + 1]
    end
    return self.stringIndex[sid - #StandardStrings + 1]
end

function Font:postscriptName()
    if self.version < 2 then
        return self.nameIndex[1]
    end
    return nil
end

function Font:getCharString(glyph)
    local str = self.topDict.CharStrings[glyph + 1]
    self.stream:seek(str.offset)
    return self.stream:bytes(str.length)
end

function Font:fdForGlyph(gid)
    local select = self.topDict.FDSelect
    if not select then
        return nil
    end
    if select.version == 0 then
        return select.fds[gid + 1]
    end
    if select.version == 3 or select.version == 4 then
        local ranges = select.ranges
        local low = 1
        local high = #ranges
        while low <= high do
            local mid = (low + high) // 2
            if gid < ranges[mid].first then
                high = mid - 1
            elseif mid < high and gid >= ranges[mid + 1].first then
                low = mid + 1
            else
                return ranges[mid].fd
            end
        end
    end
    error("Unknown FDSelect version: " .. tostring(select.version))
end

function Font:privateDictForGlyph(gid)
    if self.topDict.FDSelect then
        local fd = self:fdForGlyph(gid)
        local dict = self.topDict.FDArray[fd + 1]
        if dict then
            return dict.Private
        end
        return nil
    end
    if self.version < 2 then
        return self.topDict.Private
    end
    return self.topDict.FDArray[1].Private
end

--------------------------------------------------------------------------------
-- Encoding the subset
--------------------------------------------------------------------------------

local function same(a, b)
    if a == b then
        return true
    end
    if type(a) ~= "table" or type(b) ~= "table" then
        return false
    end
    if a.ptr or b.ptr or a.private or b.private then
        return false
    end
    if #a ~= #b then
        return false
    end
    for i = 1, #a do
        if not same(a[i], b[i]) then
            return false
        end
    end
    return true
end

local function write_key(w, id)
    if id > 255 then
        w:uint8(id >> 8)
        w:uint8(id & 255)
    else
        w:uint8(id)
    end
end

local function write_operands(w, op, value)
    if op.kind == "bool" then
        Operand.encode(w, value and 1 or 0)
    elseif op.kind == "array" then
        for i = 1, #value do
            Operand.encode(w, value[i])
        end
    elseif op.kind == "private" then
        Operand.encode(w, value.size)
        Operand.encode(w, { ptr = true, val = value.offset })
    elseif type(value) == "table" and value.ptr then
        Operand.encode(w, value)
    else
        Operand.encode(w, value)
    end
end

--- The DICT body. Pointer values must already be numbers (or `{ptr, val}`).
---
--- `charset` and `Encoding` are skipped on decode, because the subset always
--- writes its own. A `{ptr, val}` left in one of those fields is the subset's
--- offset and is written; anything else under a skip kind stays omitted.
function M.encode_dict(ops, values)
    local w = Writer.new()
    for _, op in ipairs(ops) do
        local value = values[op.name]
        local is_ptr = type(value) == "table" and value.ptr
        if value ~= nil and not same(value, op.default) and (op.kind ~= "skip" or is_ptr) then
            write_operands(w, op, value)
            write_key(w, op.id)
        end
    end
    return w:tostring()
end

function M.encode_index(items)
    local w = Writer.new()
    w:uint16(#items)
    if #items == 0 then
        return w:tostring()
    end
    local sizes = {}
    local total = 1
    for i, item in ipairs(items) do
        sizes[i] = #item
        total = total + #item
    end
    local offSize = 1
    if total > 0xFF then offSize = 2 end
    if total > 0xFFFF then offSize = 3 end
    if total > 0xFFFFFF then offSize = 4 end
    if total > 0xFFFFFFFF then
        error("Bad offset in CFFIndex")
    end
    w:uint8(offSize)
    local function write_off(v)
        if offSize == 1 then w:uint8(v)
        elseif offSize == 2 then w:uint16(v)
        elseif offSize == 3 then w:uint24(v)
        else w:uint32(v) end
    end
    local offset = 1
    write_off(offset)
    for i = 1, #items do
        offset = offset + sizes[i]
        write_off(offset)
    end
    for i = 1, #items do
        w:bytes(items[i])
    end
    return w:tostring()
end

function M.top_ops()
    return TOP_OPS
end

function M.private_ops()
    return PRIVATE_OPS
end

function M.font_ops()
    return FONT_OPS
end

function M.standard_string_count()
    return #StandardStrings
end

--- Open the CFF table `stream` is positioned at. `length` is the table length.
function M.decode(stream, _font, length)
    local bytes = stream:bytes(length)
    return Font.decode(Reader.new(bytes, 0))
end

M.Font = Font

return M
