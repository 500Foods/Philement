-- jsmath.lua -- no counterpart in PDFKit
--
-- Lua's `^` and JavaScript's Math.pow do not agree on the last bit, and for
-- once that is not a nitpick.
--
-- Lua's `^` is C's pow(). glibc's pow has been correctly rounded since 2.28: it
-- returns the double nearest the true value, every time. V8 does not call
-- libm at all -- it carries its own copy of Sun's FDLIBM e_pow.c, which is
-- accurate to under one unit in the last place but is not correctly rounded.
-- So the two disagree on roughly one input in two hundred.
--
-- That surfaces in fontkit. Path.bbox solves each curve for its extrema by
-- evaluating a cubic written with Math.pow, and TTFGlyphEncoder writes the
-- resulting box into a subset font's glyph header as four int16s -- truncating
-- toward zero on the way. A one-bit disagreement is invisible almost always,
-- and then a curve turns at a whole coordinate and the two implementations
-- straddle it. Sweeping the thirteen fonts in PDFKit's test corpus -- 20,417
-- glyphs, 81,668 box coordinates -- turns up two such values, both in GoodDog:
-- one implementation says 248 and the other says 247.99999999999997, so one
-- writes 248 into the font and the other writes 247. One byte, and the
-- subsetted font no longer matches.
--
-- Hence this: a transcription of FDLIBM's __ieee754_pow, which is the code V8
-- actually runs. It is deliberately unidiomatic. The variable names, the
-- branch order and the bit manipulation are all the C original's, because the
-- only thing that makes this worth having is being able to check it line
-- against line. The constants are given as their bit patterns rather than as
-- decimals for the same reason -- the C source lists both, and the bits are the
-- ones that are actually normative.
--
-- Only pow is here. The other Math functions PDFKit and fontkit use -- sqrt,
-- abs, floor, round, min, max -- are exact or exactly specified, and Lua's
-- agree with JavaScript's already. sqrt in particular is required by IEEE 754
-- to be correctly rounded, so math.sqrt and Math.sqrt cannot differ; that was
-- checked over 200,000 inputs before writing any of this.
--
-- Layering note: this sits beside jsstring.lua at the top of the tree rather
-- than inside fontkit/, even though fontkit is currently its only caller,
-- because it describes the host language rather than either library.

local spack, sunpack = string.pack, string.unpack
local sqrt, abs = math.sqrt, math.abs

local M = {}

--------------------------------------------------------------------------------
-- Getting at the bits
--
-- The C original reaches into a double through a union and reads two 32-bit
-- halves out of it. Lua has to go the long way round through string.pack, which
-- is slower but says exactly the same thing: "<d" pins the byte order so the
-- first four bytes are always the low half, whatever the machine prefers.
--------------------------------------------------------------------------------

--- Both halves of `d`, high first, each as an unsigned 32-bit integer.
local function hi_lo(d)
    local lo, hi = sunpack("<I4I4", spack("<d", d))
    return hi, lo
end

--- Just the high half.
local function hi_word(d)
    local _, hi = sunpack("<I4I4", spack("<d", d))
    return hi
end

--- The double with these halves.
local function from_words(hi, lo)
    return (sunpack("<d", spack("<I4I4", lo & 0xFFFFFFFF, hi & 0xFFFFFFFF)))
end

--- Reinterpret the low 32 bits as a signed value, the way C's int32_t does.
--- Needed wherever the original compares or negates a high word, since Lua's
--- integers are 64 bits wide and would read the sign bit as data.
local function i32(v)
    v = v & 0xFFFFFFFF
    if v >= 0x80000000 then v = v - 0x100000000 end
    return v
end

--- SET_LOW_WORD(d, 0) -- drop the bottom 32 bits of the mantissa.
--- FDLIBM uses this constantly to split a double into a head with 21 significant
--- bits and an exactly representable tail, so that a product of two heads is
--- exact and the error terms can be tracked by hand.
local function set_lo0(d)
    local _, hi = sunpack("<I4I4", spack("<d", d))
    return (sunpack("<d", spack("<I4I4", 0, hi)))
end

--- SET_HIGH_WORD(d, hi).
local function set_hi(d, hi)
    local lo = sunpack("<I4", spack("<d", d))
    return (sunpack("<d", spack("<I4I4", lo, hi & 0xFFFFFFFF)))
end

--- 2^n as a double, exactly, for -1022 <= n <= 1023.
local function two_pow(n)
    return from_words((n + 1023) << 20, 0)
end

