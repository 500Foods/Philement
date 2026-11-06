-- zlib/trees.lua
--   -- port of zlib's trees.c (1.3.1)
--
-- The Huffman half of DEFLATE. deflate.lua finds the matches; this file turns
-- the stream of literals and matches into bits.
--
-- The reason this is a transcription rather than an implementation deserves
-- stating, because "write a Huffman coder" sounds like a two-hour job and this
-- is not that. RFC 1951 does not say which tree to use. A block can be sent
-- three ways -- stored, with the fixed tree the spec defines, or with a tree
-- the encoder describes in the block header -- and any of the three decodes to
-- the same bytes. Within the third, several trees are optimal by total bit
-- count and differ in which code lands on which symbol. zlib picks one path
-- through all of that, and PDFKit's output *is* that path. So the goal here is
-- not a good Huffman coder; it is zlib's, down to the tie-breaks.
--
-- Three of those tie-breaks are worth naming, because they are where an
-- independent implementation would diverge while still being correct:
--
--   * The heap. Two symbols of equal frequency are ordered by `depth` -- the
--     height of the subtree already built under each -- and equal depth falls
--     back to heap position. This keeps trees shallow, and it decides code
--     assignment whenever frequencies tie, which in real text is constantly.
--
--   * The overflow repair in `gen_bitlen`. When a tree comes out deeper than
--     15 bits it is not rebuilt; leaves are moved down one level at a time
--     from the deepest length that has any, which yields a valid tree of a
--     different shape than a from-scratch rebuild would.
--
--   * The three-way block choice in `flush_block`. Stored wins on a tie with
--     dynamic; static wins on a tie with dynamic; the comparison is in whole
--     bytes after rounding, so a one-bit difference in the estimate can flip
--     the whole block to a different encoding.
--
-- INDEXING. Everything here is zero-based, as in C. Lua tables hold an index 0
-- perfectly well, and the alternative -- shifting every one of the constants
-- below by one -- would make the two files impossible to read side by side.
-- The consequence is that `#` is meaningless on these arrays; sizes are always
-- passed explicitly.

local M = {}

--------------------------------------------------------------------------------
-- Constants (deflate.h)
--------------------------------------------------------------------------------

local MAX_BITS     = 15   -- longest code the format allows for lit/len and dist
local MAX_BL_BITS  = 7    -- longest code allowed for the code-lengths tree
local LENGTH_CODES = 29   -- number of length codes, not counting the 0 length
local LITERALS     = 256  -- number of literal bytes, 0..255
local L_CODES      = LITERALS + 1 + LENGTH_CODES  -- literals + END_BLOCK + lens
local D_CODES      = 30   -- number of distance codes
local BL_CODES     = 19   -- number of codes used to transmit the bit lengths
local HEAP_SIZE    = 2 * L_CODES + 1  -- leaves plus internal nodes plus one

local END_BLOCK    = 256  -- the symbol that ends a block
local REP_3_6      = 16   -- repeat the previous length 3..6 times
local REPZ_3_10    = 17   -- repeat a zero length 3..10 times
local REPZ_11_138  = 18   -- repeat a zero length 11..138 times

local Buf_size     = 16   -- bits in the bit-accumulator, which is a uint16

local STORED_BLOCK = 0
local STATIC_TREES = 1
local DYN_TREES    = 2

local MIN_MATCH    = 3
local MAX_MATCH    = 258

M.MAX_BITS  = MAX_BITS
M.L_CODES   = L_CODES
M.D_CODES   = D_CODES
M.BL_CODES  = BL_CODES
M.HEAP_SIZE = HEAP_SIZE
M.LITERALS  = LITERALS
M.MIN_MATCH = MIN_MATCH
M.MAX_MATCH = MAX_MATCH

--------------------------------------------------------------------------------
-- Static tables
--------------------------------------------------------------------------------

-- Extra bits carried after each length code. Code 284 covers lengths 227..257
-- with five extra bits; 285 is length 258 alone with none, which is why the
-- last entry is 0 rather than 5.
local extra_lbits = { [0] =
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 }

-- Extra bits after each distance code. Distances double every two codes, so
-- this climbs by one every two entries up to 13.
local extra_dbits = { [0] =
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 }

