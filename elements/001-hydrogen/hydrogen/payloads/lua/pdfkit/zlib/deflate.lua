-- zlib/deflate.lua
--   -- port of zlib's deflate.c (1.3.1)
--
-- The match finder. trees.lua turns symbols into bits; this decides what the
-- symbols are -- which stretches of the input to send as back-references and
-- which as plain bytes -- and drives the stream from header to trailer.
--
-- WHY THIS IS A TRANSCRIPTION. DEFLATE gives an encoder complete freedom over
-- which matches to emit. Every choice below is a heuristic with no right
-- answer, and a different set of heuristics compresses just as legally to
-- different bytes:
--
--   * How far back to search. The hash chain is walked at most `max_chain`
--     links (128 at level 6), and whatever the best match was when the budget
--     ran out is the one used. Search one link further and the output changes.
--
--   * Lazy matching. On finding a match at position n, the encoder does not
--     take it -- it looks again at n+1, and prefers the later match if it is
--     strictly longer. So a match is emitted one position after it was found,
--     and the byte it displaced becomes a literal.
--
--   * The `TOO_FAR` rule. A three-byte match more than 4096 bytes back is
--     discarded, because the distance code costs more than the three literals
--     save. 4096 exactly, not 4095 or 5000.
--
--   * Where blocks end. A block closes when the symbol buffer fills at 16383
--     entries, not on any property of the data.
--
-- None of these is discoverable from the format. They are zlib's, and PDFKit's
-- output is zlib's output, so they are reproduced here rather than reinvented.
--
-- WHICH ZLIB. There are two, and they do not agree. Chromium ships a fork that
-- replaces the one thing that matters here -- the hash function that indexes
-- the match chains -- and some Node builds bundle it. Which one a given Node
-- has is a property of that build, and `process.versions.zlib` is how to tell:
-- the fork reports a version with a hash suffix, stock reports a plain
-- version. Node v24.22.2 on this machine reports "1.3.2.1-motley-285e94b",
-- and its deflateSync output matches the chromium hash below.
--
-- Stock zlib rolls a hash over three bytes -- shift left five, xor the next
-- byte, mask -- so it can carry the value from one position to the next by
-- folding in one byte. Chromium drops the rolling property to gain a wider
-- key: it reads four bytes as a little-endian word, multiplies by 66521 in
-- 32-bit arithmetic, and takes the middle of the wrapped product.
--
-- Three consequences follow, all of them visible in the output:
--
--   * A three-byte repeat whose fourth byte differs no longer lands in the
--     same chain, so it is never found and never emitted. Compressing
--     "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do
--     eiusmod..." the two disagree on four matches out of five -- stock finds
--     "t, " again at distance 29 and Chromium does not.
--
--   * The chains themselves hold different things, because two strings that
--     collide under one hash rarely collide under the other. That decides
--     which candidate a bounded search reaches first, and at level 1 the
--     budget is four links.
--
--   * `memLevel` stops shrinking the hash table. Chromium floors it at 32768
--     buckets whatever the caller asked for, on the grounds that the window
--     memory is already committed. memLevel still shrinks the symbol buffer,
--     so it still moves the block boundaries -- just not the chains.
--
-- Both are correct DEFLATE. Matching Node v24 means the chromium hash, and
-- that is the default here. `hash = "rolling"` selects stock zlib, for a Node
-- whose `process.versions.zlib` is a plain version.
--
-- Pick the wrong one and the failure is quiet. Every stream still inflates to
-- the right bytes; short inputs and runs of a repeated byte are unaffected;
-- what breaks is the three-byte repeat whose fourth byte differs, one match at
-- a time. The first one in Helvetica.afm is 124 bytes in, and skipping it
-- moves the whole block from dynamic to static Huffman -- a one-byte
-- difference that looks like a rounding error and is not. Check against
-- zlib.deflateSync on real input rather than reasoning about which fork Node
-- ought to have.
--
-- HOW FAITHFUL. Faithful enough that the output buffer is modelled. Levels 1-9
-- do not care how much output space is available -- running out only pauses
-- the encoder -- but level 0 chooses its stored-block boundaries from exactly
-- that number, so `CHUNK_SIZE` below matches what Node hands to zlib. Without
-- it, level 0 would produce correct but differently-split output.
--
-- INDEXING. Zero-based throughout, as in C, and for the same reason as
-- trees.lua: the window, the hash chains and the block offsets are all
-- arithmetic on positions, and shifting them by one would mean rewriting every
-- bound in the file. `#` is meaningless on these arrays.

local trees = require("pdfkit.zlib.trees")

local char = string.char
local byte = string.byte
local concat = table.concat

local M = {}

--------------------------------------------------------------------------------
-- Constants
--------------------------------------------------------------------------------

local MIN_MATCH = 3
local MAX_MATCH = 258

-- Enough lookahead for the longest match, plus the three bytes that must
-- follow it to be inserted in the hash table, plus one.
local MIN_LOOKAHEAD = MAX_MATCH + MIN_MATCH + 1  -- 262

-- Bytes zeroed above the data so the match loops read defined values.
local WIN_INIT = MAX_MATCH

local MAX_STORED = 65535 -- largest stored block the format allows

local NIL = 0 -- "no such position"; position 0 is never matched, deliberately

-- A three-byte match further back than this costs more than it saves.
local TOO_FAR = 4096

-- Flush values, from zlib.h.
local Z_NO_FLUSH      = 0
local Z_PARTIAL_FLUSH = 1
local Z_SYNC_FLUSH    = 2
local Z_FULL_FLUSH    = 3
local Z_FINISH        = 4
local Z_BLOCK         = 5

-- Strategies, from zlib.h.
local Z_FILTERED     = 1
local Z_HUFFMAN_ONLY = 2
local Z_RLE          = 3
local Z_FIXED        = 4

