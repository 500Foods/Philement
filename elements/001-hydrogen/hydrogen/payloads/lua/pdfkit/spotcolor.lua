-- spotcolor.lua -- port of pdfkit/lib/spotcolor.js
--
-- A named separation colour: one ink, addressed by name, with a CMYK
-- approximation for renderers that have to simulate it. The PDF object is a
-- Separation colour space whose tint transform is a type-2 (exponential)
-- function interpolating from "no ink" to the given CMYK values.
--
-- The object writes itself out immediately on construction. It appears in the
-- file as a plain indirect reference, which is why it carries to_pdf_string --
-- the JS relies on toString() being called during conversion, and this is the
-- same hook.

local object = require("pdfkit.object")

local SpotColor = {}
SpotColor.__index = SpotColor

function SpotColor.new(doc, name, C, M, Y, K)
    local self = setmetatable({}, SpotColor)

    -- Sequentially numbered from however many spot colours already exist:
    -- CS0, CS1, CS2. Note this is assigned BEFORE the document registers the
    -- new colour, so the first one is CS0.
    self.id = "CS" .. object.dict_count(doc.spotColors)
    self.name = name
    self.values = { C, M, Y, K }

    -- Marker for _normalizeColor, which has to tell a spot colour apart from a
    -- plain component array.
    self.is_spot_color = true

    local c1 = object.array({})
    for i = 1, 4 do
        c1[i] = self.values[i] / 100
    end

    self.ref = doc:ref(object.array({
        "Separation",
        object.escape_name(self.name),
        "DeviceCMYK",
        object.dict({
            { "Range",        object.array({ 0, 1, 0, 1, 0, 1, 0, 1 }) },
            { "C0",           object.array({ 0, 0, 0, 0 }) },
            { "C1",           c1 },
            { "FunctionType", 2 },
            { "Domain",       object.array({ 0, 1 }) },
            { "N",            1 },
        }),
    }))
    self.ref:finish()

    return self
end

function SpotColor:to_pdf_string()
    return self.ref.id .. " 0 R"
end

return SpotColor
