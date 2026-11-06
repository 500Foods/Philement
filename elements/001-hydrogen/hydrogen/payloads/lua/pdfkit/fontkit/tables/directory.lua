-- fontkit/tables/directory.lua -- port of fontkit/src/tables/directory.js
--
-- The table of contents at the top of every SFNT file: a short header, then one
-- entry per table giving its tag, checksum, absolute offset and length.
--
-- searchRange, entrySelector and rangeShift are a precomputed binary search over
-- the entries. Nothing here uses them -- the entries go straight into a table
-- keyed by tag -- but they are read and kept because the subset writer has to
-- emit them again.

local writer = require("pdfkit.fontkit.writer")

local M = {}

function M.decode(r)
    local dir = {}
    dir.tag = r:tag()
    dir.numTables = r:uint16()
    dir.searchRange = r:uint16()
    dir.entrySelector = r:uint16()
    dir.rangeShift = r:uint16()

    -- Read as a list, then re-key by tag -- upstream does this in `process`.
    -- A font with two entries for the same tag keeps the last, as there.
    local tables = {}
    for _ = 1, dir.numTables do
        local entry = {}
        entry.tag = r:tag()
        entry.checkSum = r:uint32()
        entry.offset = r:uint32()
        entry.length = r:uint32()
        tables[entry.tag] = entry
    end

    dir.tables = tables
    return dir
end

--------------------------------------------------------------------------------
-- Writing
--------------------------------------------------------------------------------

--- Build a font file from a list of tables.
---
--- `dir.tables` is an ordered array of `{ tag = "head", value = <table> }`, and
--- the order is the order the entries and the table data go out in. Upstream
--- takes an object and walks it with `for..in`, which in JavaScript hands back
--- string keys in the order they were inserted -- so its directory order is the
--- order of the object literal in TTFSubset. Lua's `pairs` promises nothing, so
--- the caller states the order instead of it falling out of the language.
---
--- Entries with a nil value are dropped, which is how a font with no `cvt ` or
--- `fpgm` simply produces a shorter directory.
---
--- Two things a font validator would object to, both faithful:
---
---   * The sfnt version goes out as `true`, Apple's TrueType magic, rather than
---     the 0x00010000 that a Windows-flavoured font uses. Both are legal.
---   * Every checksum is written as zero, and head.checkSumAdjustment is copied
---     from the source font rather than recomputed. Nothing in the PDF pipeline
---     verifies either.
---
--- Returns the file as a string.
function M.toBuffer(dir)
    -- Required late: tables/init.lua pulls in every table module, and this file
    -- is one of them.
    local tables = require("pdfkit.fontkit.tables")

    local entries = {}
    for _, item in ipairs(dir.tables) do
        if item.value ~= nil then
            local type_ = tables[item.tag]
            entries[#entries + 1] = {
                tag = item.tag,
                type = type_,
                value = item.value,
                -- Sizing `loca` also halves its offsets, which is why this
                -- happens once, here, and not again per write.
                length = type_.size(item.value),
            }
        end
    end

    local numTables = #entries

    -- The binary search hints. A reader that trusted them would step through the
    -- entries in powers of two; in practice everything scans the whole list, and
    -- these are written because the format says to.
    local maxExponentFor2 = math.floor(math.log(numTables) / 0.6931471805599453)
    local maxPowerOf2 = 1 << maxExponentFor2      -- Math.pow(2, e), exactly

    local w = writer.Writer.new()
    w:tag("true")
    w:uint16(numTables)
    w:uint16(maxPowerOf2 * 16)                    -- searchRange
    w:uint16(maxExponentFor2)                     -- entrySelector, ie log2(maxPowerOf2)
    w:uint16(numTables * 16 - maxPowerOf2 * 16)   -- rangeShift

    -- Header plus one 16-byte entry each; the table data follows immediately,
    -- packed with no padding between tables.
    local offset = 12 + 16 * numTables
    for _, entry in ipairs(entries) do
        w:tag(entry.tag)
        w:uint32(0)                               -- checkSum
        w:uint32(offset)
        w:uint32(entry.length)
        offset = offset + entry.length
    end

    for _, entry in ipairs(entries) do
        entry.type.encode(w, entry.value)
    end

    return w:tostring()
end

return M
