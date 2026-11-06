-- fontkit/writer.lua -- stands in for restructure's EncodeStream
--
-- The mirror of reader.lua. Only the subset writer uses it: fontkit reads about
-- forty tables and writes nine, so this is much the smaller half.
--
-- restructure encodes into a buffer it sized in advance, moving a cursor through
-- it. Nothing it writes ever goes backwards -- a struct lays down its fixed
-- fields, then the things its pointers refer to, in the order the pointers were
-- met -- so an append-only writer does the same job, and the cursor becomes a
-- running length that the directory uses to fill in its table offsets.
--
-- Integers are written the way a JavaScript DataView writes them: the value is
-- truncated toward zero and the low bits are kept. That is worth reproducing
-- rather than range-checking, because two values reach here that are not what a
-- Lua `string.pack` would accept -- the directory's entrySelector, which is a
-- logarithm and arrives as a float a hair above an integer, and loca offsets in
-- a font large enough for one to be written back out negative.

local M = {}

local Writer = {}
Writer.__index = Writer
M.Writer = Writer

--- An empty writer.
function Writer.new()
    return setmetatable({ parts = {}, pos = 0 }, Writer)
end

--- Everything written so far, as a string.
function Writer:tostring()
    -- Flatten as we go, so a writer that is asked for its bytes twice does not
    -- re-concatenate a thousand glyph buffers the second time.
    if #self.parts > 1 then
        self.parts = { table.concat(self.parts) }
    end
    return self.parts[1] or ""
end

--- ECMAScript ToInteger: truncate toward zero, and NaN becomes zero.
local function trunc(v)
    if math.type(v) == "integer" then
        return v
    end
    if v ~= v then         -- NaN, which compares unequal to itself
        return 0
    end
    return v >= 0 and math.floor(v) or math.ceil(v)
end

-- Signed and unsigned writes of the same width lay down the same bytes for the
-- same bit pattern, so each width is one function under two names. The mask is
-- what makes that true: it is DataView's modulo-2^n step, and it is also what
-- turns a negative int16 into the unsigned value `string.pack` will take.
local function packer(fmt, width, mask)
    return function(self, v)
        self.parts[#self.parts + 1] = string.pack(fmt, trunc(v) & mask)
        self.pos = self.pos + width
        return self
    end
end

Writer.uint8  = packer(">I1", 1, 0xFF)
Writer.uint16 = packer(">I2", 2, 0xFFFF)
Writer.uint24 = packer(">I3", 3, 0xFFFFFF)
Writer.uint32 = packer(">I4", 4, 0xFFFFFFFF)

Writer.int8  = Writer.uint8
Writer.int16 = Writer.uint16
Writer.int24 = Writer.uint24
Writer.int32 = Writer.uint32

--- Raw bytes, no encoding. Glyph outlines and the hinting programs go out here.
function Writer:bytes(s)
    self.parts[#self.parts + 1] = s
    self.pos = self.pos + #s
    return self
end

--- A four-byte tag. The sfnt version and every table tag are these.
---
--- Written as-is: restructure's ascii encoder keeps the low byte of each
--- character rather than rejecting anything above 127, and every tag that
--- reaches here came out of a font in the first place.
function Writer:tag(s)
    return self:bytes(s)
end

--- `n` copies of one byte. restructure's Reserved fields encode as this.
function Writer:fill(byte, n)
    if n > 0 then
        self.parts[#self.parts + 1] = string.rep(string.char(byte), n)
        self.pos = self.pos + n
    end
    return self
end

--- Write every element of a one-based Lua array with `fn`.
function Writer:array(list, fn)
    for i = 1, #list do
        fn(self, list[i])
    end
    return self
end

return M
