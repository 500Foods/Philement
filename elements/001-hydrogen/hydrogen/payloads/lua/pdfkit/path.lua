-- path.lua -- port of pdfkit/lib/path.js
--
-- Runs an SVG path string -- "M 10 10 L 50 50 A 20 20 0 0 1 90 10 Z" -- against
-- the document's drawing operators. Two halves: a parser that turns the string
-- into a list of commands, and a set of runners that each translate one command
-- into moveTo/lineTo/bezierCurveTo/closePath calls.
--
-- PDF HAS NO ARCS. It has cubic Béziers and nothing else, so every elliptical
-- arc is approximated: split into pieces of at most a quarter turn, and each
-- piece replaced by a cubic curve chosen to match the ellipse at both ends and
-- in tangent direction. The error over a quarter turn is around one part in
-- 10^4 of the radius, which is invisible at any print resolution. That is what
-- arc_to_segments and segment_to_bezier do, and the constant 8/3 in the latter
-- is where the approximation lives.
--
-- MODULE-LEVEL STATE. The original keeps the current point (cx, cy), the last
-- control point (px, py) and the start of the current subpath (sx, sy) as
-- file-scope variables rather than passing them around, and `apply` resets them
-- before each run. This does the same, for one reason that matters:
-- arc_to_segments *also assigns to px and py* -- see the comment there -- and
-- reproducing that requires them to be the same variables.
--
-- A NOTE ON NUMBERS. JavaScript has one numeric type and it is a double. Lua
-- distinguishes integers from floats, so read_number converts, keeping the
-- arithmetic below in the same domain as the original's. Nothing downstream can
-- tell the difference today -- utils.number formats both identically -- but the
-- division and rounding in arc_to_segments is the kind of code where an
-- unexpected integer eventually does surprise someone.

local SVGPath = {}

-- Current point, last control point, and subpath starting point.
local cx, cy, px, py, sx, sy
cx, cy, px, py, sx, sy = 0, 0, 0, 0, 0, 0

--------------------------------------------------------------------------------
-- Parsing
--------------------------------------------------------------------------------
--
-- Based on the path parser from SVGO -- https://github.com/svg/svgo/blob/main/lib/path.js
-- License: MIT

-- How many numbers each command takes. Membership doubles as "is this a
-- command?", which is why the zero-argument closepath is listed.
local parameters = {
    A = 7, a = 7,
    C = 6, c = 6,
    H = 1, h = 1,
    L = 2, l = 2,
    M = 2, m = 2,
    Q = 4, q = 4,
    S = 4, s = 4,
    T = 2, t = 2,
    V = 1, v = 1,
    Z = 0, z = 0,
}

local function is_command(c)
    return parameters[c] ~= nil
end

-- The four characters SVG counts as whitespace between path arguments: space,
-- tab, carriage return, line feed. Nothing else, and notably not a vertical tab
-- or a form feed.
local function is_wsp(c)
    local b = c:byte()
    return b == 0x20 or b == 0x9 or b == 0xd or b == 0xa
end

local function is_digit(c)
    local b = c:byte()
    if b == nil then return false end
    return b >= 48 and b <= 57
end

-- JavaScript's Number.parseFloat, which takes the longest valid numeric prefix
-- and ignores the rest. The state machine below already restricts the candidate
-- to [+-0-9.eE], so the only prefixes needing a trim are the ones it
-- deliberately allows to end mid-exponent: "1e" and "1e-" are both 1 to
-- parseFloat, while Lua's tonumber rejects them outright.
local function parse_float(value)
    local n = tonumber(value)
    if n then return n + 0.0 end
    for i = #value - 1, 1, -1 do
        n = tonumber(value:sub(1, i))
        if n then return n + 0.0 end
    end
    return nil
end

