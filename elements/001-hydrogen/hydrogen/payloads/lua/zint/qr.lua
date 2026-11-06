-- zint/qr.lua -- ports the QR Code part of backend/qr.c
--
-- qr.c holds five symbologies that share a bit-packing core: QR Code, Micro QR,
-- rMQR, UPNQR (a Slovenian payment variant) and the Han Xin-adjacent helpers.
-- Only QR Code proper is here. The others are reachable in the C by version
-- constants -- RMQR_VERSION 41, MICROQR_VERSION 73 -- and every branch keyed on
-- those is resolved at port time to its QRCODE arm rather than carried across
-- and left dead. Where that resolution is not obvious the comment says so.
--
-- WHAT MAKES THIS ONE DIFFERENT FROM THE OTHER FOUR
--
-- Code 39, Code 128, GS1-128 and UPC-A are table lookups with a little state.
-- QR has four things none of them have:
--
--   1. A dynamic-programming mode optimiser (`define_mode`, adapted in the C
--      from Project Nayuki) that decides, per character, whether to encode in
--      Numeric, Alphanumeric, Byte or Kanji. Costs are held in sixths of a bit
--      -- QR_MULT -- so that the 11/2 and 10/3 bits-per-character averages stay
--      exact integers.
--   2. Reed-Solomon error correction over GF(2^8) -- see zint/reedsol.lua.
--   3. A geometric layout: finder patterns, timing patterns, alignment
--      patterns, and a zig-zag data placement that skips them.
--   4. Mask selection, which builds the symbol eight times and scores each.
--
-- Any of those four can be plausible and wrong in a way that still produces a
-- scannable symbol. They are ported literally, including the parts that look
-- like they could be simplified, and checked against `zint --verbose`, which
-- prints the mode string, the estimated bit length, the codewords, the ECC
-- blocks and all eight mask penalties. That output is the reason this file
-- could be tested a stage at a time instead of all at once.
--
-- HIGH NIBBLE / LOW NIBBLE. The grid is one byte per module and the two halves
-- mean different things:
--   0x01  the module is dark
--   0x10  function pattern (finder, separator, alignment, format reservation)
--   0x20  function pattern (timing, format/version reservation)
--   0x40  set by version info, and otherwise unused
-- `& 0xf0` therefore asks "is this a function module", which is what both the
-- data placement and the masking use to know what to leave alone.

local common   = require "zint.common"
local eci      = require "zint.eci"
local reedsol  = require "zint.reedsol"
local qrt      = require "zint.qrtables"

local M = {}

-- Set ZINT_LUA_DEBUG=1 to get the same stage-by-stage trace `zint --verbose`
-- prints, in the same format, so the two can be diffed directly. That is how
-- this file was checked: mode string, estimated bit length at each version
-- probe, chosen version and ECC level, codeword stream, and the penalty score
-- for all eight masks -- each one a place the port can be wrong on its own.
--
-- An environment variable rather than a flag because the front-end wrapper is
-- fixed: `zint-lua.sh` and `zint-lua-run.lua` are supplied to the agent and
-- checksummed, so the port cannot add an option to them.
local DEBUG = os.getenv("ZINT_LUA_DEBUG") == "1"

-- ECC levels. The C numbers them 1-4 rather than 0-3, and the numbering leaks
-- into the format-information bits below, so it is kept.
local QR_LEVEL_L, QR_LEVEL_M, QR_LEVEL_Q, QR_LEVEL_H = 1, 2, 3, 4
local ECC_LEVEL_NAMES = { "L", "M", "Q", "H" }

-- Mode indices into the cost/state arrays. Order matters: it is the same order
-- as MODE_TYPES, and `posn(qr_mode_types, c)` below depends on it.
local QR_N, QR_A, QR_B, QR_K = 0, 1, 2, 3
local MODE_TYPES = { [0] = "N", "A", "B", "K" }
local MODE_INDEX = { N = 0, A = 1, B = 2, K = 3 }

-- Extra slots in the same array the head costs live in. The C uses one
-- `unsigned int state[11]` for everything so that the numeric and alpha
-- scanners can advance shared cursors through out-parameters; the port keeps
-- the single table for the same reason.
local QR_VER     = 4   -- version
local QR_N_END   = 5   -- numeric run end index
local QR_N_COST  = 6   -- numeric per-character cost
local QR_A_END   = 7   -- alpha run end index
local QR_A_COST  = 8   -- alpha per-character cost
local QR_A_PCENT = 9   -- GS1 only: 2nd char of the pair is a percent
local QR_A_PCCNT = 10  -- GS1 only: running count of percents seen

-- Costs are in sixths of a bit so that 11/2 and 10/3 are integers.
local QR_MULT = 6

-- Alphanumeric value of '%'. In GS1 mode a literal percent has to be doubled
-- (a single one is the FNC1 escape in the alphanumeric set), which is why it
-- gets a name and a special case in three separate places below.
local QR_PERCENT = 38

-- backend/qr.c `qr_is_alpha()`. The C tests `is_chr(QR_ALPHA, glyph)` against a
-- 256-entry flag table; QR_ALPHA is the union of the numeric, uppercase, space,
-- asterisk, plus, minus, silver and colon flags.
--
-- Rather than transcribe that flag table, note that the union is by
-- construction exactly the 45 characters that have a value in
-- `qrtables.alphanumeric` -- space $ % * + - . / 0-9 : A-Z -- and that table
-- spans ASCII 32 to 90 with -1 in the gaps. So the two tests agree everywhere,
-- and this one is checkable by reading one table instead of two.
--
-- The GS1 arm accepts '[' (91), which sits one past the table's end, so there
-- is no overlap to worry about. '[' is how gs1.lua's reduction marks an FNC1.
local function is_alpha(glyph, gs1)
  if glyph >= 32 and glyph <= 90 and qrt.alphanumeric[glyph - 32] >= 0 then
    return true
  end
  if gs1 and glyph == 91 then return true end -- '[' is FNC1
  return false
end

local function is_digit(g) return g >= 48 and g <= 57 end

-- backend/qr.c `qr_in_numeric()`. Returns true when position `in_posn` should
-- be considered numeric, and leaves the run end and the per-character cost in
-- the state table.
--
-- The cost is an average, which is the whole trick: numeric mode packs three
-- digits into ten bits, so a digit costs 10/3 bits when it is part of a full
-- triple, 7/2 when part of a pair, and 4 on its own. Multiplied by QR_MULT
-- those are 20, 21 and 24 -- integers, comparable, and cheap.
local function in_numeric(ddata, length, in_posn, state)
  if in_posn < state[QR_N_END] then
    return true
  end

  -- Look ahead at most three digits: that is the numeric group size, so a
  -- longer run tells us nothing more about the cost of *this* character.
  local i = in_posn
  while i < length and i < in_posn + 3 and is_digit(ddata[i]) do
    i = i + 1
  end

  local digit_cnt = i - in_posn
  if digit_cnt == 0 then
    state[QR_N_END] = 0
    return false
  end

  state[QR_N_END] = i
  state[QR_N_COST] = digit_cnt == 1 and 24        -- 4 * QR_MULT
                  or digit_cnt == 2 and 21        -- (7/2) * QR_MULT
                  or 20                           -- (10/3) * QR_MULT
  return true
