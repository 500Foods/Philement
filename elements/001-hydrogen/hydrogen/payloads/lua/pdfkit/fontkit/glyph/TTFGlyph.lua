-- fontkit/glyph/TTFGlyph.lua -- port of fontkit/src/glyph/TTFGlyph.js
--
-- A glyph whose outline lives in `glyf` as quadratic contours.
--
-- The format is aggressively packed, and three of its tricks are worth naming
-- up front because they drive the shape of this file:
--
--   Coordinates are deltas from the previous point, and each may be one byte or
--   two, or absent entirely when the point repeats the last value. The flag
--   byte for each point says which -- and one flag byte can stand for a run of
--   identical ones.
--
--   Points carry an on-curve bit. Two consecutive off-curve points imply an
--   on-curve point exactly between them, which the font does not store. Half of
--   _getPath is reconstructing those.
--
--   A glyph can be a composite: a list of other glyphs with a translation and
--   optionally a 2x2 matrix each. Accented letters are built this way, and
--   resolving one recurses into the components.

local reader = require("pdfkit.fontkit.reader")
local Glyph = require("pdfkit.fontkit.glyph.Glyph")
local Path = require("pdfkit.fontkit.glyph.Path")
local BBox = require("pdfkit.fontkit.glyph.BBox")

-- Flags for simple glyphs
local ON_CURVE       = 1 << 0
local X_SHORT_VECTOR = 1 << 1
local Y_SHORT_VECTOR = 1 << 2
local REPEAT_FLAG    = 1 << 3
local SAME_X         = 1 << 4
local SAME_Y         = 1 << 5

-- Flags for composite glyphs
local ARG_1_AND_2_ARE_WORDS    = 1 << 0
local ARGS_ARE_XY_VALUES       = 1 << 1
local ROUND_XY_TO_GRID         = 1 << 2
local WE_HAVE_A_SCALE          = 1 << 3
local MORE_COMPONENTS          = 1 << 5
local WE_HAVE_AN_X_AND_Y_SCALE = 1 << 6
local WE_HAVE_A_TWO_BY_TWO     = 1 << 7
local WE_HAVE_INSTRUCTIONS     = 1 << 8
local USE_MY_METRICS           = 1 << 9
local OVERLAP_COMPOUND         = 1 << 10
local SCALED_COMPONENT_OFFSET  = 1 << 11
local UNSCALED_COMPONENT_OFFSET = 1 << 12

--------------------------------------------------------------------------------
-- Point and Component
--------------------------------------------------------------------------------

--- A point in a simple glyph. `endContour` marks the last point of a contour;
--- contours are closed, so the next point starts a new one.
local Point = {}
Point.__index = Point

function Point.new(onCurve, endContour, x, y)
    return setmetatable({
        onCurve = onCurve,
        endContour = endContour,
        x = x or 0,
        y = y or 0,
    }, Point)
end

function Point:copy()
    return Point.new(self.onCurve, self.endContour, self.x, self.y)
end

--- One entry in a composite glyph: which glyph, where, and under what matrix.
--- `pos` is the component's offset from the start of the glyph's data, which is
--- the only thing the subset writer needs in order to rewrite the glyph id.
local Component = {}
Component.__index = Component

function Component.new(glyphID, dx, dy)
    return setmetatable({
        glyphID = glyphID,
        dx = dx,
        dy = dy,
        pos = 0,
        scaleX = 1,
        scaleY = 1,
        scale01 = 0,
        scale10 = 0,
    }, Component)
end

--------------------------------------------------------------------------------
-- TTFGlyph
--------------------------------------------------------------------------------

local TTFGlyph = setmetatable({}, { __index = Glyph })
TTFGlyph.__index = TTFGlyph

function TTFGlyph.new(id, codePoints, font)
    local self = Glyph.init(setmetatable({}, TTFGlyph), id, codePoints, font)
    self.type = "TTF"
    return self
end

--- The five int16s every glyph starts with, simple or composite.
local function decode_header(r)
    return {
        -- Negative means composite. Zero means an outline with no contours,
        -- which is legal and means nothing is drawn.
        numberOfContours = r:int16(),
        xMin = r:int16(),
        yMin = r:int16(),
        xMax = r:int16(),
        yMax = r:int16(),
    }
end

