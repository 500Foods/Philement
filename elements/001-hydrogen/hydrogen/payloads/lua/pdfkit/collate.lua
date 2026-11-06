-- collate.lua -- no direct counterpart in PDFKit
--
-- Name-tree keys are sorted with String.prototype.localeCompare, which is not
-- the same as comparing bytes. localeCompare performs Unicode collation, and
-- collation says things Lua's `<` does not:
--
--     "apple" < "Zebra"        -- letters compare by base letter first
--     "a" < "A" < "b"          -- case is a tie-breaker, not a primary key
--     "_" < "1" < "a"          -- punctuation, then digits, then letters
--
-- Compare those same strings byte-wise and "Zebra" sorts first, because 'Z' is
-- 0x5A and 'a' is 0x61. A document with mixed-case destination names would then
-- come out with its name tree in a different order -- a real difference in the
-- bytes, from a detail that is easy to never notice.
--
-- What follows is the root collation restricted to ASCII: three levels, in the
-- order collation applies them.
--
--   primary    the base character -- case folded, so 'a' and 'A' are equal
--   secondary  accents; not modelled, since ASCII has none
--   tertiary   case, with lowercase ordered before uppercase
--
-- Levels are compared one whole string at a time, not character by character.
-- "aB" and "Ab" are identical at the primary level and only separate at the
-- tertiary level, where position 1 decides it: "aB" comes first.
--
-- LIMITS, stated plainly. This is ASCII only. Bytes at 0x80 and above fall back
-- to code point order, so accented and non-Latin keys can sort differently here
-- than in the original. Doing better means shipping a collation table, which is
-- a large amount of data for a case that named destinations rarely reach. If
-- your keys are ASCII -- and they almost certainly are -- this agrees exactly.

local M = {}

-- Which broad group a character belongs to. Collation orders the groups before
-- it looks at anything within them.
local GROUP_OTHER = 1  -- punctuation, symbols, control characters
local GROUP_DIGIT = 2
local GROUP_LETTER = 3

local function group_of(byte)
    if byte >= 48 and byte <= 57 then
        return GROUP_DIGIT
    end
    if (byte >= 65 and byte <= 90) or (byte >= 97 and byte <= 122) then
        return GROUP_LETTER
    end
    return GROUP_OTHER
end

-- Primary weight: the group, then the case-folded character within it.
local function primary_weight(byte)
    local group = group_of(byte)
    local value = byte
    if group == GROUP_LETTER and byte >= 65 and byte <= 90 then
        value = byte + 32 -- fold uppercase to lowercase
    end
    return group * 0x10000 + value
end

-- Tertiary weight: lowercase (and everything that has no case) before
-- uppercase.
local function tertiary_weight(byte)
    if byte >= 65 and byte <= 90 then
        return 2
    end
    return 1
end

-- Compare two strings one level at a time, returning -1, 0 or 1. A shorter
-- string that is otherwise a prefix of the longer one sorts first.
local function compare_level(a, b, weight)
    local n = math.min(#a, #b)
    for i = 1, n do
        local wa = weight(a:byte(i))
        local wb = weight(b:byte(i))
        if wa ~= wb then
            return wa < wb and -1 or 1
        end
    end
    if #a ~= #b then
        return #a < #b and -1 or 1
    end
    return 0
end

-- True when the string is entirely ASCII, and so within what this models.
local function is_ascii(s)
    return s:find("[\128-\255]") == nil
end

-- The equivalent of a.localeCompare(b): negative, zero or positive.
function M.compare(a, b)
    if not (is_ascii(a) and is_ascii(b)) then
        -- Outside ASCII, fall back to code point order rather than guessing.
        if a == b then return 0 end
        return a < b and -1 or 1
    end

    local primary = compare_level(a, b, primary_weight)
    if primary ~= 0 then
        return primary
    end

    -- Secondary level would go here; ASCII carries no accents to compare.

    return compare_level(a, b, tertiary_weight)
end

-- The form table.sort wants.
function M.less_than(a, b)
    return M.compare(a, b) < 0
end

return M