end

-- backend/qr.c `qr_in_alpha()`. Same shape as the numeric scanner, but the
-- group size is two -- alphanumeric packs two characters into eleven bits --
-- so a character costs 11/2 bits in a pair and 6 alone: 33 and 36 in QR_MULT.
--
-- THE GS1 COMPLICATION, which is the reason this function is forty lines rather
-- than ten. In GS1 mode a literal '%' has to be encoded as two alphanumeric
-- characters. That means a percent occupies two slots in the pairing, so it
-- flips the parity of everything after it: whether the *next* character ends up
-- paired or stranded depends on how many percents have been seen so far.
-- `QR_A_PCCNT` carries that running count and `& 1` reads its parity;
-- `QR_A_PCENT` remembers that the second half of a doubled percent is still
-- owed. The costs 66 and 69 are the pair-sized versions of 33 and 36.
--
-- None of this can be exercised by data without a percent in it, and the
-- verifier has none -- but a GS1 QR payload containing a percent is entirely
-- ordinary, so the branch is ported rather than stubbed.
local function in_alpha(ddata, length, in_posn, state, gs1)
  local last = (in_posn + 1 == length)
  local two_alphas

  -- Already inside a run the scanner measured earlier: just price this
  -- character and return.
  if in_posn < state[QR_A_END] then
    if gs1 then
      if state[QR_A_PCENT] ~= 0 then
        -- Previous character's 2nd slot was a percent, so this position is the
        -- second half of the doubled-up percent.
        two_alphas = (not last) and is_alpha(ddata[in_posn + 1], gs1)
        state[QR_A_COST] = (two_alphas or (state[QR_A_PCCNT] & 1) == 0) and 66 or 69
        state[QR_A_PCENT] = 0
      else
        -- An odd number of percents so far means this will fit evenly.
        state[QR_A_COST] = ((not last) or (state[QR_A_PCCNT] & 1) == 0) and 33 or 36
      end
    else
      state[QR_A_COST] = (not last) and 33 or 36
    end
    return true
  end

  if not is_alpha(ddata[in_posn], gs1) then
    state[QR_A_END] = 0
    state[QR_A_PCENT] = 0
    state[QR_A_PCCNT] = 0
    return false
  end

  two_alphas = (not last) and is_alpha(ddata[in_posn + 1], gs1)

  if gs1 and ddata[in_posn] == 37 then -- '%' must double up, so counts as 2 chars
    state[QR_A_END] = in_posn + 1
    state[QR_A_PCENT] = 0
    state[QR_A_PCCNT] = state[QR_A_PCCNT] + 1
    state[QR_A_COST] = (two_alphas or (state[QR_A_PCCNT] & 1) == 0) and 66 or 69
    return true
  end

  state[QR_A_END] = two_alphas and in_posn + 2 or in_posn + 1
  if gs1 then
    -- Is the second character of this pair a percent? If so the pair is really
    -- three slots, and the parity count has to know.
    state[QR_A_PCENT] = (two_alphas and ddata[in_posn + 1] == 37) and 1 or 0
    state[QR_A_PCCNT] = state[QR_A_PCCNT] + state[QR_A_PCENT]
    state[QR_A_COST] = (two_alphas or (state[QR_A_PCCNT] & 1) == 0) and 33 or 36
  else
    state[QR_A_COST] = two_alphas and 33 or 36
  end
  return true
end

-- backend/qr.c `qr_head_costs()`, QRCODE arm only. The cost of *starting* a
-- segment in each mode: the mode indicator (4 bits) plus the character count
-- indicator, whose width steps up at versions 10 and 27.
--
-- These double as the mode-*switch* costs in the optimiser below, which is
-- correct because switching mode means closing one segment and opening
-- another, and opening is all this measures.
local function head_costs(state)
  local version = state[QR_VER]
  local n, a, b, k
  if version < 10 then
    n, a, b, k = (10 + 4) * QR_MULT, (9 + 4) * QR_MULT, (8 + 4) * QR_MULT, (8 + 4) * QR_MULT
  elseif version < 27 then
    n, a, b, k = (12 + 4) * QR_MULT, (11 + 4) * QR_MULT, (16 + 4) * QR_MULT, (10 + 4) * QR_MULT
  else
    n, a, b, k = (14 + 4) * QR_MULT, (13 + 4) * QR_MULT, (16 + 4) * QR_MULT, (12 + 4) * QR_MULT
  end
  state[QR_N], state[QR_A], state[QR_B], state[QR_K] = n, a, b, k
  return state
end

