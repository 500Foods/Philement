-- image/jpeg.lua -- port of pdfkit/lib/image/jpeg.js
--
-- A JPEG goes into a PDF whole. PDF's DCTDecode filter is JPEG, so nothing is
-- decoded here: the file is scanned only far enough to learn its dimensions,
-- bit depth and colour space, and then the original bytes are written out as
-- the image stream. That is why this file is short and png.lua is not.
--
-- The scan walks the marker segments looking for a start-of-frame, and along
-- the way pulls the EXIF orientation out of APP1 so that a photo taken sideways
-- is placed the right way up.
--
-- CMYK JPEGs get a /Decode array that inverts every channel. Adobe writes them
-- inverted and marks the fact in an APP14 segment that PDF readers do not
-- consult, so the inversion has to be declared here instead.

local object = require("pdfkit.object")

local byte = string.byte

local JPEG = {}
JPEG.__index = JPEG

--- Start-of-frame markers. Any of them introduces the segment carrying the
--- dimensions; which one it is says how the image is coded, which PDF does not
--- need to know.
local MARKERS = {
    [0xFFC0] = true, [0xFFC1] = true, [0xFFC2] = true, [0xFFC3] = true,
    [0xFFC5] = true, [0xFFC6] = true, [0xFFC7] = true, [0xFFC8] = true,
    [0xFFC9] = true, [0xFFCA] = true, [0xFFCB] = true, [0xFFCC] = true,
    [0xFFCD] = true, [0xFFCE] = true, [0xFFCF] = true,
}

local COLOR_SPACE_MAP = {
    [1] = "DeviceGray",
    [3] = "DeviceRGB",
    [4] = "DeviceCMYK",
}

-- Byte access. Offsets are zero-based throughout, matching the original, and
-- converted at the point of the string index -- keeping the arithmetic
-- comparable to the source is worth one addition per read.
local function at(s, o) return byte(s, o + 1) end
local function u16be(s, o)
    local a, b = byte(s, o + 1, o + 2)
    if not b then return nil end
    return (a << 8) | b
end
local function u16le(s, o)
    local a, b = byte(s, o + 1, o + 2)
    if not b then return nil end
    return (b << 8) | a
end
local function u32be(s, o)
    local a, b, c, d = byte(s, o + 1, o + 4)
    if not d then return nil end
    return (a << 24) | (b << 16) | (c << 8) | d
end
local function u32le(s, o)
    local a, b, c, d = byte(s, o + 1, o + 4)
    if not d then return nil end
    return (d << 24) | (c << 16) | (b << 8) | a
end

--- Find the EXIF orientation tag, or nil.
---
--- The path is: APP1 segment, "Exif\0\0" header, a TIFF header that says which
--- byte order the rest uses, an offset to the first IFD, then a linear scan of
--- its entries for tag 0x0112. Every step is bounds-checked, because a photo
--- with a damaged EXIF block should still embed.
local function parse_exif_orientation(data)
    if not data or #data < 20 then return nil end

    local len = #data
    local pos = 2 -- skip the SOI marker

    while pos < len - 4 do
        -- Some encoders leave padding between segments; skip to the next 0xFF.
        while pos < len and at(data, pos) ~= 0xFF do
            pos = pos + 1
        end
        if pos >= len - 4 then return nil end

        local marker = u16be(data, pos)
        pos = pos + 2

        if marker == 0xFFDA then
            return nil -- start of scan: the entropy-coded data begins
        end

        -- Standalone markers carry no length field, so there is nothing to skip.
        local standalone = (marker >= 0xFFD0 and marker <= 0xFFD9) or marker == 0xFF01
        if not standalone then
            if pos + 2 > len then return nil end
            local segmentLength = u16be(data, pos)

            if marker == 0xFFE1 and pos + 8 <= len then
                local exifHeader = data:sub(pos + 3, pos + 8)
                if exifHeader == "Exif\0\0" then
                    local tiffStart = pos + 8
                    if tiffStart + 8 > len then return nil end

                    local byteOrder = data:sub(tiffStart + 1, tiffStart + 2)
                    local isLittleEndian = (byteOrder == "II")
                    if not isLittleEndian and byteOrder ~= "MM" then return nil end

                    local read16 = isLittleEndian and u16le or u16be
                    local read32 = isLittleEndian and u32le or u32be

                    -- 42, the TIFF magic number, confirms the byte order guess.
                    if read16(data, tiffStart + 2) ~= 42 then return nil end

                    local ifdPos = tiffStart + read32(data, tiffStart + 4)
                    if ifdPos + 2 > len then return nil end

                    local entryCount = read16(data, ifdPos)

                    -- Entries are a fixed 12 bytes each: tag, type, count, and
                    -- either the value or a pointer to it. A short value like
                    -- orientation is stored inline at offset 8.
                    for i = 0, entryCount - 1 do
                        local entryPos = ifdPos + 2 + i * 12
                        if entryPos + 12 > len then return nil end

                        if read16(data, entryPos) == 0x0112 then
                            local value = read16(data, entryPos + 8)
                            if value >= 1 and value <= 8 then return value end
                            return nil
                        end
                    end
                    return nil
                end
            end

            pos = pos + segmentLength
        end
    end

    return nil
end

JPEG.parse_exif_orientation = parse_exif_orientation

function JPEG.new(data, label)
    local self = setmetatable({}, JPEG)

    self.data = data
    self.label = label

    if u16be(data, 0) ~= 0xFFD8 then
        error("SOI not found in JPEG")
    end

    self.orientation = parse_exif_orientation(data) or 1

    local len = #data
    local pos = 2
    local marker

    while pos < len do
        while pos < len and at(data, pos) ~= 0xFF do
            pos = pos + 1
        end
        if pos >= len then break end

        marker = u16be(data, pos)
        pos = pos + 2
        if MARKERS[marker] then break end
        pos = pos + u16be(data, pos)
    end

    if not MARKERS[marker] then
        error("Invalid JPEG.")
    end

    pos = pos + 2 -- past the segment length

    self.bits = at(data, pos); pos = pos + 1
    self.height = u16be(data, pos); pos = pos + 2
    self.width = u16be(data, pos); pos = pos + 2

    local channels = at(data, pos)
    self.colorSpace = COLOR_SPACE_MAP[channels]

    self.obj = nil
    return self
end

function JPEG:embed(document)
    if self.obj then return end

    self.obj = document:ref(object.dict({
        { "Type", "XObject" },
        { "Subtype", "Image" },
        { "BitsPerComponent", self.bits },
        { "Width", self.width },
        { "Height", self.height },
        { "ColorSpace", self.colorSpace },
        { "Filter", "DCTDecode" },
    }))

    -- Swapping the min and max of every channel inverts the image, which is how
    -- an Adobe-inverted CMYK JPEG is made to look right. Spec section 4.8.4.
    if self.colorSpace == "DeviceCMYK" then
        self.obj.data.Decode = object.array({ 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0 })
    end

    self.obj:finish(self.data, true) -- raw: JPEG bytes go in untouched

    self.data = nil -- free memory
end

return JPEG
