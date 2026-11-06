-- mixins/color.lua -- port of pdfkit/lib/mixins/color.js
--
-- Colour handling, plus the opacity (ExtGState) registry.
--
-- One detail here is easy to miss and produces wrong bytes if you do. Colour
-- components are written with
--
--     this.addContent(`${color.join(' ')} ${op}`)
--
-- which is JavaScript's *default* number formatting, not PDFObject.number. So
-- a grey of #808080 normalises to 128/255 and is written in full as
-- 0.5019607843137255, sixteen significant digits. Everywhere else in the
-- library numbers are clipped to six decimals; here they are not. That is why
-- this file reaches for utils.js_tostring rather than utils.number.

local utils = require("pdfkit.utils")
local object = require("pdfkit.object")
local gradient = require("pdfkit.gradient")
local pattern = require("pdfkit.pattern")

local PDFLinearGradient = gradient.PDFLinearGradient
local PDFRadialGradient = gradient.PDFRadialGradient
local PDFTilingPattern = pattern.PDFTilingPattern

local js_tostring = utils.js_tostring

local M = {}

--------------------------------------------------------------------------------
-- Named colours (CSS colour keywords)
--------------------------------------------------------------------------------

local namedColors = {
    aliceblue = { 240, 248, 255 }, antiquewhite = { 250, 235, 215 },
    aqua = { 0, 255, 255 }, aquamarine = { 127, 255, 212 },
    azure = { 240, 255, 255 }, beige = { 245, 245, 220 },
    bisque = { 255, 228, 196 }, black = { 0, 0, 0 },
    blanchedalmond = { 255, 235, 205 }, blue = { 0, 0, 255 },
    blueviolet = { 138, 43, 226 }, brown = { 165, 42, 42 },
    burlywood = { 222, 184, 135 }, cadetblue = { 95, 158, 160 },
    chartreuse = { 127, 255, 0 }, chocolate = { 210, 105, 30 },
    coral = { 255, 127, 80 }, cornflowerblue = { 100, 149, 237 },
    cornsilk = { 255, 248, 220 }, crimson = { 220, 20, 60 },
    cyan = { 0, 255, 255 }, darkblue = { 0, 0, 139 },
    darkcyan = { 0, 139, 139 }, darkgoldenrod = { 184, 134, 11 },
    darkgray = { 169, 169, 169 }, darkgreen = { 0, 100, 0 },
    darkgrey = { 169, 169, 169 }, darkkhaki = { 189, 183, 107 },
    darkmagenta = { 139, 0, 139 }, darkolivegreen = { 85, 107, 47 },
    darkorange = { 255, 140, 0 }, darkorchid = { 153, 50, 204 },
    darkred = { 139, 0, 0 }, darksalmon = { 233, 150, 122 },
    darkseagreen = { 143, 188, 143 }, darkslateblue = { 72, 61, 139 },
    darkslategray = { 47, 79, 79 }, darkslategrey = { 47, 79, 79 },
    darkturquoise = { 0, 206, 209 }, darkviolet = { 148, 0, 211 },
    deeppink = { 255, 20, 147 }, deepskyblue = { 0, 191, 255 },
    dimgray = { 105, 105, 105 }, dimgrey = { 105, 105, 105 },
    dodgerblue = { 30, 144, 255 }, firebrick = { 178, 34, 34 },
    floralwhite = { 255, 250, 240 }, forestgreen = { 34, 139, 34 },
    fuchsia = { 255, 0, 255 }, gainsboro = { 220, 220, 220 },
    ghostwhite = { 248, 248, 255 }, gold = { 255, 215, 0 },
    goldenrod = { 218, 165, 32 }, gray = { 128, 128, 128 },
    grey = { 128, 128, 128 }, green = { 0, 128, 0 },
    greenyellow = { 173, 255, 47 }, honeydew = { 240, 255, 240 },
    hotpink = { 255, 105, 180 }, indianred = { 205, 92, 92 },
    indigo = { 75, 0, 130 }, ivory = { 255, 255, 240 },
    khaki = { 240, 230, 140 }, lavender = { 230, 230, 250 },
    lavenderblush = { 255, 240, 245 }, lawngreen = { 124, 252, 0 },
    lemonchiffon = { 255, 250, 205 }, lightblue = { 173, 216, 230 },
    lightcoral = { 240, 128, 128 }, lightcyan = { 224, 255, 255 },
    lightgoldenrodyellow = { 250, 250, 210 }, lightgray = { 211, 211, 211 },
    lightgreen = { 144, 238, 144 }, lightgrey = { 211, 211, 211 },
    lightpink = { 255, 182, 193 }, lightsalmon = { 255, 160, 122 },
    lightseagreen = { 32, 178, 170 }, lightskyblue = { 135, 206, 250 },
    lightslategray = { 119, 136, 153 }, lightslategrey = { 119, 136, 153 },
    lightsteelblue = { 176, 196, 222 }, lightyellow = { 255, 255, 224 },
    lime = { 0, 255, 0 }, limegreen = { 50, 205, 50 },
    linen = { 250, 240, 230 }, magenta = { 255, 0, 255 },
    maroon = { 128, 0, 0 }, mediumaquamarine = { 102, 205, 170 },
    mediumblue = { 0, 0, 205 }, mediumorchid = { 186, 85, 211 },
    mediumpurple = { 147, 112, 219 }, mediumseagreen = { 60, 179, 113 },
    mediumslateblue = { 123, 104, 238 }, mediumspringgreen = { 0, 250, 154 },
    mediumturquoise = { 72, 209, 204 }, mediumvioletred = { 199, 21, 133 },
    midnightblue = { 25, 25, 112 }, mintcream = { 245, 255, 250 },
    mistyrose = { 255, 228, 225 }, moccasin = { 255, 228, 181 },
    navajowhite = { 255, 222, 173 }, navy = { 0, 0, 128 },
    oldlace = { 253, 245, 230 }, olive = { 128, 128, 0 },
    olivedrab = { 107, 142, 35 }, orange = { 255, 165, 0 },
    orangered = { 255, 69, 0 }, orchid = { 218, 112, 214 },
    palegoldenrod = { 238, 232, 170 }, palegreen = { 152, 251, 152 },
    paleturquoise = { 175, 238, 238 }, palevioletred = { 219, 112, 147 },
    papayawhip = { 255, 239, 213 }, peachpuff = { 255, 218, 185 },
    peru = { 205, 133, 63 }, pink = { 255, 192, 203 },
    plum = { 221, 160, 221 }, powderblue = { 176, 224, 230 },
    purple = { 128, 0, 128 }, red = { 255, 0, 0 },
    rosybrown = { 188, 143, 143 }, royalblue = { 65, 105, 225 },
    saddlebrown = { 139, 69, 19 }, salmon = { 250, 128, 114 },
    sandybrown = { 244, 164, 96 }, seagreen = { 46, 139, 87 },
    seashell = { 255, 245, 238 }, sienna = { 160, 82, 45 },
    silver = { 192, 192, 192 }, skyblue = { 135, 206, 235 },
    slateblue = { 106, 90, 205 }, slategray = { 112, 128, 144 },
    slategrey = { 112, 128, 144 }, snow = { 255, 250, 250 },
    springgreen = { 0, 255, 127 }, steelblue = { 70, 130, 180 },
    tan = { 210, 180, 140 }, teal = { 0, 128, 128 },
    thistle = { 216, 191, 216 }, tomato = { 255, 99, 71 },
    turquoise = { 64, 224, 208 }, violet = { 238, 130, 238 },
    wheat = { 245, 222, 179 }, white = { 255, 255, 255 },
    whitesmoke = { 245, 245, 245 }, yellow = { 255, 255, 0 },
    yellowgreen = { 154, 205, 50 },
}

