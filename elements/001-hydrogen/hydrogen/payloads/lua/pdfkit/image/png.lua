-- image/png.lua -- port of pdfkit/lib/image/png.js
--
-- PNG and PDF happen to agree on how pixel data is compressed -- both use
-- DEFLATE -- and PDF's FlateDecode filter can even undo PNG's scanline
-- prediction on its own, via /DecodeParms. So the fast path here embeds the
-- IDAT bytes untouched and lets the PDF reader do the work.
--
-- Everything else in this file is the cases where that does not work:
--
--   colour types 4 and 6   alpha is interleaved with the colour, and PDF wants
--                          it as a separate greyscale /SMask image, so the
--                          pixels must be decoded and the channels separated
--   indexed transparency   an alpha table indexed by palette entry, which
--                          becomes an /SMask built pixel by pixel
--   interlaced             PDF has no notion of Adam7, so the image has to be
--                          de-interlaced and re-compressed
--
-- Each of those decodes the pixels and re-compresses them, and both halves are
-- exact: for every image in PDFKit's own test corpus this port produces the same
-- dictionaries, the same objects in the same order with the same numbers, and
-- stream contents that are byte-identical -- not merely equivalent. The
-- re-compression goes through zlib/deflate.lua, which reproduces the encoder
-- Node actually ships rather than choosing its own valid encoding.
--
-- Two behaviours here look like quirks and are load-bearing; both are documented
-- where they live rather than here:
--
--   png_reader.lua  decode_pixels reproduces upstream's Buffer arithmetic, which
--                   mangles sub-byte-depth images. Decoding them correctly would
--                   disagree with PDFKit.
--   document.lua    the three decode paths split in half: the inflate runs now,
--                   the rest runs later, because upstream's inflate is async and
--                   its callbacks come back in a specific, reproducible order.

local object = require("pdfkit.object")
local flate = require("pdfkit.flate")
local PNGReader = require("pdfkit.png_reader")

local bytes_to_string = PNGReader.bytes_to_string

local PNGImage = {}
PNGImage.__index = PNGImage

function PNGImage.new(data, label)
    local self = setmetatable({}, PNGImage)

    self.label = label
    self.image = PNGReader.new(data)
    self.width = self.image.width
    self.height = self.image.height
    self.imgData = self.image.imgData
    self.obj = nil

    return self
end

