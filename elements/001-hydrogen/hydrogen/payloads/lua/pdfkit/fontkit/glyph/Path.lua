-- fontkit/glyph/Path.lua -- port of fontkit/src/glyph/Path.js
--
-- A glyph outline as a list of drawing commands. TrueType outlines arrive as
-- quadratic curves and PostScript ones as cubics, so both are kept rather than
-- converted to one form.
--
-- Two boxes are available. `cbox` is the hull of every coordinate mentioned by
-- the path including off-curve control points, which makes it a cheap
-- overestimate. `bbox` solves each curve for its extrema and is exact. Mark
-- positioning uses the control box, which is why it is the one that gets called
-- thousands of times.
--
-- toFunction() is not ported: it compiles the path against a canvas context,
-- and there is no canvas here.

local BBox = require("pdfkit.fontkit.glyph.BBox")
local pow = require("pdfkit.jsmath").pow

local Path = {}
Path.__index = Path

local SVG_COMMANDS = {
    moveTo = "M",
    lineTo = "L",
    quadraticCurveTo = "Q",
    bezierCurveTo = "C",
    closePath = "Z",
}

function Path.new()
    return setmetatable({ commands = {}, _bbox = nil, _cbox = nil }, Path)
end

--------------------------------------------------------------------------------
-- Building
--------------------------------------------------------------------------------