-- Extra bits after each code-lengths code. Only the three repeat codes carry
-- any: 2 bits for REP_3_6, 3 for REPZ_3_10, 7 for REPZ_11_138.
local extra_blbits = { [0] =
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7 }

-- The order the code-lengths tree's own lengths are sent in. Front-loaded with
-- the codes most likely to be used, so the trailing zeros can be cut off.
local bl_order = { [0] =
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 }

--------------------------------------------------------------------------------
-- Trees
--------------------------------------------------------------------------------

--- A Huffman tree, as four parallel arrays.
---
--- C overlays these in a union of two uint16 fields -- Freq/Code share one word
--- and Len/Dad the other -- because a node never needs both halves of a pair at
--- once. Four arrays say the same thing without the aliasing, at the cost of
--- some memory nobody will notice.
local function new_tree(n)
    local t = { freq = {}, code = {}, len = {}, dad = {} }
    for i = 0, n - 1 do
        t.freq[i] = 0
        t.code[i] = 0
        t.len[i] = 0
        t.dad[i] = 0
    end
    return t
end

M.new_tree = new_tree

--- Reverse the low `len` bits of `code`.
---
--- Huffman codes are assigned most-significant-bit first but DEFLATE writes
--- them least-significant-bit first, so every code is stored pre-reversed and
--- the bit writer can stay ignorant of the difference.
local function bi_reverse(code, len)
    local res = 0
    repeat
        res = res | (code & 1)
        code = code >> 1
        res = res << 1
        len = len - 1
    until len <= 0
    return res >> 1
end

--- Assign codes to a tree whose lengths are already set.
---
--- The canonical Huffman construction: codes of each length are consecutive,
--- and the first code of length n+1 is (first code of length n + count of
--- length n) << 1. Given the lengths, the codes follow with no choices left to
--- make -- which is why only the lengths ever go into the block header.
local function gen_codes(tree, max_code, bl_count)
    local next_code = {}
    local code = 0

    for bits = 1, MAX_BITS do
        code = (code + bl_count[bits - 1]) << 1
        next_code[bits] = code
    end

    for n = 0, max_code do
        local len = tree.len[n]
        if len ~= 0 then
            tree.code[n] = bi_reverse(next_code[len], len)
            next_code[len] = next_code[len] + 1
        end
    end
end

--------------------------------------------------------------------------------
-- The static trees
--------------------------------------------------------------------------------

-- zlib ships these precomputed in trees.h and can also generate them at
-- startup; the generator is kept here because it is shorter than the tables and
-- shows where the odd shape of the fixed tree comes from.

local static_ltree = new_tree(L_CODES + 2)
local static_dtree = new_tree(D_CODES)
local dist_code = {}    -- distance -> distance code, in two halves
local length_code = {}  -- (match length - 3) -> length code
local base_length = {}  -- first match length that each length code covers
local base_dist = {}    -- first distance that each distance code covers

