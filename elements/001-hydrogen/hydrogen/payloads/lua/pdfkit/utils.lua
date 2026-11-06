-- utils.lua -- port of pdfkit/lib/utils.js
--
-- The only genuinely tricky thing in here is PDFNumber(), which is not a
-- rounding function in the usual sense. See the long comment on it below.

local M = {}

--------------------------------------------------------------------------------
-- PDFNumber -- 32-bit float truncation toward zero
--------------------------------------------------------------------------------
--
-- The JS original:
--
--     export function PDFNumber(n) {
--       const rounded = Math.fround(n);
--       if (rounded <= n) return rounded;
--       fArray[0] = n;
--       if (n <= 0) { uArray[0] += 1; } else { uArray[0] -= 1; }
--       return fArray[0];
--     }
--
-- Read that carefully: it takes the nearest float32 to n, and if that landed
-- *above* n it steps one ULP back down. The net effect is "the largest float32
-- that is <= n" -- a floor onto the float32 grid, not a round-to-nearest. The
-- bit fiddling works because the IEEE-754 bit pattern read as an unsigned
-- integer is monotonic in the value: for a positive float, subtracting 1 from
-- the pattern gives the next float below; for a negative float (sign bit set),
-- adding 1 makes the magnitude larger, which is also the next float below.
--
-- This value NEVER reaches the output file. PDFKit uses it in exactly one
-- place -- line_wrapper.js -- to compare accumulated word widths and vertical
-- positions against page limits. Its job is to make those comparisons stable
-- at float32 precision so line and page breaks land in reproducible places.
-- Which means getting it wrong doesn't produce a slightly-wrong number in the
-- PDF; it produces text broken at different points. Worth being exact about.
--
-- Lua 5.5's string.pack gives us float32 conversion and raw bit access
-- directly, so this is a fairly literal transcription.

local function fround(n)
    -- Round-trip through a 32-bit float: this is Math.fround.
    return (string.unpack("<f", string.pack("<f", n)))
end

function M.PDFNumber(n)
    local rounded = fround(n)
    if rounded <= n then
        return rounded
    end

    -- The nearest float32 overshot. Step one ULP toward zero.
    local bits = string.unpack("<I4", string.pack("<f", n))
    if n <= 0 then
        bits = bits + 1
    else
        bits = bits - 1
    end
    return (string.unpack("<f", string.pack("<I4", bits)))
end

--------------------------------------------------------------------------------
-- Number formatting -- reproducing JS String(Math.round(n * 1e6) / 1e6)
--------------------------------------------------------------------------------
--
-- This one DOES reach the output, on every coordinate in every content stream
-- and every number in every dictionary. PDFObject.number in the JS is:
--
--     static number(n) {
--       if (n > -1e21 && n < 1e21) { return Math.round(n * 1e6) / 1e6; }
--       throw new Error(`unsupported number: ${n}`);
--     }
--
-- ...and then the result is interpolated into a template string, so what
-- actually lands in the file is JavaScript's default number-to-string, which
-- prints the shortest decimal that round-trips the double.
--
-- Lua's tostring() is not a substitute, in two separate ways:
--
--   tostring(100.0)      -> "100.0"    JS gives "100"
--   tostring(0.000001)   -> "1e-06"    JS gives "0.000001"
--
-- The first is Lua 5.3+ keeping floats visually distinct from integers; the
-- second is %.14g switching to exponential notation. Both would corrupt the
-- file.
--
-- The fix relies on a property of the rounding step: after Math.round(n*1e6)/1e6
-- the value is always (some integer)/1e6, so it is exactly representable to six
-- decimal places, and the shortest round-tripping decimal for it is just that
-- six-decimal form with trailing zeros removed. So "%.6f" followed by stripping
-- trailing zeros reproduces JavaScript's output for every value in range --
-- including the large ones, because JS only switches to exponential notation at
-- 1e21, which PDFKit rejects anyway.

-- JS Math.round rounds halves toward +Infinity (so -0.5 -> -0, not -1).
-- math.floor(x + 0.5) has exactly that behaviour.
local function js_round(x)
    return math.floor(x + 0.5)
end