-- Return codes from the deflate_* functions.
local NEED_MORE      = 0
local BLOCK_DONE     = 1
local FINISH_STARTED = 2
local FINISH_DONE    = 3

-- Node hands zlib 16 KB of output space at a time (Z_DEFAULT_CHUNK). Only
-- level 0 can see this, but it sees it clearly -- see the header.
local CHUNK_SIZE = 16 * 1024

--- Per-level tuning. `good_length` is the match length at which the search
--- budget is cut to a quarter; `max_lazy` is the length above which the encoder
--- stops looking for something better; `nice_length` ends the search outright;
--- `max_chain` is the hash chain budget.
---
--- Levels 1-3 use deflate_fast, which takes the first good match it finds.
--- 4-9 use deflate_slow, which is the lazy matcher. Level 0 stores.
local configuration_table = {
    [0] = { good = 0,  lazy = 0,   nice = 0,   chain = 0,    func = "stored" },
    [1] = { good = 4,  lazy = 4,   nice = 8,   chain = 4,    func = "fast"   },
    [2] = { good = 4,  lazy = 5,   nice = 16,  chain = 8,    func = "fast"   },
    [3] = { good = 4,  lazy = 6,   nice = 32,  chain = 32,   func = "fast"   },
    [4] = { good = 4,  lazy = 4,   nice = 16,  chain = 16,   func = "slow"   },
    [5] = { good = 8,  lazy = 16,  nice = 32,  chain = 32,   func = "slow"   },
    [6] = { good = 8,  lazy = 16,  nice = 128, chain = 128,  func = "slow"   },
    [7] = { good = 8,  lazy = 32,  nice = 128, chain = 256,  func = "slow"   },
    [8] = { good = 32, lazy = 128, nice = 258, chain = 1024, func = "slow"   },
    [9] = { good = 32, lazy = 258, nice = 258, chain = 4096, func = "slow"   },
}

--------------------------------------------------------------------------------
-- Output
--------------------------------------------------------------------------------