local function tr_static_init()
    -- Length (0..255, meaning match lengths 3..258) -> length code (0..28).
    local length = 0
    local code = 0
    while code < LENGTH_CODES - 1 do
        base_length[code] = length
        for _ = 1, (1 << extra_lbits[code]) do
            length_code[length] = code
            length = length + 1
        end
        code = code + 1
    end
    -- Match length 258 can be written two ways -- code 284 with five extra bits
    -- all set, or code 285 with none. The second is shorter, so the last entry
    -- is overwritten to point at it. (This is the one place the mapping is not
    -- simply the range each code covers.)
    length_code[length - 1] = code

    -- Distance (0..32767) -> distance code (0..29). The first sixteen codes
    -- cover distances 1..256 one table entry each; above that a single entry
    -- covers 128 distances, which is what the shift by 7 is doing.
    local dist = 0
    code = 0
    while code < 16 do
        base_dist[code] = dist
        for _ = 1, (1 << extra_dbits[code]) do
            dist_code[dist] = code
            dist = dist + 1
        end
        code = code + 1
    end

    dist = dist >> 7 -- from now on, all distances are divided by 128
    while code < D_CODES do
        base_dist[code] = dist << 7
        for _ = 1, (1 << (extra_dbits[code] - 7)) do
            dist_code[256 + dist] = code
            dist = dist + 1
        end
        code = code + 1
    end

    -- The fixed literal tree: 8 bits for 0..143, 9 for 144..255, 7 for the
    -- end-of-block and the low length codes, 8 for the rest.
    local bl_count = {}
    for bits = 0, MAX_BITS do bl_count[bits] = 0 end

    local n = 0
    while n <= 143 do static_ltree.len[n] = 8; bl_count[8] = bl_count[8] + 1; n = n + 1 end
    while n <= 255 do static_ltree.len[n] = 9; bl_count[9] = bl_count[9] + 1; n = n + 1 end
    while n <= 279 do static_ltree.len[n] = 7; bl_count[7] = bl_count[7] + 1; n = n + 1 end
    while n <= 287 do static_ltree.len[n] = 8; bl_count[8] = bl_count[8] + 1; n = n + 1 end

    -- Codes 286 and 287 do not exist, but they are included in the
    -- construction so the tree is canonical (the longest code is all ones).
    gen_codes(static_ltree, L_CODES + 1, bl_count)

    -- The static distance tree is trivial: thirty codes, five bits each.
    for i = 0, D_CODES - 1 do
        static_dtree.len[i] = 5
        static_dtree.code[i] = bi_reverse(i, 5)
    end
end

tr_static_init()

--- The distance code for a distance, which is stored one less than its value.
local function d_code(dist)
    if dist < 256 then
        return dist_code[dist]
    end
    return dist_code[256 + (dist >> 7)]
end

--------------------------------------------------------------------------------
-- Tree descriptors
--------------------------------------------------------------------------------

-- Each of the three trees is built by the same code, told apart by one of
-- these. `extra_base` is where the extra-bit table starts applying: the literal
-- tree's first 257 symbols are single bytes and end-of-block, which carry none.

local static_l_desc = {
    static_tree = static_ltree,
    extra_bits  = extra_lbits,
    extra_base  = LITERALS + 1,
    elems       = L_CODES,
    max_length  = MAX_BITS,
}

local static_d_desc = {
    static_tree = static_dtree,
    extra_bits  = extra_dbits,
    extra_base  = 0,
    elems       = D_CODES,
    max_length  = MAX_BITS,
}

local static_bl_desc = {
    static_tree = nil, -- there is no fixed tree for the code-lengths tree
    extra_bits  = extra_blbits,
    extra_base  = 0,
    elems       = BL_CODES,
    max_length  = MAX_BL_BITS,
}

--------------------------------------------------------------------------------
-- Bit output
--------------------------------------------------------------------------------

-- `bi_buf` is sixteen bits wide and fills from the bottom. Everything below
-- masks to 0xffff on store, because in C these are `ush` and the truncation is
-- load-bearing: `send_bits` deliberately drops the top of an over-long value
-- into the next word rather than keeping it.

