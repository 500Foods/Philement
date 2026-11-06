-- flate.lua -- no counterpart in PDFKit
--
-- PDFKit reaches for Node's `zlib` in three places: decoding PNG pixel data,
-- re-encoding the alpha channel it splits out of an RGBA image, and compressing
-- object streams. Lua has no zlib, so DEFLATE lives here.
--
-- The decompressor is in this file: RFC 1951 stored, fixed-Huffman and
-- dynamic-Huffman blocks, wrapped in the RFC 1950 zlib container. Anything zlib
-- produced, this reads.
--
-- The compressor is not, and the split is not arbitrary. Decompressing is
-- decided by the format -- there is exactly one answer, and any correct
-- implementation finds it. Compressing is not: DEFLATE lets an encoder emit any
-- of a great many valid encodings of the same bytes, and which one zlib picks
-- falls out of its match-finding heuristics -- how deep it walks a hash chain,
-- when it prefers a later match to an earlier one, where it closes a block,
-- whether it spends bits on a custom tree. Byte-identical output is this
-- library's contract, so those heuristics cannot be approximated; they have to
-- be transcribed. That is what zlib/deflate.lua and zlib/trees.lua are, and
-- `M.deflate` below is the door to them.
--
-- Sizes: inflating produces a byte array and converts once at the end, so peak
-- memory is roughly nine times the decompressed size while it runs. For the
-- images PDFKit embeds that is fine; for a hundred-megabyte stream it is not.

local deflate = require("pdfkit.zlib.deflate")

local byte = string.byte
local char = string.char
local concat = table.concat
local unpack = table.unpack

local M = {}

--------------------------------------------------------------------------------
-- RFC 1951 constants
--------------------------------------------------------------------------------

-- Length codes 257..285: the base length each encodes, and how many extra bits
-- follow. Code 285 is the odd one out -- it means exactly 258 with no extra
-- bits, where the pattern would predict a base of 259.
local LENGTH_BASE = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
    67, 83, 99, 115, 131, 163, 195, 227, 258,
}
local LENGTH_EXTRA = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 5, 5, 5, 5, 0,
}

-- Distance codes 0..29.
local DIST_BASE = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
    769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
}
local DIST_EXTRA = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8,
    9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
}

-- The code-length alphabet is itself Huffman coded, and its lengths arrive in
-- this order rather than in symbol order. The order front-loads the symbols
-- most likely to be used, so trailing zeros can be omitted.
local CLCL_ORDER = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
}

local MAX_BITS = 15

-- Width of the direct-lookup table. Nine bits covers the overwhelming majority
-- of codes in real streams while keeping the table at 512 entries, small enough
-- to rebuild for every dynamic block without the build costing more than the
-- decode it saves.
local FAST_BITS = 9
local FAST_SIZE = 1 << FAST_BITS

--------------------------------------------------------------------------------
-- Huffman tables
--------------------------------------------------------------------------------

-- Reverse the low `width` bits of `v`.
--
-- Huffman codes are written most significant bit first, but the bit stream
-- hands over least significant bit first, so a code read straight from the
-- stream arrives backwards. Reversing it once at table-build time means the
-- decoder can use the bits exactly as they arrive.
local function reverse_bits(v, width)
    local r = 0
    for _ = 1, width do
        r = (r << 1) | (v & 1)
        v = v >> 1
    end
    return r
end

--- Build a decoding table from a list of code lengths.
---
--- `lengths` is one-based with `n` entries; entry i is the code length of
--- symbol i-1, or 0 if that symbol is unused. Returns a table with:
---
---   fast    2^FAST_BITS entries, each `symbol * 16 + length`, or nil
---   count   count[len] = how many codes have this length
---   symbol  symbols in canonical order, zero-based
---
--- Codes longer than FAST_BITS are absent from `fast` and are decoded by the
--- linear walk in `decode` below.
local function build_huffman(lengths, n)
    local count = {}
    for len = 0, MAX_BITS do count[len] = 0 end
    for i = 1, n do
        count[lengths[i]] = count[lengths[i]] + 1
    end
    count[0] = 0

    -- Canonical order: shorter codes first, and within a length, lower symbols
    -- first. `offset[len]` walks along the block of symbols for each length.
    local offset = {}
    local total = 0
    for len = 1, MAX_BITS do
        offset[len] = total
        total = total + count[len]
    end

    local symbol = {}
    for i = 1, n do
        local len = lengths[i]
        if len ~= 0 then
            symbol[offset[len]] = i - 1
            offset[len] = offset[len] + 1
        end
    end

    -- The first code of each length, per RFC 1951 section 3.2.2.
    local next_code = {}
    local code = 0
    for len = 1, MAX_BITS do
        code = (code + count[len - 1]) << 1
        next_code[len] = code
    end

    -- Fill the direct-lookup table. A code shorter than FAST_BITS matches every
    -- index whose low bits are the code, so it is written into all of them --
    -- that is what lets the decoder peek a fixed number of bits without knowing
    -- the code length in advance.
    local fast = {}
    for i = 1, n do
        local len = lengths[i]
        if len ~= 0 and len <= FAST_BITS then
            local c = next_code[len]
            next_code[len] = c + 1
            local rc = reverse_bits(c, len)
            local packed = (i - 1) * 16 + len
            local step = 1 << len
            for index = rc, FAST_SIZE - 1, step do
                fast[index] = packed
            end
        elseif len ~= 0 then
            next_code[len] = next_code[len] + 1
        end
    end

    return { fast = fast, count = count, symbol = symbol }
