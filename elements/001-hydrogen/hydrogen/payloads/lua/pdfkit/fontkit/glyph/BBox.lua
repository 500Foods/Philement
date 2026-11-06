-- fontkit/glyph/BBox.lua -- port of fontkit/src/glyph/BBox.js
--
-- A rectangle that grows to contain the points fed to it. Starts inverted --
-- minima at +infinity, maxima at -infinity -- so the first point sets all four
-- edges, and an empty box reports a negative width rather than pretending to be
-- a point at the origin.

local BBox = {}
BBox.__index = BBox

local INF = math.huge

--- A new box. With no arguments, the empty one that swallows its first point.
function BBox.new(minX, minY, maxX, maxY)
    return setmetatable({
        minX = minX or INF,
        minY = minY or INF,
        maxX = maxX or -INF,
        maxY = maxY or -INF,
    }, BBox)
end

--- Width and height are getters upstream. Lua has no property syntax, so they
--- are methods here and every call site says `:width()`.
function BBox:width()
    return self.maxX - self.minX
end

function BBox:height()
    return self.maxY - self.minY
end

--- Grow to include (x, y).
---
--- Infinities are ignored on each axis independently. That is not defensive
--- coding: the bounding box walk in Path passes the box's own current maximum
--- as the coordinate it is not solving for, and on the first curve that
--- maximum is still -infinity.
function BBox:addPoint(x, y)
    if math.abs(x) ~= INF then
        if x < self.minX then self.minX = x end
        if x > self.maxX then self.maxX = x end
    end

    if math.abs(y) ~= INF then
        if y < self.minY then self.minY = y end
        if y > self.maxY then self.maxY = y end
    end
end

function BBox:copy()
    return BBox.new(self.minX, self.minY, self.maxX, self.maxY)
end

return BBox
