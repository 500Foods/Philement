-- fontkit/reader.lua -- stands in for the `restructure` package
--
-- fontkit does not read fonts by hand; it declares each table as a restructure
-- Struct and lets the library walk it. Those declarations are what the files
-- under tables/ are transcribed from, so this module only has to provide the
-- primitives they are built out of -- the big-endian integer readers, a couple
-- of string decoders, and a cursor to read them with.
--
-- Positions are zero-based throughout, matching restructure's `stream.pos` and
-- every offset a font file contains. Lua strings are one-based, so every read
-- adds one on the way to string.unpack and nowhere else. Getting that wrong is
-- an off-by-one in a table offset, which usually surfaces as garbage rather
-- than an error, so it is worth keeping in one place.

local M = {}

local Reader = {}
Reader.__index = Reader
M.Reader = Reader

--- A cursor over `data`, starting at zero-based `pos`.
function Reader.new(data, pos)
    return setmetatable({ data = data, pos = pos or 0 }, Reader)
end

function Reader:seek(pos)
    self.pos = pos
    return self
end

function Reader:skip(n)
    self.pos = self.pos + n
    return self
end

function Reader:eof()
    return self.pos >= #self.data
end

-- Each of these advances the cursor by the width it reads. `>` is big-endian,
-- which every SFNT field is.
local function reader(fmt, width)
    return function(self)
        local v = string.unpack(fmt, self.data, self.pos + 1)
        self.pos = self.pos + width
        return v
    end
end

Reader.uint8  = reader(">I1", 1)
Reader.int8   = reader(">i1", 1)
Reader.uint16 = reader(">I2", 2)
Reader.int16  = reader(">i2", 2)
Reader.uint24 = reader(">I3", 3)
Reader.uint32 = reader(">I4", 4)
Reader.int32  = reader(">i4", 4)

-- restructure's fixed32 is a signed 32-bit value with sixteen fraction bits --
-- the 16.16 format the `post` table's version and italic angle use.
function Reader:fixed32()
    return self:int32() / 65536
end

-- Raw bytes, no decoding. Table contents that are copied rather than parsed
-- (glyf, cvt, fpgm, prep) come through here.
function Reader:bytes(n)
    local s = self.data:sub(self.pos + 1, self.pos + n)
    self.pos = self.pos + n
    return s
end

--- Read `n` items with `fn`, as a one-based Lua array.
---
--- Note the indexing difference from the JS, which is worth keeping in mind when
--- reading the ports: fontkit's arrays are zero-based and are indexed by values
--- that come straight out of the font (segment numbers, glyph ids). Those all
--- get a +1 at the point of use rather than being renumbered here.
function Reader:array(n, fn)
    local out = {}
    for i = 1, n do
        out[i] = fn(self)
    end
    return out
end

--------------------------------------------------------------------------------
-- Lazy arrays
--------------------------------------------------------------------------------

-- restructure's LazyArray, which several cmap subtables are built out of. It
-- records where the array starts and how wide an element is, then decodes an
-- element only when someone asks for it.
--
-- Worth keeping rather than decoding eagerly, for two reasons. A format 4 cmap
-- with 5,000 segments is four arrays of 5,000 entries that a handful of lookups
-- will touch a dozen of. And a font whose declared subtable length runs past the
-- data -- which happens -- stays readable exactly as far as fontkit reads it,
-- instead of failing on an entry nobody wanted.
--
-- get() is zero-based and returns nil out of range, matching upstream, because
-- the cmap lookups lean on that: a code point past the end of the array comes
-- back as no-glyph rather than as an error.

local Lazy = {}
Lazy.__index = Lazy
M.Lazy = Lazy

function Lazy:get(i)
    if i < 0 or i >= self.length then
        return nil
    end
    local v = self.items[i]
    if v == nil then
        v = self.decode(Reader.new(self.data, self.base + self.size * i))
        self.items[i] = v
    end
    return v