-- The five drawing commands, defined in a loop upstream and here for the same
-- reason: they differ only in the name they record. Each invalidates the cached
-- boxes, since a path can be drawn into after it has been measured.
for _, name in ipairs({ "moveTo", "lineTo", "quadraticCurveTo", "bezierCurveTo", "closePath" }) do
    Path[name] = function(self, ...)
        self._bbox = nil
        self._cbox = nil
        self.commands[#self.commands + 1] = { command = name, args = { ... } }
        return self
    end
end

--- The path as an SVG `d` attribute.
---
--- Coordinates are rounded to two decimals, which is upstream's compromise
--- between file size and precision at any sane rendering scale.
function Path:toSVG()
    local out = {}

    for i, c in ipairs(self.commands) do
        local args = {}
        for j, arg in ipairs(c.args) do
            -- Round half away from zero, then drop a trailing `.0` -- what
            -- Math.round(x * 100) / 100 followed by string coercion produces.
            local v = math.floor(arg * 100 + 0.5) / 100
            if v == math.floor(v) then
                args[j] = string.format("%d", v)
            else
                args[j] = tostring(v)
            end
        end
        out[i] = SVG_COMMANDS[c.command] .. table.concat(args, " ")
    end

    return table.concat(out)
end

--------------------------------------------------------------------------------
-- Measurement
--------------------------------------------------------------------------------

--- The control box: every coordinate the path mentions, control points and all.
---
--- Cached. Arguments are read in pairs, which is why closePath -- with no
--- arguments at all -- contributes nothing.
function Path:cbox()
    if not self._cbox then
        local cbox = BBox.new()
        for _, command in ipairs(self.commands) do
            local args = command.args
            for i = 1, #args, 2 do
                cbox:addPoint(args[i], args[i + 1])
            end
        end
        self._cbox = cbox
    end

    return self._cbox
end

--- The exact bounding box, by solving each curve segment for its extrema.
---
--- Every curve is treated as a cubic; a quadratic is raised to one first with
--- the standard control-point conversion. For each axis the derivative is a
--- quadratic in t whose roots inside (0, 1) are where the curve turns, and each
--- of those points is added to the box along with the segment's end point.
function Path:bbox()
    if self._bbox then
        return self._bbox
    end

    local bbox = BBox.new()
    local cx, cy = 0, 0

    -- The four control points of the segment under consideration, as {x, y}.
    -- `f` evaluates the cubic at t on axis `i`, reading whichever segment is
    -- current -- upstream declares it once outside the loop and lets it close
    -- over the loop's variables, and the same shape is kept here.
    local p0, p1, p2, p3, axis

    --
    -- Every detail of this expression is upstream's rather than a stylistic
    -- choice, because the last bit of the answer follows all of them. `3 * u^2
    -- * t` and `3 * u * u * t` multiply the same three numbers but round
    -- between different pairs of them; and jsmath.pow is here instead of Lua's
    -- `^` because the two disagree on the last bit for one input in two
    -- hundred. Four of the thirteen test fonts disagreed with fontkit in the
    -- fifteenth digit until both of those matched.
    local function f(t)
        local u = 1 - t
        return pow(u, 3) * p0[axis]
            + 3 * pow(u, 2) * t * p1[axis]
            + 3 * u * pow(t, 2) * p2[axis]
            + pow(t, 3) * p3[axis]
    end

    for _, cmd in ipairs(self.commands) do
        local name = cmd.command
        local args = cmd.args

        if name == "moveTo" or name == "lineTo" then
            local x, y = args[1], args[2]
            bbox:addPoint(x, y)
            cx, cy = x, y
        elseif name == "quadraticCurveTo" or name == "bezierCurveTo" then
            local cp1x, cp1y, cp2x, cp2y, p3x, p3y

            if name == "quadraticCurveTo" then
                -- Raise the quadratic to a cubic: the two cubic control points
                -- sit two thirds of the way from each end towards the single
                -- quadratic control point. See http://fontforge.org/bezier.html
                local qp1x, qp1y
                qp1x, qp1y, p3x, p3y = args[1], args[2], args[3], args[4]
                cp1x = cx + 2 / 3 * (qp1x - cx)
                cp1y = cy + 2 / 3 * (qp1y - cy)
                cp2x = p3x + 2 / 3 * (qp1x - p3x)
                cp2y = p3y + 2 / 3 * (qp1y - p3y)
            else
                cp1x, cp1y, cp2x, cp2y, p3x, p3y = args[1], args[2], args[3], args[4], args[5], args[6]
            end

            -- The end point is always on the curve.
            bbox:addPoint(p3x, p3y)

            p0 = { cx, cy }
            p1 = { cp1x, cp1y }
            p2 = { cp2x, cp2y }
            p3 = { p3x, p3y }

            for i = 1, 2 do
                axis = i

                -- Coefficients of the derivative, at² + bt + c.
                local b = 6 * p0[i] - 12 * p1[i] + 6 * p2[i]
                local a = -3 * p0[i] + 9 * p1[i] - 9 * p2[i] + 3 * p3[i]
                local c = 3 * p1[i] - 3 * p0[i]

                -- The other axis is passed as the box's current maximum, which
                -- addPoint ignores when it is still infinite. Upstream does the
                -- same; the effect is "leave that axis alone".
                local function add(t)
                    if 0 < t and t < 1 then
                        if i == 1 then
                            bbox:addPoint(f(t), bbox.maxY)
                        else
                            bbox:addPoint(bbox.maxX, f(t))
                        end
                    end
                end

                if a == 0 then
                    -- Degenerate: the derivative is linear, so at most one root.
                    if b ~= 0 then
                        add(-c / b)
                    end
                else
                    local b2ac = pow(b, 2) - 4 * c * a
                    if b2ac >= 0 then
                        local root = math.sqrt(b2ac)
                        add((-b + root) / (2 * a))
                        add((-b - root) / (2 * a))
                    end
                end
            end

            cx, cy = p3x, p3y
        end
    end

    self._bbox = bbox
    return bbox
end

--------------------------------------------------------------------------------
-- Transforms
--------------------------------------------------------------------------------

--- A new path with `fn(x, y)` applied to every coordinate pair.
function Path:mapPoints(fn)
    local path = Path.new()

    for _, c in ipairs(self.commands) do
        local args = {}
        local n = 0
        for i = 1, #c.args, 2 do
            local x, y = fn(c.args[i], c.args[i + 1])
            args[n + 1] = x
            args[n + 2] = y
            n = n + 2
        end
        path[c.command](path, table.unpack(args, 1, n))
    end

    return path
end

--- A new path under the affine matrix [m0 m1 m2 m3 m4 m5].
function Path:transform(m0, m1, m2, m3, m4, m5)
    return self:mapPoints(function(x, y)
        return m0 * x + m2 * y + m4, m1 * x + m3 * y + m5
    end)
end

function Path:translate(x, y)
    return self:transform(1, 0, 0, 1, x, y)
end

--- Rotate by `angle` radians, counter-clockwise.
function Path:rotate(angle)
    local cos = math.cos(angle)
    local sin = math.sin(angle)
    return self:transform(cos, sin, -sin, cos, 0, 0)
end

--- Scale. One argument scales both axes.
function Path:scale(scaleX, scaleY)
    scaleY = scaleY or scaleX
    return self:transform(scaleX, 0, 0, scaleY, 0, 0)
end

return Path