function PNGImage:embed(document)
    self.document = document
    if self.obj then return end

    local image = self.image
    local width, height = self.width, self.height

    local hasAlphaChannel = image.hasAlphaChannel
    local isInterlaced = (image.interlaceMethod == 1)

    local obj = document:ref(object.dict({
        { "Type", "XObject" },
        { "Subtype", "Image" },
        -- Splitting the alpha channel normalises 16-bit samples down to 8, so
        -- the declared depth follows what will actually be written.
        { "BitsPerComponent", hasAlphaChannel and 8 or image.bits },
        { "Width", width },
        { "Height", height },
        { "Filter", "FlateDecode" },
    }))
    self.obj = obj

    if not hasAlphaChannel then
        -- Predictor 15 tells the reader to undo PNG's per-scanline filtering,
        -- which is what makes passing IDAT through untouched work at all. An
        -- interlaced image has already been de-interlaced and unfiltered by the
        -- time it is written, so it asks for no prediction.
        local params = document:ref(object.dict({
            { "Predictor", isInterlaced and 1 or 15 },
            { "Colors", image.colors },
            { "BitsPerComponent", image.bits },
            { "Columns", width },
        }))

        obj.data.DecodeParms = params
        params:finish()
    end

    if #image.palette == 0 then
        obj.data.ColorSpace = image.colorSpace
    else
        -- An indexed image carries its palette as a separate stream, and the
        -- colour space names it: how many entries, and what they are entries of.
        local palette = document:ref()
        palette:finish(bytes_to_string(image.palette), true) -- raw: palette bytes go in untouched

        obj.data.ColorSpace = object.array({
            "Indexed",
            "DeviceRGB",
            #image.palette // 3 - 1,
            palette,
        })
    end

    -- For colour types 0, 2 and 3 transparency lives in its own chunk rather
    -- than in the pixels.
    if image.transparency.grayscale ~= nil then
        -- Colour key masking, spec section 4.8.5: a range of sample values that
        -- are to be treated as transparent. One component, so two entries.
        local val = image.transparency.grayscale
        obj.data.Mask = object.array({ val, val })
    elseif image.transparency.rgb then
        -- The same, with a range per component: min and max of each, which here
        -- are the same value repeated.
        local mask = {}
        for _, x in ipairs(image.transparency.rgb) do
            mask[#mask + 1] = x
            mask[#mask + 1] = x
        end
        obj.data.Mask = object.array(mask)
    elseif image.transparency.indexed then
        -- Per-palette-entry alpha. There is no PDF equivalent, so it becomes a
        -- greyscale soft mask built from the pixel data.
        return self:load_indexed_alpha_channel()
    elseif hasAlphaChannel then
        -- Colour types 4 and 6 interleave alpha with colour. PDF wants them
        -- apart, as an image and a separate soft mask.
        return self:split_alpha_channel()
    end

    if isInterlaced then
        return self:decode_data()
    end

    self:finalize()
end

function PNGImage:finalize()
    if self.alphaChannel then
        local sMask = self.document:ref(object.dict({
            { "Type", "XObject" },
            { "Subtype", "Image" },
            { "Height", self.height },
            { "Width", self.width },
            { "BitsPerComponent", 8 },
            { "Filter", "FlateDecode" },
            { "ColorSpace", "DeviceGray" },
            { "Decode", object.array({ 0, 1 }) },
        }))

        sMask:finish(self.alphaChannel, true) -- raw: alpha bytes go in untouched
        self.obj.data.SMask = sMask
    end

    self.obj:finish(self.imgData, true) -- raw: pixel bytes go in untouched

    -- free memory
    self.image = nil
    self.imgData = nil
end

--- Separate interleaved alpha (colour types 4 and 6) into image and mask.
---
--- Split in two, like the other two decode paths. Upstream, decodePixels starts
--- an asynchronous zlib.inflate and everything below the first line is its
--- callback -- so the decompression happens now, during doc.image(), and the
--- rest of the work happens once the document's synchronous part is written.
--- Splitting it the same way here keeps both halves in the right place, and
--- gives Document:_defer the inflated size it needs to order the callbacks
--- against any other image's. See Document:_drain_deferred.
function PNGImage:split_alpha_channel()
    local pixels = self.image:decode_pixels()
    self.document:_defer(function() self:_split_alpha_channel(pixels) end, #pixels)
end

function PNGImage:_split_alpha_channel(pixels)
    local colorCount = self.image.colors
    local pixelCount = self.width * self.height
    local imgData = {}
    local alphaChannel = {}

    local i, p, a = 1, 0, 0
    local len = #pixels

    -- 16-bit samples are cut down to 8 by keeping the high byte. PNG stores
    -- samples big-endian, so that is the one that arrives first.
    local skipByteCount = (self.image.bits == 16) and 1 or 0

    while i <= len do
        for _ = 1, colorCount do
            p = p + 1
            imgData[p] = pixels[i]
            i = i + 1 + skipByteCount
        end
        a = a + 1
        alphaChannel[a] = pixels[i]
        i = i + 1 + skipByteCount
    end

    -- Guard against a truncated final pixel leaving holes: the buffers are
    -- allocated by size in the original, so any tail not written is zero.
    for k = p + 1, pixelCount * colorCount do imgData[k] = 0 end
    for k = a + 1, pixelCount do alphaChannel[k] = 0 end

    self.imgData = flate.deflate(bytes_to_string(imgData, 1, pixelCount * colorCount))
    self.alphaChannel = flate.deflate(bytes_to_string(alphaChannel, 1, pixelCount))

    self:finalize()
end

--- Build a soft mask from an indexed image's per-palette-entry alpha table.
---
--- Split for the same reason as split_alpha_channel.
function PNGImage:load_indexed_alpha_channel()
    local pixels = self.image:decode_pixels()
    self.document:_defer(function() self:_load_indexed_alpha_channel(pixels) end, #pixels)
end

function PNGImage:_load_indexed_alpha_channel(pixels)
    local transparency = self.image.transparency.indexed
    local isInterlaced = (self.image.interlaceMethod == 1)

    local alphaChannel = {}
    for j = 1, #pixels do
        -- The pixel value is a palette index, and the table is zero-based.
        alphaChannel[j] = transparency[pixels[j] + 1] or 0
    end
    for k = #pixels + 1, self.width * self.height do
        alphaChannel[k] = 0
    end

    -- De-interlacing rewrote the pixels, so the compressed data on file no
    -- longer describes them.
    if isInterlaced then
        self.imgData = flate.deflate(bytes_to_string(pixels))
    end

    self.alphaChannel = flate.deflate(
        bytes_to_string(alphaChannel, 1, self.width * self.height)
    )
    self:finalize()
end

--- De-interlace and re-compress.
---
--- Split for the same reason as split_alpha_channel.
function PNGImage:decode_data()
    local pixels = self.image:decode_pixels()
    self.document:_defer(function() self:_decode_data(pixels) end, #pixels)
end

function PNGImage:_decode_data(pixels)
    self.imgData = flate.deflate(bytes_to_string(pixels))
    self:finalize()
end

return PNGImage