--- x * 2^n. Only reached for a subnormal result, which no font ever produces,
--- but the original has it and leaving it out would be a lie about the range.
local function scalbn(x, n)
    if n > 1023 then
        x = x * two_pow(1023); n = n - 1023
        if n > 1023 then
            x = x * two_pow(1023); n = n - 1023
            if n > 1023 then n = 1023 end
        end
    elseif n < -1022 then
        -- Two steps down, because 2^-1074 is not representable as a normal.
        x = x * two_pow(-1022) * two_pow(-52); n = n + 1074
        if n < -1022 then
            x = x * two_pow(-1022) * two_pow(-52); n = n + 1074
            if n < -1022 then n = -1022 end
        end
    end
    return x * two_pow(n)
end

--------------------------------------------------------------------------------
-- FDLIBM's constants, by bit pattern
--------------------------------------------------------------------------------

local bp   = { [0] = 1.0, [1] = 1.5 }
local dp_h = { [0] = 0.0, [1] = from_words(0x3FE2B803, 0x40000000) }
local dp_l = { [0] = 0.0, [1] = from_words(0x3E4CFDEB, 0x43CFD006) }

local two53 = from_words(0x43400000, 0x00000000)
local huge  = 1.0e300
local tiny  = 1.0e-300

-- Coefficients of the minimax polynomial for (3/2)*(log(x) - 2s - (2/3)s^3).
local L1 = from_words(0x3FE33333, 0x33333303)
local L2 = from_words(0x3FDB6DB6, 0xDB6FABFF)
local L3 = from_words(0x3FD55555, 0x518F264D)
local L4 = from_words(0x3FD17460, 0xA91D4101)
local L5 = from_words(0x3FCD864A, 0x93C9DB65)
local L6 = from_words(0x3FCA7E28, 0x4A454EEF)

-- Coefficients for exp(x) - 1 - x, used to finish 2^(p_h+p_l).
local P1 = from_words(0x3FC55555, 0x5555553E)
local P2 = from_words(0xBF66C16C, 0x16BEBD93)
local P3 = from_words(0x3F11566A, 0xAF25DE2C)
local P4 = from_words(0xBEBBBD41, 0xC5D26BF1)
local P5 = from_words(0x3E663769, 0x72BEA4D0)

local lg2   = from_words(0x3FE62E42, 0xFEFA39EF)  -- ln 2
local lg2_h = from_words(0x3FE62E43, 0x00000000)  -- its head
local lg2_l = from_words(0xBE205C61, 0x0CA86C39)  -- and its tail

local cp    = from_words(0x3FEEC709, 0xDC3A03FD)  -- 2/(3 ln 2)
local cp_h  = from_words(0x3FEEC709, 0xE0000000)
local cp_l  = from_words(0xBE3E2FE0, 0x145B01F5)

local ivln2   = from_words(0x3FF71547, 0x652B82FE)  -- 1/ln 2
local ivln2_h = from_words(0x3FF71547, 0x60000000)
local ivln2_l = from_words(0x3E54AE0B, 0xF85DDF44)

-- -(1024 - log2(overflow threshold + half an ulp)).
local ovt = 8.0085662595372944372e-17

--------------------------------------------------------------------------------
-- pow
--------------------------------------------------------------------------------

