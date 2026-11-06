-- fontkit/subset/CFFSubset.lua -- port of fontkit/src/subset/CFFSubset.js
--
-- Builds the CFF that an embedded OpenType font carries. PDFKit does not copy
-- the source CFF table: it keeps the charstrings the document used, stubs every
-- subroutine they do not call with a single `return`, and writes a new
-- CID-keyed CFF around them. The bytes are what FontFile3 contains, so the
-- layout here is fontkit's restructure layout:
--
--   header, name index, top dict, string index, global subrs,
--   charset, charstrings, FDSelect, FDArray, then each private dict
--   with its local subrs straight after it.
--
-- Pointer operands are always five bytes, so the dicts can be measured with
-- dummy offsets and patched once the positions are known. The numbers do not
-- change the widths.

local Subset = require("pdfkit.fontkit.subset.Subset")
local CFF = require("pdfkit.fontkit.cff.font")
local Writer = require("pdfkit.fontkit.writer").Writer

local CFFSubset = setmetatable({}, { __index = Subset })
CFFSubset.__index = CFFSubset

local NAME_KEYS = {
    "version", "Notice", "Copyright", "FullName", "FamilyName",
    "Weight", "PostScript", "BaseFontName", "FontName",
}

local function shallow_copy(t)
    local out = {}
    for k, v in pairs(t) do
        out[k] = v
    end
    return out
end

function CFFSubset.new(font)
    local self = Subset.init(setmetatable({}, CFFSubset), font)
    self.cff = font["CFF "]
    if not self.cff then
        error("Not a CFF Font")
    end
    return self
end

--- Copy the subroutines a glyph actually called. The rest become `return`.
---
--- `used` is keyed by the subroutine's index in the source index, which is
--- zero-based, so this walks by count rather than with `ipairs` (that skips 0).
function CFFSubset:subsetSubrs(subrs, used)
    local res = {}
    for i = 1, #subrs do
        local index = i - 1
        if used[index] then
            local subr = subrs[i]
            self.cff.stream:seek(subr.offset)
            res[i] = self.cff.stream:bytes(subr.length)
        else
            res[i] = "\11"
        end
    end
    return res
end

