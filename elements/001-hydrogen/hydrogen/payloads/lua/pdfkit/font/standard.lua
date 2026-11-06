-- font/standard.lua -- port of pdfkit/lib/font/standard.js
--
-- The fourteen fonts every PDF reader is required to have. Because the reader
-- already has them, nothing is embedded: the font object is four keys naming
-- the font and its encoding, and all the width information stays on our side,
-- used only to work out where to put each glyph.
--
-- The metrics are generated Lua modules, the same numbers PDFKit 0.20.2 loads
-- from lib/font/generated. They are required by name so a rock install carries
-- them. A path beside this file would work in a checkout and disappear under
-- luarocks make, which installs the listed modules and nothing next to them.

local AFM = require("pdfkit.font.afm")
local Font = require("pdfkit.font")

local StandardFont = Font.subclass()

-- The fourteen names, as a set. Membership decides whether a string passed to
-- font() is a standard font name or a path to a font file on disk.
local STANDARD_FONTS = {
    ["Courier"] = true,
    ["Courier-Bold"] = true,
    ["Courier-Oblique"] = true,
    ["Courier-BoldOblique"] = true,
    ["Helvetica"] = true,
    ["Helvetica-Bold"] = true,
    ["Helvetica-Oblique"] = true,
    ["Helvetica-BoldOblique"] = true,
    ["Times-Roman"] = true,
    ["Times-Bold"] = true,
    ["Times-Italic"] = true,
    ["Times-BoldItalic"] = true,
    ["Symbol"] = true,
    ["ZapfDingbats"] = true,
}

StandardFont.STANDARD_FONTS = STANDARD_FONTS

function StandardFont.isStandardFont(name)
    return STANDARD_FONTS[name] == true
end

-- PostScript name to the generated module. The filenames are PDFKit's, which
-- drop the hyphen ("Courier-Bold" is CourierBold).
local METRIC_MODULES = {
    ["Courier"] = "pdfkit.font.generated.Courier",
    ["Courier-Bold"] = "pdfkit.font.generated.CourierBold",
    ["Courier-Oblique"] = "pdfkit.font.generated.CourierOblique",
    ["Courier-BoldOblique"] = "pdfkit.font.generated.CourierBoldOblique",
    ["Helvetica"] = "pdfkit.font.generated.Helvetica",
    ["Helvetica-Bold"] = "pdfkit.font.generated.HelveticaBold",
    ["Helvetica-Oblique"] = "pdfkit.font.generated.HelveticaOblique",
    ["Helvetica-BoldOblique"] = "pdfkit.font.generated.HelveticaBoldOblique",
    ["Times-Roman"] = "pdfkit.font.generated.TimesRoman",
    ["Times-Bold"] = "pdfkit.font.generated.TimesBold",
    ["Times-Italic"] = "pdfkit.font.generated.TimesItalic",
    ["Times-BoldItalic"] = "pdfkit.font.generated.TimesBoldItalic",
    ["Symbol"] = "pdfkit.font.generated.Symbol",
    ["ZapfDingbats"] = "pdfkit.font.generated.ZapfDingbats",
}

-- Expanded metrics are cached per font name: a document that switches back
-- and forth between two fonts should not expand either of them twice.
local metric_cache = {}

local function load_metrics(name)
    if metric_cache[name] then
        return metric_cache[name]
    end

    local module_name = METRIC_MODULES[name]
    if not module_name then
        error("pdfkit: no generated metrics for " .. tostring(name))
    end

    local font = AFM.new(require(module_name))
    metric_cache[name] = font
    return font
end

function StandardFont.new(document, name, id)
    local self = setmetatable({}, StandardFont)

    self.document = document
    self.name = name
    self.id = id
    self.font = load_metrics(name)

    self.ascender = self.font.ascender
    self.descender = self.font.descender
    self.bbox = self.font.bbox
    self.lineGap = self.font.lineGap
    self.xHeight = self.font.xHeight
    self.capHeight = self.font.capHeight

    return self
end

function StandardFont:embed()
    local object = require("pdfkit.object")

    self.dictionary.data = object.dict({
        { "Type",     "Font" },
        { "BaseFont", self.name },
        { "Subtype",  "Type1" },
        { "Encoding", "WinAnsiEncoding" },
    })

    return self.dictionary:finish()
end

-- Returns the encoded text and, for each glyph, how far to advance afterwards.
-- The caller turns those advances into the kern values of a TJ array.
function StandardFont:encode(text)
    local encoded = self.font:encodeText(text)
    local glyphs = self.font:glyphsForString(tostring(text))
    local advances = self.font:advancesForGlyphs(glyphs)

    local positions = {}
    for i = 1, #glyphs do
        positions[i] = {
            xAdvance = advances[i],
            yAdvance = 0,
            xOffset = 0,
            yOffset = 0,
            advanceWidth = self.font:widthOfGlyph(glyphs[i]),
        }
    end

    return encoded, positions
end

function StandardFont:widthOfString(str, size)
    local glyphs = self.font:glyphsForString(tostring(str))
    local advances = self.font:advancesForGlyphs(glyphs)

    local width = 0
    for _, advance in ipairs(advances) do
        width = width + advance
    end

    -- Metrics are per 1000 em, so scale down to the requested point size.
    return width * (size / 1000)
end

return StandardFont
