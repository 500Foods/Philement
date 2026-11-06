-- png_reader.lua -- port of the `png-js` npm package, which PDFKit uses
--
-- Reads the chunk structure of a PNG: dimensions and colour type from IHDR,
-- the palette from PLTE, transparency from tRNS, and the compressed pixel
-- stream from IDAT.
--
-- Most of the time the pixel stream is never decoded. PDF's FlateDecode filter
-- and PNG's IDAT compression are the same format, so an ordinary RGB or
-- greyscale PNG is embedded by handing the IDAT bytes over untouched. Decoding
-- is only needed when the pixels have to be taken apart: an alpha channel to
-- split out, a palette transparency table to turn into a soft mask, or an
-- interlaced image to de-interlace. That work is decode_pixels below.
--
-- THE ANIMATION CHUNKS ARE NOT HERE, and that is not an omission. The package
-- ships two builds -- a browser one that reads acTL/fcTL/fdAT, and a Node one
-- that does not. PDFKit imports the Node build, so an animated PNG embeds as
-- its first frame. This follows the same build.
--
-- Byte values are one-based Lua indices into a string throughout; `pos` is the
-- one-based offset of the next byte to read, where the original counts from
-- zero. Chunk sizes and offsets are unaffected -- only the indexing shifts.

local flate = require("pdfkit.flate")

local byte = string.byte
local char = string.char
local concat = table.concat
local unpack = table.unpack

local PNGReader = {}
PNGReader.__index = PNGReader

local SIGNATURE = "\137PNG\13\10\26\10"

--------------------------------------------------------------------------------
-- Chunk parsing
--------------------------------------------------------------------------------

