-- fontkit/glyph/TTFGlyphEncoder.lua
--   -- port of fontkit/src/glyph/TTFGlyphEncoder.js
--
-- The inverse of TTFGlyph's decoder: a path back into the bytes a `glyf` entry
-- holds. Contour ends, a flag per point, then the x deltas and the y deltas as
-- two separate runs.
--
-- Only one caller, and it is conditional -- TTFSubset re-encodes an outline when
-- the source is a variable font, because there the stored outline is the default
-- instance and the document was laid out with the points moved. Every other
-- glyph is copied through as bytes and never comes near this file. Since the
-- variation processor is not ported, nothing reaches here yet.
--
-- Worth being plain about: the upstream version of this function does not run
-- either. Its last few lines build a restructure EncodeStream from a byte count,
-- and restructure has taken a buffer rather than a length since version 3. So
-- the shape below follows the source's intent -- allocate, encode, pad -- rather
-- than transcribing a line that would throw.
--
-- Three details in here are worth reading twice, because they are what make the
-- output smaller than the input rather than larger:
--
--   * A quadratic curve's end point is dropped when it is the midpoint between
--     its own control point and the next curve's. TrueType reconstructs it, and
--     in a smooth outline nearly every end point qualifies.
--   * A flag byte that repeats sets a REPEAT bit and is followed by a count, so
--     a run of forty identical points costs two bytes.
--   * A coordinate delta in -255..255 goes out as one unsigned byte with its
--     sign in the flags.

local writer = require("pdfkit.fontkit.writer")

-- Flags for simple glyphs.
local ON_CURVE       = 1 << 0
local X_SHORT_VECTOR = 1 << 1
local Y_SHORT_VECTOR = 1 << 2
local REPEAT_FLAG    = 1 << 3
local SAME_X         = 1 << 4
local SAME_Y         = 1 << 5

local TTFGlyphEncoder = {}
TTFGlyphEncoder.__index = TTFGlyphEncoder

function TTFGlyphEncoder.new()
    return setmetatable({}, TTFGlyphEncoder)
end

-- A coordinate delta is one byte when it fits unsigned, two otherwise. The
-- one-byte case is only ever reached for a delta the flags marked short, which
-- is also where the value was made positive -- so the test here and the test in
-- _encodePoint agree without either knowing about the other.
local function point_size(val)
    return (val >= 0 and val <= 255) and 1 or 2
end

local function point_encode(w, val)
    if val >= 0 and val <= 255 then
        w:uint8(val)
    else
        w:int16(val)
    end
end

--- Turn one coordinate into a delta, a flag, and possibly nothing at all.
---
--- `shortFlag` and `sameFlag` are the x or y pair; the same code runs twice per
--- point. Note what `sameFlag` means in each branch: on an unchanged coordinate
--- it says "no value follows", and on a short one it says "the byte that follows
--- is positive". One bit, two jobs, which is the format's own compression.
function TTFGlyphEncoder:_encodePoint(value, last, points, flag, shortFlag, sameFlag)
    local diff = value - last

    if value == last then
        flag = flag | sameFlag
    else
        if -255 <= diff and diff <= 255 then
            flag = flag | shortFlag
            if diff < 0 then
                diff = -diff
            else
                flag = flag | sameFlag
            end
        end

        points[#points + 1] = diff
    end

    return flag
end

--- Encode a simple (non-composite) outline. Returns the glyph's bytes.
---
--- `instructions` is the hinting program to keep, which the caller took off the
--- decoded glyph it is replacing.
function TTFGlyphEncoder:encodeSimple(path, instructions)
    instructions = instructions or {}

    local endPtsOfContours = {}
    local xPoints = {}
    local yPoints = {}
    local flags = {}
    local same = 0
    local lastX, lastY, lastFlag = 0, 0, 0
    local pointCount = 0

    local commands = path.commands

    for i = 1, #commands do
        local c = commands[i]
        local args = c.args

        local j = 1
        while j <= #args do
            local x = args[j]
            local y = args[j + 1]
            local flag = 0
            local skip = false

            -- If the ending point of a quadratic curve is the midpoint between
            -- the control point and the control point of the next quadratic
            -- curve, it can be omitted -- the rasteriser puts it back.
            if c.command == "quadraticCurveTo" and j == 3 then
                local next_ = commands[i + 1]
                if next_ and next_.command == "quadraticCurveTo" then
                    local midX = (lastX + next_.args[1]) / 2
                    local midY = (lastY + next_.args[2]) / 2

                    if x == midX and y == midY then
                        skip = true
                    end
                end
            end

            if not skip then
                -- All points except control points are on curve.
                if not (c.command == "quadraticCurveTo" and j == 1) then
                    flag = flag | ON_CURVE
                end

                flag = self:_encodePoint(x, lastX, xPoints, flag, X_SHORT_VECTOR, SAME_X)
                flag = self:_encodePoint(y, lastY, yPoints, flag, Y_SHORT_VECTOR, SAME_Y)

                -- A repeated flag sets the REPEAT bit on the byte already
                -- written and bumps a counter, which is flushed as the byte
                -- after it the moment the run breaks. The 255 cap is the
                -- counter's width.
                if flag == lastFlag and same < 255 then
                    flags[#flags] = flags[#flags] | REPEAT_FLAG
                    same = same + 1
                else
                    if same > 0 then
                        flags[#flags + 1] = same
                        same = 0
                    end

                    flags[#flags + 1] = flag
                    lastFlag = flag
                end

                lastX = x
                lastY = y
                pointCount = pointCount + 1
            end

            j = j + 2
        end

        if c.command == "closePath" then
            endPtsOfContours[#endPtsOfContours + 1] = pointCount - 1
        end
    end

    -- Close the path if the last command didn't already.
    if #commands > 1 and commands[#commands].command ~= "closePath" then
        endPtsOfContours[#endPtsOfContours + 1] = pointCount - 1
    end

    -- The stored bounding box, which is the exact one rather than the control
    -- box: the points have moved, so the source font's box no longer describes
    -- them. It goes out as int16, so a box computed a fraction outside the
    -- integer grid is truncated toward zero here.
    local bbox = path:bbox()

    local size = 10                                 -- the five int16 header fields
        + 2 * #endPtsOfContours
        + 2 + #instructions                         -- a uint16 count, then bytes
        + #flags

    for _, v in ipairs(xPoints) do size = size + point_size(v) end
    for _, v in ipairs(yPoints) do size = size + point_size(v) end

    local w = writer.Writer.new()

    w:int16(#endPtsOfContours)                      -- negative would mean composite
    w:int16(bbox.minX)
    w:int16(bbox.minY)
    w:int16(bbox.maxX)
    w:int16(bbox.maxY)

    w:array(endPtsOfContours, w.uint16)

    w:uint16(#instructions)
    w:array(instructions, w.uint8)

    w:array(flags, w.uint8)
    w:array(xPoints, point_encode)
    w:array(yPoints, point_encode)

    -- Align to a 4-byte length. Note that an already-aligned glyph gets four
    -- bytes of padding rather than none -- `4 - (size % 4)` is never zero. That
    -- is upstream's arithmetic and it is harmless: the padding is inside the
    -- length loca records, and a reader stops at the last contour's end point.
    local tail = 4 - (size % 4)
    if tail ~= 0 then
        w:fill(0, tail)
    end

    return w:tostring()
end

return TTFGlyphEncoder
