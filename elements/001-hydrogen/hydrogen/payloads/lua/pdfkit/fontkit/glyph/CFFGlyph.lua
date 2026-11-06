-- fontkit/glyph/CFFGlyph.lua -- port of fontkit/src/glyph/CFFGlyph.js
--
-- A glyph whose outline is a Type 2 charstring in the CFF table.
--
-- The PDF does not store the path. It stores the charstring bytes, and the
-- global and local subroutines those bytes call. Parsing is how the subset
-- learns which subroutines are reachable: `_usedSubrs` and `_usedGsubrs` are
-- the whole product, and the path is only built so the interpreter can run.
--
-- The arithmetic follows fontkit, including the places it disagrees with the
-- Type 2 spec (sub and div subtract the deeper operand from the top one, and
-- roll turns the bottom of the stack). A subset has to name the same
-- subroutines fontkit names, so a "fixed" operator would embed different bytes.

local Glyph = require("pdfkit.fontkit.glyph.Glyph")
local Path = require("pdfkit.fontkit.glyph.Path")

local CFFGlyph = setmetatable({}, { __index = Glyph })
CFFGlyph.__index = CFFGlyph

function CFFGlyph.new(id, codePoints, font)
    local self = setmetatable({}, CFFGlyph)
    Glyph.init(self, id, codePoints, font)
    self.type = "CFF"
    return self
end

local function bias(subrs)
    local n = #subrs
    if n < 1240 then
        return 107
    elseif n < 33900 then
        return 1131
    end
    return 32768
end

-- 0 is false in JavaScript and true in Lua, and `and` / `or` / `not` in a
-- charstring are JavaScript's.
local function js_truthy(v)
    if v == nil or v == false or v == "" then
        return false
    end
    if type(v) == "number" and (v == 0 or v ~= v) then
        return false
    end
    return true
end