M.namedColors = namedColors

--------------------------------------------------------------------------------

function M.initColor(doc)
    -- Ordered, because every page appends its spot colours to /ColorSpace in
    -- registration order and a shuffled walk would reorder those keys.
    doc.spotColors = object.dict()
    doc._opacityRegistry = {}
    doc._opacityCount = 0
    doc._patternCount = 0
    doc._gradCount = 0
end

-- Accepts "#rgb", "#rrggbb", a CSS colour keyword, a registered spot colour
-- name, a 3-element RGB array (0-255) or a 4-element CMYK array (0-100), and
-- returns components scaled into the 0-1 range PDF wants. Returns nil if the
-- value is not a colour at all -- callers treat that as "no colour given".
function M._normalizeColor(doc, color)
    if type(color) == "string" then
        if color:sub(1, 1) == "#" then
            local body = color:sub(2)
            -- Expand the three-digit shorthand: #abc -> #aabbcc
            if #color == 4 then
                body = body:gsub("(%x)", "%1%1")
            end
            local hex = tonumber(body, 16)
            if hex then
                color = { (hex >> 16) & 0xff, (hex >> 8) & 0xff, hex & 0xff }
            end
        elseif namedColors[color] then
            local c = namedColors[color]
            color = { c[1], c[2], c[3] }
        elseif doc.spotColors[color] then
            return doc.spotColors[color]
        end
    end

    if type(color) == "table" and not color.is_spot_color then
        local n = #color
        local scaled = {}
        if n == 3 then
            for i = 1, 3 do scaled[i] = color[i] / 255 end
            return scaled
        elseif n == 4 then
            for i = 1, 4 do scaled[i] = color[i] / 100 end
            return scaled
        end
        return color
    end

    if type(color) == "table" then
        return color
    end

    return nil