--- x raised to y, with JavaScript's Math.pow's exact rounding.
---
--- Structure, straight from the original:
---
---   1. Dispose of the cases the general path cannot handle -- NaN, zero and
---      infinite exponents, negative bases, the exponents with shortcuts.
---   2. Compute log2(|x|) to about 84 bits, carried as an unevaluated sum
---      t1 + t2 where t1 has its low mantissa half zeroed.
---   3. Multiply by y in the same split form to get p_h + p_l.
---   4. Raise 2 to that, again in two pieces, and reassemble.
---
--- The splitting is the whole trick: every product of two heads is exact, so
--- the error never compounds and the result lands within one ulp.
function M.pow(x, y)
    -- Lua would do these in integer arithmetic given integer arguments, which
    -- wraps at 2^63 where JavaScript would have gone to a double long before.
    -- Multiplying rather than adding, because `-0.0 + 0.0` is `+0.0` under
    -- round-to-nearest and the sign of a zero base decides the sign of the
    -- answer: (-0)^3 is -0 and (-0)^-3 is -infinity.
    x = x * 1.0
    y = y * 1.0

    local hx, lx = hi_lo(x)
    local hy, ly = hi_lo(y)
    local shx, shy = i32(hx), i32(hy)
    local ix, iy = hx & 0x7fffffff, hy & 0x7fffffff

    -- x^0 is 1 for every x, NaN included.
    if (iy | ly) == 0 then return 1.0 end

    -- Either argument NaN. Adding them propagates it.
    if ix > 0x7ff00000 or (ix == 0x7ff00000 and lx ~= 0)
        or iy > 0x7ff00000 or (iy == 0x7ff00000 and ly ~= 0) then
        return x + y
    end

    -- Is y a whole number, and if so is it odd? Only a negative base cares, and
    -- only then is it worth the bit-picking: y is an integer exactly when every
    -- mantissa bit below the binary point is zero, which the shift-and-shift-
    -- back tests without going near a division.
    --   0 = not an integer, 1 = odd, 2 = even
    local yisint = 0
    if shx < 0 then
        if iy >= 0x43400000 then
            yisint = 2                       -- past 2^52 every double is even
        elseif iy >= 0x3ff00000 then
            local k = (iy >> 20) - 0x3ff     -- unbiased exponent, 0..51
            if k > 20 then
                local j = ly >> (52 - k)
                if ((j << (52 - k)) & 0xFFFFFFFF) == ly then yisint = 2 - (j & 1) end
            elseif ly == 0 then
                local j = iy >> (20 - k)
                if ((j << (20 - k)) & 0xFFFFFFFF) == iy then yisint = 2 - (j & 1) end
            end
        end
    end

    -- Exponents with an answer that needs no work.
    if ly == 0 then
        if iy == 0x7ff00000 then                       -- y is +-inf
            if ((ix - 0x3ff00000) & 0xFFFFFFFF | lx) == 0 then
                return y - y                           -- 1^inf is NaN
            elseif ix >= 0x3ff00000 then
                return shy >= 0 and y or 0.0           -- (|x|>1)^+-inf
            else
                return shy < 0 and -y or 0.0           -- (|x|<1)^+-inf
            end
        end
        if iy == 0x3ff00000 then                       -- y is +-1
            if shy < 0 then return 1.0 / x end
            return x
        end
        if hy == 0x40000000 then return x * x end      -- y is 2
        if hy == 0x3fe00000 and shx >= 0 then          -- y is 1/2
            return sqrt(x)
        end
    end

    local ax = abs(x)

    -- Bases with an answer that needs no work: +-0, +-inf, +-1.
    if lx == 0 and (ix == 0x7ff00000 or ix == 0 or ix == 0x3ff00000) then
        local z = ax
        if shy < 0 then z = 1.0 / z end
        if shx < 0 then
            if ((ix - 0x3ff00000) & 0xFFFFFFFF | yisint) == 0 then
                z = (z - z) / (z - z)      -- (-1)^non-integer is NaN
            elseif yisint == 1 then
                z = -z                     -- (x<0)^odd keeps the sign
            end
        end
        return z
    end

    -- A flag for the sign of the base, used twice below and nowhere else. The
    -- original writes `(hx >> 31) + 1` over a signed word, giving 1 for a
    -- non-negative base and 0 for a negative one; here hx is unsigned, so the
    -- shift yields 0 or 1 the other way round and `- 1` restores the property
    -- that actually matters -- zero exactly when the base is negative.
    local n = (hx >> 31) - 1

    -- A negative base with a fractional exponent has no real answer.
    if (n | yisint) == 0 then return (x - x) / (x - x) end

    -- The sign of the answer, factored out so the rest can work on |x|.
    local s = 1.0
    if (n | (yisint - 1)) == 0 then s = -1.0 end

    local t1, t2, t, u, v, w, r, p_h, p_l, z

    if iy > 0x41e00000 then
        -- |y| > 2^31. Either the result over/underflows, or |x| is so close to
        -- one that four terms of the log series are enough.
        if iy > 0x43f00000 then                        -- |y| > 2^64: certain
            if ix <= 0x3fefffff then return shy < 0 and huge * huge or tiny * tiny end
            if ix >= 0x3ff00000 then return shy > 0 and huge * huge or tiny * tiny end
        end
        if ix < 0x3fefffff then return shy < 0 and s * huge * huge or s * tiny * tiny end
        if ix > 0x3ff00000 then return shy > 0 and s * huge * huge or s * tiny * tiny end

        -- |1-x| is at most 2^-20, so log(x) = x - x^2/2 + x^3/3 - x^4/4 suffices.
        t = ax - 1.0                                   -- 20 trailing zero bits
        w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25))
        u = ivln2_h * t                                -- ivln2_h has 21 bits
        v = t * ivln2_l - w * ivln2
        t1 = set_lo0(u + v)
        t2 = v - (t1 - u)
    else
        local ss, s2, s_h, s_l, t_h, t_l
        n = 0

        -- Pull a subnormal base up into the normal range first.
        if ix < 0x00100000 then
            ax = ax * two53
            n = n - 53
            ix = hi_word(ax)
        end

        n = n + (ix >> 20) - 0x3ff                     -- binary exponent of ax
        local j = ix & 0x000fffff
        ix = j | 0x3ff00000                            -- ax scaled into [1, 2)

        -- Two sub-intervals, so that (ax - bp[k]) / (ax + bp[k]) stays small.
        local k
        if j <= 0x3988E then                           -- ax < sqrt(3/2)
            k = 0
        elseif j < 0xBB67A then                        -- ax < sqrt(3)
            k = 1
        else                                           -- fold into [1, sqrt(3/2))
            k = 0
            n = n + 1
            ix = ix - 0x00100000
        end
        ax = set_hi(ax, ix)

        -- ss = (ax - bp[k]) / (ax + bp[k]), split into s_h + s_l.
        u = ax - bp[k]
        v = 1.0 / (ax + bp[k])
        ss = u * v
        s_h = set_lo0(ss)
        -- t_h is the head of ax + bp[k], built from its exponent directly.
        t_h = set_hi(0.0, ((ix >> 1) | 0x20000000) + 0x00080000 + (k << 18))
        t_l = ax - (t_h - bp[k])
        s_l = v * ((u - s_h * t_h) - s_h * t_l)

        -- log(ax) as ss * (3 + ss^2 + polynomial), again carried split.
        s2 = ss * ss
        r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 + s2 * L6)))))
        r = r + s_l * (s_h + ss)
        s2 = s_h * s_h
        t_h = set_lo0(3.0 + s2 + r)
        t_l = r - ((t_h - 3.0) - s2)

        u = s_h * t_h
        v = s_l * t_h + t_l * ss

        -- Scale by 2/(3 log 2) to turn a natural log into a log base 2.
        p_h = set_lo0(u + v)
        p_l = v - (p_h - u)
        local z_h = cp_h * p_h
        local z_l = cp_l * p_h + p_l * cp + dp_l[k]

        -- log2(ax) = n + dp_h[k] + z_h + z_l, redivided into head and tail.
        t = n + 0.0
        t1 = set_lo0(((z_h + z_l) + dp_h[k]) + t)
        t2 = z_l - (((t1 - t) - dp_h[k]) - z_h)
    end

    -- Multiply the split log by the split y.
    local y1 = set_lo0(y)
    p_l = (y - y1) * t1 + y * t2
    p_h = y1 * t1
    z = p_l + p_h

    local jhi, jlo = hi_lo(z)
    local sj = i32(jhi)
    if sj >= 0x40900000 then                           -- z >= 1024
        if ((jhi - 0x40900000) & 0xFFFFFFFF | jlo) ~= 0 then
            return s * huge * huge
        elseif p_l + ovt > z - p_h then
            return s * huge * huge
        end
    elseif (jhi & 0x7fffffff) >= 0x4090cc00 then       -- z <= -1075
        if ((jhi - 0xc090cc00) & 0xFFFFFFFF | jlo) ~= 0 then
            return s * tiny * tiny
        elseif p_l <= z - p_h then
            return s * tiny * tiny
        end
    end

    -- 2^(p_h + p_l): peel off the integer part into n, then a polynomial for
    -- what is left, which is now at most half.
    local i = jhi & 0x7fffffff
    local k = (i >> 20) - 0x3ff
    n = 0
    if i > 0x3fe00000 then
        n = i32(sj + (0x00100000 >> (k + 1)))          -- round z to nearest
        k = ((n & 0x7fffffff) >> 20) - 0x3ff
        t = set_hi(0.0, n & ~(0x000fffff >> k))        -- that rounding, as a double
        n = ((n & 0x000fffff) | 0x00100000) >> (20 - k)
        if sj < 0 then n = -n end
        p_h = p_h - t
    end

    t = set_lo0(p_l + p_h)
    u = t * lg2_h
    v = (p_l - (t - p_h)) * lg2 + t * lg2_l
    z = u + v
    w = v - (z - u)
    t = z * z
    t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))))
    -- The whole of `(t1 - 2) - (w + z*w)` is the divisor. Binding the division
    -- to the first term alone still produces a plausible-looking pow -- accurate
    -- to a part in 10^16 -- and disagrees with the real one on 3% of inputs.
    r = (z * t1) / ((t1 - 2.0) - (w + z * w))
    z = 1.0 - (r - z)

    -- Fold the integer part back in by adding to the exponent field.
    local jz = i32(i32(hi_word(z)) + (n << 20))
    if jz // 0x100000 <= 0 then
        z = scalbn(z, n)                               -- exponent underflowed
    else
        z = set_hi(z, jz)
    end

    return s * z
end

return M