function CFFGlyph:_getPath()
    local cff = self._font["CFF "]
    if not cff then
        error("CFFGlyph: font has no CFF table")
    end

    local stream = cff.stream
    local str = cff.topDict.CharStrings[self.id + 1]
    local limit = str.offset + str.length
    stream:seek(str.offset)

    local path = Path.new()
    local stack = {}
    local sp = 0
    local trans = {}

    local width = nil
    local nStems = 0
    local x, y = 0, 0
    local open = false

    local usedGsubrs = {}
    local usedSubrs = {}
    self._usedGsubrs = usedGsubrs
    self._usedSubrs = usedSubrs

    local gsubrs = cff.globalSubrIndex or {}
    local gsubrsBias = bias(gsubrs)

    local privateDict = cff:privateDictForGlyph(self.id) or {}
    local subrs = privateDict.Subrs or {}
    local subrsBias = bias(subrs)
    local nominalWidthX = privateDict.nominalWidthX or 0

    local function push(v)
        sp = sp + 1
        stack[sp] = v
    end

    local function pop()
        local v = stack[sp]
        stack[sp] = nil
        sp = sp - 1
        return v
    end

    local function shift()
        local v = stack[1]
        for i = 1, sp - 1 do
            stack[i] = stack[i + 1]
        end
        stack[sp] = nil
        sp = sp - 1
        return v
    end

    -- 0-based, matching the array fontkit's operators index into.
    local function sget(i)
        return stack[i + 1]
    end

    local function sset(i, v)
        stack[i + 1] = v
        if i + 1 > sp then
            sp = i + 1
        end
    end

    local function sclear()
        for i = 1, sp do
            stack[i] = nil
        end
        sp = 0
    end

    local function checkWidth()
        if width == nil then
            width = shift() + nominalWidthX
        end
    end

    local function parseStems()
        if sp % 2 ~= 0 then
            checkWidth()
        end
        nStems = nStems + (sp >> 1)
        sclear()
    end

    local function moveTo(nx, ny)
        if open then
            path:closePath()
        end
        path:moveTo(nx, ny)
        open = true
    end

    -- Declared before `call` so the recursive subroutine calls see this local
    -- and not a global of the same name.
    local parse

    local function call(list, index, used)
        local subr = index >= 0 and list[index + 1] or nil
        if not subr then
            return
        end
        used[index] = true
        local savedPos = stream.pos
        local savedEnd = limit
        stream:seek(subr.offset)
        limit = subr.offset + subr.length
        parse()
        stream:seek(savedPos)
        limit = savedEnd
    end

    parse = function()
        while stream.pos < limit do
            local op = stream:uint8()
            if op < 32 then
                if op == 1 or op == 3 or op == 18 or op == 23 then
                    parseStems()

                elseif op == 4 then -- vmoveto
                    if sp > 1 then
                        checkWidth()
                    end
                    y = y + shift()
                    moveTo(x, y)

                elseif op == 5 then -- rlineto
                    while sp >= 2 do
                        x = x + shift()
                        y = y + shift()
                        path:lineTo(x, y)
                    end

                elseif op == 6 or op == 7 then -- hlineto / vlineto
                    local phase = op == 6
                    while sp >= 1 do
                        if phase then
                            x = x + shift()
                        else
                            y = y + shift()
                        end
                        path:lineTo(x, y)
                        phase = not phase
                    end

                elseif op == 8 then -- rrcurveto
                    while sp > 0 do
                        local c1x = x + shift()
                        local c1y = y + shift()
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        x = c2x + shift()
                        y = c2y + shift()
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)
                    end

                elseif op == 10 then -- callsubr
                    call(subrs, pop() + subrsBias, usedSubrs)

                elseif op == 11 then -- return
                    if cff.version < 2 then
                        return
                    end

                elseif op == 14 then -- endchar
                    if cff.version < 2 then
                        if sp > 0 then
                            checkWidth()
                        end
                        if open then
                            path:closePath()
                            open = false
                        end
                    end

                elseif op == 15 then
                    error("vsindex operator not supported in CFF v1")

                elseif op == 16 then
                    error("blend operator not supported in CFF v1")

                elseif op == 19 or op == 20 then -- hintmask / cntrmask
                    parseStems()
                    stream:skip((nStems + 7) >> 3)

                elseif op == 21 then -- rmoveto
                    if sp > 2 then
                        checkWidth()
                    end
                    x = x + shift()
                    y = y + shift()
                    moveTo(x, y)

                elseif op == 22 then -- hmoveto
                    if sp > 1 then
                        checkWidth()
                    end
                    x = x + shift()
                    moveTo(x, y)

                elseif op == 24 then -- rcurveline
                    while sp >= 8 do
                        local c1x = x + shift()
                        local c1y = y + shift()
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        x = c2x + shift()
                        y = c2y + shift()
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)
                    end
                    x = x + shift()
                    y = y + shift()
                    path:lineTo(x, y)

                elseif op == 25 then -- rlinecurve
                    while sp >= 8 do
                        x = x + shift()
                        y = y + shift()
                        path:lineTo(x, y)
                    end
                    local c1x = x + shift()
                    local c1y = y + shift()
                    local c2x = c1x + shift()
                    local c2y = c1y + shift()
                    x = c2x + shift()
                    y = c2y + shift()
                    path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)

                elseif op == 26 then -- vvcurveto
                    if sp % 2 ~= 0 then
                        x = x + shift()
                    end
                    while sp >= 4 do
                        local c1x = x
                        local c1y = y + shift()
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        x = c2x
                        y = c2y + shift()
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)
                    end

                elseif op == 27 then -- hhcurveto
                    if sp % 2 ~= 0 then
                        y = y + shift()
                    end
                    while sp >= 4 do
                        local c1x = x + shift()
                        local c1y = y
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        x = c2x + shift()
                        y = c2y
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)
                    end

                elseif op == 28 then -- shortint
                    push(stream:int16())

                elseif op == 29 then -- callgsubr
                    call(gsubrs, pop() + gsubrsBias, usedGsubrs)

                elseif op == 30 or op == 31 then -- vhcurveto / hvcurveto
                    local phase = op == 31
                    while sp >= 4 do
                        local c1x, c1y, c2x, c2y
                        if phase then
                            c1x = x + shift()
                            c1y = y
                            c2x = c1x + shift()
                            c2y = c1y + shift()
                            y = c2y + shift()
                            x = c2x + (sp == 1 and shift() or 0)
                        else
                            c1x = x
                            c1y = y + shift()
                            c2x = c1x + shift()
                            c2y = c1y + shift()
                            x = c2x + shift()
                            y = c2y + (sp == 1 and shift() or 0)
                        end
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, x, y)
                        phase = not phase
                    end

                elseif op == 12 then
                    local op2 = stream:uint8()
                    if op2 == 3 then -- and
                        local a = pop()
                        local b = pop()
                        push(js_truthy(a) and js_truthy(b) and 1 or 0)
                    elseif op2 == 4 then -- or
                        local a = pop()
                        local b = pop()
                        push((js_truthy(a) or js_truthy(b)) and 1 or 0)
                    elseif op2 == 5 then -- not
                        push(js_truthy(pop()) and 0 or 1)
                    elseif op2 == 9 then -- abs
                        push(math.abs(pop()))
                    elseif op2 == 10 then -- add
                        local a = pop()
                        local b = pop()
                        push(a + b)
                    elseif op2 == 11 then -- sub
                        local a = pop()
                        local b = pop()
                        push(a - b)
                    elseif op2 == 12 then -- div
                        local a = pop()
                        local b = pop()
                        push(a / b)
                    elseif op2 == 14 then -- neg
                        push(-pop())
                    elseif op2 == 15 then -- eq
                        local a = pop()
                        local b = pop()
                        push(a == b and 1 or 0)
                    elseif op2 == 18 then -- drop
                        pop()
                    elseif op2 == 20 then -- put
                        local val = pop()
                        local idx = pop()
                        trans[idx] = val
                    elseif op2 == 21 then -- get
                        local idx = pop()
                        local val = trans[idx]
                        push(js_truthy(val) and val or 0)
                    elseif op2 == 22 then -- ifelse
                        local s1 = pop()
                        local s2 = pop()
                        local v1 = pop()
                        local v2 = pop()
                        push(v1 <= v2 and s1 or s2)
                    elseif op2 == 23 then -- random
                        push(math.random())
                    elseif op2 == 24 then -- mul
                        local a = pop()
                        local b = pop()
                        push(a * b)
                    elseif op2 == 26 then -- sqrt
                        push(math.sqrt(pop()))
                    elseif op2 == 27 then -- dup
                        local a = pop()
                        push(a)
                        push(a)
                    elseif op2 == 28 then -- exch
                        -- fontkit pushes the pair back in the order it popped
                        -- them, so this is a no-op there. Kept, because the
                        -- used-subroutine set has to agree with it.
                        local a = pop()
                        local b = pop()
                        push(b)
                        push(a)
                    elseif op2 == 29 then -- index
                        local idx = pop()
                        if idx < 0 then
                            idx = 0
                        elseif idx > sp - 1 then
                            idx = sp - 1
                        end
                        push(sget(idx))
                    elseif op2 == 30 then -- roll
                        local n = pop()
                        local j = pop()
                        if j >= 0 then
                            while j > 0 do
                                local t = sget(n - 1)
                                for i = n - 2, 0, -1 do
                                    sset(i + 1, sget(i))
                                end
                                sset(0, t)
                                j = j - 1
                            end
                        else
                            while j < 0 do
                                local t = sget(0)
                                for i = 0, n do
                                    sset(i, sget(i + 1))
                                end
                                sset(n - 1, t)
                                j = j + 1
                            end
                        end
                    elseif op2 == 34 then -- hflex
                        local c1x = x + shift()
                        local c1y = y
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        local c3x = c2x + shift()
                        local c3y = c2y
                        local c4x = c3x + shift()
                        local c4y = c3y
                        local c5x = c4x + shift()
                        local c5y = c4y
                        local c6x = c5x + shift()
                        local c6y = c5y
                        x, y = c6x, c6y
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, c3x, c3y)
                        path:bezierCurveTo(c4x, c4y, c5x, c5y, c6x, c6y)
                    elseif op2 == 35 then -- flex
                        local pts = {}
                        for _ = 0, 5 do
                            x = x + shift()
                            y = y + shift()
                            pts[#pts + 1] = x
                            pts[#pts + 1] = y
                        end
                        path:bezierCurveTo(pts[1], pts[2], pts[3], pts[4], pts[5], pts[6])
                        path:bezierCurveTo(pts[7], pts[8], pts[9], pts[10], pts[11], pts[12])
                        shift()
                    elseif op2 == 36 then -- hflex1
                        local c1x = x + shift()
                        local c1y = y + shift()
                        local c2x = c1x + shift()
                        local c2y = c1y + shift()
                        local c3x = c2x + shift()
                        local c3y = c2y
                        local c4x = c3x + shift()
                        local c4y = c3y
                        local c5x = c4x + shift()
                        local c5y = c4y + shift()
                        local c6x = c5x + shift()
                        local c6y = c5y
                        x, y = c6x, c6y
                        path:bezierCurveTo(c1x, c1y, c2x, c2y, c3x, c3y)
                        path:bezierCurveTo(c4x, c4y, c5x, c5y, c6x, c6y)
                    elseif op2 == 37 then -- flex1
                        local startx, starty = x, y
                        local pts = {}
                        for _ = 0, 4 do
                            x = x + shift()
                            y = y + shift()
                            pts[#pts + 1] = x
                            pts[#pts + 1] = y
                        end
                        if math.abs(x - startx) > math.abs(y - starty) then
                            x = x + shift()
                            y = starty
                        else
                            x = startx
                            y = y + shift()
                        end
                        pts[#pts + 1] = x
                        pts[#pts + 1] = y
                        path:bezierCurveTo(pts[1], pts[2], pts[3], pts[4], pts[5], pts[6])
                        path:bezierCurveTo(pts[7], pts[8], pts[9], pts[10], pts[11], pts[12])
                    else
                        error("Unknown op: 12 " .. op2)
                    end

                else
                    error("Unknown op: " .. op)
                end

            elseif op < 247 then
                push(op - 139)
            elseif op < 251 then
                local b1 = stream:uint8()
                push((op - 247) * 256 + b1 + 108)
            elseif op < 255 then
                local b1 = stream:uint8()
                push(-(op - 251) * 256 - b1 - 108)
            else
                push(stream:int32() / 65536)
            end
        end
    end

    parse()

    if open then
        path:closePath()
    end

    return path
end

return CFFGlyph
