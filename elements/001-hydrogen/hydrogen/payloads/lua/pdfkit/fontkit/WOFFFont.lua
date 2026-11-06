-- fontkit/WOFFFont.lua -- port of fontkit/src/WOFFFont.js
--
-- WOFF is a TrueType or OpenType font with its tables zlib-compressed for the
-- web. PDF does not store WOFF. Opening one inflates each table back to the
-- sfnt bytes and then uses the ordinary TrueType or CFF path, which is what
-- pdfkit does before it subsets.
--
-- WOFF2 is a different container (brotli, a transformed glyf) and is not
-- handled here.

local TTFFont = require("pdfkit.fontkit.TTFFont")
local Reader = require("pdfkit.fontkit.reader").Reader
local flate = require("pdfkit.flate")

local WOFFFont = {}

local woff_mt = {
    __index = function(self, key)
        local lazy = rawget(self, "_lazyTables")
        local entry = lazy and lazy[key]
        if entry then
            return TTFFont._getTable(self, entry)
        end
        if WOFFFont[key] ~= nil then
            return WOFFFont[key]
        end
        return TTFFont[key]
    end,
}

function WOFFFont.probe(buffer)
    return buffer:sub(1, 4) == "wOFF"
end

--- The WOFF directory, re-keyed by tag the way an sfnt directory is.
---
--- Offsets are absolute positions in the WOFF file. `length` is the
--- uncompressed size, which is what a table decoder asks to read; `compLength`
--- is how many bytes of the file that table occupies.
function WOFFFont:_decodeDirectory()
    local r = Reader.new(self.stream.data, self._directoryPos)
    local dir = {}
    dir.tag = r:tag()
    dir.flavor = r:uint32()
    dir.length = r:uint32()
    dir.numTables = r:uint16()
    r:uint16() -- reserved
    dir.totalSfntSize = r:uint32()
    dir.majorVersion = r:uint16()
    dir.minorVersion = r:uint16()
    dir.metaOffset = r:uint32()
    dir.metaLength = r:uint32()
    dir.metaOrigLength = r:uint32()
    dir.privOffset = r:uint32()
    dir.privLength = r:uint32()

    local tables = {}
    for _ = 1, dir.numTables do
        local entry = {}
        entry.tag = r:tag()
        entry.offset = r:uint32()
        entry.compLength = r:uint32()
        entry.length = r:uint32()
        entry.origChecksum = r:uint32()
        tables[entry.tag] = entry
    end
    dir.tables = tables
    self.directory = dir
    return dir
end

--- A reader over one table's sfnt bytes.
---
--- A compressed table is a zlib stream. fontkit skips the two-byte header and
--- inflates the rest with a raw inflater; the zlib wrapper here reads that
--- header itself and yields the same bytes. An uncompressed table is already
--- the sfnt table and is handed back in place.
function WOFFFont:_getTableStream(tag)
    local entry = self.directory.tables[tag]
    if not entry then
        return nil
    end

    if entry.compLength < entry.length then
        local raw = self.stream.data:sub(entry.offset + 1, entry.offset + entry.compLength)
        return Reader.new(flate.inflate(raw), 0)
    end

    return Reader.new(self.stream.data, entry.offset)
end

function WOFFFont.new(stream, variationCoords)
    local self = TTFFont.new(stream, variationCoords, woff_mt)
    self.type = "WOFF"
    return self
end

return WOFFFont
