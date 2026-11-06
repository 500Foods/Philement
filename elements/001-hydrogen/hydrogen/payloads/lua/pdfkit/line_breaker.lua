-- line_breaker.lua -- port of the `linebreak` npm package, which PDFKit uses
--
-- The Unicode line breaking algorithm, UAX #14. Given a string, it walks
-- forward handing back the positions where a line MAY be broken and the
-- positions where one MUST be. It knows that "hello, world" may break after
-- the space but not inside "hello", that a hyphen invites a break after it but
-- an en dash does not, that Chinese breaks almost anywhere and Thai does not
-- break without a dictionary.
--
-- The character data lives in linebreak/data.lua, extracted from the npm
-- package by solution/tools/gen-linebreak-data.js. See that file for why.
--
-- ONE DELIBERATE DIVERGENCE. JavaScript strings are UTF-16, so the original
-- reports positions as counts of 16-bit code units and treats a character
-- outside the Basic Multilingual Plane as two positions. Lua strings are bytes,
-- so positions here are byte offsets into UTF-8. The numbers therefore differ;
-- the substrings they delimit do not, which is all any caller uses them for.
-- Positions are zero-based, matching the original, so a caller slicing with
-- string.sub writes s:sub(last + 1, position).

local classes = require("pdfkit.linebreak.classes")
local pairs_table = require("pdfkit.linebreak.pairs")
local data = require("pdfkit.linebreak.data")

local M = {}

-- Line breaking classes, pulled into locals because they are read constantly.
local OP, CL, CP, QU, GL, NS = classes.OP, classes.CL, classes.CP, classes.QU, classes.GL, classes.NS
local EX, SY, IS, PR, PO, NU = classes.EX, classes.SY, classes.IS, classes.PR, classes.PO, classes.NU
local AL, HL, ID, IN, HY, BA = classes.AL, classes.HL, classes.ID, classes.IN, classes.HY, classes.BA
local BB, B2, ZW, CM, WJ     = classes.BB, classes.B2, classes.ZW, classes.CM, classes.WJ
local RI, ZWJ, CB            = classes.RI, classes.ZWJ, classes.CB
local AI, BK, CJ, CR, LF, NL = classes.AI, classes.BK, classes.CJ, classes.CR, classes.LF, classes.NL
local SA, SG, SP, XX         = classes.SA, classes.SG, classes.SP, classes.XX

-- Pair table actions.
local DI_BRK, IN_BRK, CI_BRK = classes.DI_BRK, classes.IN_BRK, classes.CI_BRK
local CP_BRK, PR_BRK         = classes.CP_BRK, classes.PR_BRK

local STARTS, CLASSES = data.starts, data.classes
local RANGE_COUNT = #STARTS
local PAIRS, PAIR_N = pairs_table.data, pairs_table.N

-- Silence the unused-local complaints from the block above: these are declared
-- for the reader's benefit, so that the algorithm below can be checked against
-- the UAX #14 rules by name. Lua has no "unused is fine" pragma, so they are
-- simply listed here.
local _ = { OP, CL, CP, QU, GL, NS, EX, SY, PR, PO, NU, ID, IN, BB, B2, ZW, CM,
    WJ, CB, AI, CJ, SA, SG, XX }

--- The line breaking class of a codepoint.
---
--- The data is a sorted list of range starts; the answer is the class attached
--- to the last start that is not past the codepoint. Binary search.
local function class_of(codepoint)
    local lo, hi = 1, RANGE_COUNT
    while lo < hi do
        -- Bias the midpoint upward so that lo can advance; with the usual
        -- downward bias this loop would not terminate when hi == lo + 1.
        local mid = (lo + hi + 1) // 2
        if STARTS[mid] <= codepoint then
            lo = mid
        else
            hi = mid - 1
        end
    end
    return CLASSES[lo]
end

M.class_of = class_of

--- Classes the pair table has no column for, folded onto ones it does.
---
--- AI is "ambiguous, alphabetic or ideographic" and is resolved as alphabetic.
--- SA is South-East Asian, which needs a dictionary to break properly; without
--- one it behaves as a letter. SG is a lone surrogate and XX is unassigned;
--- both are treated as letters so that unknown text still wraps somewhere.
--- CJ is a small kana that a Japanese typesetter may or may not allow at the
--- start of a line, and is resolved to the stricter non-starter.
local function map_class(c)
    if c == AI then
        return AL
    elseif c == SA or c == SG or c == XX then
        return AL
    elseif c == CJ then
        return NS
    end
    return c
end

--- Extra folding applied only to the very first character of the string.
---
--- A leading line feed or next-line is a mandatory break like any other, and a
--- leading space cannot break before itself, so it is treated as a word joiner.
local function map_first(c)
    if c == LF or c == NL then
        return BK
    elseif c == SP then
        return WJ
    end
    return c
end

-- --- UTF-8 ------------------------------------------------------------------