-- backend/qr.c `qr_define_mode()` -- "Adapted from Project Nayuki (MIT)".
--
-- A shortest-path problem in disguise. Each character can be encoded in any of
-- four modes; staying in a mode costs the per-character price, switching costs
-- the head price of the mode being switched to. `prev_costs[j]` is the cheapest
-- way to encode everything up to here and *end* in mode j, and `char_modes` is
-- the back-pointer array that lets the optimal path be recovered at the end.
--
-- Two details that look like bugs and are not:
--
--   * The switch loop reads `cur_costs[k]` for values of k that an earlier
--     iteration of the same loop may already have written. That is the C's
--     behaviour, not an accident of ordering, and changing it changes which
--     mode string comes out. Ported in the same order for that reason.
--   * `char_modes[...]` doubles as "is this mode reachable at all": the C
--     stores a NUL there and tests it for truth. The port uses false, and the
--     `cur_costs` entry alongside it stays 0, exactly as the C's memset leaves
--     it.
local function define_mode(mode, ddata, length, gs1, version, debug_print)
  local state = {
    [0] = 0, 0, 0, 0,     -- head/switch costs, indices QR_N..QR_K
    0,                    -- QR_VER
    0, 0, 0, 0, 0, 0,     -- the numeric/alpha cursors
  }
  state[QR_VER] = version

  -- char_modes[i * 4 + j] is the mode to encode character i in, such that the
  -- final segment ends in MODE_TYPES[j] and the total bit count is minimal.
  local char_modes = {}
  for i = 0, length * 4 - 1 do char_modes[i] = false end

  local prev_costs = {}
  head_costs(state)
  for j = 0, 3 do prev_costs[j] = state[j] end

  local cur_costs = {}

  local cm_i = 0
  for i = 0, length - 1 do
    for j = 0, 3 do cur_costs[j] = 0 end

    if ddata[i] > 0xFF then
      -- Only reachable through the Shift-JIS fallback in prep_data: a
      -- double-byte value means Kanji mode is on the table.
      cur_costs[QR_B] = prev_costs[QR_B] + 96   -- 16 * QR_MULT
      char_modes[cm_i + QR_B] = "B"
      cur_costs[QR_K] = prev_costs[QR_K] + 78   -- 13 * QR_MULT
      char_modes[cm_i + QR_K] = "K"
    else
      if in_numeric(ddata, length, i, state) then
        cur_costs[QR_N] = prev_costs[QR_N] + state[QR_N_COST]
        char_modes[cm_i + QR_N] = "N"
      end
      if in_alpha(ddata, length, i, state, gs1) then
        cur_costs[QR_A] = prev_costs[QR_A] + state[QR_A_COST]
        char_modes[cm_i + QR_A] = "A"
      end
      cur_costs[QR_B] = prev_costs[QR_B] + 48   -- 8 * QR_MULT
      char_modes[cm_i + QR_B] = "B"
    end

    -- Start a new segment at the end to switch modes. Switch costs are the
    -- same as head costs, which is why state[j] is read directly.
    for j = 0, 3 do          -- to mode
      for k = 0, 3 do        -- from mode
        if j ~= k and char_modes[cm_i + k] then
          local new_cost = cur_costs[k] + state[j]
          if (not char_modes[cm_i + j]) or new_cost < cur_costs[j] then
            cur_costs[j] = new_cost
            char_modes[cm_i + j] = MODE_TYPES[k]
          end
        end
      end
    end

    for j = 0, 3 do prev_costs[j] = cur_costs[j] end
    cm_i = cm_i + 4
  end

  -- Cheapest ending mode, then walk the back-pointers to recover the path.
  local min_cost = prev_costs[0]
  local cur_mode = MODE_TYPES[0]
  for i = 1, 3 do
    if prev_costs[i] < min_cost then
      min_cost = prev_costs[i]
      cur_mode = MODE_TYPES[i]
    end
  end

  cm_i = (length - 1) * 4
  for i = length - 1, 0, -1 do
    local j = MODE_INDEX[cur_mode]
    cur_mode = char_modes[cm_i + j]
    mode[i] = cur_mode
    cm_i = cm_i - 4
  end

  if debug_print then
    local s = {}
    for i = 0, length - 1 do s[#s + 1] = mode[i] end
    io.stderr:write("  Mode: ", table.concat(s), "\n")
  end
end

-- backend/qr.c `qr_mode_indicator()`, `qr_mode_bits()`, `qr_cci_bits()`,
-- `qr_terminator_bits()` -- all four collapsed to their QRCODE arms, where the
-- version is always below RMQR_VERSION.
local MODE_INDICATOR = { N = 1, A = 2, B = 4, K = 8 }

local function mode_bits() return 4 end
local function terminator_bits() return 4 end

-- Character count indicator width, which steps up at versions 10 and 27. This
-- is the same stepping the head costs above encode, and the two have to agree
-- or the estimate and the actual bit length diverge.
local CCI_BITS = {
  { N = 10, A = 9,  B = 8,  K = 8  },   -- versions 1-9
  { N = 12, A = 11, B = 16, K = 10 },   -- versions 10-26
  { N = 14, A = 13, B = 16, K = 12 },   -- versions 27-40
}

local function cci_bits(version, m)
  if version < 10 then return CCI_BITS[1][m] end
  if version < 27 then return CCI_BITS[2][m] end
  return CCI_BITS[3][m]
end

-- backend/common.c `bin_append_posn()`. Appends `length` bits of `arg`, most
-- significant first, to a 0-based array of 0/1 at position `bp`, and returns
-- the new position.
local function bin_append(arg, length, binary, bp)
  local last = length - 1
  for i = 0, last do
    binary[bp + i] = (arg >> (last - i)) & 1
  end
  return bp + length
end

-- backend/qr.c `qr_binary()`. Turn the mode string plus the data into the bit
-- stream. This is the counterpart of `calc_binlen` below and the two MUST agree
-- exactly on the length -- the estimate picks the version, and a version chosen
-- from a wrong estimate produces a symbol whose data does not fit.
--
-- `eci` is 0 for Latin-1 and for the Shift-JIS fallback. The driver sets it
-- only on the automatic-ECI retry (warning 222). A non-zero value is a header
-- before the data: mode indicator 7 in four bits, then the ECI number in 8,
-- 16, or 24 bits. `calc_binlen` counts the same bits. The two have to agree or
-- the version chosen from the estimate is the wrong size for the stream.
local function qr_binary(binary, bp, version, mode, ddata, length, gs1, eci, debug_print)
  local position = 0
  local percent = 0
  local modebits = mode_bits()

  if eci ~= 0 then
    bp = bin_append(7, 4, binary, bp)
    if eci <= 127 then
      bp = bin_append(eci, 8, binary, bp)
    elseif eci <= 16383 then
      bp = bin_append(0x8000 + eci, 16, binary, bp)
    else
      bp = bin_append(0xC00000 + eci, 24, binary, bp)
    end
  end

  repeat
    local data_block = mode[position]
    local short_len = 0
    local double_byte = 0

    -- Measure the run of like modes starting here; that run is one segment.
    repeat
      if data_block == "B" and ddata[position + short_len] > 0xFF then
        double_byte = double_byte + 1
      end
      short_len = short_len + 1
    until (short_len + position) >= length or mode[position + short_len] ~= data_block

    bp = bin_append(MODE_INDICATOR[data_block], modebits, binary, bp)

    if data_block == "K" then
      -- Kanji mode. Reachable only via the Shift-JIS fallback; see prep_data.
      bp = bin_append(short_len, cci_bits(version, "K"), binary, bp)
      local dbg = {}
      for i = 0, short_len - 1 do
        local jis = ddata[position + i]
        if jis >= 0x8140 and jis <= 0x9ffc then
          jis = jis - 0x8140
        elseif jis >= 0xe040 and jis <= 0xebbf then
          jis = jis - 0xc140
        end
        -- Shift-JIS is a 0xc0-wide grid, so the two bytes collapse to a single
        -- 13-bit index rather than being stored as-is.
        local prod = ((jis >> 8) * 0xc0) + (jis & 0xff)
        bp = bin_append(prod, 13, binary, bp)
        if debug_print then dbg[#dbg + 1] = ("0x%04X "):format(prod) end
      end
      if debug_print then
        io.stderr:write(("Kanji block (length %d)\n\t"):format(short_len),
                        table.concat(dbg), "\n")
      end

    elseif data_block == "B" then
      -- Byte mode. The count is in bytes, so a double-byte value counts twice.
      bp = bin_append(short_len + double_byte, cci_bits(version, "B"), binary, bp)
      local dbg = {}
      for i = 0, short_len - 1 do
        local byte = ddata[position + i]
        if gs1 and byte == 91 then byte = 0x1d end -- '[' -> FNC1 (GS)
        bp = bin_append(byte, byte > 0xFF and 16 or 8, binary, bp)
        if debug_print then dbg[#dbg + 1] = ("0x%02X(%d) "):format(byte, byte) end
      end
      if debug_print then
        io.stderr:write(("Byte block (length %d)\n\t"):format(short_len + double_byte),
                        table.concat(dbg), "\n")
      end

    elseif data_block == "A" then
      -- Alphanumeric mode: characters are paired and packed as a*45+b into 11
      -- bits, with a stranded final character taking 6.
      --
      -- The percent bookkeeping mirrors `in_alpha`'s cost model. A GS1 percent
      -- expands to two alphanumeric slots, so a pair can straddle the input in
      -- three different ways and `percent` carries the "second half still
      -- owed" state across loop iterations.
      local percent_count = 0
      if gs1 then
        for i = 0, short_len - 1 do
          if ddata[position + i] == 37 then percent_count = percent_count + 1 end
        end
      end

      bp = bin_append(short_len + percent_count, cci_bits(version, "A"), binary, bp)

      local dbg = {}
      local i = 0
      while i < short_len do
        local count, first, second, prod
        first, second = 0, 0

        if percent == 0 then
          if gs1 and ddata[position + i] == 37 then
            -- A percent on its own fills the whole pair by doubling.
            first, second = QR_PERCENT, QR_PERCENT
            count = 2
            prod = (first * 45) + second
            i = i + 1
          else
            if gs1 and ddata[position + i] == 91 then
              first = QR_PERCENT      -- '[' encodes as FNC1, which shares %'s value
            else
              first = qrt.alphanumeric[ddata[position + i] - 32]
            end
            count = 1
            i = i + 1
            prod = first

            if i < short_len and mode[position + i] == "A" then
              if gs1 and ddata[position + i] == 37 then
                -- Second slot is a percent: take its first half now and leave
                -- the second half for the next iteration. `i` deliberately
                -- does not advance.
                second = QR_PERCENT
                count = 2
                prod = (first * 45) + second
                percent = 1
              else
                if gs1 and ddata[position + i] == 91 then
                  second = QR_PERCENT
                else
                  second = qrt.alphanumeric[ddata[position + i] - 32]
                end
                count = 2
                i = i + 1
                prod = (first * 45) + second
              end
            end
          end
        else
          -- Paying off the second half of a doubled percent.
          first = QR_PERCENT
          count = 1
          i = i + 1
          prod = first
          percent = 0

          if i < short_len and mode[position + i] == "A" then
            if gs1 and ddata[position + i] == 37 then
              second = QR_PERCENT
              count = 2
              prod = (first * 45) + second
              percent = 1
            else
              if gs1 and ddata[position + i] == 91 then
                second = QR_PERCENT
              else
                second = qrt.alphanumeric[ddata[position + i] - 32]
              end
              count = 2
              i = i + 1
              prod = (first * 45) + second
            end
          end
        end

        bp = bin_append(prod, 1 + (5 * count), binary, bp)
        if debug_print then dbg[#dbg + 1] = ("0x%X "):format(prod) end
      end
      if debug_print then
        io.stderr:write(("Alpha block (length %d)\n\t"):format(short_len + percent_count),
                        table.concat(dbg), "\n")
      end

    elseif data_block == "N" then
      -- Numeric mode: three digits into ten bits, with 7 and 4 bit tails.
      bp = bin_append(short_len, cci_bits(version, "N"), binary, bp)
      local dbg = {}
      local i = 0
      while i < short_len do
        -- The C calls ctoi() here; every character in an N block is already
        -- known to be a digit, so the subtraction is the same thing without
        -- building a one-character string per digit.
        local count = 1
        local prod = ddata[position + i] - 48

        if i + 1 < short_len and mode[position + i + 1] == "N" then
          count = 2
          prod = (prod * 10) + (ddata[position + i + 1] - 48)
          if i + 2 < short_len and mode[position + i + 2] == "N" then
            count = 3
            prod = (prod * 10) + (ddata[position + i + 2] - 48)
          end
        end

        bp = bin_append(prod, 1 + (3 * count), binary, bp)
        if debug_print then dbg[#dbg + 1] = ("0x%X(%d) "):format(prod, prod) end
        i = i + count
      end
      if debug_print then
        io.stderr:write(("Number block (length %d)\n\t"):format(short_len),
                        table.concat(dbg), "\n")
      end
    end

    position = position + short_len
  until position >= length

  return bp
end

-- backend/qr.c `qr_binary_segs()`. Wraps `qr_binary` with the GS1 header, the
-- terminator, the bit padding and the pad codewords.
--
-- Single-segment only: multiple segments need --eci, which the front end does
-- not have. Structured Append likewise needs --structapp. Both of the C's
-- prefixes are therefore unreachable, and the Structured Append one is left out
-- while the GS1 one -- which IS reachable, via --gs1 -- is kept.
-- The C takes `est_binlen` too, purely to size an alloca for the bit buffer. A
-- Lua table grows on its own, so the parameter is dropped rather than carried
-- as an ignored argument.
local function binary_segs(datastream, version, target_codewords, mode, ddata,
                           length, gs1, eci, debug_print)
  local binary = {}
  local bp = 0

  if gs1 then
    bp = bin_append(5, 4, binary, bp)   -- FNC1 in first position
  end

  bp = qr_binary(binary, bp, version, mode, ddata, length, gs1, eci, debug_print)

  -- Terminator. Up to four zero bits, but fewer if the stream already ends on a
  -- byte boundary AND the symbol is already full -- the condition reads oddly
  -- because it is really two rules: always terminate if there is room to grow,
  -- and never overrun the last codeword.
  local termbits = 8 - bp % 8
  if termbits == 8 then termbits = 0 end
  local current_bytes = (bp + termbits) // 8
  if termbits ~= 0 or current_bytes < target_codewords then
    local max_termbits = terminator_bits()
    termbits = (termbits < max_termbits and current_bytes == target_codewords)
               and termbits or max_termbits
    bp = bin_append(0, termbits, binary, bp)
  end

  -- Pad up to the next byte boundary.
  local padbits = 8 - bp % 8
  if padbits == 8 then padbits = 0 end
  if padbits ~= 0 then
    current_bytes = (bp + padbits) // 8
    bin_append(0, padbits, binary, bp)   -- last use, so bp is not advanced
  end

  if debug_print then
    local s = {}
    for i = 0, bp - 1 do s[#s + 1] = tostring(binary[i]) end
    io.stderr:write(("Terminated binary (%d): %s (padbits %d)\n")
                    :format(bp, table.concat(s), padbits))
  end

  for i = 0, current_bytes - 1 do
    local j = i * 8
    local b = 0
    for p = 0, 7 do
      if binary[j + p] == 1 then b = b | (0x80 >> p) end
    end
    datastream[i] = b
  end

  -- Pad codewords: the standard's alternating 0xEC / 0x11 filler.
  local toggle = 0
  for i = current_bytes, target_codewords - 1 do
    if toggle == 0 then
      datastream[i] = 0xec
      toggle = 1
    else
      datastream[i] = 0x11
      toggle = 0
    end
  end

  if debug_print then
    local s = {}
    for i = 0, target_codewords - 1 do s[#s + 1] = ("0x%02X "):format(datastream[i]) end
    io.stderr:write(("Resulting codewords (%d):\n\t"):format(target_codewords),
                    table.concat(s), "\n")
  end
end

-- backend/qr.c `qr_blockLength()`. Length of the run of identical modes at
-- `start`.
local function block_length(start, mode, length)
  local count = 0
  local start_mode = mode[start]
  repeat
    count = count + 1
  until (start + count) >= length or mode[start + count] ~= start_mode
  return count
end

-- backend/qr.c `qr_calc_binlen()`. The bit length the stream WOULD have, used
-- to choose the version before the stream is built. It must agree with
-- `qr_binary` to the bit; the two are separate functions in the C and this port
-- keeps them separate for the same reason -- they are called at different times
-- with different versions.
local function calc_binlen(version, mode, ddata, length, mode_preset, gs1, eci, debug_print)
  if not mode_preset then
    define_mode(mode, ddata, length, gs1, version, debug_print)
  end

  local current_mode = " "   -- null: forces a header on the first character
  local count = 0

  if eci ~= 0 then
    count = count + 4
    if eci <= 127 then
      count = count + 8
    elseif eci <= 16383 then
      count = count + 16
    else
      count = count + 24
    end
  end

  for i = 0, length - 1 do
    if mode[i] ~= current_mode then
      count = count + mode_bits() + cci_bits(version, mode[i])
      local blocklength = block_length(i, mode, length)

      if mode[i] == "K" then
        count = count + blocklength * 13

      elseif mode[i] == "B" then
        for j = i, i + blocklength - 1 do
          count = count + (ddata[j] > 0xff and 16 or 8)
        end

      elseif mode[i] == "A" then
        local alphalength = blocklength
        if gs1 then
          -- In alphanumeric mode % becomes %%, so it costs two slots.
          for j = i, i + blocklength - 1 do
            if ddata[j] == 37 then alphalength = alphalength + 1 end
          end
        end
        if alphalength % 2 == 0 then
          count = count + (alphalength // 2) * 11
        else
          count = count + ((alphalength - 1) // 2) * 11 + 6
        end

      elseif mode[i] == "N" then
        local r = blocklength % 3
        if r == 0 then
          count = count + (blocklength // 3) * 10
        elseif r == 1 then
          count = count + ((blocklength - 1) // 3) * 10 + 4
        else
          count = count + ((blocklength - 2) // 3) * 10 + 7
        end
      end

      current_mode = mode[i]
    end
  end

  return count
end

-- backend/qr.c `qr_calc_binlen_segs()`, single segment. The GS1 header is four
-- bits and has to be counted here as well as emitted in `binary_segs`.
local function calc_binlen_segs(version, mode, ddata, length, mode_preset, gs1, eci, debug_print)
  local count = 0
  if gs1 then count = count + 4 end
  count = count + calc_binlen(version, mode, ddata, length, mode_preset, gs1, eci, debug_print)
  if debug_print then
    io.stderr:write(("Estimated Binary Length: %d (version %d, gs1 %d)\n")
                    :format(count, version, gs1 and 1 or 0))
  end
  return count
end

-- backend/qr.c `qr_add_ecc()`. Split the data into blocks, Reed-Solomon each
-- one, then interleave both the data and the ECC across the blocks.
--
-- Interleaving is what makes QR robust to a burst of damage: consecutive
-- codewords in the symbol belong to different blocks, so a blot that destroys
-- twenty adjacent codewords costs each block only a few.
--
-- Blocks come in two lengths -- `short_data_block_length` and one more -- with
-- the long ones last. The interleave writes the common prefix column-major and
-- then appends the long blocks' extra codeword in a run at the end, which is
-- exactly what the standard's layout diagram shows.
local function add_ecc(fullstream, datastream, version, data_cw, blocks)
  local ecc_cw = qrt.total_codewords[version - 1] - data_cw

  local short_data_block_length = data_cw // blocks
  local qty_long_blocks = data_cw % blocks
  local qty_short_blocks = blocks - qty_long_blocks
  local ecc_block_length = ecc_cw // blocks

  local data_block, ecc_block = {}, {}
  local interleaved_data, interleaved_ecc = {}, {}

  local rs = reedsol.init_gf(0x11d)
  reedsol.init_code(rs, ecc_block_length, 0)

  local in_posn = 0

  for i = 0, blocks - 1 do
    local length_this_block = (i < qty_short_blocks)
                              and short_data_block_length
                              or short_data_block_length + 1

    for j = 0, length_this_block - 1 do
      data_block[j] = datastream[in_posn + j]
    end

    reedsol.encode(rs, length_this_block, data_block, ecc_block)

    for j = 0, short_data_block_length - 1 do
      interleaved_data[(j * blocks) + i] = data_block[j]
    end

    if i >= qty_short_blocks then
      interleaved_data[(short_data_block_length * blocks) + (i - qty_short_blocks)]
        = data_block[short_data_block_length]
    end

    -- reedsol.encode leaves the ECC reversed -- see the note in reedsol.lua.
    for j = 0, ecc_block_length - 1 do
      interleaved_ecc[(j * blocks) + i] = ecc_block[ecc_block_length - j - 1]
    end

    in_posn = in_posn + length_this_block
  end

  for j = 0, data_cw - 1 do fullstream[j] = interleaved_data[j] end
  for j = 0, ecc_cw - 1 do fullstream[j + data_cw] = interleaved_ecc[j] end
end

-- backend/qr.c `qr_place_finder()`. The 7x7 bullseye, three of them, one per
-- corner except bottom-right. Rows are given as bit patterns read from bit 6
-- down, which is why the mask is `0x40 >> xp`.
local FINDER = { [0] = 0x7F, 0x41, 0x5D, 0x5D, 0x5D, 0x41, 0x7F }

local function place_finder(grid, size, x, y)
  for xp = 0, 6 do
    for yp = 0, 6 do
      grid[((yp + y) * size) + (xp + x)] =
        (FINDER[yp] & (0x40 >> xp)) ~= 0 and 0x11 or 0x10
    end
  end
end

-- backend/qr.c `qr_place_align()`. The 5x5 version of the same, and the
-- coordinates given are its CENTRE, hence the -2.
local ALIGNMENT = { [0] = 0x1F, 0x11, 0x15, 0x11, 0x1F }

local function place_align(grid, size, x, y)
  x = x - 2
  y = y - 2
  for xp = 0, 4 do
    for yp = 0, 4 do
      grid[((yp + y) * size) + (xp + x)] =
        (ALIGNMENT[yp] & (0x10 >> xp)) ~= 0 and 0x11 or 0x10
    end
  end
end

-- backend/qr.c `qr_setup_grid()`. Everything that is not data: timing patterns,
-- finders, separators, alignment patterns, and the reserved areas for format
-- and version information.
--
-- Order matters. Timing goes down first and finders overwrite it, and the
-- alignment loop then skips any candidate position already claimed -- that
-- `& 0x10` test is what keeps alignment patterns off the finders.
local function setup_grid(grid, size, version)
  local toggle = 1
  for i = 0, size - 1 do
    local v = toggle == 1 and 0x21 or 0x20
    grid[(6 * size) + i] = v
    grid[(i * size) + 6] = v
    toggle = toggle == 1 and 0 or 1
  end

  place_finder(grid, size, 0, 0)
  place_finder(grid, size, 0, size - 7)
  place_finder(grid, size, size - 7, 0)

  -- Separators: the one-module light border around each finder.
  for i = 0, 6 do
    grid[(7 * size) + i] = 0x10
    grid[(i * size) + 7] = 0x10
    grid[(7 * size) + (size - 1 - i)] = 0x10
    grid[(i * size) + (size - 8)] = 0x10
    grid[((size - 8) * size) + i] = 0x10
    grid[((size - 1 - i) * size) + 7] = 0x10
  end
  grid[(7 * size) + 7] = 0x10
  grid[(7 * size) + (size - 8)] = 0x10
  grid[((size - 8) * size) + 7] = 0x10

  if version ~= 1 then   -- version 1 has no alignment patterns
    local loopsize = qrt.align_loopsize[version - 1]
    for x = 0, loopsize - 1 do
      for y = 0, loopsize - 1 do
        local xcoord = qrt.table_e1[((version - 2) * 7) + x]
        local ycoord = qrt.table_e1[((version - 2) * 7) + y]
        if (grid[(ycoord * size) + xcoord] & 0x10) == 0 then
          place_align(grid, size, xcoord, ycoord)
        end
      end
    end
  end

  -- Reserve space for format information.
  for i = 0, 7 do
    grid[(8 * size) + i] = grid[(8 * size) + i] | 0x20
    grid[(i * size) + 8] = grid[(i * size) + 8] | 0x20
    grid[(8 * size) + (size - 1 - i)] = 0x20
    grid[((size - 1 - i) * size) + 8] = 0x20
  end
  grid[(8 * size) + 8] = grid[(8 * size) + 8] | 0x20
  grid[((size - 1 - 7) * size) + 8] = 0x21   -- the dark module, Figure 25

  if version >= 7 then
    for i = 0, 5 do
      grid[((size - 9) * size) + i] = 0x20
      grid[((size - 10) * size) + i] = 0x20
      grid[((size - 11) * size) + i] = 0x20
      grid[(i * size) + (size - 9)] = 0x20
      grid[(i * size) + (size - 10)] = 0x20
      grid[(i * size) + (size - 11)] = 0x20
    end
  end
end

-- backend/qr.c `qr_cwbit()`. Bit `i` of the codeword stream, MSB first.
local function cwbit(fullstream, i)
  return (fullstream[i >> 3] & (0x80 >> (i & 0x07))) ~= 0 and 1 or 0
end

-- backend/qr.c `qr_populate_grid()`. The zig-zag: two-module-wide columns
-- walked bottom-to-top then top-to-bottom, right pair before left pair within
-- each column, skipping anything with a high nibble.
--
-- The vertical timing pattern in column 6 is the one irregularity -- once the
-- walk reaches it the column indices shift left by one for the rest of the
-- symbol, which is the `if x < 6 then x = x - 1` below.
local function populate_grid(grid, size, fullstream, cw)
  local x_start = size - 2
  local direction = 1   -- 1 = up
  local row = 0         -- which two-wide column, counting from the right
  local n = cw * 8
  local y = size - 1
  local i = 0

  while i < n do
    local x = x_start - (row * 2)
    local r = y * size

    if x < 6 then x = x - 1 end   -- skip over the vertical timing pattern

    if (grid[r + (x + 1)] & 0xf0) == 0 then
      grid[r + (x + 1)] = cwbit(fullstream, i)
      i = i + 1
    end

    if i < n then
      if (grid[r + x] & 0xf0) == 0 then
        grid[r + x] = cwbit(fullstream, i)
        i = i + 1
      end
    end

    if direction ~= 0 then
      y = y - 1
      if y == -1 then       -- reached the top
        row = row + 1
        y = 0
        direction = 0
      end
    else
      y = y + 1
      if y == size then     -- reached the bottom
        row = row + 1
        y = size - 1
        direction = 1
      end
    end
  end
end

-- backend/qr.c `qr_evaluate()`. The standard's four mask penalty rules, summed.
-- Lower is better. `lgrid` is the C's `local`, renamed because `local` is a Lua
-- keyword; by the time it arrives every cell is 0 or 1.
local function evaluate(lgrid, size)
  local result = 0

  -- Test 1: runs of five or more same-coloured modules, scored (run - 2).
  -- Vertical.
  for x = 0, size - 1 do
    local block, state = 0, 0
    for y = 0, size - 1 do
      if lgrid[(y * size) + x] == state then
        block = block + 1
      else
        if block >= 5 then result = result + block - 2 end
        block = 1
        state = lgrid[(y * size) + x]
      end
    end
    if block >= 5 then result = result + block - 2 end
  end

  -- Horizontal, and the dark-module count for Test 4 rides along here exactly
  -- as it does in the C -- `state` always equals the current cell by the time
  -- it is tested, because the else arm assigns it.
  local dark_mods = 0
  for y = 0, size - 1 do
    local r = y * size
    local block, state = 0, 0
    for x = 0, size - 1 do
      if lgrid[r + x] == state then
        block = block + 1
      else
        if block >= 5 then result = result + block - 2 end
        block = 1
        state = lgrid[r + x]
      end
      if state ~= 0 then dark_mods = dark_mods + 1 end
    end
    if block >= 5 then result = result + block - 2 end
  end

  -- Test 2: every 2x2 block of one colour costs 3.
  for x = 0, size - 2 do
    for y = 0, size - 2 do
      local k = lgrid[(y * size) + x]
      if k == lgrid[((y + 1) * size) + x]
        and k == lgrid[(y * size) + (x + 1)]
        and k == lgrid[((y + 1) * size) + (x + 1)] then
        result = result + 3
      end
    end
  end

  -- Test 3: the 1:1:3:1:1 finder-lookalike, 1011101, with four light modules on
  -- either side. Costs 40 each. Checking both sides is why this is the longest
  -- of the four tests; running off the edge counts as light.
  -- Vertical.
  for x = 0, size - 1 do
    local y = 0
    while y <= size - 7 do
      if lgrid[y * size + x] == 1 and lgrid[(y + 1) * size + x] == 0
        and lgrid[(y + 2) * size + x] == 1 and lgrid[(y + 3) * size + x] == 1
        and lgrid[(y + 4) * size + x] == 1 and lgrid[(y + 5) * size + x] == 0
        and lgrid[(y + 6) * size + x] == 1 then
        local beforeCount = 0
        for b = y - 1, y - 4, -1 do
          if b < 0 then          -- past the edge counts as light
            beforeCount = 4
            break
          end
          if lgrid[(b * size) + x] ~= 0 then break end
          beforeCount = beforeCount + 1
        end
        if beforeCount == 4 then
          result = result + 40
        else
          local afterCount = 0
          for a = y + 7, y + 10 do
            if a >= size then
              afterCount = 4
              break
            end
            if lgrid[(a * size) + x] ~= 0 then break end
            afterCount = afterCount + 1
          end
          if afterCount == 4 then result = result + 40 end
        end
        y = y + 3   -- skip to the next possible match
      end
      y = y + 1
    end
  end

  -- Horizontal.
  for y = 0, size - 1 do
    local r = y * size
    local x = 0
    while x <= size - 7 do
      if lgrid[r + x] == 1 and lgrid[r + x + 1] == 0 and lgrid[r + x + 2] == 1
        and lgrid[r + x + 3] == 1 and lgrid[r + x + 4] == 1
        and lgrid[r + x + 5] == 0 and lgrid[r + x + 6] == 1 then
        local beforeCount = 0
        for b = x - 1, x - 4, -1 do
          if b < 0 then
            beforeCount = 4
            break
          end
          if lgrid[r + b] ~= 0 then break end
          beforeCount = beforeCount + 1
        end
        if beforeCount == 4 then
          result = result + 40
        else
          local afterCount = 0
          for a = x + 7, x + 10 do
            if a >= size then
              afterCount = 4
              break
            end
            if lgrid[r + a] ~= 0 then break end
            afterCount = afterCount + 1
          end
          if afterCount == 4 then result = result + 40 end
        end
        x = x + 3
      end
      x = x + 1
    end
  end

  -- Test 4: deviation from a 50/50 light/dark split, 10 points per 5%.
  -- Double arithmetic in the C, so no float32 narrowing here -- Lua numbers are
  -- doubles and this is one of the few places in the port where that is simply
  -- correct rather than something to work around.
  local percentage = (100.0 * dark_mods) / (size * size)
  local k = math.floor(math.abs(percentage - 50.0) / 5.0)
  result = result + 10 * k

  return result
end

-- backend/qr.c `qr_add_format_info()`. Fifteen bits, twice: once around the
-- top-left finder and once split between the other two. The bit values come
-- from `annex_c`, a BCH(15,5) code already masked with 0x5412.
--
-- Note this ORs rather than assigns, which is what makes it safe to run twice
-- over the same grid -- and it IS run twice for mask 7, see apply_bitmask.
local function add_format_info(grid, size, ecc_level, pattern)
  local format = pattern
  if ecc_level == QR_LEVEL_L then format = format | 0x08
  elseif ecc_level == QR_LEVEL_Q then format = format | 0x18
  elseif ecc_level == QR_LEVEL_H then format = format | 0x10 end
  -- QR_LEVEL_M contributes nothing, which is why it has no arm in the C either.

  local seq = qrt.annex_c[format]

  for i = 0, 5 do
    grid[(i * size) + 8] = grid[(i * size) + 8] | ((seq >> i) & 0x01)
  end
  for i = 0, 7 do
    grid[(8 * size) + (size - i - 1)] = grid[(8 * size) + (size - i - 1)] | ((seq >> i) & 0x01)
  end
  for i = 0, 5 do
    grid[(8 * size) + (5 - i)] = grid[(8 * size) + (5 - i)] | ((seq >> (i + 9)) & 0x01)
  end
  for i = 0, 6 do
    grid[(((size - 7) + i) * size) + 8] = grid[(((size - 7) + i) * size) + 8] | ((seq >> (i + 8)) & 0x01)
  end
  grid[(7 * size) + 8] = grid[(7 * size) + 8] | ((seq >> 6) & 0x01)
  grid[(8 * size) + 8] = grid[(8 * size) + 8] | ((seq >> 7) & 0x01)
  grid[(8 * size) + 7] = grid[(8 * size) + 7] | ((seq >> 8) & 0x01)
end

-- backend/qr.c `qr_apply_bitmask()`. Build all eight masks at once -- one per
-- bit of a byte per module -- then score each and keep the cheapest.
--
-- `fast_encode` (FAST_MODE, --fast on the real CLI) evaluates only masks
-- 0, 2, 4 and 7. The front end here has no such flag, so the fast arms are
-- resolved to their slow form and left out.
--
-- The mask-7 special case at the end is not an optimisation detail that can be
-- dropped: when pattern 7 wins, `lgrid` already holds the finished symbol from
-- the last scoring iteration, INCLUDING its format information, so the C copies
-- it back wholesale. Reproducing that matters because the copy also strips the
-- high nibble (`grid[k] & 0x0f` in the loop), and the alternative path does
-- not. Nothing downstream reads the high nibble, so the two agree -- but they
-- agree by luck, not by construction, and rewriting either one would break it.
local function apply_bitmask(grid, size, ecc_level, debug_print)
  local size_squared = size * size
  local mask, lgrid = {}, {}

  for y = 0, size - 1 do
    local r = y * size
    for x = 0, size - 1 do
      local m = 0
      if (grid[r + x] & 0xf0) == 0 then   -- data modules only
        if ((y + x) & 1) == 0 then m = m | 0x01 end
        if (y & 1) == 0 then m = m | 0x02 end
        if (x % 3) == 0 then m = m | 0x04 end
        if ((y + x) % 3) == 0 then m = m | 0x08 end
        if (((y // 2) + (x // 3)) & 1) == 0 then m = m | 0x10 end
        -- The C's comment: equivalent to (y*x) % 2 + (y*x) % 3 == 0
        if ((y * x) % 6) == 0 then m = m | 0x20 end
        if ((((y * x) & 1) + ((y * x) % 3)) & 1) == 0 then m = m | 0x40 end
        if ((((y + x) & 1) + ((y * x) % 3)) & 1) == 0 then m = m | 0x80 end
      end
      mask[r + x] = m
    end
  end

  local penalty = {}
  local best_pattern = 0
  for pattern = 0, 7 do
    local bit = 1 << pattern
    for k = 0, size_squared - 1 do
      if (mask[k] & bit) ~= 0 then
        lgrid[k] = grid[k] ~ 0x01
      else
        lgrid[k] = grid[k] & 0x0f
      end
    end
    add_format_info(lgrid, size, ecc_level, pattern)
    penalty[pattern] = evaluate(lgrid, size)
    if penalty[pattern] < penalty[best_pattern] then
      best_pattern = pattern
    end
  end

  if debug_print then
    local s = {}
    for pattern = 0, 7 do s[#s + 1] = (" %d:%d"):format(pattern, penalty[pattern]) end
    io.stderr:write(("Mask: %d (automatic)"):format(best_pattern), table.concat(s), "\n")
  end

  if best_pattern == 7 then     -- reuse the last scoring pass
    for k = 0, size_squared - 1 do grid[k] = lgrid[k] end
  else
    local bit = 1 << best_pattern
    for y = 0, size_squared - 1 do
      if (mask[y] & bit) ~= 0 then grid[y] = grid[y] ~ 0x01 end
    end
  end

  return best_pattern
end

-- backend/qr.c `qr_add_version_info()`. Eighteen bits of BCH(18,6), duplicated
-- near the bottom-left and top-right finders. Versions 7 and up only.
--
-- The `& 0x41` and `+=` look wrong and are not: bit 0 is the module value and
-- bit 6 is a marker the C adds on top of the 0x20 that setup_grid reserved
-- there. The result still has a non-zero high nibble, so the masker leaves it
-- alone, which is the point.
local function add_version_info(grid, size, version)
  local version_data = qrt.annex_d[version - 7]
  for i = 0, 5 do
    local a = (version_data >> (i * 3)) & 0x41
    local b = (version_data >> ((i * 3) + 1)) & 0x41
    local c = (version_data >> ((i * 3) + 2)) & 0x41
    local i11, i10, i9 = (size - 11) * size + i, (size - 10) * size + i, (size - 9) * size + i
    grid[i11] = grid[i11] + a
    grid[i10] = grid[i10] + b
    grid[i9]  = grid[i9] + c
    local j11, j10, j9 = i * size + (size - 11), i * size + (size - 10), i * size + (size - 9)
    grid[j11] = grid[j11] + a
    grid[j10] = grid[j10] + b
    grid[j9]  = grid[j9] + c
  end
end

-- backend/qr.c `qr_prep_data()`. Source bytes to `ddata`, an array of code
-- points. One segment. DATA_MODE is unreachable (no --binary).
--
-- ECI 0 tries ISO/IEC 8859-1 first. That is the whole story for `café`,
-- `Ünicode`, and `£100`: every value is 0..255 and there is no warning.
-- If Latin-1 fails and the segment ECI is still 0, Shift JIS is next. Success
-- warns 760 and does not set an ECI, so Kanji mode can be used and the stream
-- has no ECI header. If Shift JIS fails too, this returns Error 800 (or 242
-- for a 4-byte scalar) and the driver retries with `symbol.eci` set.
--
-- Returns ddata (0-based), its length, and an optional warning. On failure the
-- second return is the message.
local function prep_data(symbol, source)
  local seg_eci = symbol.eci or 0
  if seg_eci ~= 20 then
    local ddata, n = eci.sjis_utf8_to_eci(seg_eci, source)
    if ddata then return ddata, n end
    if seg_eci ~= 0 then
      return nil, string.format("Error 575: Invalid character in input for ECI '%d'", seg_eci)
    end
  end
  local ddata, n_or_err = eci.sjis_utf8(source)
  if not ddata then return nil, n_or_err end
  if seg_eci ~= 20 then
    return ddata, n_or_err, "Warning 760: Converted to Shift JIS but no ECI specified"
  end
  return ddata, n_or_err
end

-- backend/qr.c `qrcode()`. The driver.
--
-- Resolved away because the front end cannot reach them: --secure (option_1),
-- --vers (option_2), --mask (option_3 high bits), --fast and --structapp.
-- There is no --eci flag either. `symbol.eci` is still set when the driver
-- retries a character-set failure; that is the automatic-ECI path, not the flag.
-- What is left besides that is the autosize path.
function M.qrcode(symbol, source, opts)
  local debug_print = DEBUG
  local gs1 = (symbol.input_mode & 0x07) == common.GS1_MODE
  local seg_eci = symbol.eci or 0

  local ddata, length, warning = prep_data(symbol, source)
  if not ddata then return nil, length end
  if warning then symbol.warning = warning end

  local mode = {}

  -- Estimate at version 40 first: the character count indicators are widest
  -- there, so the estimate is an upper bound and the version chosen from it can
  -- only be too big, never too small.
  local est_binlen = calc_binlen_segs(40, mode, ddata, length, false, gs1, seg_eci, debug_print)

  -- ECC level starts at L with the matching maximum codeword count. The C
  -- switches on option_1 here; without --secure it is always -1, so L it is --
  -- but see the boost below, which is what actually decides the level.
  local ecc_level = QR_LEVEL_L
  local max_cw = 2956

  if est_binlen > (8 * max_cw) then
    return nil, "Error 561: Input too long for selected error correction level"
  end

  -- Smallest version whose data capacity covers the estimate. The loop runs
  -- downwards and keeps overwriting, so it ends holding the smallest match.
  local autosize = 40
  for i = 39, 0, -1 do
    if (8 * qrt.data_codewords_L[i]) >= est_binlen then autosize = i + 1 end
  end

  if autosize ~= 40 then
    -- Re-estimate at the chosen version: narrower count indicators may make the
    -- stream shorter, and may also change which modes the optimiser picks.
    --
    -- The C guards this with `if (prev_est_binlen < est_binlen) assert(0)` and
    -- a defensive re-run, for a case its own comment says should not happen.
    -- An assert that fires is a crash, so there is nothing to reproduce: the
    -- port would have to invent behaviour the C does not have.
    est_binlen = calc_binlen_segs(autosize, mode, ddata, length, false, gs1, seg_eci, debug_print)
  end

  -- Now see whether the optimised stream fits in a smaller symbol still. Each
  -- round re-runs the optimiser one version down, because a smaller version has
  -- narrower count indicators and can flip the mode choice.
  local prev_mode = {}
  local canShrink = true
  repeat
    if autosize == 1 then
      canShrink = false
    else
      local prev_est_binlen = est_binlen
      for i = 0, length - 1 do prev_mode[i] = mode[i] end
      est_binlen = calc_binlen_segs(autosize - 1, mode, ddata, length, false, gs1, seg_eci, debug_print)

      if (8 * qrt.data_codewords_L[autosize - 2]) < est_binlen then
        canShrink = false
      end

      if canShrink then
        autosize = autosize - 1
      else
        est_binlen = prev_est_binlen
        for i = 0, length - 1 do mode[i] = prev_mode[i] end
      end
    end
  until not canShrink

  local version = autosize

  -- Ensure maximum error correction capacity. With no --secure this always
  -- runs, and it is why a short numeric payload comes out as 1-H rather than
  -- 1-L: the level is raised as far as it can go WITHOUT growing the symbol.
  if est_binlen <= qrt.data_codewords_M[version - 1] * 8 then ecc_level = QR_LEVEL_M end
  if est_binlen <= qrt.data_codewords_Q[version - 1] * 8 then ecc_level = QR_LEVEL_Q end
  if est_binlen <= qrt.data_codewords_H[version - 1] * 8 then ecc_level = QR_LEVEL_H end

  local target_codewords, blocks
  if ecc_level == QR_LEVEL_M then
    target_codewords, blocks = qrt.data_codewords_M[version - 1], qrt.blocks_M[version - 1]
  elseif ecc_level == QR_LEVEL_Q then
    target_codewords, blocks = qrt.data_codewords_Q[version - 1], qrt.blocks_Q[version - 1]
  elseif ecc_level == QR_LEVEL_H then
    target_codewords, blocks = qrt.data_codewords_H[version - 1], qrt.blocks_H[version - 1]
  else
    target_codewords, blocks = qrt.data_codewords_L[version - 1], qrt.blocks_L[version - 1]
  end

  if debug_print then
    io.stderr:write(("Minimum codewords: %d\n"):format((est_binlen + 7) // 8))
    io.stderr:write(("Selected version: %d-%s (%dx%d)\n"):format(
      version, ECC_LEVEL_NAMES[ecc_level], qrt.sizes[version - 1], qrt.sizes[version - 1]))
    io.stderr:write(("Number of data codewords in symbol: %d\n"):format(target_codewords))
    io.stderr:write(("Number of ECC blocks: %d\n"):format(blocks))
  end

  local datastream, fullstream = {}, {}
  binary_segs(datastream, version, target_codewords, mode, ddata, length, gs1, seg_eci, debug_print)
  add_ecc(fullstream, datastream, version, target_codewords, blocks)

  local size = qrt.sizes[version - 1]
  local grid = {}
  for i = 0, size * size - 1 do grid[i] = 0 end

  setup_grid(grid, size, version)
  populate_grid(grid, size, fullstream, qrt.total_codewords[version - 1])

  if version >= 7 then
    add_version_info(grid, size, version)
  end

  local bitmask = apply_bitmask(grid, size, ecc_level, debug_print)
  add_format_info(grid, size, ecc_level, bitmask)

  symbol.width = size
  symbol.rows = size
  for i = 0, size - 1 do
    local r = i * size
    for j = 0, size - 1 do
      if (grid[r + j] & 0x01) ~= 0 then
        common.set_module(symbol, i, j)
      end
    end
    symbol.row_height[i] = 1
  end
  symbol.height = size

  return true
end

return M