--- A reader over `glyf`, positioned at this glyph's data.
---
--- No bounds check, matching upstream. A zero-length glyph -- a space -- has
--- its offset pointing at wherever the next glyph starts, so a header read here
--- returns the neighbour's numbers rather than failing. Harmless in practice:
--- the only consumer of an empty glyph's box is the vertical metric fallback,
--- which pdfkit does not read.
function TTFGlyph:_glyfReader()
    local r = self._font:_getTableStream("glyf")
    r:skip(self._font.loca.offsets[self.id + 1])
    return r
end

--- The control box, straight out of the glyph header.
---
--- This is why TrueType mark positioning is cheap: the font already stores the
--- box, so nothing has to be decoded to read it. `internal` is set by callers
--- that are inside the decode already and must not recurse back into it.
function TTFGlyph:_getCBox(internal)
    -- Variation processing moves the points, so the stored box is stale and the
    -- outline has to be decoded after all.
    if self._font._variationProcessor and not internal then
        return self:path():cbox()
    end

    local glyph = decode_header(self:_glyfReader())
    return BBox.new(glyph.xMin, glyph.yMin, glyph.xMax, glyph.yMax)
end

--- One delta-encoded coordinate.
---
--- `short` and `same` come from the point's flag byte, and `same` means two
--- different things depending on `short`: with a one-byte value it is the sign,
--- and with a two-byte value it means there is no value at all and the previous
--- coordinate repeats. Not a mistake in the spec -- it is how a run of points
--- on a straight vertical edge costs nothing.
function TTFGlyph:_parseGlyphCoord(r, prev, short, same)
    local val

    if short then
        val = r:uint8()
        if not same then
            val = -val
        end
        val = val + prev
    else
        if same then
            val = prev
        else
            val = prev + r:int16()
        end
    end

    return val
end

--- Decode this glyph's data into points or components, and return its header.
---
--- Returns nil for an empty glyph, which is how a space is stored: loca gives
--- it the same start and end offset.
function TTFGlyph:_decode()
    local offsets = self._font.loca.offsets
    local glyfPos = offsets[self.id + 1]
    local nextPos = offsets[self.id + 2]

    -- Nothing to do if there is no data for this glyph.
    if glyfPos == nextPos then
        return nil
    end

    local r = self:_glyfReader()
    local startPos = r.pos

    local glyph = decode_header(r)

    if glyph.numberOfContours > 0 then
        self:_decodeSimple(glyph, r)
    elseif glyph.numberOfContours < 0 then
        self:_decodeComposite(glyph, r, startPos)
    end

    return glyph
end

