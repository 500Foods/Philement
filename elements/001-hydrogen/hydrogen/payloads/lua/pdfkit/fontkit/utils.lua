-- fontkit/utils.lua -- port of fontkit/src/utils.js
--
-- Two small helpers the cmap code leans on. The base64 decoder and the ascii
-- TextDecoder upstream also keeps here are not ported: the first is for the
-- built-in fallback font data, which this port does not carry, and the second
-- is covered by encodings.lua.

local M = {}

--- Binary search over a one-based array, with a three-way comparator.
---
--- `cmp(element)` returns negative to search left, positive to search right,
--- zero on a hit. Upstream returns -1 for "not found"; this returns nil, and
--- the callers test for nil in the same places they test for -1. The result is
--- the index of *a* match, not necessarily the first, exactly as upstream.
function M.binary_search(arr, cmp)
    local min = 1
    local max = #arr

    while min <= max do
        local mid = (min + max) // 2
        local res = cmp(arr[mid])

        if res < 0 then
            max = mid - 1
        elseif res > 0 then
            min = mid + 1
        else
            return mid
        end
    end

    return nil
end

--- The half-open integer range [index, end_), as a one-based array.
---
--- Used to expand a cmap's character coverage. Empty when end_ <= index.
function M.range(index, end_)
    local out = {}
    local n = 0
    while index < end_ do
        n = n + 1
        out[n] = index
        index = index + 1
    end
    return out
end

--- Append `src` to `dst` in place. Upstream spreads with `res.push(...range)`,
--- which is the same thing but blows the argument limit on a long range.
function M.extend(dst, src)
    local n = #dst
    for i = 1, #src do
        dst[n + i] = src[i]
    end
    return dst
end

--- The code points of a UTF-8 string, as a one-based array.
---
--- Has no counterpart upstream, where a string is already a sequence of UTF-16
--- code units and every caller decodes surrogate pairs inline. Text arrives here
--- as UTF-8 bytes instead, so the decoding happens once, in one place, and the
--- ported loops read code points straight out of the result.
---
--- Malformed bytes each become one U+FFFD rather than raising. That is what a
--- JS string built from bad input would already contain by the time fontkit saw
--- it, and a font is not the right place to fail on someone else's mojibake.
function M.code_points(s)
    local out = {}
    local n = 0
    local i = 1
    local len = #s

    while i <= len do
        local b = s:byte(i)
        local cp, width

        if b < 0x80 then
            cp, width = b, 1
        elseif b < 0xC2 then
            -- A continuation byte on its own, or an overlong two-byte lead.
            cp, width = nil, 1
        elseif b < 0xE0 then
            cp, width = b & 0x1F, 2
        elseif b < 0xF0 then
            cp, width = b & 0x0F, 3
        elseif b < 0xF5 then
            cp, width = b & 0x07, 4
        else
            cp, width = nil, 1
        end

        if cp and i + width - 1 <= len then
            for j = i + 1, i + width - 1 do
                local c = s:byte(j)
                if c < 0x80 or c > 0xBF then
                    cp = nil
                    -- Resynchronise at the offending byte, not past it.
                    width = j - i
                    break
                end
                cp = (cp << 6) | (c & 0x3F)
            end
        elseif cp then
            cp, width = nil, len - i + 1
        end

        -- Surrogates and out-of-range values are ill-formed even when the byte
        -- pattern is not; overlong forms are already ruled out by the lead-byte
        -- ranges above except in the three- and four-byte cases.
        if cp and (cp > 0x10FFFF or (cp >= 0xD800 and cp <= 0xDFFF)
            or (width == 3 and cp < 0x800) or (width == 4 and cp < 0x10000)) then
            cp = nil
        end

        n = n + 1
        out[n] = cp or 0xFFFD
        i = i + width
    end

    return out
end

return M
