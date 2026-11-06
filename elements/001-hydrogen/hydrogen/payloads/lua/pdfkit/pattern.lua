-- pattern.lua -- port of pdfkit/lib/pattern.js
--
-- Tiling patterns: a small content stream stamped repeatedly across whatever it
-- fills. Uncoloured only (PaintType 2), which means the tile's stream carries no
-- colour of its own -- it draws shapes and the colour is supplied at paint time.
-- That is why apply() takes a colour as well as a stroke flag.
--
-- An uncoloured pattern's colour space has to name the space that colour lives
-- in: [/Pattern /DeviceRGB] rather than plain /Pattern. Both variants get
-- registered on the page (that is embedPatternColorSpaces), and apply() picks
-- whichever matches the colour it was handed.
--
-- The matrix is the same composition gradient.lua does, but against the identity
-- rather than a user transform -- a tiling pattern has no setTransform. It is
-- there because pattern space is unaffected by the current transformation
-- matrix, and the default document CTM flips the y axis; without undoing that
-- the tiles would be upside down relative to everything else.

local object = require("pdfkit.object")
local utils = require("pdfkit.utils")

local js_tostring = utils.js_tostring
local js_to_fixed = utils.js_to_fixed

-- The colour spaces an uncoloured pattern can be painted in. Each becomes its
-- own /ColorSpace entry on the page.
local underlyingColorSpaces = { "DeviceCMYK", "DeviceRGB" }

local PDFTilingPattern = {}
PDFTilingPattern.__index = PDFTilingPattern

-- Marker for mixins/color.lua's _setColor, standing in for the JS
-- `color[0] instanceof PDFTilingPattern`.
PDFTilingPattern.is_tiling_pattern = true

function PDFTilingPattern.new(doc, bBox, xStep, yStep, stream)
    local self = setmetatable({}, PDFTilingPattern)
    self.doc = doc
    self.bBox = bBox
    self.xStep = xStep
    self.yStep = yStep
    self.stream = stream
    return self
end

function PDFTilingPattern:createPattern()
    -- /Resources is required even when the tile needs nothing, so an empty
    -- dictionary is written out to point at.
    local resources = self.doc:ref()
    resources:finish()

    -- The default transform, composed with the document's CTM. See the note at
    -- the top of the file, and the same composition in gradient.lua's apply.
    local ctm = self.doc._ctm
    local m0, m1, m2, m3, m4, m5 = ctm[1], ctm[2], ctm[3], ctm[4], ctm[5], ctm[6]
    local m11, m12, m21, m22, dx, dy = 1, 0, 0, 1, 0, 0
    local m = {
        m0 * m11 + m2 * m12,
        m1 * m11 + m3 * m12,
        m0 * m21 + m2 * m22,
        m1 * m21 + m3 * m22,
        m0 * dx + m2 * dy + m4,
        m1 * dx + m3 * dy + m5,
    }

    -- Five decimals here, not the six every other number in the library gets.
    -- Upstream writes `m.map((v) => +v.toFixed(5))`, and the difference is
    -- visible in the file, so it is reproduced rather than normalised.
    local matrix = object.array()
    for i = 1, 6 do
        matrix[i] = js_to_fixed(m[i], 5)
    end

    local pattern = self.doc:ref(object.dict({
        { "Type",        "Pattern" },
        { "PatternType", 1 }, -- tiling
        { "PaintType",   2 }, -- 1 = coloured, 2 = uncoloured
        { "TilingType",  2 }, -- 2 = no distortion
        { "BBox",        object.array({ table.unpack(self.bBox) }) },
        { "XStep",       self.xStep },
        { "YStep",       self.yStep },
        { "Matrix",      matrix },
        { "Resources",   resources },
    }))
    pattern:finish(self.stream)
    return pattern
end

--- Register a /ColorSpace entry on the current page for each underlying space.
---
--- Both are registered whether or not they end up used, because which one is
--- needed is not known until a colour arrives in apply().
function PDFTilingPattern:embedPatternColorSpaces()
    for _, csName in ipairs(underlyingColorSpaces) do
        local csId = self:getPatternColorSpaceId(csName)

        if not self.doc.page:colorSpaces()[csId] then
            local cs = self.doc:ref(object.array({ "Pattern", csName }))
            cs:finish()
            self.doc.page:colorSpaces()[csId] = cs
        end
    end
end

function PDFTilingPattern:getPatternColorSpaceId(underlyingColorspace)
    return "CsP" .. underlyingColorspace
end

--- Write the pattern object once, and register it on the current page.
---
--- The two halves are separate on purpose: the object is created the first time
--- the pattern is used anywhere, but the page resource entry is added again for
--- every page it appears on.
function PDFTilingPattern:embed()
    if not self.id then
        self.doc._patternCount = self.doc._patternCount + 1
        self.id = "P" .. self.doc._patternCount
        self.pattern = self:createPattern()
    end

    if not self.doc.page:patterns()[self.id] then
        self.doc.page:patterns()[self.id] = self.pattern
    end
end

--- Paint with this pattern in `patternColor`, filling or stroking.
function PDFTilingPattern:apply(stroke, patternColor)
    self:embedPatternColorSpaces()
    self:embed()

    local normalizedColor = self.doc:_normalizeColor(patternColor)
    if not normalizedColor then
        error("invalid pattern color. (value: " .. tostring(patternColor) .. ")")
    end

    local csId = self:getPatternColorSpaceId(self.doc:_getColorSpace(normalizedColor))
    self.doc:_setColorSpace(csId, stroke)

    -- Components first, then the pattern name: an uncoloured pattern's `scn`
    -- takes the colour in the underlying space followed by the pattern.
    local op = stroke and "SCN" or "scn"
    local parts = {}
    for i = 1, #normalizedColor do
        parts[i] = js_tostring(normalizedColor[i])
    end
    return self.doc:addContent(
        table.concat(parts, " ") .. " /" .. self.id .. " " .. op
    )
end

return { PDFTilingPattern = PDFTilingPattern }
