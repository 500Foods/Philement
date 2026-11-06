-- md5.lua -- RFC 1321, in pure Lua
--
-- PDFKit derives every document's /ID from an MD5 of its info dictionary, so a
-- port that wants to reproduce the trailer needs MD5. Node reaches for a
-- library; we cannot, so here it is. Lua 5.3 onwards has real integers and real
-- bitwise operators, which makes this a direct transcription of the spec rather
-- than the float-arithmetic contortion older Lua MD5s had to resort to.
--
-- The one thing to stay alert to: Lua integers are 64-bit, so every operation
-- that could carry above bit 31 -- addition, left shift, and bitwise NOT --
-- must be masked back to 32 bits. Missing one of those masks produces a hash
-- that is wrong only for some inputs, which is the worst kind of wrong.

local M = {}

local MASK = 0xFFFFFFFF

-- Per-round constants: K[i] = floor(abs(sin(i)) * 2^32), i counted from 1.
local K = {}
for i = 1, 64 do
    K[i] = math.floor(math.abs(math.sin(i)) * 4294967296.0) & MASK
end

-- Per-round left-rotation amounts.
local S = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
}

local function rotl32(x, n)
    x = x & MASK
    return ((x << n) | (x >> (32 - n))) & MASK
end

-- Append the 0x80 terminator, zero-pad to 56 bytes mod 64, then the original
-- length in bits as a little-endian 64-bit integer.
local function pad(msg)
    local len = #msg
    local padlen = (56 - (len + 1) % 64) % 64
    return msg .. "\128" .. string.rep("\0", padlen) .. string.pack("<I8", (len * 8) & 0xFFFFFFFFFFFFFFFF)
end

-- Returns the digest as a 16-byte binary string.
function M.digest(msg)
    local data = pad(msg)

    local h0, h1, h2, h3 = 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476

    for chunk = 1, #data, 64 do
        -- The chunk as sixteen little-endian 32-bit words, indexed 1..16.
        local w = { string.unpack("<I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4I4", data, chunk) }

        local a, b, c, d = h0, h1, h2, h3

        for i = 0, 63 do
            local f, g
            if i < 16 then
                f = (b & c) | ((~b & MASK) & d)
                g = i
            elseif i < 32 then
                f = (d & b) | ((~d & MASK) & c)
                g = (5 * i + 1) % 16
            elseif i < 48 then
                f = b ~ c ~ d
                g = (3 * i + 5) % 16
            else
                f = c ~ (b | (~d & MASK))
                g = (7 * i) % 16
            end

            f = (f + a + K[i + 1] + w[g + 1]) & MASK
            a = d
            d = c
            c = b
            b = (b + rotl32(f, S[i + 1])) & MASK
        end

        h0 = (h0 + a) & MASK
        h1 = (h1 + b) & MASK
        h2 = (h2 + c) & MASK
        h3 = (h3 + d) & MASK
    end

    return string.pack("<I4I4I4I4", h0, h1, h2, h3)
end

-- Returns the digest as 32 lowercase hex characters.
function M.hex(msg)
    return (M.digest(msg):gsub(".", function(ch)
        return string.format("%02x", ch:byte())
    end))
end

return M
