-- base64.lua -- no direct counterpart in PDFKit
--
-- JavaScript gets base64 for free from Buffer.from(s, 'base64'); Lua does not,
-- and data: URIs are an ordinary way to hand over an attachment or an image, so
-- the decoder has to exist somewhere. Here it is, on its own.
--
-- Decoding is deliberately forgiving in the same way Buffer is: whitespace and
-- padding are skipped, and anything else outside the alphabet is ignored rather
-- than raising. A truncated final group contributes whatever whole bytes it can.

local M = {}

local ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

-- Reverse lookup, built once. URL-safe variants map onto the same values, so a
-- data URI using - and _ decodes without special handling.
local DECODE = {}
for i = 1, #ALPHABET do
    DECODE[ALPHABET:byte(i)] = i - 1
end
DECODE[string.byte("-")] = 62
DECODE[string.byte("_")] = 63

function M.decode(s)
    local out = {}
    local acc, bits = 0, 0

    for i = 1, #s do
        local v = DECODE[s:byte(i)]
        if v then
            acc = (acc << 6) | v
            bits = bits + 6
            if bits >= 8 then
                bits = bits - 8
                out[#out + 1] = string.char((acc >> bits) & 0xFF)
            end
        end
    end

    return table.concat(out)
end

function M.encode(s)
    local out = {}

    for i = 1, #s, 3 do
        local a, b, c = s:byte(i, i + 2)
        local n = a << 16 | (b or 0) << 8 | (c or 0)

        out[#out + 1] = ALPHABET:sub((n >> 18 & 63) + 1, (n >> 18 & 63) + 1)
        out[#out + 1] = ALPHABET:sub((n >> 12 & 63) + 1, (n >> 12 & 63) + 1)
        out[#out + 1] = b and ALPHABET:sub((n >> 6 & 63) + 1, (n >> 6 & 63) + 1) or "="
        out[#out + 1] = c and ALPHABET:sub((n & 63) + 1, (n & 63) + 1) or "="
    end

    return table.concat(out)
end

return M