-- Read one number starting at `cursor`. Returns the index of its last character
-- and the value, or (cursor, nil) if there is no number there.
--
-- The state machine is what makes "1.5.5" two numbers rather than one error: a
-- second decimal point is not accepted in the `decimal` state, so the scan stops
-- and the parent loop starts a fresh number at the point. SVG path data relies
-- on that -- "M.5.5" is a perfectly ordinary moveto.
local function read_number(s, cursor)
    local i = cursor
    local value = {}
    local state = "none"

    while i <= #s do
        local c = s:sub(i, i)
        local consumed = false

        if c == "+" or c == "-" then
            if state == "none" then
                state = "sign"
                value[#value + 1] = c
                consumed = true
            elseif state == "e" then
                state = "exponent_sign"
                value[#value + 1] = c
                consumed = true
            end
        end

        if not consumed and is_digit(c) then
            if state == "none" or state == "sign" or state == "whole" then
                state = "whole"
                value[#value + 1] = c
                consumed = true
            elseif state == "decimal_point" or state == "decimal" then
                state = "decimal"
                value[#value + 1] = c
                consumed = true
            elseif state == "e" or state == "exponent_sign" or state == "exponent" then
                state = "exponent"
                value[#value + 1] = c
                consumed = true
            end
        end

        if not consumed and c == "." then
            if state == "none" or state == "sign" or state == "whole" then
                state = "decimal_point"
                value[#value + 1] = c
                consumed = true
            end
        end

        if not consumed and (c == "E" or c == "e") then
            if state == "whole" or state == "decimal_point" or state == "decimal" then
                state = "e"
                value[#value + 1] = c
                consumed = true
            end
        end

        if not consumed then break end
        i = i + 1
    end

    local number = parse_float(table.concat(value))
    if number == nil then
        return cursor, nil
    end

    -- Step back to delegate iteration to the parent loop, which will advance
    -- past this character itself.
    return i - 1, number
end

-- Turn a path string into a list of { command = "L", args = { 10, 20 } }.
--
-- Malformed input is not an error: the parser returns everything it understood
-- up to the problem and drops the rest, which is what SVG renderers do. So a
-- path with a typo halfway through still draws its first half.
local function parse(path)
    local pathData = {}
    local command = nil
    local args = {}
    local argsCount = 0
    local canHaveComma = false
    local hadComma = false

    local i = 1
    while i <= #path do
        local c = path:sub(i, i)

        if is_wsp(c) then
            goto continue
        end

        -- Allow a comma only between arguments, and only one.
        if canHaveComma and c == "," then
            if hadComma then break end
            hadComma = true
            goto continue
        end

        if is_command(c) then
            if hadComma then return pathData end

            if command == nil then
                -- A moveto must be the leading command.
                if c ~= "M" and c ~= "m" then return pathData end
            else
                -- Stop if the previous command's arguments are not flushed.
                if #args ~= 0 then return pathData end
            end

            command = c
            args = {}
            argsCount = parameters[command]
            canHaveComma = false

            -- Flush a command that takes no arguments.
            if argsCount == 0 then
                pathData[#pathData + 1] = { command = command, args = args }
            end
            goto continue
        end

        -- Avoid parsing arguments if no command has been detected.
        if command == nil then return pathData end

        do
            local newCursor = i
            local number = nil

            if command == "A" or command == "a" then
                -- The arc command's seven arguments are not all plain numbers,
                -- so each position is read by its own rule.
                local position = #args

                if position == 0 or position == 1 then
                    -- Radii: a positive number, and a sign is not allowed --
                    -- "A-5" is the flag-packing ambiguity SVG chose to forbid.
                    if c ~= "+" and c ~= "-" then
                        newCursor, number = read_number(path, i)
                    end
                end
                if position == 2 or position == 5 or position == 6 then
                    -- Rotation and the end point: ordinary numbers.
                    newCursor, number = read_number(path, i)
                end
                if position == 3 or position == 4 then
                    -- The large-arc and sweep flags are single characters, so
                    -- "0110" is four flags and not the number one hundred and
                    -- ten. This is why arcs need a special case at all.
                    if c == "0" then number = 0 end
                    if c == "1" then number = 1 end
                end
            else
                newCursor, number = read_number(path, i)
            end

            if number == nil then return pathData end

            args[#args + 1] = number
            canHaveComma = true
            hadComma = false
            i = newCursor

            -- Flush the arguments when the necessary count is reached.
            if #args == argsCount then
                pathData[#pathData + 1] = { command = command, args = args }

                -- Subsequent moveto coordinates are treated as implicit linetos,
                -- so "M 0 0 10 10" draws a line rather than jumping twice.
                if command == "M" then command = "L" end
                if command == "m" then command = "l" end

                args = {}
            end
        end

        ::continue::
        i = i + 1
    end

    return pathData
end

--------------------------------------------------------------------------------
-- Arc approximation
--------------------------------------------------------------------------------

-- Break an elliptical arc into pieces of at most a quarter turn, each described
-- by the centre, start and end angles, radii and the rotation's sine and cosine.
--
-- From Inkscape's svgtopdf, thanks!
--
-- THIS FUNCTION WRITES TO px AND py. In the original those names are declared
-- at file scope for the *smooth curve* control point, and this function assigns
-- to them without a `let` -- so an arc leaves its own half-vector behind in the
-- variables that a following S, s, T or t will reflect. That is upstream
-- behaviour and it is observable in the output, so it is reproduced here rather
-- than quietly localised. If it is ever fixed upstream, fix it here too, and not
-- before.
local function arc_to_segments(x, y, rx, ry, large, sweep, rotateX, ox, oy)
    local th = rotateX * (math.pi / 180)
    local sin_th = math.sin(th)
    local cos_th = math.cos(th)
    rx = math.abs(rx)
    ry = math.abs(ry)

    px = cos_th * (ox - x) * 0.5 + sin_th * (oy - y) * 0.5
    py = cos_th * (oy - y) * 0.5 - sin_th * (ox - x) * 0.5

    -- If the requested radii are too small to reach from start to end, scale
    -- them up until they just do. The spec asks for exactly this.
    local pl = (px * px) / (rx * rx) + (py * py) / (ry * ry)
    if pl > 1 then
        pl = math.sqrt(pl)
        rx = rx * pl
        ry = ry * pl
    end

    -- Map both endpoints into a space where the ellipse is the unit circle;
    -- finding the centre is then a matter of circle geometry.
    local a00 = cos_th / rx
    local a01 = sin_th / rx
    local a10 = -sin_th / ry
    local a11 = cos_th / ry
    local x0 = a00 * ox + a01 * oy
    local y0 = a10 * ox + a11 * oy
    local x1 = a00 * x + a01 * y
    local y1 = a10 * x + a11 * y

    local d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)
    local sfactor_sq = 1 / d - 0.25
    if sfactor_sq < 0 then
        sfactor_sq = 0
    end
    local sfactor = math.sqrt(sfactor_sq)

    -- Two circles pass through both points; the flags pick which one, and which
    -- way round it is travelled.
    if sweep == large then
        sfactor = -sfactor
    end

    local xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0)
    local yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0)

    -- Two arguments to math.atan is Lua's atan2.
    local th0 = math.atan(y0 - yc, x0 - xc)
    local th1 = math.atan(y1 - yc, x1 - xc)

    local th_arc = th1 - th0
    if th_arc < 0 and sweep == 1 then
        th_arc = th_arc + 2 * math.pi
    elseif th_arc > 0 and sweep == 0 then
        th_arc = th_arc - 2 * math.pi
    end

    -- At most a quarter turn per piece. The 0.001 keeps an exact quarter turn
    -- from rounding up to two pieces.
    local segments = math.ceil(math.abs(th_arc / (math.pi * 0.5 + 0.001)))
    local result = {}

    for i = 0, segments - 1 do
        local th2 = th0 + (i * th_arc) / segments
        local th3 = th0 + ((i + 1) * th_arc) / segments
        result[i + 1] = { xc, yc, th2, th3, rx, ry, sin_th, cos_th }
    end

    return result
end

-- One unit-circle arc from th0 to th1, as a cubic Bézier, mapped back out
-- through the ellipse's radii and rotation.
--
-- The control points sit along the tangents at each end, at a distance t chosen
-- so the curve's midpoint lands on the circle. Solving that gives
-- t = (8/3) sin²(θ/4) / sin(θ/2), which is the expression below.
local function segment_to_bezier(pcx, pcy, th0, th1, rx, ry, sin_th, cos_th)
    local a00 = cos_th * rx
    local a01 = -sin_th * ry
    local a10 = sin_th * rx
    local a11 = cos_th * ry

    local th_half = 0.5 * (th1 - th0)
    local t = ((8 / 3) * math.sin(th_half * 0.5) * math.sin(th_half * 0.5))
        / math.sin(th_half)

    local x1 = pcx + math.cos(th0) - t * math.sin(th0)
    local y1 = pcy + math.sin(th0) + t * math.cos(th0)
    local x3 = pcx + math.cos(th1)
    local y3 = pcy + math.sin(th1)
    local x2 = x3 + t * math.sin(th1)
    local y2 = y3 - t * math.cos(th1)

    return {
        a00 * x1 + a01 * y1,
        a10 * x1 + a11 * y1,
        a00 * x2 + a01 * y2,
        a10 * x2 + a11 * y2,
        a00 * x3 + a01 * y3,
        a10 * x3 + a11 * y3,
    }
end

local function solve_arc(doc, x, y, coords)
    local rx, ry, rot = coords[1], coords[2], coords[3]
    local large, sweep = coords[4], coords[5]
    local ex, ey = coords[6], coords[7]

    local segs = arc_to_segments(ex, ey, rx, ry, large, sweep, rot, x, y)

    for _, seg in ipairs(segs) do
        local bez = segment_to_bezier(
            seg[1], seg[2], seg[3], seg[4], seg[5], seg[6], seg[7], seg[8]
        )
        doc:bezierCurveTo(bez[1], bez[2], bez[3], bez[4], bez[5], bez[6])
    end
end

--------------------------------------------------------------------------------
-- Runners
--------------------------------------------------------------------------------
--
-- One per command letter. Uppercase means absolute coordinates, lowercase means
-- relative to the current point -- which is the only difference between each
-- pair, and why they are written out separately rather than folded together.
--
-- Setting px and py to nil means "there is no previous control point", which is
-- what S and T test to decide whether to reflect or to start flat.

local runners = {}

function runners.M(doc, a)
    cx = a[1]
    cy = a[2]
    px, py = nil, nil
    sx = cx
    sy = cy
    return doc:moveTo(cx, cy)
end

function runners.m(doc, a)
    cx = cx + a[1]
    cy = cy + a[2]
    px, py = nil, nil
    sx = cx
    sy = cy
    return doc:moveTo(cx, cy)
end

function runners.C(doc, a)
    cx = a[5]
    cy = a[6]
    px = a[3]
    py = a[4]
    return doc:bezierCurveTo(a[1], a[2], a[3], a[4], a[5], a[6])
end

function runners.c(doc, a)
    doc:bezierCurveTo(
        a[1] + cx, a[2] + cy,
        a[3] + cx, a[4] + cy,
        a[5] + cx, a[6] + cy
    )
    px = cx + a[3]
    py = cy + a[4]
    cx = cx + a[5]
    cy = cy + a[6]
    return cy
end

-- Smooth cubic: the first control point is the reflection of the previous
-- curve's second one. With no previous curve there is nothing to reflect, so it
-- collapses onto the current point and the curve leaves flat.
function runners.S(doc, a)
    if px == nil then
        px = cx
        py = cy
    end

    doc:bezierCurveTo(cx - (px - cx), cy - (py - cy), a[1], a[2], a[3], a[4])
    px = a[1]
    py = a[2]
    cx = a[3]
    cy = a[4]
    return cy
end

function runners.s(doc, a)
    if px == nil then
        px = cx
        py = cy
    end

    doc:bezierCurveTo(
        cx - (px - cx), cy - (py - cy),
        cx + a[1], cy + a[2],
        cx + a[3], cy + a[4]
    )
    px = cx + a[1]
    py = cy + a[2]
    cx = cx + a[3]
    cy = cy + a[4]
    return cy
end

function runners.Q(doc, a)
    px = a[1]
    py = a[2]
    cx = a[3]
    cy = a[4]
    return doc:quadraticCurveTo(a[1], a[2], cx, cy)
end

function runners.q(doc, a)
    doc:quadraticCurveTo(a[1] + cx, a[2] + cy, a[3] + cx, a[4] + cy)
    px = cx + a[1]
    py = cy + a[2]
    cx = cx + a[3]
    cy = cy + a[4]
    return cy
end

-- Smooth quadratic. Note that px and py are reflected, used, and then reflected
-- *again* before the current point moves -- which is what the original does, so
-- it is what this does.
function runners.T(doc, a)
    if px == nil then
        px = cx
        py = cy
    else
        px = cx - (px - cx)
        py = cy - (py - cy)
    end

    doc:quadraticCurveTo(px, py, a[1], a[2])
    px = cx - (px - cx)
    py = cy - (py - cy)
    cx = a[1]
    cy = a[2]
    return cy
end

function runners.t(doc, a)
    if px == nil then
        px = cx
        py = cy
    else
        px = cx - (px - cx)
        py = cy - (py - cy)
    end

    doc:quadraticCurveTo(px, py, cx + a[1], cy + a[2])
    cx = cx + a[1]
    cy = cy + a[2]
    return cy
end

function runners.A(doc, a)
    solve_arc(doc, cx, cy, a)
    cx = a[6]
    cy = a[7]
    return cy
end

-- The relative form rewrites its own end point in place before solving, so
-- solve_arc only ever sees absolute coordinates.
function runners.a(doc, a)
    a[6] = a[6] + cx
    a[7] = a[7] + cy
    solve_arc(doc, cx, cy, a)
    cx = a[6]
    cy = a[7]
    return cy
end

function runners.L(doc, a)
    cx = a[1]
    cy = a[2]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

function runners.l(doc, a)
    cx = cx + a[1]
    cy = cy + a[2]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

function runners.H(doc, a)
    cx = a[1]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

function runners.h(doc, a)
    cx = cx + a[1]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

function runners.V(doc, a)
    cy = a[1]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

function runners.v(doc, a)
    cy = cy + a[1]
    px, py = nil, nil
    return doc:lineTo(cx, cy)
end

-- Closepath returns the current point to where the subpath began, so a command
-- following it continues from there rather than from the last drawn point.
function runners.Z(doc)
    doc:closePath()
    cx = sx
    cy = sy
    return cy
end

runners.z = runners.Z

--------------------------------------------------------------------------------
-- Entry point
--------------------------------------------------------------------------------

local function run(commands, doc)
    -- Current point, control point, and subpath starting point.
    cx, cy, px, py, sx, sy = 0, 0, 0, 0, 0, 0

    for i = 1, #commands do
        local c = commands[i]
        if type(runners[c.command]) == "function" then
            runners[c.command](doc, c.args)
        end
    end
end

function SVGPath.apply(doc, path)
    local commands = parse(path)
    run(commands, doc)
end

-- Exposed for testing: the parse step on its own, with no document involved.
SVGPath._parse = parse

return SVGPath
