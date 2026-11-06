-- mixins/vector.lua -- port of pdfkit/lib/mixins/vector.js
--
-- Path construction and the graphics state: the operators that draw shapes and
-- the transformation matrix that positions them.
--
-- Every number written here goes through utils.number, the six-decimal
-- formatter -- NOT utils.PDFNumber, which is a different function with
-- different semantics that never reaches the file. Getting those two confused
-- is the single easiest way to produce output that looks plausible and is
-- wrong in the last decimal place of every coordinate.
--
-- Even a document that draws nothing at all uses this file: addPage() calls
-- transform() to flip the coordinate system, and that emits the leading
-- "1 0 0 -1 0 792 cm" of every content stream.

local utils = require("pdfkit.utils")

local number = utils.number

-- Approximates a quarter circle with a cubic Bezier.
local KAPPA = 4.0 * ((math.sqrt(2) - 1.0) / 3.0)

local M = {}

function M.initVector(doc)
    doc._ctm = { 1, 0, 0, 1, 0, 0 } -- current transformation matrix
    doc._ctmStack = {}
end

--------------------------------------------------------------------------------
-- Graphics state stack
--------------------------------------------------------------------------------

function M.save(doc)
    local copy = {}
    for i = 1, 6 do copy[i] = doc._ctm[i] end
    doc._ctmStack[#doc._ctmStack + 1] = copy
    return doc:addContent("q")
end

function M.restore(doc)
    local top = doc._ctmStack[#doc._ctmStack]
    if top then
        doc._ctmStack[#doc._ctmStack] = nil
        doc._ctm = top
    else
        doc._ctm = { 1, 0, 0, 1, 0, 0 }
    end
    return doc:addContent("Q")
end

--------------------------------------------------------------------------------
-- Line and dash styling
--------------------------------------------------------------------------------

function M.closePath(doc)
    return doc:addContent("h")
end

function M.lineWidth(doc, w)
    return doc:addContent(number(w) .. " w")
end

local CAP_STYLES = { BUTT = 0, ROUND = 1, SQUARE = 2 }
local JOIN_STYLES = { MITER = 0, ROUND = 1, BEVEL = 2 }

M._CAP_STYLES = CAP_STYLES
M._JOIN_STYLES = JOIN_STYLES

function M.lineCap(doc, c)
    if type(c) == "string" then
        c = CAP_STYLES[c:upper()]
    end
    return doc:addContent(tostring(c) .. " J")
end

function M.lineJoin(doc, j)
    if type(j) == "string" then
        j = JOIN_STYLES[j:upper()]
    end
    return doc:addContent(tostring(j) .. " j")
end

function M.miterLimit(doc, m)
    return doc:addContent(number(m) .. " M")
end

function M.dash(doc, length, options)
    options = options or {}

    local lengths
    if type(length) == "table" then
        lengths = length
    else
        lengths = { length, options.space or length }
    end

    for _, x in ipairs(lengths) do
        if type(x) ~= "number" or x ~= x or x == math.huge or x == -math.huge or x <= 0 then
            error("dash() invalid, lengths must be numeric and greater than zero")
        end
    end

    local parts = {}
    for i, x in ipairs(lengths) do
        parts[i] = number(x)
    end

    return doc:addContent(
        "[" .. table.concat(parts, " ") .. "] " .. number(options.phase or 0) .. " d"
    )
end

function M.undash(doc)
    return doc:addContent("[] 0 d")
end

--------------------------------------------------------------------------------
-- Path construction
--------------------------------------------------------------------------------

function M.moveTo(doc, x, y)
    return doc:addContent(number(x) .. " " .. number(y) .. " m")
end

function M.lineTo(doc, x, y)
    return doc:addContent(number(x) .. " " .. number(y) .. " l")
end

function M.bezierCurveTo(doc, cp1x, cp1y, cp2x, cp2y, x, y)
    return doc:addContent(
        number(cp1x) .. " " .. number(cp1y) .. " " ..
        number(cp2x) .. " " .. number(cp2y) .. " " ..
        number(x) .. " " .. number(y) .. " c"
    )
end

function M.quadraticCurveTo(doc, cpx, cpy, x, y)
    return doc:addContent(
        number(cpx) .. " " .. number(cpy) .. " " ..
        number(x) .. " " .. number(y) .. " v"
    )
end

function M.rect(doc, x, y, w, h)
    return doc:addContent(
        number(x) .. " " .. number(y) .. " " ..
        number(w) .. " " .. number(h) .. " re"
    )
end

-- Radii follow the CSS border-radius order: top-left, top-right, bottom-right,
-- bottom-left. Each is clamped to half the shorter side so the corners cannot
-- overlap.
function M.roundedRect(doc, x, y, w, h, borderRadius)
    if borderRadius == nil then borderRadius = 0 end

    local radii
    if type(borderRadius) == "table" then
        radii = { borderRadius[1], borderRadius[2], borderRadius[3], borderRadius[4] }
    else
        radii = { borderRadius, borderRadius, borderRadius, borderRadius }
    end

    local limit = math.min(0.5 * w, 0.5 * h)
    local function clamp(r) return math.max(0, math.min(r or 0, limit)) end

    local rTL, rTR, rBR, rBL = clamp(radii[1]), clamp(radii[2]), clamp(radii[3]), clamp(radii[4])

    local cpTR = rTR * (1.0 - KAPPA)
    local cpBR = rBR * (1.0 - KAPPA)
    local cpBL = rBL * (1.0 - KAPPA)
    local cpTL = rTL * (1.0 - KAPPA)

    doc:moveTo(x + rTL, y)

    doc:lineTo(x + w - rTR, y)
    if rTR > 0 then
        doc:bezierCurveTo(x + w - cpTR, y, x + w, y + cpTR, x + w, y + rTR)
    end

    doc:lineTo(x + w, y + h - rBR)
    if rBR > 0 then
        doc:bezierCurveTo(x + w, y + h - cpBR, x + w - cpBR, y + h, x + w - rBR, y + h)
    end

    doc:lineTo(x + rBL, y + h)
    if rBL > 0 then
        doc:bezierCurveTo(x + cpBL, y + h, x, y + h - cpBL, x, y + h - rBL)
    end

    doc:lineTo(x, y + rTL)
    if rTL > 0 then
        doc:bezierCurveTo(x, y + cpTL, x + cpTL, y, x + rTL, y)
    end

    return doc:closePath()
end

function M.ellipse(doc, x, y, r1, r2)
    if r2 == nil then r2 = r1 end

    x = x - r1
    y = y - r2

    local ox = r1 * KAPPA
    local oy = r2 * KAPPA
    local xe = x + r1 * 2
    local ye = y + r2 * 2
    local xm = x + r1
    local ym = y + r2

    doc:moveTo(x, ym)
    doc:bezierCurveTo(x, ym - oy, xm - ox, y, xm, y)
    doc:bezierCurveTo(xm + ox, y, xe, ym - oy, xe, ym)
    doc:bezierCurveTo(xe, ym + oy, xm + ox, ye, xm, ye)
    doc:bezierCurveTo(xm - ox, ye, x, ym + oy, x, ym)
    return doc:closePath()
end

function M.circle(doc, x, y, radius)
    return doc:ellipse(x, y, radius)
end

function M.arc(doc, x, y, radius, startAngle, endAngle, anticlockwise)
    if anticlockwise == nil then anticlockwise = false end

    local TWO_PI = 2.0 * math.pi
    local HALF_PI = 0.5 * math.pi

    local deltaAng = endAngle - startAngle

    if math.abs(deltaAng) > TWO_PI then
        -- More than a full turn was asked for; draw exactly one.
        deltaAng = TWO_PI
    elseif deltaAng ~= 0 and (anticlockwise ~= (deltaAng < 0)) then
        -- Flip the direction of travel.
        local dir = anticlockwise and -1 or 1
        deltaAng = dir * TWO_PI + deltaAng
    end

    local numSegs = math.ceil(math.abs(deltaAng) / HALF_PI)
    local segAng = deltaAng / numSegs
    local handleLen = (segAng / HALF_PI) * KAPPA * radius
    local curAng = startAngle

    local deltaCx = -math.sin(curAng) * handleLen
    local deltaCy = math.cos(curAng) * handleLen

    local ax = x + math.cos(curAng) * radius
    local ay = y + math.sin(curAng) * radius

    doc:moveTo(ax, ay)

    for _ = 1, numSegs do
        local cp1x = ax + deltaCx
        local cp1y = ay + deltaCy

        curAng = curAng + segAng

        ax = x + math.cos(curAng) * radius
        ay = y + math.sin(curAng) * radius

        deltaCx = -math.sin(curAng) * handleLen
        deltaCy = math.cos(curAng) * handleLen

        local cp2x = ax - deltaCx
        local cp2y = ay - deltaCy

        doc:bezierCurveTo(cp1x, cp1y, cp2x, cp2y, ax, ay)
    end

    return doc
end

-- polygon({x, y}, {x, y}, ...)
function M.polygon(doc, ...)
    local points = { ... }
    local first = points[1] or {}
    doc:moveTo(first[1], first[2])
    for i = 2, #points do
        local p = points[i] or {}
        doc:lineTo(p[1], p[2])
    end
    return doc:closePath()
end

function M.path(doc, path)
    local SVGPath = require("pdfkit.path")
    SVGPath.apply(doc, path)
    return doc
end

--------------------------------------------------------------------------------
-- Painting
--------------------------------------------------------------------------------

-- fill() and fillAndStroke() accept a winding rule in the colour position, so
-- both have to sniff their arguments to work out which is which.
local function is_even_odd(v)
    return type(v) == "string" and v:match("even%-?odd") ~= nil
end

local function is_fill_rule(v)
    return type(v) == "string"
        and (v:match("even%-?odd") ~= nil or v:match("non%-?zero") ~= nil)
end

function M._windingRule(doc, rule)
    if is_even_odd(rule) then
        return "*"
    end
    return ""
end

function M.fill(doc, color, rule)
    if is_fill_rule(color) then
        rule = color
        color = nil
    end

    if color then
        doc:fillColor(color)
    end
    return doc:addContent("f" .. doc:_windingRule(rule))
end

function M.stroke(doc, color)
    if color then
        doc:strokeColor(color)
    end
    return doc:addContent("S")
end

function M.fillAndStroke(doc, fillColor, strokeColor, rule)
    if strokeColor == nil then
        strokeColor = fillColor
    end

    if is_fill_rule(fillColor) then
        rule = fillColor
        fillColor = nil
    end

    if is_fill_rule(strokeColor) then
        rule = strokeColor
        strokeColor = fillColor
    end

    if fillColor then
        doc:fillColor(fillColor)
        doc:strokeColor(strokeColor)
    end

    return doc:addContent("B" .. doc:_windingRule(rule))
end

function M.clip(doc, rule)
    return doc:addContent("W" .. doc:_windingRule(rule) .. " n")
end

--------------------------------------------------------------------------------
-- Transformations
--------------------------------------------------------------------------------

function M.transform(doc, m11, m12, m21, m22, dx, dy)
    -- An identity transform is dropped entirely -- it emits no operator at all,
    -- so a document that "transforms" by nothing has no trace of it in the
    -- content stream.
    if m11 == 1 and m12 == 0 and m21 == 0 and m22 == 1 and dx == 0 and dy == 0 then
        return doc
    end

    local m = doc._ctm
    local m0, m1, m2, m3, m4, m5 = m[1], m[2], m[3], m[4], m[5], m[6]
    m[1] = m0 * m11 + m2 * m12
    m[2] = m1 * m11 + m3 * m12
    m[3] = m0 * m21 + m2 * m22
    m[4] = m1 * m21 + m3 * m22
    m[5] = m0 * dx + m2 * dy + m4
    m[6] = m1 * dx + m3 * dy + m5

    local values = table.concat({
        number(m11), number(m12), number(m21),
        number(m22), number(dx), number(dy),
    }, " ")

    return doc:addContent(values .. " cm")
end

function M.translate(doc, x, y)
    return doc:transform(1, 0, 0, 1, x, y)
end

function M.rotate(doc, angle, options)
    options = options or {}

    local rad = (angle * math.pi) / 180
    local cos = math.cos(rad)
    local sin = math.sin(rad)
    local x, y = 0, 0

    if options.origin ~= nil then
        x, y = options.origin[1], options.origin[2]
        local x1 = x * cos - y * sin
        local y1 = x * sin + y * cos
        x = x - x1
        y = y - y1
    end

    return doc:transform(cos, sin, -sin, cos, x, y)
end

function M.scale(doc, xFactor, yFactor, options)
    options = options or {}

    if yFactor == nil then
        yFactor = xFactor
    end
    if type(yFactor) == "table" then
        options = yFactor
        yFactor = xFactor
    end

    local x, y = 0, 0
    if options.origin ~= nil then
        x, y = options.origin[1], options.origin[2]
        x = x - xFactor * x
        y = y - yFactor * y
    end

    return doc:transform(xFactor, 0, 0, yFactor, x, y)
end

return M