end

-- The fixed trees of RFC 1951 section 3.2.6, built once and shared. Literal
-- lengths are 8, 9, 7, 8 over four ranges; every distance code is 5 bits.
local FIXED_LIT, FIXED_DIST
do
    local lengths = {}
    for i = 1, 144 do lengths[i] = 8 end
    for i = 145, 256 do lengths[i] = 9 end
    for i = 257, 280 do lengths[i] = 7 end
    for i = 281, 288 do lengths[i] = 8 end
    FIXED_LIT = build_huffman(lengths, 288)

    local dist = {}
    for i = 1, 30 do dist[i] = 5 end
    FIXED_DIST = build_huffman(dist, 30)
end

--------------------------------------------------------------------------------
-- Inflate
--------------------------------------------------------------------------------

--- Decompress a raw DEFLATE stream (RFC 1951).
---
--- `start` is the one-based byte offset to begin at, default 1. Returns the
--- decompressed string and the one-based offset just past the stream.
function M.inflate_raw(s, start)
    local pos = start or 1
    local slen = #s

    -- Bit accumulator. Bits are taken from the low end; bytes are appended at
    -- the high end. Lua integers are 64-bit, so there is ample headroom for the
    -- 15-bit peeks below without ever needing to normalise.
    local bitbuf, bitcnt = 0, 0

    local function need(want)
        while bitcnt < want do
            local b = byte(s, pos)
            if b == nil then
                -- Past the end. Feeding zeros lets a truncated stream fail at
                -- the structural check below rather than in arithmetic.
                b = 0
            end
            pos = pos + 1
            bitbuf = bitbuf | (b << bitcnt)
            bitcnt = bitcnt + 8
        end
    end

    local function bits(want)
        if want == 0 then return 0 end
        need(want)
        local v = bitbuf & ((1 << want) - 1)
        bitbuf = bitbuf >> want
        bitcnt = bitcnt - want
        return v
    end

    --- Decode one symbol from `tree`.
    local function decode(tree)
        need(FAST_BITS)
        local packed = tree.fast[bitbuf & (FAST_SIZE - 1)]
        if packed then
            local len = packed & 15
            bitbuf = bitbuf >> len
            bitcnt = bitcnt - len
            return packed >> 4
        end

        -- Longer than the table covers. Walk the canonical code lengths,
        -- consuming one bit at a time: at each length, the codes of that length
        -- occupy a contiguous run starting at `first`, so a code inside the run
        -- identifies its symbol by its offset within it.
        local count, symbol = tree.count, tree.symbol
        local code, first, index = 0, 0, 0
        for len = 1, MAX_BITS do
            code = code | bits(1)
            local n = count[len]
            if code - first < n then
                return symbol[index + code - first]
            end
            index = index + n
            first = (first + n) << 1
            code = code << 1
        end
        error("pdfkit: corrupt deflate stream (no code matched)")
    end

    -- Output as a flat array of byte values. Back-references reach up to 32 KB
    -- into what has already been produced, so it has to stay addressable until
    -- the stream ends.
    local out, n = {}, 0

    repeat
        local final = bits(1)
        local btype = bits(2)

        if btype == 0 then
            -- Stored: discard the rest of the current byte, then copy through.
            bitbuf, bitcnt = 0, 0
            local len = byte(s, pos) | (byte(s, pos + 1) << 8)
            -- Bytes 3 and 4 are the one's complement of the length; checking
            -- them is the only integrity check a stored block offers.
            local nlen = byte(s, pos + 2) | (byte(s, pos + 3) << 8)
            if (~len & 0xFFFF) ~= nlen then
                error("pdfkit: corrupt deflate stream (stored length mismatch)")
            end
            pos = pos + 4
            for i = 0, len - 1 do
                n = n + 1
                out[n] = byte(s, pos + i)
            end
            pos = pos + len
        elseif btype == 1 or btype == 2 then
            local lit, dist

            if btype == 1 then
                lit, dist = FIXED_LIT, FIXED_DIST
            else
                -- Dynamic: the two trees for this block are themselves encoded,
                -- using a third tree over the code-length alphabet.
                local hlit = bits(5) + 257
                local hdist = bits(5) + 1
                local hclen = bits(4) + 4

                local cl_lengths = {}
                for i = 1, 19 do cl_lengths[i] = 0 end
                for i = 1, hclen do
                    cl_lengths[CLCL_ORDER[i] + 1] = bits(3)
                end
                local cl_tree = build_huffman(cl_lengths, 19)

                -- Symbols 16, 17 and 18 are run-length escapes; everything else
                -- is a literal code length. The two trees are decoded as one
                -- run, so a repeat can straddle the boundary between them.
                local all = {}
                local want = hlit + hdist
                local i = 1
                while i <= want do
                    local sym = decode(cl_tree)
                    if sym < 16 then
                        all[i] = sym
                        i = i + 1
                    elseif sym == 16 then
                        local prev = all[i - 1]
                        if prev == nil then
                            error("pdfkit: corrupt deflate stream (repeat with no previous length)")
                        end
                        for _ = 1, 3 + bits(2) do
                            all[i] = prev
                            i = i + 1
                        end
                    elseif sym == 17 then
                        for _ = 1, 3 + bits(3) do
                            all[i] = 0
                            i = i + 1
                        end
                    else
                        for _ = 1, 11 + bits(7) do
                            all[i] = 0
                            i = i + 1
                        end
                    end
                end

                local lit_lengths = {}
                for k = 1, hlit do lit_lengths[k] = all[k] end
                local dist_lengths = {}
                for k = 1, hdist do dist_lengths[k] = all[hlit + k] end

                lit = build_huffman(lit_lengths, hlit)
                dist = build_huffman(dist_lengths, hdist)
            end

            while true do
                local sym = decode(lit)
                if sym < 256 then
                    n = n + 1
                    out[n] = sym
                elseif sym == 256 then
                    break -- end of block
                else
                    local li = sym - 256
                    if li > 29 then
                        error("pdfkit: corrupt deflate stream (bad length code)")
                    end
                    local length = LENGTH_BASE[li] + bits(LENGTH_EXTRA[li])

                    local di = decode(dist) + 1
                    if di > 30 then
                        error("pdfkit: corrupt deflate stream (bad distance code)")
                    end
                    local distance = DIST_BASE[di] + bits(DIST_EXTRA[di])
                    if distance > n then
                        error("pdfkit: corrupt deflate stream (distance before start)")
                    end

                    -- Copied one byte at a time on purpose: a run shorter than
                    -- the length it fills is how DEFLATE encodes repetition, so
                    -- the copy is expected to read bytes this very loop wrote.
                    local from = n - distance
                    for k = 1, length do
                        n = n + 1
                        out[n] = out[from + k]
                    end
                end
            end
        else
            error("pdfkit: corrupt deflate stream (reserved block type)")
        end
    until final == 1

    -- Hand back any whole bytes still sitting in the accumulator.
    pos = pos - (bitcnt // 8)

    -- Assemble in chunks: table.unpack has a ceiling on how many values it can
    -- return, well below the size of a decoded image.
    local chunks = {}
    for i = 1, n, 4096 do
        local last = i + 4095
        if last > n then last = n end
        chunks[#chunks + 1] = char(unpack(out, i, last))
    end

    if pos > slen + 1 then pos = slen + 1 end
    return concat(chunks), pos
end

--- Adler-32, the checksum the zlib container carries (RFC 1950 section 9).
function M.adler32(s)
    local a, b = 1, 0
    -- 5552 is the largest run that cannot overflow the 32-bit accumulators,
    -- which is why the modulo sits outside the inner loop.
    for start = 1, #s, 5552 do
        local last = start + 5551
        if last > #s then last = #s end
        for i = start, last do
            a = a + byte(s, i)
            b = b + a
        end
        a = a % 65521
        b = b % 65521
    end
    return (b << 16) | a
end

--- Decompress a zlib stream (RFC 1950): two header bytes, deflate data, then a
--- four-byte Adler-32 of the decompressed content.
function M.inflate(s, verify)
    if #s < 2 then
        error("pdfkit: zlib stream too short")
    end

    local cmf = byte(s, 1)
    local flg = byte(s, 2)

    if (cmf & 0x0F) ~= 8 then
        error("pdfkit: unsupported zlib compression method")
    end
    if ((cmf << 8) | flg) % 31 ~= 0 then
        error("pdfkit: bad zlib header check")
    end

    local start = 3
    if (flg & 0x20) ~= 0 then
        start = start + 4 -- FDICT: a preset dictionary id we cannot satisfy
        error("pdfkit: zlib preset dictionaries are not supported")
    end

    local out, after = M.inflate_raw(s, start)

    if verify then
        local want = 0
        for i = after, after + 3 do
            want = (want << 8) | (byte(s, i) or 0)
        end
        if want ~= M.adler32(out) then
            error("pdfkit: zlib checksum mismatch")
        end
    end

    return out
end

--------------------------------------------------------------------------------
-- Deflate
--------------------------------------------------------------------------------

--- Compress `s` into a zlib stream, as `zlib.deflateSync(buf, opts)` would.
---
--- The work is in zlib/deflate.lua and zlib/trees.lua; this is the name the
--- rest of the library calls. Options are Node's -- `level`, `windowBits`,
--- `memLevel`, `strategy` -- all defaulted to Node's defaults.
function M.deflate(s, opts)
    return deflate.deflate(s, opts)
end

return M