end

function M._setColor(doc, color, stroke)
    -- Gradients and tiling patterns paint themselves rather than emitting a
    -- plain colour operator.
    if type(color) == "table" then
        if color.is_gradient then
            color:apply(stroke)
            return true
        end
        if type(color[1]) == "table" and color[1].is_tiling_pattern then
            color[1]:apply(stroke, color[2])
            return true
        end
    end

    return doc:_setColorCore(color, stroke)
end

function M._setColorCore(doc, color, stroke)
    color = doc:_normalizeColor(color)
    if not color then
        return false
    end

    local op = stroke and "SCN" or "scn"
    local space = doc:_getColorSpace(color)
    doc:_setColorSpace(space, stroke)

    if color.is_spot_color then
        doc.page:colorSpaces()[color.id] = color.ref
        doc:addContent("1 " .. op)
    else
        local parts = {}
        for i = 1, #color do
            parts[i] = js_tostring(color[i])
        end
        doc:addContent(table.concat(parts, " ") .. " " .. op)
    end

    return true
end

function M._setColorSpace(doc, space, stroke)
    local op = stroke and "CS" or "cs"
    return doc:addContent("/" .. space .. " " .. op)
end

function M._getColorSpace(doc, color)
    if color.is_spot_color then
        return color.id
    end
    return (#color == 4) and "DeviceCMYK" or "DeviceRGB"
end

function M.fillColor(doc, color, opacity)
    local set = doc:_setColor(color, false)
    if set then
        doc:fillOpacity(opacity)
    end

    -- Remembered so the text wrapper can restore it when it starts a new page.
    doc._fillColor = { color, opacity }
    return doc
end

function M.strokeColor(doc, color, opacity)
    local set = doc:_setColor(color, true)
    if set then
        doc:strokeOpacity(opacity)
    end
    return doc
end

function M.opacity(doc, opacity)
    doc:_doOpacity(opacity, opacity)
    return doc
end

function M.fillOpacity(doc, opacity)
    doc:_doOpacity(opacity, nil)
    return doc
end

function M.strokeOpacity(doc, opacity)
    doc:_doOpacity(nil, opacity)
    return doc
end

-- Opacity is a graphics-state parameter, so each distinct fill/stroke pair
-- becomes an ExtGState object. They are cached by value, so repeating an
-- opacity reuses the same /GsN rather than creating another object.
function M._doOpacity(doc, fillOpacity, strokeOpacity)
    if fillOpacity == nil and strokeOpacity == nil then
        return
    end

    local function clamp(v)
        return math.max(0, math.min(1, v))
    end

    if fillOpacity ~= nil then fillOpacity = clamp(fillOpacity) end
    if strokeOpacity ~= nil then strokeOpacity = clamp(strokeOpacity) end

    -- The cache key mirrors the JS template literal, where an absent value
    -- stringifies as "null".
    local key = (fillOpacity ~= nil and js_tostring(fillOpacity) or "null")
        .. "_" ..
        (strokeOpacity ~= nil and js_tostring(strokeOpacity) or "null")

    local dictionary, name
    local cached = doc._opacityRegistry[key]
    if cached then
        dictionary, name = cached[1], cached[2]
    else
        local d = object.dict({ { "Type", "ExtGState" } })
        if fillOpacity ~= nil then d.ca = fillOpacity end
        if strokeOpacity ~= nil then d.CA = strokeOpacity end

        dictionary = doc:ref(d)
        dictionary:finish()

        doc._opacityCount = doc._opacityCount + 1
        name = "Gs" .. doc._opacityCount
        doc._opacityRegistry[key] = { dictionary, name }
    end

    doc.page:ext_gstates()[name] = dictionary
    return doc:addContent("/" .. name .. " gs")
end

-- Gradients and patterns are constructed here but do nothing until they are
-- handed to fillColor or strokeColor, which is what triggers _setColor's
-- dispatch above and, through that, writes the objects out.

function M.linearGradient(doc, x1, y1, x2, y2)
    return PDFLinearGradient.new(doc, x1, y1, x2, y2)
end

function M.radialGradient(doc, x1, y1, r1, x2, y2, r2)
    return PDFRadialGradient.new(doc, x1, y1, r1, x2, y2, r2)
end

function M.pattern(doc, bbox, xStep, yStep, stream)
    return PDFTilingPattern.new(doc, bbox, xStep, yStep, stream)
end

function M.addSpotColor(doc, name, C, Mc, Y, K)
    local SpotColor = require("pdfkit.spotcolor")
    doc.spotColors[name] = SpotColor.new(doc, name, C, Mc, Y, K)
    return doc
end

return M