-- Decode the codepoint starting at zero-based byte offset `pos`.
-- Returns the codepoint and the offset just past it.
--
-- Deliberately total: a byte that does not begin a well-formed sequence is
-- returned as its own value and consumes one byte. Invalid UTF-8 then produces
-- odd breaking rather than an error, which is the friendlier failure for a
-- library whose job is to lay out whatever it is handed.
local function next_codepoint(s, pos)
    local b1 = s:byte(pos + 1)
    if not b1 then
        return nil, pos
    end

    if b1 < 0x80 then
        return b1, pos + 1
    end

    local need, cp
    if b1 >= 0xF0 and b1 <= 0xF4 then
        need, cp = 3, b1 & 0x07
    elseif b1 >= 0xE0 and b1 <= 0xEF then
        need, cp = 2, b1 & 0x0F
    elseif b1 >= 0xC2 and b1 <= 0xDF then
        need, cp = 1, b1 & 0x1F
    else
        return b1, pos + 1 -- continuation byte or overlong lead; pass it through
    end

    for i = 1, need do
        local b = s:byte(pos + 1 + i)
        if not b or b < 0x80 or b > 0xBF then
            return b1, pos + 1 -- truncated sequence; pass the lead byte through
        end
        cp = (cp << 6) | (b & 0x3F)
    end

    return cp, pos + 1 + need
end

M.next_codepoint = next_codepoint

-- --- the breaker ------------------------------------------------------------

local LineBreaker = {}
LineBreaker.__index = LineBreaker

M.LineBreaker = LineBreaker

function LineBreaker.new(s)
    return setmetatable({
        string = s,
        pos = 0,     -- zero-based byte offset of the next character to read
        lastPos = 0, -- offset of the character boundary under consideration
        curClass = nil,
        nextClass = nil,
        -- Rule state that the pair table alone cannot express.
        LB8a = false, -- previous character was a zero width joiner
        LB21a = false, -- previous character was a Hebrew letter
        LB30a = 0,    -- how many regional indicators in a row
    }, LineBreaker)
end

function LineBreaker:nextCharClass()
    local cp, nextPos = next_codepoint(self.string, self.pos)
    self.pos = nextPos
    if cp == nil then
        return AL
    end
    return map_class(class_of(cp))
end

--- Rules that do not go through the pair table at all.
--- Returns true or false for a decision, or nil to say "not mine".
function LineBreaker:getSimpleBreak()
    local nextClass = self.nextClass

    if nextClass == SP then
        -- Never break before a space: it belongs to the line it trails.
        return false
    elseif nextClass == BK or nextClass == LF or nextClass == NL then
        -- Remember that a mandatory break is pending; it is emitted on the
        -- next pass round the loop, once we know what follows it.
        self.curClass = BK
        return false
    elseif nextClass == CR then
        self.curClass = CR
        return false
    end

    return nil
end

function LineBreaker:getPairTableBreak(lastClass)
    local shouldBreak = false

    local action = PAIRS[self.curClass * PAIR_N + self.nextClass + 1]

    if action == DI_BRK then
        shouldBreak = true
    elseif action == IN_BRK then
        -- Indirect: a break is allowed only if a space intervenes.
        shouldBreak = (lastClass == SP)
    elseif action == CI_BRK then
        shouldBreak = (lastClass == SP)
        if not shouldBreak then
            -- Returns early, so none of the rule state below is updated and
            -- curClass is left alone. That is what the original does.
            return false
        end
    elseif action == CP_BRK then
        if lastClass ~= SP then
            return shouldBreak
        end
    elseif action == PR_BRK then
        -- Prohibited outright.
    end

    -- LB8a: never break directly after a zero width joiner. It exists to fuse
    -- emoji into a single glyph; breaking there would split the picture.
    if self.LB8a then
        shouldBreak = false
    end

    -- LB21a: after a Hebrew letter, a following hyphen or break-after class
    -- does not license a break. HL HY AL stays together.
    if self.LB21a and (self.curClass == HY or self.curClass == BA) then
        shouldBreak = false
        self.LB21a = false
    else
        self.LB21a = (self.curClass == HL)
    end

    -- LB30a: regional indicators pair up into flags, so break between pairs
    -- but never inside one.
    if self.curClass == RI then
        self.LB30a = self.LB30a + 1
        if self.LB30a == 2 and self.nextClass == RI then
            shouldBreak = true
            self.LB30a = 0
        end
    else
        self.LB30a = 0
    end

    self.curClass = self.nextClass
    return shouldBreak
end

--- The next place the string may be broken, or nil once past the end.
--- Returns a table { position = <zero-based byte offset>, required = <boolean> }.
function LineBreaker:nextBreak()
    -- Prime the state with the first character.
    if self.curClass == nil then
        local firstClass = self:nextCharClass()
        self.curClass = map_first(firstClass)
        self.nextClass = firstClass
        self.LB8a = (firstClass == ZWJ)
        self.LB30a = 0
    end

    local len = #self.string

    while self.pos < len do
        self.lastPos = self.pos
        local lastClass = self.nextClass
        self.nextClass = self:nextCharClass()

        -- An explicit newline. A carriage return only counts on its own; CR LF
        -- is one break, not two, so a CR followed by LF falls through and the
        -- LF triggers it.
        if self.curClass == BK or (self.curClass == CR and self.nextClass ~= LF) then
            self.curClass = map_first(map_class(self.nextClass))
            return { position = self.lastPos, required = true }
        end

        local shouldBreak = self:getSimpleBreak()
        if shouldBreak == nil then
            shouldBreak = self:getPairTableBreak(lastClass)
        end

        self.LB8a = (self.nextClass == ZWJ)

        if shouldBreak then
            return { position = self.lastPos, required = false }
        end
    end

    -- The end of the string is always a break opportunity, reported once.
    if self.lastPos < len then
        self.lastPos = len
        return { position = len, required = false }
    end

    return nil
end

return M