local function put_byte(s, b)
    s.pending[#s.pending + 1] = b
end

local function put_short(s, w)
    put_byte(s, w & 0xff)
    put_byte(s, (w >> 8) & 0xff)
end

--- Append `length` bits of `value` to the stream, low bit first.
local function send_bits(s, value, length)
    if s.bi_valid > Buf_size - length then
        -- Does not fit: fill the accumulator, flush it, and keep the remainder.
        s.bi_buf = (s.bi_buf | ((value & 0xffff) << s.bi_valid)) & 0xffff
        put_short(s, s.bi_buf)
        s.bi_buf = (value & 0xffff) >> (Buf_size - s.bi_valid)
        s.bi_valid = s.bi_valid + length - Buf_size
    else
        s.bi_buf = (s.bi_buf | ((value & 0xffff) << s.bi_valid)) & 0xffff
        s.bi_valid = s.bi_valid + length
    end
end

--- Send symbol `c` using `tree`.
local function send_code(s, c, tree)
    send_bits(s, tree.code[c], tree.len[c])
end

--- Flush whole bytes out of the accumulator, keeping any partial byte.
local function bi_flush(s)
    if s.bi_valid == 16 then
        put_short(s, s.bi_buf)
        s.bi_buf = 0
        s.bi_valid = 0
    elseif s.bi_valid >= 8 then
        put_byte(s, s.bi_buf & 0xff)
        s.bi_buf = s.bi_buf >> 8
        s.bi_valid = s.bi_valid - 8
    end
end

--- Flush the accumulator and align to a byte boundary, padding with zeros.
local function bi_windup(s)
    if s.bi_valid > 8 then
        put_short(s, s.bi_buf)
    elseif s.bi_valid > 0 then
        put_byte(s, s.bi_buf & 0xff)
    end
    s.bi_buf = 0
    s.bi_valid = 0
end

M.bi_windup = bi_windup

--------------------------------------------------------------------------------
-- Building a tree
--------------------------------------------------------------------------------

local SMALLEST = 1 -- index of the least frequent node in the heap

--- Order two heap nodes: by frequency, then by subtree depth.
---
--- The depth tie-break is what keeps the tree balanced when frequencies are
--- equal, and it is also what makes the result depend on the order symbols were
--- pushed. It is not an optimisation detail -- it decides the codes.
local function smaller(tree, n, m, depth)
    local fn, fm = tree.freq[n], tree.freq[m]
    return fn < fm or (fn == fm and depth[n] <= depth[m])
end

--- Sift heap[k] down until the subtree rooted there is a valid heap.
local function pqdownheap(s, tree, k)
    local v = s.heap[k]
    local j = k << 1 -- left son of k

    while j <= s.heap_len do
        -- Set j to the smallest of the two sons
        if j < s.heap_len and smaller(tree, s.heap[j + 1], s.heap[j], s.depth) then
            j = j + 1
        end
        -- Exit if v is smaller than both sons
        if smaller(tree, v, s.heap[j], s.depth) then
            break
        end

        -- Exchange v with the smallest son
        s.heap[k] = s.heap[j]
        k = j

        j = j << 1
    end

    s.heap[k] = v
end

--- Turn the parent links left by build_tree into code lengths.
---
--- Walking the heap from `heap_max` upward visits nodes parent-before-child, so
--- a node's depth is always one more than its already-computed parent's. The
--- second half deals with the case where that comes out deeper than the format
--- allows.
local function gen_bitlen(s, desc)
    local tree       = desc.dyn_tree
    local max_code   = desc.max_code
    local stree      = desc.stat_desc.static_tree
    local extra      = desc.stat_desc.extra_bits
    local base       = desc.stat_desc.extra_base
    local max_length = desc.stat_desc.max_length
    local overflow   = 0

    for bits = 0, MAX_BITS do
        s.bl_count[bits] = 0
    end

    -- The root of the heap has length 0 and everything else counts from it.
    tree.len[s.heap[s.heap_max]] = 0

    for h = s.heap_max + 1, HEAP_SIZE - 1 do
        local n = s.heap[h]
        local bits = tree.len[tree.dad[n]] + 1
        if bits > max_length then
            bits = max_length
            overflow = overflow + 1
        end
        tree.len[n] = bits
        -- tree.dad[n] is overwritten here in C; nothing reads it again.

        if n <= max_code then -- a leaf, not an internal node
            s.bl_count[bits] = s.bl_count[bits] + 1
            local xbits = 0
            if n >= base then
                xbits = extra[n - base]
            end
            local f = tree.freq[n]
            s.opt_len = s.opt_len + f * (bits + xbits)
            if stree then
                s.static_len = s.static_len + f * (stree.len[n] + xbits)
            end
        end
    end

    if overflow == 0 then
        return
    end

    -- Codes came out longer than the format allows -- which does happen, on
    -- inputs with a very skewed symbol distribution. Rather than rebuild, move
    -- leaves down one level at a time: take one from the deepest length that
    -- still has a leaf to give, and it plus one overflowing leaf become a pair
    -- one level down. Two overflows are absorbed per round.
    repeat
        local bits = max_length - 1
        while s.bl_count[bits] == 0 do
            bits = bits - 1
        end
        s.bl_count[bits] = s.bl_count[bits] - 1        -- move one leaf down
        s.bl_count[bits + 1] = s.bl_count[bits + 1] + 2 -- its brother comes too
        s.bl_count[max_length] = s.bl_count[max_length] - 1
        overflow = overflow - 2
    until overflow <= 0

    -- Recompute every length from the corrected counts, walking the heap in
    -- increasing frequency. All of them, not just the wrong ones -- it is
    -- simpler, and zlib credits the idea to Haruhiko Okumura's `ar`.
    local h = HEAP_SIZE
    for bits = max_length, 1, -1 do
        local n = s.bl_count[bits]
        while n ~= 0 do
            h = h - 1
            local m = s.heap[h]
            if m <= max_code then
                if tree.len[m] ~= bits then
                    s.opt_len = s.opt_len + (bits - tree.len[m]) * tree.freq[m]
                    tree.len[m] = bits
                end
                n = n - 1
            end
        end
    end
end

--- Build a Huffman tree from the frequencies already tallied into it.
local function build_tree(s, desc)
    local tree  = desc.dyn_tree
    local stree = desc.stat_desc.static_tree
    local elems = desc.stat_desc.elems
    local max_code = -1

    -- Build the initial heap: every symbol that actually occurred, least
    -- frequent at heap[1]. Symbols that never occurred get length 0 and are
    -- left out entirely.
    s.heap_len = 0
    s.heap_max = HEAP_SIZE

    for n = 0, elems - 1 do
        if tree.freq[n] ~= 0 then
            s.heap_len = s.heap_len + 1
            s.heap[s.heap_len] = n
            max_code = n
            s.depth[n] = 0
        else
            tree.len[n] = 0
        end
    end

    -- The format requires at least one distance code, and that at least one
    -- bit be sent even where only one code is possible. Forcing two non-zero
    -- frequencies here avoids special cases everywhere downstream.
    while s.heap_len < 2 do
        s.heap_len = s.heap_len + 1
        local node
        if max_code < 2 then
            max_code = max_code + 1
            node = max_code
        else
            node = 0
        end
        s.heap[s.heap_len] = node
        tree.freq[node] = 1
        s.depth[node] = 0
        s.opt_len = s.opt_len - 1
        if stree then
            s.static_len = s.static_len - stree.len[node]
        end
        -- node is 0 or 1, so it carries no extra bits
    end

    desc.max_code = max_code

    -- heap[heap_len/2 + 1 .. heap_len] are already valid one-element heaps;
    -- sift the rest down to make the whole thing a heap.
    for n = s.heap_len // 2, 1, -1 do
        pqdownheap(s, tree, n)
    end

    -- Combine the two least frequent nodes, over and over. Each new internal
    -- node goes back into the heap, and the pair that formed it is recorded at
    -- the top of the array in the order they were removed -- which is what
    -- gen_bitlen later walks.
    local node = elems
    repeat
        -- pqremove
        local n = s.heap[SMALLEST]
        s.heap[SMALLEST] = s.heap[s.heap_len]
        s.heap_len = s.heap_len - 1
        pqdownheap(s, tree, SMALLEST)

        local m = s.heap[SMALLEST] -- node of next least frequency

        s.heap_max = s.heap_max - 1
        s.heap[s.heap_max] = n -- keep the nodes sorted by frequency
        s.heap_max = s.heap_max - 1
        s.heap[s.heap_max] = m

        -- Create a new node, father of n and m
        tree.freq[node] = tree.freq[n] + tree.freq[m]
        s.depth[node] = (s.depth[n] >= s.depth[m] and s.depth[n] or s.depth[m]) + 1
        tree.dad[n] = node
        tree.dad[m] = node

        -- and insert the new node in the heap
        s.heap[SMALLEST] = node
        node = node + 1
        pqdownheap(s, tree, SMALLEST)
    until s.heap_len < 2

    s.heap_max = s.heap_max - 1
    s.heap[s.heap_max] = s.heap[SMALLEST]

    gen_bitlen(s, desc)
    gen_codes(tree, desc.max_code, s.bl_count)
end

--------------------------------------------------------------------------------
-- The code-lengths tree
--------------------------------------------------------------------------------

-- A dynamic block header carries the code lengths of the two trees, and those
-- lengths are themselves Huffman-coded, by a third tree whose lengths are sent
-- raw in three bits each. Runs are collapsed first -- a stretch of equal
-- lengths, or of zeros, becomes a repeat code -- which matters because a
-- literal tree usually has long runs of unused symbols.

--- Count what a run-length-encoded tree would cost, into bl_tree's frequencies.
local function scan_tree(s, tree, max_code)
    local prevlen = -1              -- last emitted length
    local nextlen = tree.len[0]     -- length of next code
    local count = 0                 -- repeat count of the current code
    local max_count = 7             -- max repeat count
    local min_count = 4             -- min repeat count

    if nextlen == 0 then
        max_count = 138
        min_count = 3
    end
    tree.len[max_code + 1] = 0xffff -- guard: never equal to a real length

    for n = 0, max_code do
        local curlen = nextlen
        nextlen = tree.len[n + 1]
        count = count + 1

        if count < max_count and curlen == nextlen then
            goto continue
        elseif count < min_count then
            s.bl_tree.freq[curlen] = s.bl_tree.freq[curlen] + count
        elseif curlen ~= 0 then
            if curlen ~= prevlen then
                s.bl_tree.freq[curlen] = s.bl_tree.freq[curlen] + 1
            end
            s.bl_tree.freq[REP_3_6] = s.bl_tree.freq[REP_3_6] + 1
        elseif count <= 10 then
            s.bl_tree.freq[REPZ_3_10] = s.bl_tree.freq[REPZ_3_10] + 1
        else
            s.bl_tree.freq[REPZ_11_138] = s.bl_tree.freq[REPZ_11_138] + 1
        end

        count = 0
        prevlen = curlen

        if nextlen == 0 then
            max_count = 138
            min_count = 3
        elseif curlen == nextlen then
            max_count = 6
            min_count = 3
        else
            max_count = 7
            min_count = 4
        end

        ::continue::
    end
end

--- Emit the run-length-encoded tree. Must mirror scan_tree exactly.
local function send_tree(s, tree, max_code)
    local prevlen = -1
    local nextlen = tree.len[0]
    local count = 0
    local max_count = 7
    local min_count = 4

    -- tree.len[max_code + 1] was already set to the guard value by scan_tree.
    if nextlen == 0 then
        max_count = 138
        min_count = 3
    end

    for n = 0, max_code do
        local curlen = nextlen
        nextlen = tree.len[n + 1]
        count = count + 1

        if count < max_count and curlen == nextlen then
            goto continue
        elseif count < min_count then
            repeat
                send_code(s, curlen, s.bl_tree)
                count = count - 1
            until count == 0
        elseif curlen ~= 0 then
            if curlen ~= prevlen then
                send_code(s, curlen, s.bl_tree)
                count = count - 1
            end
            send_code(s, REP_3_6, s.bl_tree)
            send_bits(s, count - 3, 2)
        elseif count <= 10 then
            send_code(s, REPZ_3_10, s.bl_tree)
            send_bits(s, count - 3, 3)
        else
            send_code(s, REPZ_11_138, s.bl_tree)
            send_bits(s, count - 11, 7)
        end

        count = 0
        prevlen = curlen

        if nextlen == 0 then
            max_count = 138
            min_count = 3
        elseif curlen == nextlen then
            max_count = 6
            min_count = 3
        else
            max_count = 7
            min_count = 4
        end

        ::continue::
    end
end

--- Build the code-lengths tree, and say how many of its lengths must be sent.
local function build_bl_tree(s)
    scan_tree(s, s.dyn_ltree, s.l_desc.max_code)
    scan_tree(s, s.dyn_dtree, s.d_desc.max_code)

    build_tree(s, s.bl_desc)

    -- The trailing entries of bl_order are the least-used codes, so any that
    -- ended up unused can simply not be sent. At least four always are: the
    -- format says three, but four is the value everyone actually uses.
    local max_blindex = BL_CODES - 1
    while max_blindex >= 3 do
        if s.bl_tree.len[bl_order[max_blindex]] ~= 0 then
            break
        end
        max_blindex = max_blindex - 1
    end

    -- Account for the header this tree costs, so flush_block's comparison is
    -- against the full size of a dynamic block.
    s.opt_len = s.opt_len + 3 * (max_blindex + 1) + 5 + 5 + 4

    return max_blindex
end

--- Send the two trees, preceded by the tree that describes them.
local function send_all_trees(s, lcodes, dcodes, blcodes)
    send_bits(s, lcodes - 257, 5) -- not +255, whatever appnote.txt says
    send_bits(s, dcodes - 1, 5)
    send_bits(s, blcodes - 4, 4)  -- not -3, likewise

    for rank = 0, blcodes - 1 do
        send_bits(s, s.bl_tree.len[bl_order[rank]], 3)
    end

    send_tree(s, s.dyn_ltree, lcodes - 1)
    send_tree(s, s.dyn_dtree, dcodes - 1)
end

--------------------------------------------------------------------------------
-- Blocks
--------------------------------------------------------------------------------

--- Reset the frequency counts for a new block.
local function init_block(s)
    for n = 0, L_CODES - 1 do s.dyn_ltree.freq[n] = 0 end
    for n = 0, D_CODES - 1 do s.dyn_dtree.freq[n] = 0 end
    for n = 0, BL_CODES - 1 do s.bl_tree.freq[n] = 0 end

    -- Every block ends with END_BLOCK, so it always has a frequency of one.
    s.dyn_ltree.freq[END_BLOCK] = 1
    s.opt_len = 0
    s.static_len = 0
    s.sym_next = 0
    s.matches = 0
end

--- Set up the tree state. Called once per stream.
function M.tr_init(s)
    s.dyn_ltree = new_tree(HEAP_SIZE)
    s.dyn_dtree = new_tree(2 * D_CODES + 1)
    s.bl_tree   = new_tree(2 * BL_CODES + 1)

    s.l_desc  = { dyn_tree = s.dyn_ltree, max_code = 0, stat_desc = static_l_desc }
    s.d_desc  = { dyn_tree = s.dyn_dtree, max_code = 0, stat_desc = static_d_desc }
    s.bl_desc = { dyn_tree = s.bl_tree,   max_code = 0, stat_desc = static_bl_desc }

    s.bi_buf = 0
    s.bi_valid = 0

    s.heap = {}
    s.depth = {}
    for i = 0, HEAP_SIZE - 1 do
        s.heap[i] = 0
        s.depth[i] = 0
    end
    s.bl_count = {}
    for i = 0, MAX_BITS do
        s.bl_count[i] = 0
    end

    -- Symbol buffer: three bytes per entry, distance low, distance high,
    -- length-or-literal. zlib overlays this on the pending output buffer to
    -- save memory; here they are separate, which changes nothing in the output
    -- and removes the one assertion in compress_block that guards the overlap.
    s.sym_buf = {}
    s.sym_end = (s.lit_bufsize - 1) * 3

    init_block(s)
end

--- Emit a stored block: the bytes verbatim, with a length and its complement.
local function tr_stored_block(s, buf, stored_len, last)
    send_bits(s, (STORED_BLOCK << 1) + last, 3)
    bi_windup(s) -- a stored block starts on a byte boundary
    put_short(s, stored_len & 0xffff)
    put_short(s, (~stored_len) & 0xffff)
    for i = 0, stored_len - 1 do
        put_byte(s, s.window[buf + i])
    end
end

M.tr_stored_block = tr_stored_block

--- Send an empty static block, to flush the bit buffer to a byte boundary.
---
--- Used by Z_SYNC_FLUSH and friends, and by Z_PARTIAL_FLUSH. Nothing PDFKit
--- does reaches it, since it compresses each object in one shot with Z_FINISH.
function M.tr_align(s)
    send_bits(s, STATIC_TREES << 1, 3)
    send_code(s, END_BLOCK, static_ltree)
    bi_flush(s)
end

--- Write out the block's symbols with the trees just built.
local function compress_block(s, ltree, dtree)
    local sx = 0

    if s.sym_next ~= 0 then
        repeat
            local dist = s.sym_buf[sx] & 0xff
            sx = sx + 1
            dist = dist + ((s.sym_buf[sx] & 0xff) << 8)
            sx = sx + 1
            local lc = s.sym_buf[sx]
            sx = sx + 1

            if dist == 0 then
                send_code(s, lc, ltree) -- a literal byte
            else
                -- lc is the match length minus MIN_MATCH
                local code = length_code[lc]
                send_code(s, code + LITERALS + 1, ltree)
                local extra = extra_lbits[code]
                if extra ~= 0 then
                    send_bits(s, lc - base_length[code], extra)
                end

                dist = dist - 1 -- dist is now the match distance minus one
                code = d_code(dist)
                send_code(s, code, dtree)
                extra = extra_dbits[code]
                if extra ~= 0 then
                    send_bits(s, dist - base_dist[code], extra)
                end
            end
        until sx >= s.sym_next
    end

    send_code(s, END_BLOCK, ltree)
end

--- Close a block: build the trees, pick an encoding, and emit it.
---
--- `buf` is the window offset the block's raw bytes start at, or nil when the
--- window has slid far enough that they are no longer all there -- in which
--- case a stored block is not available even if it would be smaller.
function M.tr_flush_block(s, buf, stored_len, last)
    local opt_lenb, static_lenb
    local max_blindex = 0

    if s.level > 0 then
        -- zlib also sets strm.data_type here, from detect_data_type(), which
        -- reports Z_TEXT or Z_BINARY to the caller. It has no effect on the
        -- output, and nothing in this port asks, so it is not ported.

        build_tree(s, s.l_desc)
        build_tree(s, s.d_desc)
        -- opt_len and static_len are now the block's cost in bits under each
        -- of the two trees, not counting the trees themselves.

        max_blindex = build_bl_tree(s)

        -- Compare in bytes, rounding up, with three bits of block header.
        opt_lenb = (s.opt_len + 3 + 7) >> 3
        static_lenb = (s.static_len + 3 + 7) >> 3

        if static_lenb <= opt_lenb or s.strategy == 4 then -- Z_FIXED
            opt_lenb = static_lenb
        end
    else
        -- Level 0 never builds trees; everything is stored.
        opt_lenb = stored_len + 5
        static_lenb = opt_lenb
    end

    if stored_len + 4 <= opt_lenb and buf ~= nil then
        -- 4: the two length words. Stored wins ties, which is deliberate --
        -- it is cheaper to decode and it cannot be worse by more than the
        -- rounding.
        tr_stored_block(s, buf, stored_len, last)

    elseif s.strategy == 4 or static_lenb == opt_lenb then -- Z_FIXED
        send_bits(s, (STATIC_TREES << 1) + last, 3)
        compress_block(s, static_ltree, static_dtree)

    else
        send_bits(s, (DYN_TREES << 1) + last, 3)
        send_all_trees(s, s.l_desc.max_code + 1, s.d_desc.max_code + 1,
            max_blindex + 1)
        compress_block(s, s.dyn_ltree, s.dyn_dtree)
    end

    init_block(s)

    if last == 1 then
        bi_windup(s)
    end
end

--------------------------------------------------------------------------------
-- Tallying
--------------------------------------------------------------------------------

--- Record a literal or a match, and say whether the block is full.
---
--- `dist` of 0 means `lc` is a literal byte; otherwise `dist` is the match
--- distance and `lc` the match length minus MIN_MATCH. Returns true when the
--- symbol buffer is full and the caller should flush.
function M.tr_tally(s, dist, lc)
    s.sym_buf[s.sym_next] = dist & 0xff
    s.sym_buf[s.sym_next + 1] = (dist >> 8) & 0xff
    s.sym_buf[s.sym_next + 2] = lc & 0xff
    s.sym_next = s.sym_next + 3

    if dist == 0 then
        s.dyn_ltree.freq[lc] = s.dyn_ltree.freq[lc] + 1
    else
        s.matches = s.matches + 1
        dist = dist - 1 -- dist = match distance - 1
        local lcode = length_code[lc] + LITERALS + 1
        s.dyn_ltree.freq[lcode] = s.dyn_ltree.freq[lcode] + 1
        local dcode = d_code(dist)
        s.dyn_dtree.freq[dcode] = s.dyn_dtree.freq[dcode] + 1
    end

    return s.sym_next == s.sym_end
end

return M