function M.number(n)
    if not (n > -1e21 and n < 1e21) then
        error("unsupported number: " .. tostring(n))
    end

    local v = js_round(n * 1e6) / 1e6

    local s = string.format("%.6f", v)

    -- Strip trailing zeros, then a bare trailing decimal point.
    s = s:gsub("0+$", "")
    s = s:gsub("%.$", "")

    -- JS prints negative zero as "0". "%.6f" gives us "-0.000000", which the
    -- stripping above turns into "-0". Normalise it.
    if s == "-0" then
        s = "0"
    end

    return s
end

-- PDFObject.number returns a *number*, and M.number above returns its printed
-- form. Almost every caller wants the string, but a couple of places -- the
-- gradient's /Matrix, written as `this.matrix.map(number)` -- put the rounded
-- values back into a dictionary and let the usual conversion print them later.
-- This is that intermediate value.
--
-- Rounding it here and again on the way out is harmless: the result is always
-- some integer over 1e6, so the second rounding recovers the same integer.
function M.round6(n)
    if not (n > -1e21 and n < 1e21) then
        error("unsupported number: " .. tostring(n))
    end
    return js_round(n * 1e6) / 1e6
end

--- JavaScript's Number.prototype.toFixed, returning a number (`+v.toFixed(d)`).
---
--- string.format("%.5f", v) is nearly this, and differs in exactly one case. Both
--- round the double's *exact* decimal expansion rather than some approximation of
--- it, but where that expansion lands precisely halfway C rounds to even and
--- JavaScript rounds the magnitude up.
---
--- Landing exactly halfway needs |v| * 10^d to have a fractional part of exactly
--- one half, i.e. |v| = odd / (2 * 10^d). A double is a dyadic rational, so the
--- 5^d in that denominator has to cancel, leaving |v| = odd / 2^(d+1) -- odd
--- sixty-fourths, for the five decimals asked for here. That is rare but it is
--- reachable from a scale factor, so the case is handled rather than assumed away.
function M.js_to_fixed(v, digits)
    local neg = v < 0
    local a = neg and -v or v

    -- Scaling by a power of two is exact, so this test is exact.
    local halves = a * 2 ^ (digits + 1)
    local result
    if halves % 1 == 0 and halves % 2 == 1 then
        -- Halfway. a * 10^d is exactly k + 0.5 here and is computed without
        -- error (its numerator is odd * 5^d, far below 2^53), so rounding up is
        -- just a floor of one more half.
        result = js_round(a * 10 ^ digits) / 10 ^ digits
    else
        result = tonumber(string.format("%." .. digits .. "f", a))
    end

    return neg and -result or result
end

--------------------------------------------------------------------------------
-- js_tostring -- JavaScript's String(Number), for the places that bypass number()
--------------------------------------------------------------------------------
--
-- Not every number PDFKit writes goes through PDFObject.number. Colour
-- components are the notable exception: _setColorCore does
--
--     this.addContent(`${color.join(' ')} ${op}`)
--
-- and Array.prototype.join uses JavaScript's default number-to-string. So a
-- fill colour of #808080 becomes 128/255 and is written out in full as
-- 0.5019607843137255 -- sixteen significant digits, not six. Rounding it to six
-- would be wrong, and so would printing seventeen.
--
-- JavaScript's rule is "the shortest decimal string that reads back as exactly
-- this double". C's printf has no such mode, but it can be recovered by asking
-- for increasing precision until the result round-trips, which is what this
-- does. Then the notation has to be adjusted: C writes 5e-07 and JS writes
-- 5e-7, and the two disagree about when to use exponential form at all.

-- Decompose a positive double into the pair the ECMAScript spec calls (s, n):
-- `digits` is the shortest string of significant decimal digits that reads back
-- as exactly this double, and `n` is where the decimal point sits relative to
-- them, so that the value is 0.<digits> x 10^n.
--
-- The shortest form is found by asking printf for one significant digit, then
-- two, and so on, until the result round-trips. %e is used rather than %g
-- because it always reports the exponent explicitly, which is what we need.
local function decompose(x)
    for p = 1, 17 do
        local s = string.format("%." .. (p - 1) .. "e", x)
        if tonumber(s) == x then
            local mant, exp = s:match("^(%d[%d%.]*)[eE]([-+]?%d+)$")
            local digits = mant:gsub("%.", ""):gsub("0+$", "")
            if digits == "" then digits = "0" end
            return digits, tonumber(exp) + 1
        end
    end
    -- Unreachable for finite doubles: 17 significant digits always round-trips.
    local mant, exp = string.format("%.16e", x):match("^(%d[%d%.]*)[eE]([-+]?%d+)$")
    return (mant:gsub("%.", "")), tonumber(exp) + 1