function PNGReader.new(data)
    local self = setmetatable({}, PNGReader)

    self.data = data
    self.pos = 9 -- skip the 8-byte signature

    self.palette = {}
    self.transparency = {}
    self.text = {}

    if data:sub(1, 8) ~= SIGNATURE then
        error("pdfkit: not a PNG file")
    end

    -- IDAT may be split across any number of chunks; the pieces are collected
    -- here and joined once, because the compressed stream runs across the chunk
    -- boundaries and means nothing until it is whole again.
    local idat = {}

    while true do
        local chunkSize = self:readUInt32()
        local section = data:sub(self.pos, self.pos + 3)
        self.pos = self.pos + 4

        if section == "IHDR" then
            self.width = self:readUInt32()
            self.height = self:readUInt32()
            self.bits = self:readByte()
            self.colorType = self:readByte()
            self.compressionMethod = self:readByte()
            self.filterMethod = self:readByte()
            self.interlaceMethod = self:readByte()
        elseif section == "PLTE" then
            self.palette = self:read(chunkSize)
        elseif section == "IDAT" then
            idat[#idat + 1] = data:sub(self.pos, self.pos + chunkSize - 1)
            self.pos = self.pos + chunkSize
        elseif section == "tRNS" then
            -- Occurs at most once, after PLTE and before IDAT. What it means
            -- depends entirely on the colour type.
            self.transparency = {}
            if self.colorType == 3 then
                -- Indexed. One alpha byte per palette entry, truncated after
                -- the last non-opaque one -- so the tail has to be filled back
                -- in as fully opaque.
                local indexed = self:read(chunkSize)
                local short = 255 - #indexed
                for _ = 1, short do
                    indexed[#indexed + 1] = 255
                end
                self.transparency.indexed = indexed
            elseif self.colorType == 0 then
                -- Greyscale: one two-byte value, the shade that is transparent.
                -- Only the first byte is kept, matching the original.
                self.transparency.grayscale = self:read(chunkSize)[1]
            elseif self.colorType == 2 then
                -- Truecolour: an RGB triple that is transparent.
                self.transparency.rgb = self:read(chunkSize)
            end
        elseif section == "tEXt" then
            local text = self:read(chunkSize)
            local index = nil
            for i = 1, #text do
                if text[i] == 0 then index = i break end
            end
            index = index or (#text + 1)
            local key = char(unpack(text, 1, index - 1))
            local value = (index < #text) and char(unpack(text, index + 1, #text)) or ""
            self.text[key] = value
        elseif section == "IEND" then
            -- Everything needed is in hand; work out the derived values and
            -- stop. Anything after IEND is not ours to read.
            if self.colorType == 0 or self.colorType == 3 or self.colorType == 4 then
                self.colors = 1
            elseif self.colorType == 2 or self.colorType == 6 then
                self.colors = 3
            end

            self.hasAlphaChannel = (self.colorType == 4 or self.colorType == 6)
            local colors = self.colors + (self.hasAlphaChannel and 1 or 0)
            self.pixelBitlength = self.bits * colors

            if self.colors == 1 then
                self.colorSpace = "DeviceGray"
            elseif self.colors == 3 then
                self.colorSpace = "DeviceRGB"
            end

            self.imgData = concat(idat)
            return self
        else
            self.pos = self.pos + chunkSize -- unknown or uninteresting
        end

        self.pos = self.pos + 4 -- skip the CRC

        if self.pos > #data + 1 then
            error("pdfkit: incomplete or corrupt PNG file")
        end
    end
end

function PNGReader:readByte()
    local b = byte(self.data, self.pos)
    self.pos = self.pos + 1
    return b
end

--- The next `n` bytes as an array of numbers.
function PNGReader:read(n)
    local result = {}
    for i = 1, n do
        result[i] = byte(self.data, self.pos + i - 1)
    end
    self.pos = self.pos + n
    return result
end

function PNGReader:readUInt32()
    local a, b, c, d = byte(self.data, self.pos, self.pos + 3)
    self.pos = self.pos + 4
    return (a << 24) | (b << 16) | (c << 8) | d
end

function PNGReader:readUInt16()
    local a, b = byte(self.data, self.pos, self.pos + 1)
    self.pos = self.pos + 2
    return (a << 8) | b
end

--------------------------------------------------------------------------------
-- Pixel decoding
--------------------------------------------------------------------------------

--- Decompress and unfilter the pixel data. Returns a byte array of
--- width * height * (pixelBitlength / 8) values.
---
--- Two things happen here. Each scanline carries a filter byte saying how it
--- was predicted from its neighbours, and that prediction has to be undone --
--- filters are what make PNG compress, and they are applied per byte against
--- the pixel to the left and the scanline above. Then, if the image is
--- interlaced, seven separate reduced images have to be woven back into one.
---
--- The original is callback-style because Node's zlib.inflate is async. This
--- returns the array instead; the callers in image/png.lua use the result
--- directly rather than passing it on.
---
--- BE CAREFUL EDITING THIS. It is written against JavaScript semantics that a
--- straight transliteration loses, and two of them decide the output:
---
---   pixelBytes IS OFTEN A FRACTION. It is pixelBitlength / 8, and a 1-, 2- or
---   4-bit image has a pixelBitlength below 8 -- a 1-bit palette image gives
---   0.125. Rounding it to an integer, which is the obvious thing to do in a
---   language with an integer division operator, makes the scanline length zero
---   and the reader walks straight off into the pixel data. The fractional value
---   is load-bearing: `i < scanlineLength` with a scanlineLength of 36.25 runs
---   37 times, which is exactly the byte count of a 290-pixel 1-bit scanline.
---
---   THE PIXEL ARRAY IS A FIXED-SIZE Buffer. Its length is truncated -- a
---   1-bit 290x50 image asks for 1812.5 bytes and gets 1812 -- writes past the
---   end are dropped, and reads outside it, or at a fractional index, give
---   `undefined` rather than throwing. That `undefined` then flows through the
---   filter arithmetic as NaN and is stored back as 0.
---
--- Together those mean PDFKit decodes a sub-byte-depth image to something that
--- is not the image: the tail is missing, and any scanline using a filter other
--- than None comes out as zeros. Reproducing it is the point -- a port that
--- decoded these correctly would disagree with PDFKit on every such file. The
--- affected inputs are 1-, 2- and 4-bit PNGs that need decoding at all, which
--- means palette images carrying a tRNS chunk. Everything 8-bit and up is
--- decoded exactly.
function PNGReader:decode_pixels()
    local data = flate.inflate(self.imgData)

    local width, height = self.width, self.height

    -- Float division, deliberately: see the header above.
    local pixelBytes = self.pixelBitlength / 8
    local length = #data

    -- JavaScript's `undefined` where it reaches arithmetic. NaN behaves the same
    -- way in both languages -- it poisons every operation it touches and loses
    -- every comparison -- so the filter code below needs no special cases.
    local NaN = 0 / 0

    --- A stand-in for Buffer: fixed capacity, zero-filled, forgiving at the
    --- edges. Indices are zero-based like the original's, and the backing table
    --- is one-based, so every access shifts by one in `get`/`set` and nowhere
    --- else.
    local function new_buffer(size)
        -- Buffer.alloc truncates a fractional size rather than rejecting it.
        local capacity = math.floor(size)
        local t = {}
        for i = 1, capacity do t[i] = 0 end
        return { t = t, capacity = capacity }
    end

    local function buf_get(buf, i)
        -- Out of range, or not a whole number: `undefined` in JavaScript.
        if i < 0 or i >= buf.capacity or i % 1 ~= 0 then
            return NaN
        end
        return buf.t[i + 1]
    end

    local function buf_set(buf, i, v)
        if i < 0 or i >= buf.capacity or i % 1 ~= 0 then
            return -- silently dropped, as a Buffer drops it
        end
        -- A Buffer stores NaN as 0.
        buf.t[i + 1] = (v ~= v) and 0 or (math.floor(v) & 0xff)
    end

    -- The final, de-interlaced image.
    local pixels = new_buffer(width * height * pixelBytes)

    local pos = 1 -- one-based read cursor into the inflated data

    --- Unfilter one reduced image.
    ---
    --- (x0, y0) is where this pass starts and (dx, dy) how far apart its pixels
    --- sit. A non-interlaced image is the single pass (0, 0, 1, 1), and it
    --- writes straight into `pixels` rather than through a staging buffer.
    local function pass(x0, y0, dx, dy, singlePass)
        local w = math.ceil((width - x0) / dx)
        local h = math.ceil((height - y0) / dy)
        local scanlineLength = pixelBytes * w

        local buffer
        if singlePass then
            buffer = pixels
        else
            buffer = new_buffer(scanlineLength * h)
        end

        local row = 0
        local c = 0 -- zero-based write cursor into `buffer`

        --- One filtered byte, or NaN past the end of the data.
        local function next_byte()
            local b = byte(data, pos)
            pos = pos + 1
            return b or NaN
        end

        --- The byte directly above position i on the previous scanline.
        local function above(i, colOffset)
            local col = (i - (i % pixelBytes)) / pixelBytes + (colOffset or 0)
            return buf_get(buffer,
                (row - 1) * scanlineLength + col * pixelBytes + (i % pixelBytes))
        end

        while row < h and pos <= length do
            local filter = byte(data, pos)
            pos = pos + 1

            -- `i < scanlineLength` with a fractional bound, so a while loop
            -- rather than a numeric for: the count is ceil(scanlineLength).
            local i

            if filter == 0 then
                -- None.
                i = 0
                while i < scanlineLength do
                    buf_set(buffer, c, next_byte())
                    c = c + 1
                    i = i + 1
                end
            elseif filter == 1 then
                -- Sub: the byte pixelBytes to the left.
                i = 0
                while i < scanlineLength do
                    local b = next_byte()
                    local left = (i < pixelBytes) and 0 or buf_get(buffer, c - pixelBytes)
                    buf_set(buffer, c, (b + left) % 256)
                    c = c + 1
                    i = i + 1
                end
            elseif filter == 2 then
                -- Up: the byte directly above.
                i = 0
                while i < scanlineLength do
                    local b = next_byte()
                    -- `row && ...` upstream: on the first row this is the
                    -- number 0, not a lookup.
                    local upper = (row ~= 0) and above(i) or 0
                    buf_set(buffer, c, (upper + b) % 256)
                    c = c + 1
                    i = i + 1
                end
            elseif filter == 3 then
                -- Average of left and above, rounded down.
                i = 0
                while i < scanlineLength do
                    local b = next_byte()
                    local left = (i < pixelBytes) and 0 or buf_get(buffer, c - pixelBytes)
                    local upper = (row ~= 0) and above(i) or 0
                    buf_set(buffer, c, (b + math.floor((left + upper) / 2)) % 256)
                    c = c + 1
                    i = i + 1
                end
            elseif filter == 4 then
                -- Paeth: whichever of left, above and above-left is closest to
                -- their linear combination. It is the best general predictor
                -- PNG has, and the most expensive.
                i = 0
                while i < scanlineLength do
                    local b = next_byte()
                    local col = (i - (i % pixelBytes)) / pixelBytes
                    local left = (i < pixelBytes) and 0 or buf_get(buffer, c - pixelBytes)
                    local upper, upperLeft

                    if row == 0 then
                        upper, upperLeft = 0, 0
                    else
                        upper = above(i)
                        -- `col && ...`: column zero has nothing to its left.
                        upperLeft = (col ~= 0) and above(i, -1) or 0
                    end

                    local p = left + upper - upperLeft
                    local pa = math.abs(p - left)
                    local pb = math.abs(p - upper)
                    local pc = math.abs(p - upperLeft)

                    -- Every comparison is false when NaN is involved, in both
                    -- languages, so a poisoned neighbour falls through to
                    -- upperLeft and the result is stored as 0.
                    local paeth
                    if pa <= pb and pa <= pc then
                        paeth = left
                    elseif pb <= pc then
                        paeth = upper
                    else
                        paeth = upperLeft
                    end

                    buf_set(buffer, c, (b + paeth) % 256)
                    c = c + 1
                    i = i + 1
                end
            else
                error("pdfkit: invalid PNG filter algorithm: " .. tostring(filter))
            end

            if not singlePass then
                -- Scatter this pass's scanline into its real positions.
                local pixelsPos = ((y0 + row * dy) * width + x0) * pixelBytes
                local bufferPos = row * scanlineLength
                for _ = 1, w do
                    local j = 0
                    while j < pixelBytes do
                        buf_set(pixels, pixelsPos, buf_get(buffer, bufferPos))
                        pixelsPos = pixelsPos + 1
                        bufferPos = bufferPos + 1
                        j = j + 1
                    end
                    pixelsPos = pixelsPos + (dx - 1) * pixelBytes
                end
            end

            row = row + 1
        end
    end

    if self.interlaceMethod == 1 then
        -- Adam7. Seven passes over the image, each picking up the pixels marked
        -- with its number:
        --
        --   1 6 4 6 2 6 4 6
        --   7 7 7 7 7 7 7 7
        --   5 6 5 6 5 6 5 6
        --   7 7 7 7 7 7 7 7
        --   3 6 4 6 3 6 4 6
        --   7 7 7 7 7 7 7 7
        --   5 6 5 6 5 6 5 6
        --   7 7 7 7 7 7 7 7
        pass(0, 0, 8, 8)
        pass(4, 0, 8, 8)
        pass(0, 4, 4, 8)
        pass(2, 0, 4, 4)
        pass(0, 2, 2, 4)
        pass(1, 0, 2, 2)
        pass(0, 1, 1, 2)
    else
        pass(0, 0, 1, 1, true)
    end

    -- Hand back the plain array. It is zero-filled to its full capacity, so `#`
    -- gives the Buffer's length even where the decode left holes -- which for a
    -- sub-byte-depth image it will have.
    return pixels.t
end

--- Turn an array of byte values into a string.
function PNGReader.bytes_to_string(t, from, to)
    from = from or 1
    to = to or #t
    local chunks = {}
    for i = from, to, 4096 do
        local last = i + 4095
        if last > to then last = to end
        chunks[#chunks + 1] = char(unpack(t, i, last))
    end
    return concat(chunks)
end

return PNGReader