function CFFSubset:subsetCharstrings()
    self.charstrings = {}
    local used = {}
    for _, gid in ipairs(self.glyphs) do
        self.charstrings[#self.charstrings + 1] = self.cff:getCharString(gid)
        local glyph = self.font:getGlyph(gid)
        glyph:path()
        for index in pairs(glyph._usedGsubrs) do
            used[index] = true
        end
    end
    self.gsubrs = self:subsetSubrs(self.cff.globalSubrIndex or {}, used)
end

--- One private dict and the subroutine index that sits immediately after it.
---
--- The Subrs operand is an offset from the start of this dict. With the index
--- written next, that offset is the dict's own length, and the length includes
--- the five-byte operand but not the index.
local function encode_private(dict)
    local subrs = dict.Subrs
    local values = shallow_copy(dict)
    if subrs ~= nil then
        values.Subrs = { ptr = true, val = 0 }
    end
    local body = CFF.encode_dict(CFF.private_ops(), values)
    if subrs ~= nil then
        values.Subrs = { ptr = true, val = #body }
        body = CFF.encode_dict(CFF.private_ops(), values)
    end
    local index = ""
    if subrs ~= nil then
        index = CFF.encode_index(subrs)
    end
    return body, index
end

local function encode_fontdict(dict, private_size, private_off)
    local values = shallow_copy(dict)
    values.FontName = nil
    if values.Private ~= nil then
        values.Private = { size = private_size, offset = private_off }
    end
    return CFF.encode_dict(CFF.font_ops(), values)
end

local function encode_charset(count)
    local w = Writer.new()
    if count > 255 then
        w:uint8(2)
        w:uint16(1)
        w:uint16(count - 2)
    else
        w:uint8(1)
        w:uint16(1)
        w:uint8(count - 2)
    end
    return w:tostring()
end

local function encode_fdselect_v3(sentinel)
    local w = Writer.new()
    w:uint8(3)
    w:uint16(1)
    w:uint16(0)
    w:uint8(0)
    w:uint16(sentinel)
    return w:tostring()
end

local function encode_fdselect_v0(fds)
    local w = Writer.new()
    w:uint8(0)
    for i = 1, #fds do
        w:uint8(fds[i])
    end
    return w:tostring()
end

--- A name-keyed font has one private dict. The subset pretends it is a CID
--- font with a single FD covering every glyph, which is what PDF readers
--- expect of a CIDFontType0C.
function CFFSubset:createCIDFontdict()
    local used = {}
    for _, gid in ipairs(self.glyphs) do
        local glyph = self.font:getGlyph(gid)
        glyph:path()
        for index in pairs(glyph._usedSubrs) do
            used[index] = true
        end
    end

    local src = self.cff.topDict.Private
    local privateDict = src and shallow_copy(src) or {}
    if src and src.Subrs then
        privateDict.Subrs = self:subsetSubrs(src.Subrs, used)
    end

    local body, subrs_index = encode_private(privateDict)
    return encode_fdselect_v3(#self.charstrings), {
        {
            source_dict = { Private = privateDict },
            body = body,
            subrs_index = subrs_index,
        },
    }
end

--- A CID font already has an FDArray. Keep the dicts the subset's glyphs use,
--- in the order they are first seen, and renumber FDSelect to match.
function CFFSubset:subsetFontdict()
    local seen = {}
    local used_subrs = {}
    local fd_of = {}
    local fds = {}
    local sources = {}

    for _, gid in ipairs(self.glyphs) do
        local fd = self.cff:fdForGlyph(gid)
        if fd ~= nil then
            if not seen[fd] then
                sources[#sources + 1] = shallow_copy(self.cff.topDict.FDArray[fd + 1])
                used_subrs[#used_subrs + 1] = {}
                fd_of[fd] = #sources - 1
                seen[fd] = true
            end
            fds[#fds + 1] = fd_of[fd]

            local glyph = self.font:getGlyph(gid)
            glyph:path()
            local bucket = used_subrs[fd_of[fd] + 1]
            for index in pairs(glyph._usedSubrs) do
                bucket[index] = true
            end
        end
    end

    local privates = {}
    for i, dict in ipairs(sources) do
        dict.FontName = nil
        if dict.Private and dict.Private.Subrs then
            local original = dict.Private.Subrs
            dict.Private = shallow_copy(dict.Private)
            dict.Private.Subrs = self:subsetSubrs(original, used_subrs[i])
        end
        local body, subrs_index = "", ""
        if dict.Private then
            body, subrs_index = encode_private(dict.Private)
        end
        privates[i] = {
            source_dict = dict,
            body = body,
            subrs_index = subrs_index,
        }
    end

    return encode_fdselect_v0(fds), privates
end

function CFFSubset:addString(s)
    if s == nil or s == "" then
        return nil
    end
    self.strings[#self.strings + 1] = s
    return CFF.standard_string_count() + #self.strings - 1
end

function CFFSubset:encode()
    self:subsetCharstrings()

    local count = #self.charstrings
    local charset = encode_charset(count)
    local charstrings_index = CFF.encode_index(self.charstrings)

    local top = shallow_copy(self.cff.topDict)
    top.Private = nil
    top.Encoding = nil

    self.strings = {}
    for _, key in ipairs(NAME_KEYS) do
        top[key] = self:addString(self.cff:string(top[key]))
    end
    top.ROS = { self:addString("Adobe"), self:addString("Identity"), 0 }
    top.CIDCount = count

    local fdselect, privates
    if self.cff.isCIDFont then
        fdselect, privates = self:subsetFontdict()
    else
        fdselect, privates = self:createCIDFontdict()
    end

    local function font_dicts_at(private_off)
        local dicts = {}
        local cursor = private_off
        for i, priv in ipairs(privates) do
            if priv.source_dict.Private ~= nil then
                dicts[i] = encode_fontdict(priv.source_dict, #priv.body, cursor)
                cursor = cursor + #priv.body + #priv.subrs_index
            else
                dicts[i] = encode_fontdict(priv.source_dict, 0, 0)
            end
        end
        return dicts
    end

    -- The FDArray index is measured with a dummy private offset. The offset is
    -- a five-byte integer either way, so the index does not change length when
    -- the real offset is filled in, and the private dicts start just after it.
    local fdarray_index = CFF.encode_index(font_dicts_at(0))

    local name = self.cff:postscriptName() or ""
    local name_index = CFF.encode_index({ name })
    local string_index = CFF.encode_index(self.strings)
    local gsubr_index = CFF.encode_index(self.gsubrs)

    local function top_bytes(off)
        local values = shallow_copy(top)
        values.charset = { ptr = true, val = off.charset }
        values.CharStrings = { ptr = true, val = off.charstrings }
        values.FDSelect = { ptr = true, val = off.fdselect }
        values.FDArray = { ptr = true, val = off.fdarray }
        values.Private = nil
        values.Encoding = nil
        return CFF.encode_dict(CFF.top_ops(), values)
    end

    local dummy_off = { charset = 0, charstrings = 0, fdselect = 0, fdarray = 0 }
    local top_index = CFF.encode_index({ top_bytes(dummy_off) })

    local base = 4 + #name_index + #top_index + #string_index + #gsubr_index
    local off = {
        charset = base,
        charstrings = base + #charset,
    }
    off.fdselect = off.charstrings + #charstrings_index
    off.fdarray = off.fdselect + #fdselect

    top_index = CFF.encode_index({ top_bytes(off) })
    fdarray_index = CFF.encode_index(font_dicts_at(off.fdarray + #fdarray_index))

    local w = Writer.new()
    w:int16(256)
    w:uint8(self.cff.hdrSize)
    w:uint8(4)
    w:bytes(name_index)
    w:bytes(top_index)
    w:bytes(string_index)
    w:bytes(gsubr_index)
    w:bytes(charset)
    w:bytes(charstrings_index)
    w:bytes(fdselect)
    w:bytes(fdarray_index)
    for _, priv in ipairs(privates) do
        if priv.source_dict.Private ~= nil then
            w:bytes(priv.body)
            w:bytes(priv.subrs_index)
        end
    end
    return w:tostring()
end

return CFFSubset
