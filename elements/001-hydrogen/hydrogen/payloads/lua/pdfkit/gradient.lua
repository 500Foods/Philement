-- gradient.lua -- port of pdfkit/lib/gradient.js
--
-- A gradient is not a colour in PDF; it is a *pattern*, and painting with one
-- means switching the colour space to /Pattern and naming the pattern rather
-- than giving components. So a gradient assembles a small tower of objects:
--
--   function(s)   one exponential (type 2) function per pair of adjacent stops,
--                 each interpolating C0 -> C1; if there is more than one pair,
--                 a stitching (type 3) function joins them over [0, 1]
--   shading       type 2 (axial) or type 3 (radial), naming the function and
--                 the geometry -- the part the subclasses supply
--   pattern       PatternType 2, wrapping the shading with a matrix
--
-- and the page's /Pattern resource dictionary gains an entry pointing at it.
--
-- Per-stop opacity needs a second tower on top of that, because PDF has nowhere
-- to put alpha in a shading. The trick upstream uses is a luminosity soft mask:
-- build a *second* gradient in DeviceGray whose greys are the alphas, paint it
-- into a form XObject, and hand that form to an ExtGState as an /SMask. Grey
-- level becomes opacity. The visible gradient then has to be painted through
-- that graphics state, which a shading pattern cannot do on its own -- so it is
-- wrapped once more in a *tiling* pattern whose one tile does `gs`, then `scn`.
--
-- Two details in embed() below are easy to get wrong and are marked where they
-- happen: the loop counts the stops from before the closing stop is appended,
-- and the opacity tower refers to its inner patterns by the literal name /Sh1
-- regardless of what the gradients are actually called.

local object = require("pdfkit.object")
local utils = require("pdfkit.utils")

local js_tostring = utils.js_tostring
local round6 = utils.round6

--------------------------------------------------------------------------------
-- PDFGradient -- the shared half
--------------------------------------------------------------------------------

local PDFGradient = {}
PDFGradient.__index = PDFGradient

-- Marker for mixins/color.lua's _setColor, which has to tell a gradient from a
-- plain colour. It lives on the base table so both subclasses inherit it, and
-- stands in for the JS `color instanceof PDFGradient`.
PDFGradient.is_gradient = true

function PDFGradient.new(doc)
    local self = setmetatable({}, PDFGradient)
    self.doc = doc
    self.stops = {}
    self.embedded = false
    self.transform = { 1, 0, 0, 1, 0, 0 }
    return self
end