--- Move bytes from the pending buffer to the output, as far as space allows.
---
--- In C this is a memcpy into the caller's buffer bounded by `avail_out`. Here
--- the output is a list of strings joined at the end, but `avail_out` is still
--- tracked and still runs out, because deflate_stored reads it.
local function flush_pending(s)
    local pending = s.pending
    local n = #pending
    if n == 0 then
        return
    end

    local take = n
    if take > s.avail_out then
        take = s.avail_out
    end
    if take == 0 then
        return
    end

    -- string.char in slices: one call per byte is slow, and one call for
    -- 65535 of them overflows the argument stack.
    local out = s.output
    local i = 1
    while i <= take do
        local j = i + 4095
        if j > take then j = take end
        out[#out + 1] = char(table.unpack(pending, i, j))
        i = j + 1
    end

    s.total_out = s.total_out + take
    s.avail_out = s.avail_out - take

    if take == n then
        s.pending = {}
    else
        -- Keep what did not fit. This only happens when the caller's buffer
        -- fills mid-block, which for our unbounded output means only when
        -- CHUNK_SIZE runs out.
        local rest = {}
        for k = take + 1, n do
            rest[k - take] = pending[k]
        end
        s.pending = rest
    end
end

--- Append a 16-bit value most significant byte first (the zlib header order).
local function putShortMSB(s, b)
    s.pending[#s.pending + 1] = (b >> 8) & 0xff
    s.pending[#s.pending + 1] = b & 0xff
end

--------------------------------------------------------------------------------
-- Adler-32
--------------------------------------------------------------------------------

local BASE = 65521 -- largest prime smaller than 65536
local NMAX = 5552  -- largest n such that 255n(n+1)/2 + (n+1)(BASE-1) fits in 32 bits

--- Adler-32 over a slice of a Lua string, continued from `adler`.
local function adler32_str(adler, str, from, len)
    local s1 = adler & 0xffff
    local s2 = (adler >> 16) & 0xffff
    local pos = from

    while len > 0 do
        local n = len < NMAX and len or NMAX
        len = len - n
        -- The modulo is deferred to the end of each run of NMAX, which is the
        -- whole point of NMAX: the accumulators cannot overflow before then.
        for i = pos, pos + n - 1 do
            s1 = s1 + byte(str, i)
            s2 = s2 + s1
        end
        pos = pos + n
        s1 = s1 % BASE
        s2 = s2 % BASE
    end

    return (s2 << 16) | s1
end

--------------------------------------------------------------------------------
-- State
--------------------------------------------------------------------------------

--- Reset the hash chains. Every slot is NIL, meaning "nothing here".
local function clear_hash(s)
    local head = s.head
    for i = 0, s.hash_size - 1 do
        head[i] = NIL
    end
end

--- Shift every position in the hash tables down by one window.
---
--- Called when the window slides. A position that would go negative is dropped
--- to NIL rather than wrapped -- it is no longer reachable, and a wrapped value
--- would be a match against the wrong bytes.
local function slide_hash(s)
    local wsize = s.w_size
    local head = s.head
    for n = 0, s.hash_size - 1 do
        local m = head[n]
        head[n] = m >= wsize and m - wsize or NIL
    end
    local prev = s.prev
    for n = 0, wsize - 1 do
        local m = prev[n]
        prev[n] = m >= wsize and m - wsize or NIL
        -- If n is not on any chain, prev[n] is garbage, but nothing reads it.
    end
end

--- Set the match-finder parameters and empty the window.
local function lm_init(s)
    s.window_size = 2 * s.w_size

    clear_hash(s)

    local cfg = configuration_table[s.level]
    s.max_lazy_match = cfg.lazy
    s.good_match = cfg.good
    s.nice_match = cfg.nice
    s.max_chain_length = cfg.chain

    s.strstart = 0
    s.block_start = 0
    s.lookahead = 0
    s.insert = 0
    s.match_length = MIN_MATCH - 1
    s.prev_length = MIN_MATCH - 1
    s.match_available = 0
    s.ins_h = 0
end

--- Build a compressor state. Mirrors deflateInit2 followed by deflateReset.
local function deflate_init(level, method, windowBits, memLevel, strategy, hash)
    level = level or -1
    method = method or 8 -- Z_DEFLATED
    windowBits = windowBits or 15
    memLevel = memLevel or 8
    strategy = strategy or 0 -- Z_DEFAULT_STRATEGY
    hash = hash or "chromium"

    if hash ~= "chromium" and hash ~= "rolling" then
        error("pdfkit: hash must be \"chromium\" or \"rolling\"")
    end

    if level == -1 then
        level = 6
    end

    -- A raw stream is windowBits negative; a gzip stream is +16. Only the zlib
    -- wrapper and the raw form are supported here, which is all Node's zlib
    -- asks of the deflate side in PDFKit.
    local wrap = 1
    if windowBits < 0 then
        wrap = 0
        windowBits = -windowBits
    elseif windowBits > 15 then
        error("pdfkit: gzip wrapping is not implemented")
    end

    if method ~= 8 or windowBits < 8 or windowBits > 15
        or level < 0 or level > 9 or strategy < 0 or strategy > 4
        or memLevel < 1 or memLevel > 9 then
        error("pdfkit: bad deflate parameters")
    end

    if windowBits == 8 then
        windowBits = 9 -- until 256-byte windows work, as zlib puts it
    end

    local s = {}

    s.wrap = wrap
    s.w_bits = windowBits
    s.w_size = 1 << windowBits
    s.w_mask = s.w_size - 1

    -- Stock zlib's rolling hash unless the caller asks for Chromium's. See the
    -- top of this file; the default has to match whichever zlib Node links.
    s.rolling_hash = (hash == "rolling")

    s.hash_bits = memLevel + 7
    -- Chromium's hash ignores memLevel's effect on the table and always uses at
    -- least 32768 buckets, on the grounds that the memory is already committed
    -- for the window. So `memLevel` still shrinks the symbol buffer -- and so
    -- still changes where blocks end -- but no longer shrinks the hash table.
    if not s.rolling_hash and s.hash_bits < 15 then
        s.hash_bits = 15
    end
    s.hash_size = 1 << s.hash_bits
    s.hash_mask = s.hash_size - 1
    -- Each byte shifts the running hash left by this much, so that after three
    -- bytes the first has just left the field. 5 for the default memLevel.
    -- Unused by Chromium's hash, which takes all four bytes at once.
    s.hash_shift = (s.hash_bits + MIN_MATCH - 1) // MIN_MATCH

    s.lit_bufsize = 1 << (memLevel + 6)
    s.pending_buf_size = s.lit_bufsize * 4

    s.level = level
    s.strategy = strategy
    s.method = method

    -- The window is two windows wide: the second half is filled while the
    -- first still holds the history matches can reach back into. Zeroed at
    -- allocation, as zlib has done since 1.2.12 -- the match loops read a
    -- little past the data, and the values they read there must be the same
    -- ones zlib reads or the chosen match can differ.
    s.window = {}
    for i = 0, 2 * s.w_size + MAX_MATCH do
        s.window[i] = 0
    end
    s.prev = {}
    for i = 0, s.w_size - 1 do
        s.prev[i] = 0
    end
    s.head = {}

    s.pending = {}
    s.output = {}
    s.total_out = 0
    s.avail_out = 0

    s.high_water = 0
    s.last_flush = Z_NO_FLUSH
    s.status = "INIT"
    s.adler = 1

    trees.tr_init(s)
    lm_init(s)

    return s
end

--------------------------------------------------------------------------------
-- Reading input
--------------------------------------------------------------------------------

--- Copy up to `size` bytes of input into the window at `pos`, checksumming.
local function read_buf(s, pos, size)
    local len = s.avail_in
    if len > size then
        len = size
    end
    if len == 0 then
        return 0
    end

    s.avail_in = s.avail_in - len

    local input = s.input
    local next_in = s.next_in
    local window = s.window
    for i = 0, len - 1 do
        window[pos + i] = byte(input, next_in + i)
    end

    if s.wrap == 1 then
        s.adler = adler32_str(s.adler, input, next_in, len)
    end

    s.next_in = next_in + len
    s.total_in = (s.total_in or 0) + len

    return len
end

--- Insert one position into the hash table; defined below, next to the two
--- hashes it chooses between. Declared here because fill_window needs it.
local insert_string

--- Top up the window, sliding it down first if there is no room.
local function fill_window(s)
    local wsize = s.w_size
    local more

    repeat
        more = s.window_size - s.lookahead - s.strstart

        -- If the window is almost full and there is insufficient lookahead,
        -- move the upper half to the lower one to make room in the upper half.
        if s.strstart >= wsize + wsize - MIN_LOOKAHEAD then
            local window = s.window
            -- Exactly `wsize - more` bytes move, which is exactly the live
            -- data. Everything above it keeps whatever it held -- zlib does
            -- not clear it, the match loops may read it, and so this port
            -- must leave the same bytes there.
            for i = 0, wsize - more - 1 do
                window[i] = window[wsize + i]
            end
            s.match_start = (s.match_start or 0) - wsize
            s.strstart = s.strstart - wsize
            s.block_start = s.block_start - wsize
            if s.insert > s.strstart then
                s.insert = s.strstart
            end
            slide_hash(s)
            more = more + wsize
        end

        if s.avail_in == 0 then
            break
        end

        local n = read_buf(s, s.strstart + s.lookahead, more)
        s.lookahead = s.lookahead + n

        -- Initialize the hash value now that there is some input.
        --
        -- The two hashes need different amounts of it. The rolling hash needs
        -- three bytes and has to be primed with the first two before the loop
        -- can fold in the third; Chromium's reads four and primes nothing. So
        -- the guard is `>= MIN_MATCH` for one and `> MIN_MATCH` for the other,
        -- and the position one short of four bytes is inserted by the first
        -- and left out by the second.
        if s.rolling_hash then
            if s.lookahead + s.insert >= MIN_MATCH then
                local str = s.strstart - s.insert
                local window = s.window
                s.ins_h = ((window[str] << s.hash_shift) ~ window[str + 1])
                    & s.hash_mask

                while s.insert ~= 0 do
                    insert_string(s, str)
                    str = str + 1
                    s.insert = s.insert - 1
                    if s.lookahead + s.insert < MIN_MATCH then
                        break
                    end
                end
            end
        elseif s.lookahead + s.insert > MIN_MATCH then
            local str = s.strstart - s.insert
            while s.insert ~= 0 do
                insert_string(s, str)
                str = str + 1
                s.insert = s.insert - 1
                if s.lookahead + s.insert <= MIN_MATCH then
                    break
                end
            end
        end
        -- If the whole input is shorter than MIN_MATCH, ins_h is garbage --
        -- which does not matter, since only literals will be emitted.

    until s.lookahead >= MIN_LOOKAHEAD or s.avail_in == 0

    -- Zero the bytes just above the data, so the match loops -- which scan up
    -- to MAX_MATCH past strstart regardless of lookahead -- read defined
    -- values. zlib does this to keep memory checkers quiet; here it is what
    -- makes the port's window agree with zlib's byte for byte.
    if s.high_water < s.window_size then
        local curr = s.strstart + s.lookahead
        local window = s.window
        if s.high_water < curr then
            local init = s.window_size - curr
            if init > WIN_INIT then init = WIN_INIT end
            for i = curr, curr + init - 1 do window[i] = 0 end
            s.high_water = curr + init
        elseif s.high_water < curr + WIN_INIT then
            local init = curr + WIN_INIT - s.high_water
            if init > s.window_size - s.high_water then
                init = s.window_size - s.high_water
            end
            for i = s.high_water, s.high_water + init - 1 do window[i] = 0 end
            s.high_water = s.high_water + init
        end
    end
end

--------------------------------------------------------------------------------
-- Match finding
--------------------------------------------------------------------------------

--- Find the longest match for the string at strstart, starting from cur_match.
---
--- Walks the hash chain backwards through the window, keeping the best match
--- found, and stops on any of three budgets: the chain length, a match long
--- enough to be worth stopping for (`nice_match`), or the edge of the window.
--- Sets s.match_start and returns the length.
---
--- The cheap rejection at the top is the reason this is fast enough to be
--- practical: before comparing anything, it checks the byte one past the best
--- length found so far. A candidate that does not beat the incumbent fails on
--- that single comparison.
local function longest_match(s, cur_match)
    local chain_length = s.max_chain_length
    local window = s.window
    local prev = s.prev
    local wmask = s.w_mask
    local strstart = s.strstart
    local scan = strstart
    local best_len = s.prev_length
    local nice_match = s.nice_match
    local match_start = s.match_start or 0

    -- Stop when cur_match becomes <= limit. Position 0 is excluded, which is
    -- how a string is prevented from matching itself at the start of input.
    local max_dist = s.w_size - MIN_LOOKAHEAD
    local limit = strstart > max_dist and strstart - max_dist or NIL

    local strend = strstart + MAX_MATCH
    local scan_end1 = window[scan + best_len - 1]
    local scan_end = window[scan + best_len]

    -- Do not waste too much time if we already have a good match.
    if s.prev_length >= s.good_match then
        chain_length = chain_length >> 2
    end

    -- Do not look for matches beyond the end of the input. This is what makes
    -- deflate deterministic.
    if nice_match > s.lookahead then
        nice_match = s.lookahead
    end

    local scan0 = window[scan]
    local scan1 = window[scan + 1]

    repeat
        local match = cur_match

        -- Skip to the next candidate unless it could beat the incumbent. The
        -- order matters: the two end bytes reject almost everything, and the
        -- two start bytes are then nearly free.
        if window[match + best_len] == scan_end
            and window[match + best_len - 1] == scan_end1
            and window[match] == scan0
            and window[match + 1] == scan1
        then
            -- The check at best_len - 1 can be skipped; it is made again
            -- below. Byte 2 need not be compared either -- it is implied by
            -- the hash matching, given hash_bits >= 8.
            local sc = scan + 2
            local mt = match + 2

            while true do
                sc = sc + 1
                mt = mt + 1
                if window[sc] ~= window[mt] then break end
                if sc >= strend then break end
            end

            local len = MAX_MATCH - (strend - sc)

            if len > best_len then
                match_start = cur_match
                best_len = len
                if len >= nice_match then
                    break
                end
                scan_end1 = window[scan + best_len - 1]
                scan_end = window[scan + best_len]
            end
        end

        cur_match = prev[cur_match & wmask]
        chain_length = chain_length - 1
    until cur_match <= limit or chain_length == 0

    s.match_start = match_start

    if best_len <= s.lookahead then
        return best_len
    end
    return s.lookahead
end

--------------------------------------------------------------------------------
-- Block flushing
--------------------------------------------------------------------------------

--- Close the current block. Mirrors the FLUSH_BLOCK_ONLY macro.
local function flush_block_only(s, last)
    local buf = s.block_start >= 0 and s.block_start or nil
    trees.tr_flush_block(s, buf, s.strstart - s.block_start, last)
    s.block_start = s.strstart
    flush_pending(s)
end

--------------------------------------------------------------------------------
-- The compressors
--------------------------------------------------------------------------------

--- Insert the three bytes at `str` into the hash table; return the chain head.
---
--- Stock zlib's hash is rolling: each call folds in one new byte, so this must
--- be called for every position in order. Skipping one corrupts every hash
--- after it, which is why the match loops below take such care to insert the
--- positions they pass over.
local function insert_string_rolling(s, str)
    local ins_h = ((s.ins_h << s.hash_shift) ~ s.window[str + MIN_MATCH - 1])
        & s.hash_mask
    s.ins_h = ins_h
    local head = s.head[ins_h]
    s.prev[str & s.w_mask] = head
    s.head[ins_h] = str
    return head
end

--- The same, Chromium's way: a multiply-shift over the four bytes at `str`.
---
--- The four bytes are read as a little-endian word, multiplied by 66521 in
--- 32-bit arithmetic (so the product wraps), and the middle of the result is
--- taken -- the classic multiplicative hash, where the high bits of a wrapped
--- product mix all the input bits and the low bits do not. The `+ 66521` is
--- there so that a run of zero bytes does not hash to bucket zero.
---
--- Reading `str + 3` can go one byte past the data. That is deliberate in
--- Chromium too, and safe here for the same reason: the window is zeroed at
--- allocation and re-zeroed above the data by fill_window, so both read the
--- same byte and reach the same chain.
local function insert_string_chromium(s, str)
    local window = s.window
    local value = window[str]
        | (window[str + 1] << 8)
        | (window[str + 2] << 16)
        | (window[str + 3] << 24)

    local ins_h = (((value * 66521 + 66521) & 0xffffffff) >> 16) & s.hash_mask
    s.ins_h = ins_h

    local head = s.head[ins_h]
    s.prev[str & s.w_mask] = head
    s.head[ins_h] = str
    return head
end

--- Insert one position, by whichever hash this stream was built with.
function insert_string(s, str)
    if s.rolling_hash then
        return insert_string_rolling(s, str)
    end
    return insert_string_chromium(s, str)
end

--- Levels 1-3: take the first match found, no second look.
local function deflate_fast(s, flush)
    local max_dist = s.w_size - MIN_LOOKAHEAD

    while true do
        -- Always keep enough lookahead for a longest match plus the three
        -- bytes after it, except at the end of the input.
        if s.lookahead < MIN_LOOKAHEAD then
            fill_window(s)
            if s.lookahead < MIN_LOOKAHEAD and flush == Z_NO_FLUSH then
                return NEED_MORE
            end
            if s.lookahead == 0 then
                break
            end
        end

        local hash_head = NIL
        if s.lookahead >= MIN_MATCH then
            hash_head = insert_string(s, s.strstart)
        end

        if hash_head ~= NIL and s.strstart - hash_head <= max_dist then
            s.match_length = longest_match(s, hash_head)
        end

        local bflush
        if s.match_length >= MIN_MATCH then
            bflush = trees.tr_tally(s, s.strstart - s.match_start,
                s.match_length - MIN_MATCH)

            s.lookahead = s.lookahead - s.match_length

            -- Insert the positions covered by the match only if it was short.
            -- Skipping them for long matches saves time and costs compression.
            if s.match_length <= s.max_lazy_match and s.lookahead >= MIN_MATCH then
                s.match_length = s.match_length - 1 -- strstart is already in
                repeat
                    s.strstart = s.strstart + 1
                    insert_string(s, s.strstart)
                    s.match_length = s.match_length - 1
                until s.match_length == 0
                s.strstart = s.strstart + 1
            else
                s.strstart = s.strstart + s.match_length
                s.match_length = 0
                -- The positions inside the match were skipped, so the rolling
                -- hash has lost its place and must be re-primed from the two
                -- bytes at the new strstart. The CRC32-C hash carries nothing
                -- forward and so needs nothing here.
                if s.rolling_hash then
                    local window = s.window
                    s.ins_h = ((window[s.strstart] << s.hash_shift)
                        ~ window[s.strstart + 1]) & s.hash_mask
                end
                -- If lookahead < MIN_MATCH, ins_h is garbage, but it will be
                -- recomputed at the next call.
            end
        else
            -- No match: output a literal byte
            bflush = trees.tr_tally(s, 0, s.window[s.strstart])
            s.lookahead = s.lookahead - 1
            s.strstart = s.strstart + 1
        end

        if bflush then
            flush_block_only(s, 0)
            if s.avail_out == 0 then return NEED_MORE end
        end
    end

    s.insert = s.strstart < MIN_MATCH - 1 and s.strstart or MIN_MATCH - 1

    if flush == Z_FINISH then
        flush_block_only(s, 1)
        if s.avail_out == 0 then return FINISH_STARTED end
        return FINISH_DONE
    end
    if s.sym_next ~= 0 then
        flush_block_only(s, 0)
        if s.avail_out == 0 then return NEED_MORE end
    end
    return BLOCK_DONE
end

--- Levels 4-9: lazy matching.
---
--- The shape to hold on to: a match found at position n is not emitted at n.
--- It is remembered, the search runs again at n+1, and only if the second
--- search does no better is the first one emitted -- by which time strstart has
--- already moved on, which is why everything here refers to `strstart - 1`.
local function deflate_slow(s, flush)
    local max_dist = s.w_size - MIN_LOOKAHEAD

    while true do
        if s.lookahead < MIN_LOOKAHEAD then
            fill_window(s)
            if s.lookahead < MIN_LOOKAHEAD and flush == Z_NO_FLUSH then
                return NEED_MORE
            end
            if s.lookahead == 0 then
                break
            end
        end

        local hash_head = NIL
        if s.lookahead >= MIN_MATCH then
            hash_head = insert_string(s, s.strstart)
        end

        -- Find the longest match, discarding those <= prev_length.
        s.prev_length = s.match_length
        s.prev_match = s.match_start
        s.match_length = MIN_MATCH - 1

        if hash_head ~= NIL and s.prev_length < s.max_lazy_match
            and s.strstart - hash_head <= max_dist
        then
            s.match_length = longest_match(s, hash_head)

            if s.match_length <= 5 and (s.strategy == Z_FILTERED
                or (s.match_length == MIN_MATCH
                    and s.strstart - s.match_start > TOO_FAR))
            then
                -- A three-byte match this far back costs more than the three
                -- literals it replaces. Drop it.
                s.match_length = MIN_MATCH - 1
            end
        end

        local bflush = false

        if s.prev_length >= MIN_MATCH and s.match_length <= s.prev_length then
            -- The previous match was at least as good: emit it now.
            local max_insert = s.strstart + s.lookahead - MIN_MATCH

            bflush = trees.tr_tally(s, s.strstart - 1 - s.prev_match,
                s.prev_length - MIN_MATCH)

            -- Insert every position the match covers into the hash table.
            -- strstart - 1 and strstart are already in; the last two are left
            -- out when there is not enough lookahead to hash them.
            s.lookahead = s.lookahead - (s.prev_length - 1)
            s.prev_length = s.prev_length - 2
            repeat
                s.strstart = s.strstart + 1
                if s.strstart <= max_insert then
                    insert_string(s, s.strstart)
                end
                s.prev_length = s.prev_length - 1
            until s.prev_length == 0

            s.match_available = 0
            s.match_length = MIN_MATCH - 1
            s.strstart = s.strstart + 1

            if bflush then
                flush_block_only(s, 0)
                if s.avail_out == 0 then return NEED_MORE end
            end

        elseif s.match_available ~= 0 then
            -- No match at the previous position, or this one is longer: the
            -- previous position becomes a single literal.
            bflush = trees.tr_tally(s, 0, s.window[s.strstart - 1])
            if bflush then
                flush_block_only(s, 0)
            end
            s.strstart = s.strstart + 1
            s.lookahead = s.lookahead - 1
            if s.avail_out == 0 then return NEED_MORE end

        else
            -- Nothing to compare against yet; wait one position.
            s.match_available = 1
            s.strstart = s.strstart + 1
            s.lookahead = s.lookahead - 1
        end
    end

    if s.match_available ~= 0 then
        trees.tr_tally(s, 0, s.window[s.strstart - 1])
        s.match_available = 0
    end

    s.insert = s.strstart < MIN_MATCH - 1 and s.strstart or MIN_MATCH - 1

    if flush == Z_FINISH then
        flush_block_only(s, 1)
        if s.avail_out == 0 then return FINISH_STARTED end
        return FINISH_DONE
    end
    if s.sym_next ~= 0 then
        flush_block_only(s, 0)
        if s.avail_out == 0 then return NEED_MORE end
    end
    return BLOCK_DONE
end

--- Z_RLE: only ever look one byte back, so runs compress and nothing else does.
local function deflate_rle(s, flush)
    while true do
        -- Enough lookahead for the longest run, plus one for the unrolled loop.
        if s.lookahead <= MAX_MATCH then
            fill_window(s)
            if s.lookahead <= MAX_MATCH and flush == Z_NO_FLUSH then
                return NEED_MORE
            end
            if s.lookahead == 0 then
                break
            end
        end

        s.match_length = 0
        if s.lookahead >= MIN_MATCH and s.strstart > 0 then
            local window = s.window
            local scan = s.strstart - 1
            local prev = window[scan]
            if prev == window[scan + 1] and prev == window[scan + 2]
                and prev == window[scan + 3]
            then
                scan = scan + 3
                local strend = s.strstart + MAX_MATCH
                repeat
                    scan = scan + 1
                until prev ~= window[scan] or scan >= strend
                s.match_length = MAX_MATCH - (strend - scan)
                if s.match_length > s.lookahead then
                    s.match_length = s.lookahead
                end
            end
        end

        local bflush
        if s.match_length >= MIN_MATCH then
            bflush = trees.tr_tally(s, 1, s.match_length - MIN_MATCH)
            s.lookahead = s.lookahead - s.match_length
            s.strstart = s.strstart + s.match_length
            s.match_length = 0
        else
            bflush = trees.tr_tally(s, 0, s.window[s.strstart])
            s.lookahead = s.lookahead - 1
            s.strstart = s.strstart + 1
        end

        if bflush then
            flush_block_only(s, 0)
            if s.avail_out == 0 then return NEED_MORE end
        end
    end

    s.insert = 0

    if flush == Z_FINISH then
        flush_block_only(s, 1)
        if s.avail_out == 0 then return FINISH_STARTED end
        return FINISH_DONE
    end
    if s.sym_next ~= 0 then
        flush_block_only(s, 0)
        if s.avail_out == 0 then return NEED_MORE end
    end
    return BLOCK_DONE
end

--- Z_HUFFMAN_ONLY: no matches at all, just entropy coding of the bytes.
local function deflate_huff(s, flush)
    while true do
        if s.lookahead == 0 then
            fill_window(s)
            if s.lookahead == 0 then
                if flush == Z_NO_FLUSH then
                    return NEED_MORE
                end
                break
            end
        end

        s.match_length = 0
        local bflush = trees.tr_tally(s, 0, s.window[s.strstart])
        s.lookahead = s.lookahead - 1
        s.strstart = s.strstart + 1

        if bflush then
            flush_block_only(s, 0)
            if s.avail_out == 0 then return NEED_MORE end
        end
    end

    s.insert = 0

    if flush == Z_FINISH then
        flush_block_only(s, 1)
        if s.avail_out == 0 then return FINISH_STARTED end
        return FINISH_DONE
    end
    if s.sym_next ~= 0 then
        flush_block_only(s, 0)
        if s.avail_out == 0 then return NEED_MORE end
    end
    return BLOCK_DONE
end

--- Level 0: no compression, just stored blocks.
---
--- Much the longest of the five, and none of the length is about compression.
--- It is about copying: bytes go straight from the input to the output without
--- passing through the window when there is room, which means the block
--- boundaries fall wherever the output buffer runs out. That is the one place
--- in this file where `avail_out` changes what gets written rather than just
--- when.
local function deflate_stored(s, flush)
    -- Smallest block worth writing when not flushing. 32 KB by default.
    local min_block = s.pending_buf_size - 5
    if min_block > s.w_size then min_block = s.w_size end

    local len, left, have
    local last = 0
    local used = s.avail_in

    repeat
        -- The largest block that can be copied directly, given the input on
        -- hand and the output space left.
        len = MAX_STORED
        have = (s.bi_valid + 42) >> 3 -- header bytes, including pending bits
        if s.avail_out < have then
            break -- no room even for the header
        end
        have = s.avail_out - have
        left = s.strstart - s.block_start -- bytes still in the window
        if len > left + s.avail_in then
            len = left + s.avail_in
        end
        if len > have then
            len = have
        end

        -- Fall through to the window/pending path if the block would be too
        -- small to be worth it, or if flushing but unable to take all the
        -- input. Never write an empty block when flushing -- deflate does that.
        if len < min_block and ((len == 0 and flush ~= Z_FINISH)
            or flush == Z_NO_FLUSH
            or len ~= left + s.avail_in)
        then
            break
        end

        -- Write a zero-length stored block to get the header bytes out,
        -- including whatever bits were pending, then patch in the real length.
        last = (flush == Z_FINISH and len == left + s.avail_in) and 1 or 0
        trees.tr_stored_block(s, nil, 0, last)

        local p = s.pending
        local n = #p
        p[n - 3] = len & 0xff
        p[n - 2] = (len >> 8) & 0xff
        p[n - 1] = (~len) & 0xff
        p[n] = ((~len) >> 8) & 0xff

        flush_pending(s)

        -- Copy whatever the window still holds.
        if left ~= 0 then
            if left > len then left = len end
            local window = s.window
            local out = s.output
            local block_start = s.block_start
            local i = 0
            while i < left do
                local j = i + 4095
                if j >= left then j = left - 1 end
                out[#out + 1] = char(table.unpack(window,
                    block_start + i, block_start + j))
                i = j + 1
            end
            s.avail_out = s.avail_out - left
            s.total_out = s.total_out + left
            s.block_start = s.block_start + left
            len = len - left
        end

        -- Copy the rest straight from the input, checksumming as it goes.
        if len ~= 0 then
            local from = s.next_in
            s.output[#s.output + 1] = s.input:sub(from, from + len - 1)
            if s.wrap == 1 then
                s.adler = adler32_str(s.adler, s.input, from, len)
            end
            s.next_in = from + len
            s.avail_in = s.avail_in - len
            s.total_in = (s.total_in or 0) + len
            s.avail_out = s.avail_out - len
            s.total_out = s.total_out + len
        end
    until last ~= 0

    -- Put the last window's worth of what was copied back into the window, so
    -- that a later deflateParams switching to a real level has some history.
    used = used - s.avail_in
    if used ~= 0 then
        local window = s.window
        if used >= s.w_size then
            -- Everything before is unreachable now; replace the history.
            s.matches = 2 -- signals a pending hash clear
            local from = s.next_in - s.w_size
            for i = 0, s.w_size - 1 do
                window[i] = byte(s.input, from + i)
            end
            s.strstart = s.w_size
            s.insert = s.strstart
        else
            if s.window_size - s.strstart <= used then
                s.strstart = s.strstart - s.w_size
                for i = 0, s.strstart - 1 do
                    window[i] = window[s.w_size + i]
                end
                if s.matches < 2 then
                    s.matches = s.matches + 1 -- a pending slide_hash
                end
                if s.insert > s.strstart then
                    s.insert = s.strstart
                end
            end
            local from = s.next_in - used
            for i = 0, used - 1 do
                window[s.strstart + i] = byte(s.input, from + i)
            end
            s.strstart = s.strstart + used
            local grow = s.w_size - s.insert
            s.insert = s.insert + (used < grow and used or grow)
        end
        s.block_start = s.strstart
    end
    if s.high_water < s.strstart then
        s.high_water = s.strstart
    end

    if last ~= 0 then
        return FINISH_DONE
    end

    if flush ~= Z_NO_FLUSH and flush ~= Z_FINISH
        and s.avail_in == 0 and s.strstart == s.block_start
    then
        return BLOCK_DONE
    end

    -- Fill the window with any remaining input.
    have = s.window_size - s.strstart
    if s.avail_in > have and s.block_start >= s.w_size then
        local window = s.window
        s.block_start = s.block_start - s.w_size
        s.strstart = s.strstart - s.w_size
        for i = 0, s.strstart - 1 do
            window[i] = window[s.w_size + i]
        end
        if s.matches < 2 then
            s.matches = s.matches + 1
        end
        have = have + s.w_size
        if s.insert > s.strstart then
            s.insert = s.strstart
        end
    end
    if have > s.avail_in then
        have = s.avail_in
    end
    if have ~= 0 then
        read_buf(s, s.strstart, have)
        s.strstart = s.strstart + have
        local grow = s.w_size - s.insert
        s.insert = s.insert + (have < grow and have or grow)
    end
    if s.high_water < s.strstart then
        s.high_water = s.strstart
    end

    -- Not enough output space for a direct copy. Write a stored block through
    -- the pending buffer instead, if there is enough input to make it worth it.
    have = (s.bi_valid + 42) >> 3
    have = s.pending_buf_size - have
    if have > MAX_STORED then have = MAX_STORED end
    min_block = have < s.w_size and have or s.w_size
    left = s.strstart - s.block_start

    if left >= min_block
        or ((left ~= 0 or flush == Z_FINISH) and flush ~= Z_NO_FLUSH
            and s.avail_in == 0 and left <= have)
    then
        len = left < have and left or have
        last = (flush == Z_FINISH and s.avail_in == 0 and len == left) and 1 or 0
        trees.tr_stored_block(s, s.block_start, len, last)
        s.block_start = s.block_start + len
        flush_pending(s)
    end

    return last ~= 0 and FINISH_STARTED or NEED_MORE
end

--------------------------------------------------------------------------------
-- The stream driver
--------------------------------------------------------------------------------

local Z_OK = 0
local Z_STREAM_END = 1

--- One call of zlib's deflate(). Consumes input and produces up to avail_out.
local function deflate(s, flush)
    s.last_flush = flush

    -- Flush as much pending output as possible.
    if #s.pending ~= 0 then
        flush_pending(s)
        if s.avail_out == 0 then
            s.last_flush = -1
            return Z_OK
        end
    end

    -- Write the zlib header.
    if s.status == "INIT" and s.wrap == 0 then
        s.status = "BUSY"
    end
    if s.status == "INIT" then
        -- Byte one is the method and window size; byte two carries a hint at
        -- the compression level and a check that makes the pair a multiple of
        -- 31. For the defaults this comes out as the familiar 78 9C.
        local header = (s.method + ((s.w_bits - 8) << 4)) << 8
        local level_flags
        if s.strategy >= Z_HUFFMAN_ONLY or s.level < 2 then
            level_flags = 0
        elseif s.level < 6 then
            level_flags = 1
        elseif s.level == 6 then
            level_flags = 2
        else
            level_flags = 3
        end
        header = header | (level_flags << 6)
        -- No preset dictionary is supported, so the PRESET_DICT bit is never
        -- set and strstart is always 0 here.
        header = header + (31 - (header % 31))

        putShortMSB(s, header)

        s.adler = 1
        s.status = "BUSY"

        -- Compression must start with an empty pending buffer.
        flush_pending(s)
        if #s.pending ~= 0 then
            s.last_flush = -1
            return Z_OK
        end
    end

    -- Start a new block or continue the current one.
    if s.avail_in ~= 0 or s.lookahead ~= 0
        or (flush ~= Z_NO_FLUSH and s.status ~= "FINISH")
    then
        local bstate
        if s.level == 0 then
            bstate = deflate_stored(s, flush)
        elseif s.strategy == Z_HUFFMAN_ONLY then
            bstate = deflate_huff(s, flush)
        elseif s.strategy == Z_RLE then
            bstate = deflate_rle(s, flush)
        elseif configuration_table[s.level].func == "fast" then
            bstate = deflate_fast(s, flush)
        else
            bstate = deflate_slow(s, flush)
        end

        if bstate == FINISH_STARTED or bstate == FINISH_DONE then
            s.status = "FINISH"
        end
        if bstate == NEED_MORE or bstate == FINISH_STARTED then
            if s.avail_out == 0 then
                s.last_flush = -1
            end
            return Z_OK
        end
        if bstate == BLOCK_DONE then
            if flush == Z_PARTIAL_FLUSH then
                trees.tr_align(s)
            elseif flush ~= Z_BLOCK then
                -- A full or sync flush ends with an empty stored block, which
                -- inflate_sync recognises as a restart marker.
                trees.tr_stored_block(s, nil, 0, 0)
                if flush == Z_FULL_FLUSH then
                    clear_hash(s) -- forget history
                    if s.lookahead == 0 then
                        s.strstart = 0
                        s.block_start = 0
                        s.insert = 0
                    end
                end
            end
            flush_pending(s)
            if s.avail_out == 0 then
                s.last_flush = -1
                return Z_OK
            end
        end
    end

    if flush ~= Z_FINISH then
        return Z_OK
    end
    if s.wrap <= 0 then
        return Z_STREAM_END
    end

    -- The trailer: adler-32 of the uncompressed data, most significant first.
    putShortMSB(s, (s.adler >> 16) & 0xffff)
    putShortMSB(s, s.adler & 0xffff)
    flush_pending(s)

    -- Write the trailer only once.
    if s.wrap > 0 then
        s.wrap = -s.wrap
    end

    return #s.pending ~= 0 and Z_OK or Z_STREAM_END
end

--------------------------------------------------------------------------------
-- Public entry point
--------------------------------------------------------------------------------

--- Compress `str`, returning a zlib stream.
---
--- The options mirror Node's, which mirror zlib's: `level` 0-9 or -1 for the
--- default, `windowBits` 8-15 (negative for a raw stream with no zlib header
--- or checksum), `memLevel` 1-9, `strategy` 0-4. All defaulted, and the
--- defaults are what `zlib.deflateSync(buf)` uses.
---
--- One option is not Node's: `hash`, which is `"chromium"` (the default, and
--- what Node v24's motley zlib links) or `"rolling"` for stock zlib. See the
--- top of this file for why that is a choice anyone has to make.
function M.deflate(str, opts)
    opts = opts or {}

    local s = deflate_init(opts.level, 8, opts.windowBits, opts.memLevel,
        opts.strategy, opts.hash)

    s.input = str
    s.next_in = 1
    s.avail_in = #str
    s.total_in = 0

    -- Loop the way Node does: hand zlib a fixed slab of output space, let it
    -- fill it, hand it another. The slab size is invisible to every level but
    -- zero, where it sets the stored-block boundaries.
    local ret
    repeat
        s.avail_out = CHUNK_SIZE
        ret = deflate(s, Z_FINISH)
    until ret == Z_STREAM_END

    return concat(s.output)
end

return M