--- Read the contours of a simple glyph into `glyph.points`.
function TTFGlyph:_decodeSimple(glyph, r)
    glyph.points = {}

    -- The index of the last point in each contour, so the contour boundaries
    -- are implied rather than stored.
    local endPtsOfContours = r:array(glyph.numberOfContours, r.uint16)

    -- Hinting instructions, kept so a subset can copy them.
    local instructionLength = r:uint16()
    glyph.instructions = r:array(instructionLength, r.uint8)

    local numCoords = endPtsOfContours[#endPtsOfContours] + 1

    -- Flags, one per point, with a run-length escape.
    local flags = {}
    while #flags < numCoords do
        local flag = r:uint8()
        flags[#flags + 1] = flag

        if flag & REPEAT_FLAG ~= 0 then
            local count = r:uint8()
            for _ = 1, count do
                flags[#flags + 1] = flag
            end
        end
    end

    -- Upstream tests contour membership with indexOf on every point, which is
    -- quadratic. A set gives the same answer -- it is a membership test either
    -- way -- without the quadratic.
    local isEnd = {}
    for i = 1, #endPtsOfContours do
        isEnd[endPtsOfContours[i]] = true
    end

    for i = 1, #flags do
        local flag = flags[i]
        glyph.points[i] = Point.new(flag & ON_CURVE ~= 0, isEnd[i - 1] or false, 0, 0)
    end

    -- All the x deltas come first, then all the y deltas -- not interleaved.
    local px = 0
    for i = 1, #flags do
        local flag = flags[i]
        px = self:_parseGlyphCoord(r, px, flag & X_SHORT_VECTOR ~= 0, flag & SAME_X ~= 0)
        glyph.points[i].x = px
    end

    local py = 0
    for i = 1, #flags do
        local flag = flags[i]
        py = self:_parseGlyphCoord(r, py, flag & Y_SHORT_VECTOR ~= 0, flag & SAME_Y ~= 0)
        glyph.points[i].y = py
    end

    if self._font._variationProcessor then
        local points = {}
        for i = 1, #glyph.points do
            points[i] = glyph.points[i]
        end
        for _, p in ipairs(self:_getPhantomPoints(glyph)) do
            points[#points + 1] = p
        end

        self._font._variationProcessor:transformPoints(self.id, points)

        local phantom = {}
        for i = 0, 3 do
            phantom[4 - i] = points[#points - i]
        end
        glyph.phantomPoints = phantom
    end
end

--- Read the component list of a composite glyph into `glyph.components`.
---
--- Returns whether any component carried hinting instructions, which the subset
--- writer needs in order to know whether to copy them.
function TTFGlyph:_decodeComposite(glyph, r, offset)
    offset = offset or 0
    glyph.components = {}

    local haveInstructions = false
    local flags = MORE_COMPONENTS

    while flags & MORE_COMPONENTS ~= 0 do
        flags = r:uint16()

        -- Where this component's glyph id sits, relative to the start of the
        -- glyph. The subset writer patches the id in place at this offset.
        local gPos = r.pos - offset
        local glyphID = r:uint16()

        if not haveInstructions then
            haveInstructions = (flags & WE_HAVE_INSTRUCTIONS) ~= 0
        end

        local dx, dy
        if flags & ARG_1_AND_2_ARE_WORDS ~= 0 then
            dx = r:int16()
            dy = r:int16()
        else
            dx = r:int8()
            dy = r:int8()
        end

        local component = Component.new(glyphID, dx, dy)
        component.pos = gPos

        -- The scale values are F2Dot14 -- two integer bits and fourteen
        -- fractional. Upstream reads them a byte at a time, shifts them into the
        -- top half of a word and divides by 2^30.
        --
        -- That only comes out right because JS's << and | produce a signed
        -- 32-bit result: a scale of -1 is stored as 0xC000, becomes 0xC0000000,
        -- and JS reads that back as -1073741824. Lua's integers are 64 bits
        -- wide and would keep it at +3221225472, turning every mirrored
        -- component into a triple-size one -- so the wrap is done by hand.
        local function f2dot14()
            local v = ((r:uint8() << 24) | (r:uint8() << 16)) & 0xFFFFFFFF
            if v >= 0x80000000 then v = v - 0x100000000 end
            return v / 1073741824
        end

        if flags & WE_HAVE_A_SCALE ~= 0 then
            local s = f2dot14()
            component.scaleX = s
            component.scaleY = s
        elseif flags & WE_HAVE_AN_X_AND_Y_SCALE ~= 0 then
            component.scaleX = f2dot14()
            component.scaleY = f2dot14()
        elseif flags & WE_HAVE_A_TWO_BY_TWO ~= 0 then
            component.scaleX = f2dot14()
            component.scale01 = f2dot14()
            component.scale10 = f2dot14()
            component.scaleY = f2dot14()
        end

        glyph.components[#glyph.components + 1] = component
    end

    if self._font._variationProcessor then
        local points = {}
        for i = 1, #glyph.components do
            local component = glyph.components[i]
            points[i] = Point.new(true, true, component.dx, component.dy)
        end
        for _, p in ipairs(self:_getPhantomPoints(glyph)) do
            points[#points + 1] = p
        end

        self._font._variationProcessor:transformPoints(self.id, points)

        -- The last four points are the phantoms; the rest are the components,
        -- whose offsets the variation may have moved.
        local phantom = {}
        for i = 4, 1, -1 do
            phantom[i] = table.remove(points)
        end
        glyph.phantomPoints = phantom

        for i = 1, #points do
            glyph.components[i].dx = points[i].x
            glyph.components[i].dy = points[i].y
        end
    end

    return haveInstructions
end

--- The four synthetic points a variable font may move: the two horizontal
--- sidebearing points and the two vertical ones. Moving them is how a variation
--- changes a glyph's advance width.
function TTFGlyph:_getPhantomPoints(glyph)
    local cbox = self:_getCBox(true)
    if self._metrics == nil then
        self._metrics = Glyph._getMetrics(self, cbox)
    end

    local m = self._metrics

    return {
        Point.new(false, true, glyph.xMin - m.leftBearing, 0),
        Point.new(false, true, glyph.xMin - m.leftBearing + m.advanceWidth, 0),
        Point.new(false, true, 0, glyph.yMax + m.topBearing),
        Point.new(false, true, 0, glyph.yMax + m.topBearing + m.advanceHeight),
    }
end

--- The glyph as a list of contours, each a list of points, with composites
--- resolved into their components' points.
function TTFGlyph:_getContours()
    local glyph = self:_decode()
    if not glyph then
        return {}
    end

    local points = {}

    if glyph.numberOfContours < 0 then
        -- Resolve composite glyphs. Recursive: a component may itself be
        -- composite, which is how a letter with two accents is built.
        for _, component in ipairs(glyph.components) do
            local contours = self._font:getGlyph(component.glyphID):_getContours()
            for _, contour in ipairs(contours) do
                for _, point in ipairs(contour) do
                    local x = point.x * component.scaleX + point.y * component.scale01 + component.dx
                    local y = point.y * component.scaleY + point.x * component.scale10 + component.dy
                    points[#points + 1] = Point.new(point.onCurve, point.endContour, x, y)
                end
            end
        end
    else
        points = glyph.points or {}
    end

    -- Recompute and cache metrics if we performed variation processing, and
    -- don't have an HVAR table -- in which case the moved phantom points are
    -- the only record of the new advance.
    if glyph.phantomPoints and not self._font.directory.tables.HVAR then
        local p = glyph.phantomPoints
        self._metrics.advanceWidth = p[2].x - p[1].x
        self._metrics.advanceHeight = p[4].y - p[3].y
        self._metrics.leftBearing = glyph.xMin - p[1].x
        self._metrics.topBearing = p[3].y - glyph.yMax
    end

    local contours = {}
    local cur = {}
    for _, point in ipairs(points) do
        cur[#cur + 1] = point
        if point.endContour then
            contours[#contours + 1] = cur
            cur = {}
        end
    end

    return contours
end

--- Metrics, with the control box taken from the header rather than the outline.
function TTFGlyph:_getMetrics()
    if self._metrics then
        return self._metrics
    end

    local cbox = self:_getCBox(true)
    Glyph._getMetrics(self, cbox)

    if self._font._variationProcessor and not self._font.HVAR then
        -- No HVAR table, decode the glyph. This triggers recomputation of
        -- metrics in _getContours.
        self:path()
    end

    return self._metrics
end

--- Turn the contours into a drawable path.
---
--- The work here is the implied on-curve points. A contour is a cycle of
--- points, each on or off the curve; two adjacent off-curve points have an
--- implied on-curve point midway between them, and the contour may start on
--- either kind.
function TTFGlyph:_getPath()
    local contours = self:_getContours()
    local path = Path.new()

    for _, contour in ipairs(contours) do
        local firstPt = contour[1]
        local lastPt = contour[#contour]
        local start = 1
        local curvePt

        if firstPt.onCurve then
            -- The first point will be consumed by the moveTo command, so skip
            -- it in the loop.
            curvePt = nil
            start = 2
        else
            if lastPt.onCurve then
                -- Start at the last point if the first point is off curve and
                -- the last point is on curve.
                firstPt = lastPt
            else
                -- Start at the middle if both the first and last points are off
                -- curve -- the implied point between them.
                firstPt = Point.new(false, false, (firstPt.x + lastPt.x) / 2, (firstPt.y + lastPt.y) / 2)
            end

            curvePt = firstPt
        end

        path:moveTo(firstPt.x, firstPt.y)

        for j = start, #contour do
            local pt = contour[j]
            -- At j == 1 the previous point is the synthesised start, not the
            -- contour's last point. Only reachable when the contour began off
            -- curve, since otherwise the loop starts at 2.
            local prevPt = (j == 1) and firstPt or contour[j - 1]

            if prevPt.onCurve and pt.onCurve then
                path:lineTo(pt.x, pt.y)
            elseif prevPt.onCurve and not pt.onCurve then
                -- Remember the control point; the curve is emitted when the
                -- next on-curve point arrives.
                curvePt = pt
            elseif not prevPt.onCurve and not pt.onCurve then
                -- Two control points in a row: close off the first curve at the
                -- implied midpoint and start the next.
                local midX = (prevPt.x + pt.x) / 2
                local midY = (prevPt.y + pt.y) / 2
                path:quadraticCurveTo(prevPt.x, prevPt.y, midX, midY)
                curvePt = pt
            elseif not prevPt.onCurve and pt.onCurve then
                path:quadraticCurveTo(curvePt.x, curvePt.y, pt.x, pt.y)
                curvePt = nil
            else
                error("Unknown TTF path state")
            end
        end

        -- Connect the first and last points.
        if curvePt then
            path:quadraticCurveTo(curvePt.x, curvePt.y, firstPt.x, firstPt.y)
        end

        path:closePath()
    end

    return path
end

TTFGlyph.Point = Point
TTFGlyph.Component = Component

return TTFGlyph