end

--- One-based get.
---
--- Both accessors exist because the two kinds of index that reach a lazy array
--- are numbered differently, and conflating them is an off-by-one that reads a
--- neighbouring record rather than failing. `get` takes an index the font
--- supplied -- a segment number, a lookup list index -- which is zero-based
--- because the font file says so. `at` takes an index this port computed, such
--- as a position in a coverage table, which is one-based like every other Lua
--- array here.
function Lazy:at(i)
    return self:get(i - 1)
end

--- Every element, as a one-based Lua array.
function Lazy:toArray()
    local out = {}
    for i = 0, self.length - 1 do
        out[i + 1] = self:get(i)
    end
    return out
end

--- Read a lazy array of `length` elements, each `size` bytes wide.
---
--- The cursor jumps the whole span, as restructure does -- nothing is read here.
function Reader:lazy(length, size, decode)
    local a = setmetatable({
        data = self.data,
        base = self.pos,
        length = length,
        size = size,
        decode = decode,
        items = {},
    }, Lazy)
    self.pos = self.pos + length * size
    return a
end

--------------------------------------------------------------------------------
-- Strings
--------------------------------------------------------------------------------

--- A tag: four bytes, read as-is. Table tags and script/feature tags are these.
function Reader:tag()
    return self:bytes(4)
end

-- Encode one code point as UTF-8. Font strings arrive in UTF-16BE or a legacy
-- single-byte encoding and are handed on as UTF-8, which is what a Lua string
-- holding text is expected to be here.
local function utf8_encode(cp)
    if cp < 0x80 then
        return string.char(cp)
    elseif cp < 0x800 then
        return string.char(0xC0 | (cp >> 6), 0x80 | (cp & 0x3F))
    elseif cp < 0x10000 then
        return string.char(0xE0 | (cp >> 12), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F))
    else
        return string.char(
            0xF0 | (cp >> 18),
            0x80 | ((cp >> 12) & 0x3F),
            0x80 | ((cp >> 6) & 0x3F),
            0x80 | (cp & 0x3F)
        )
    end
end

M.utf8_encode = utf8_encode

--- Decode UTF-16BE, surrogate pairs included.
---
--- Malformed input follows what TextDecoder does in its non-fatal mode, which is
--- how restructure calls it: an unpaired surrogate of either half, and a stray
--- odd byte at the end, each become one U+FFFD rather than being dropped or
--- passed through. Fonts in the wild do contain truncated name strings, so this
--- is not hypothetical.
function M.decode_utf16be(s)
    local out = {}
    local i = 1
    local n = #s
    while i + 1 <= n do
        local unit = string.unpack(">I2", s, i)
        i = i + 2
        if unit >= 0xD800 and unit <= 0xDBFF then
            -- High half. Only a low half immediately after completes it.
            local low = (i + 1 <= n) and string.unpack(">I2", s, i) or nil
            if low and low >= 0xDC00 and low <= 0xDFFF then
                i = i + 2
                unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00)
            else
                unit = 0xFFFD
            end
        elseif unit >= 0xDC00 and unit <= 0xDFFF then
            -- Low half with nothing before it.
            unit = 0xFFFD
        end
        out[#out + 1] = utf8_encode(unit)
    end
    if i == n then
        -- Odd length: one byte left over.
        out[#out + 1] = utf8_encode(0xFFFD)
    end
    return table.concat(out)
end

--- Decode a single-byte encoding given its high half as a code point table.
---
--- `high` maps byte values 0x80..0xFF (as one-based indices 1..128) to code
--- points; the low half is ASCII in every encoding this is used for.
function M.decode_single_byte(s, high)
    local out = {}
    for i = 1, #s do
        local b = s:byte(i)
        if b < 0x80 then
            out[i] = string.char(b)
        else
            out[i] = utf8_encode(high[b - 0x7F] or b)
        end
    end
    return table.concat(out)
end

return M