end

-- Port of the ECMAScript Number::toString algorithm. The four branches below
-- are the spec's, in the spec's order; between them they decide whether a value
-- prints as 100, 0.5, 0.000001 or 1e-7. The thresholds are not arbitrary and
-- not the same as C's: JavaScript stays in plain decimal notation down to 1e-6
-- and up to just under 1e21, and switches to exponential outside that.
function M.js_tostring(x)
    if x ~= x then return "NaN" end
    if x == math.huge then return "Infinity" end
    if x == -math.huge then return "-Infinity" end
    if x == 0 then return "0" end -- also normalises -0, as JS does

    local sign = ""
    if x < 0 then
        sign = "-"
        x = -x
    end

    local digits, n = decompose(x)
    local k = #digits

    if k <= n and n <= 21 then
        -- All the digits are integral, with trailing zeros to make up the rest.
        return sign .. digits .. string.rep("0", n - k)
    elseif 0 < n and n <= 21 then
        -- Decimal point falls inside the digits.
        return sign .. digits:sub(1, n) .. "." .. digits:sub(n + 1)
    elseif -6 < n and n <= 0 then
        -- Leading "0." and some zeros before the digits start.
        return sign .. "0." .. string.rep("0", -n) .. digits
    end

    -- Exponential notation. The exponent is written with an explicit sign and
    -- no zero padding, so 1e-7 rather than C's 1e-07.
    local e = n - 1
    local mantissa = (k == 1) and digits or (digits:sub(1, 1) .. "." .. digits:sub(2))
    return sign .. mantissa .. "e" .. (e < 0 and "-" or "+") .. tostring(math.abs(e))
end

--------------------------------------------------------------------------------
-- Trigonometry with the exact-angle special cases PDFKit applies
--------------------------------------------------------------------------------
--
-- These exist so that rotate(90) produces a clean 0 and 1 rather than 6.12e-17,
-- which would otherwise survive into the content stream as a visible artifact.

function M.cosine(a)
    if a == 0 then return 1 end
    if a == 90 then return 0 end
    if a == 180 then return -1 end
    if a == 270 then return 0 end
    return math.cos((a * math.pi) / 180)
end

function M.sine(a)
    if a == 0 then return 0 end
    if a == 90 then return 1 end
    if a == 180 then return 0 end
    if a == 270 then return -1 end
    return math.sin((a * math.pi) / 180)
end

--------------------------------------------------------------------------------
-- Side definitions (margins, padding)
--------------------------------------------------------------------------------
--
-- Port of normalizeSides(). Accepts a scalar, a {y, x} pair, a
-- {top, right, bottom, left} quad, or a table with vertical/horizontal or
-- explicit side keys, and always returns the explicit four-key form.

function M.normalizeSides(sides, default, transformer)
    transformer = transformer or function(v) return v end

    local function is_empty_table(t)
        return type(t) == "table" and next(t) == nil
    end

    if sides == nil or is_empty_table(sides) then
        sides = default
    end

    if type(sides) ~= "table" then
        sides = { top = sides, right = sides, bottom = sides, left = sides }
    elseif #sides > 0 then
        if #sides == 2 then
            sides = { vertical = sides[1], horizontal = sides[2] }
        else
            sides = {
                top = sides[1],
                right = sides[2],
                bottom = sides[3],
                left = sides[4],
            }
        end
    end

    if sides.vertical ~= nil or sides.horizontal ~= nil then
        sides = {
            top = sides.vertical,
            right = sides.horizontal,
            bottom = sides.vertical,
            left = sides.horizontal,
        }
    end

    return {
        top = transformer(sides.top),
        right = transformer(sides.right),
        bottom = transformer(sides.bottom),
        left = transformer(sides.left),
    }
end

M.MM_TO_CM = 1 / 10
M.CM_TO_IN = 1 / 2.54
M.PX_TO_IN = 1 / 96
M.IN_TO_PT = 72
M.PC_TO_PT = 12

return M