--- Add a colour stop at `pos` (0 to 1) with an optional opacity (default 1).
---
--- The first stop fixes the gradient's colour space, and every later one has to
--- agree with it -- a gradient cannot interpolate from RGB to CMYK.
function PDFGradient:stop(pos, color, opacity)
    if opacity == nil then
        opacity = 1
    end
    color = self.doc:_normalizeColor(color)

    if #self.stops == 0 then
        if #color == 3 then
            self._colorSpace = "DeviceRGB"
        elseif #color == 4 then
            self._colorSpace = "DeviceCMYK"
        elseif #color == 1 then
            self._colorSpace = "DeviceGray"
        else
            error("Unknown color space")
        end
    elseif (self._colorSpace == "DeviceRGB" and #color ~= 3)
        or (self._colorSpace == "DeviceCMYK" and #color ~= 4)
        or (self._colorSpace == "DeviceGray" and #color ~= 1) then
        error("All gradient stops must use the same color space")
    end

    opacity = math.max(0, math.min(1, opacity))
    self.stops[#self.stops + 1] = { pos, color, opacity }
    return self
end

function PDFGradient:setTransform(m11, m12, m21, m22, dx, dy)
    self.transform = { m11, m12, m21, m22, dx, dy }
    return self
end

--- Write the gradient out under the transformation matrix `m`.
---
--- Returns the shading pattern object. Note that the *return value* is always
--- the plain pattern even when opacity is in play -- it is the page resource
--- entry, not the return value, that switches to the tiling wrapper.
function PDFGradient:embed(m)
    local fn

    -- Captured before the closing stop is appended below, and used as the loop
    -- bound and in the /Bounds guard. Appending first would add a function
    -- segment for a stop pair that the encode/bounds arrays do not describe.
    local stopsLength = #self.stops
    if stopsLength == 0 then
        return
    end
    self.embedded = true
    self.matrix = m

    -- If the last stop comes before 100%, add a copy at 100%, so the final
    -- colour holds rather than the shading running off the end of its functions.
    local last = self.stops[stopsLength]
    if last[1] < 1 then
        self.stops[#self.stops + 1] = { 1, last[2], last[3] }
    end

    local bounds = object.array()
    local encode = object.array()
    local stops = object.array()

    for i = 1, stopsLength - 1 do
        -- Each segment maps its slice of the parametric range onto the whole of
        -- its own function's domain.
        encode[#encode + 1] = 0
        encode[#encode + 1] = 1

        -- /Bounds holds the *interior* boundaries only: one fewer than the
        -- number of functions. The last pair contributes no bound.
        if i + 1 ~= stopsLength then
            bounds[#bounds + 1] = self.stops[i + 1][1]
        end

        fn = self.doc:ref(object.dict({
            { "FunctionType", 2 },
            { "Domain",       object.array({ 0, 1 }) },
            { "C0",           object.array({ table.unpack(self.stops[i][2]) }) },
            { "C1",           object.array({ table.unpack(self.stops[i + 1][2]) }) },
            { "N",            1 },
        }))

        stops[#stops + 1] = fn
        fn:finish()
    end

    if stopsLength == 1 then
        -- A single stop leaves no pairs, so the loop above ran zero times and
        -- there is no function to point at. Upstream has the same hole and
        -- writes a shading with a dangling /Function; the resulting file is
        -- malformed either way, so this reproduces it rather than inventing a
        -- fix that would disagree.
        fn = stops[1]
    else
        fn = self.doc:ref(object.dict({
            { "FunctionType", 3 }, -- stitching function
            { "Domain",       object.array({ 0, 1 }) },
            { "Functions",    stops },
            { "Bounds",       bounds },
            { "Encode",       encode },
        }))

        fn:finish()
    end

    self.doc._gradCount = self.doc._gradCount + 1
    self.id = "Sh" .. self.doc._gradCount

    local shader = self:shader(fn)
    shader:finish()

    local pattern = self.doc:ref(object.dict({
        { "Type",        "Pattern" },
        { "PatternType", 2 },
        { "Shading",     shader },
        { "Matrix",      object.array(self:_rounded_matrix()) },
    }))

    pattern:finish()

    local translucent = false
    for _, stop in ipairs(self.stops) do
        if stop[3] < 1 then
            translucent = true
            break
        end
    end

    if translucent then
        -- The luminosity mask described in the file header. A parallel gradient
        -- in DeviceGray, with each stop's alpha as its grey level.
        local grad = self:opacityGradient()
        grad._colorSpace = "DeviceGray"

        for _, stop in ipairs(self.stops) do
            grad:stop(stop[1], { stop[3] })
        end

        -- Reassigned to the *pattern object* the nested embed returns, not the
        -- gradient -- that is what the form's resources need to name.
        grad = grad:embed(self.matrix)

        -- The mask has to cover anything the gradient could be painted on, so
        -- it is sized to the page rather than to the gradient's own geometry.
        local pageBBox = { 0, 0, self.doc.page.width, self.doc.page.height }
        local pageBBoxText = table.concat({
            js_tostring(pageBBox[1]), js_tostring(pageBBox[2]),
            js_tostring(pageBBox[3]), js_tostring(pageBBox[4]),
        }, " ")

        local form = self.doc:ref(object.dict({
            { "Type",     "XObject" },
            { "Subtype",  "Form" },
            { "FormType", 1 },
            { "BBox",     object.array({ table.unpack(pageBBox) }) },
            { "Group", object.dict({
                { "Type", "Group" },
                { "S",    "Transparency" },
                { "CS",   "DeviceGray" },
            }) },
            { "Resources", object.dict({
                { "ProcSet", object.array({ "PDF", "Text", "ImageB", "ImageC", "ImageI" }) },
                -- /Sh1 is a literal here, not grad.id. Inside this form it is
                -- the only pattern there is, so the name need only be
                -- self-consistent -- and upstream picks the same one every time.
                { "Pattern", object.dict({ { "Sh1", grad } }) },
            }) },
        }))

        form:write("/Pattern cs /Sh1 scn")
        form:finish(pageBBoxText .. " re f")

        local gstate = self.doc:ref(object.dict({
            { "Type", "ExtGState" },
            { "SMask", object.dict({
                { "Type", "Mask" },
                { "S",    "Luminosity" },
                { "G",    form },
            }) },
        }))

        gstate:finish()

        -- One tile the size of the page, which paints the real gradient with the
        -- mask's graphics state applied. A shading pattern has no content
        -- stream of its own, so this is the only place a `gs` can go.
        local opacityPattern = self.doc:ref(object.dict({
            { "Type",        "Pattern" },
            { "PatternType", 1 },
            { "PaintType",   1 },
            { "TilingType",  2 },
            { "BBox",        object.array({ table.unpack(pageBBox) }) },
            { "XStep",       pageBBox[3] },
            { "YStep",       pageBBox[4] },
            { "Resources", object.dict({
                { "ProcSet", object.array({ "PDF", "Text", "ImageB", "ImageC", "ImageI" }) },
                { "Pattern",   object.dict({ { "Sh1", pattern } }) },
                { "ExtGState", object.dict({ { "Gs1", gstate } }) },
            }) },
        }))

        opacityPattern:write("/Gs1 gs /Pattern cs /Sh1 scn")
        opacityPattern:finish(pageBBoxText .. " re f")

        self.doc.page:patterns()[self.id] = opacityPattern
    else
        self.doc.page:patterns()[self.id] = pattern
    end

    return pattern
end

-- `this.matrix.map(number)` -- rounded to six decimals here, then printed by the
-- usual conversion later. See utils.round6 for why rounding twice is safe.
function PDFGradient:_rounded_matrix()
    local out = {}
    for i = 1, 6 do
        out[i] = round6(self.matrix[i])
    end
    return out
end

--- Paint with this gradient, filling or stroking.
---
--- The gradient's own transform composes with the document's current one, so
--- moving the page origin moves the gradient with it. If that product differs
--- from the matrix it was last written under, it has to be written again --
--- a gradient used on two pages, or under two transforms, becomes two patterns.
function PDFGradient:apply(stroke)
    local ctm = self.doc._ctm
    local m0, m1, m2, m3, m4, m5 = ctm[1], ctm[2], ctm[3], ctm[4], ctm[5], ctm[6]
    local t = self.transform
    local m11, m12, m21, m22, dx, dy = t[1], t[2], t[3], t[4], t[5], t[6]
    local m = {
        m0 * m11 + m2 * m12,
        m1 * m11 + m3 * m12,
        m0 * m21 + m2 * m22,
        m1 * m21 + m3 * m22,
        m0 * dx + m2 * dy + m4,
        m1 * dx + m3 * dy + m5,
    }

    -- Compared as text, matching the JS, which joins both matrices and compares
    -- the strings. `or` short-circuits before self.matrix is read, which matters
    -- on the first call -- there is no matrix yet.
    local function join(v)
        local parts = {}
        for i = 1, 6 do parts[i] = js_tostring(v[i]) end
        return table.concat(parts, " ")
    end

    if not self.embedded or join(m) ~= join(self.matrix) then
        self:embed(m)
    end
    self.doc:_setColorSpace("Pattern", stroke)
    local op = stroke and "SCN" or "scn"
    return self.doc:addContent("/" .. self.id .. " " .. op)
end

--------------------------------------------------------------------------------
-- PDFLinearGradient -- an axial shading, from one point to another
--------------------------------------------------------------------------------

local PDFLinearGradient = setmetatable({}, { __index = PDFGradient })
PDFLinearGradient.__index = PDFLinearGradient

function PDFLinearGradient.new(doc, x1, y1, x2, y2)
    local self = setmetatable(PDFGradient.new(doc), PDFLinearGradient)
    self.x1 = x1
    self.y1 = y1
    self.x2 = x2
    self.y2 = y2
    return self
end

function PDFLinearGradient:shader(fn)
    return self.doc:ref(object.dict({
        { "ShadingType", 2 },
        { "ColorSpace",  self._colorSpace },
        { "Coords",      object.array({ self.x1, self.y1, self.x2, self.y2 }) },
        { "Function",    fn },
        -- Extended at both ends, so the first and last colours continue past
        -- the axis rather than leaving the rest of the area unpainted.
        { "Extend",      object.array({ true, true }) },
    }))
end

function PDFLinearGradient:opacityGradient()
    return PDFLinearGradient.new(self.doc, self.x1, self.y1, self.x2, self.y2)
end

--------------------------------------------------------------------------------
-- PDFRadialGradient -- a radial shading, between two circles
--------------------------------------------------------------------------------

local PDFRadialGradient = setmetatable({}, { __index = PDFGradient })
PDFRadialGradient.__index = PDFRadialGradient

function PDFRadialGradient.new(doc, x1, y1, r1, x2, y2, r2)
    local self = setmetatable(PDFGradient.new(doc), PDFRadialGradient)
    self.x1 = x1
    self.y1 = y1
    self.r1 = r1
    self.x2 = x2
    self.y2 = y2
    self.r2 = r2
    return self
end

function PDFRadialGradient:shader(fn)
    return self.doc:ref(object.dict({
        { "ShadingType", 3 },
        { "ColorSpace",  self._colorSpace },
        { "Coords",      object.array({ self.x1, self.y1, self.r1, self.x2, self.y2, self.r2 }) },
        { "Function",    fn },
        { "Extend",      object.array({ true, true }) },
    }))
end

function PDFRadialGradient:opacityGradient()
    return PDFRadialGradient.new(
        self.doc,
        self.x1,
        self.y1,
        self.r1,
        self.x2,
        self.y2,
        self.r2
    )
end

return {
    PDFGradient = PDFGradient,
    PDFLinearGradient = PDFLinearGradient,
    PDFRadialGradient = PDFRadialGradient,
}
