-- src/zint/code128.lua -- Code 128, GS1-128, NVE-18, and Code 128 suppress C.
--
-- The symbol characters are chosen by the divide-and-conquer encoder in
-- zint 2.16 backend/code128.c (Alex Geller's minimal encoding, with the
-- extended-ASCII states from BWIPP). Code set states are A0, B0, A1, B1,
-- C0 and C1: the letter is the set, and 1 means the extended-ASCII shift
-- is latched. The encoder never starts in C1.

local common = require "zint.common"
local gs1 = require "zint.gs1"

local M = {}
local ctoi = common.ctoi
local f32 = common.stripf

local C128_MAX = 256
local C128_SYMBOL_MAX = 102

local A0, B0, A1, B1, C0, C1 = 1, 2, 3, 4, 5, 6

-- backend/code128.c `C128Table`. 107 rows, six element widths, each row
-- summing to 11. 0-102 are data, 103-105 the starts, 106 the stop. The last
-- row is CODE16K-only and is never indexed here.
local C128Table = {
  "212222", "222122", "222221", "121223", "121322", "131222", "122213", "122312",
  "132212", "221213", "221312", "231212", "112232", "122132", "122231", "113222",
  "123122", "123221", "223211", "221132", "221231", "213212", "223112", "312131",
  "311222", "321122", "321221", "312212", "322112", "322211", "212123", "212321",
  "232121", "111323", "131123", "131321", "112313", "132113", "132311", "211313",
  "231113", "231311", "112133", "112331", "132131", "113123", "113321", "133121",
  "313121", "211331", "231131", "213113", "213311", "213131", "311123", "311321",
  "331121", "312113", "312311", "332111", "314111", "221411", "431111", "111224",
  "111422", "121124", "121421", "141122", "141221", "112214", "112412", "122114",
  "122411", "142112", "142211", "241211", "221114", "413111", "241112", "134111",
  "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112",
  "421211", "212141", "214121", "412121", "111143", "111341", "131141", "114113",
  "114311", "411113", "411311", "113141", "114131", "311141", "411131", "211412",
  "211214", "211232", "211133",
}

-- Latch codewords to move from a prior state (row) to a current state (column).
-- A length of 64 marks a transition the encoder must not take. C0 to C1 and
-- C1 to C0 are that kind, and so is a start in C1.
local LATCH_LEN = {
  [A0] = { [A0] = 0, [B0] = 1, [A1] = 2, [B1] = 3, [C0] = 1, [C1] = 3 },
  [B0] = { [A0] = 1, [B0] = 0, [A1] = 3, [B1] = 2, [C0] = 1, [C1] = 3 },
  [A1] = { [A0] = 2, [B0] = 3, [A1] = 0, [B1] = 1, [C0] = 3, [C1] = 1 },
  [B1] = { [A0] = 3, [B0] = 2, [A1] = 1, [B1] = 0, [C0] = 3, [C1] = 1 },
  [C0] = { [A0] = 1, [B0] = 1, [A1] = 3, [B1] = 3, [C0] = 0, [C1] = 64 },
  [C1] = { [A0] = 3, [B0] = 3, [A1] = 1, [B1] = 1, [C0] = 64, [C1] = 0 },
}
local LATCH_SEQ = {
  [A0] = { [A0] = {}, [B0] = { 100 }, [A1] = { 101, 101 }, [B1] = { 100, 100, 100 },
           [C0] = { 99 }, [C1] = { 101, 101, 99 } },
  [B0] = { [A0] = { 101 }, [B0] = {}, [A1] = { 101, 101, 101 }, [B1] = { 100, 100 },
           [C0] = { 99 }, [C1] = { 100, 100, 99 } },
  [A1] = { [A0] = { 101, 101 }, [B0] = { 100, 100, 100 }, [A1] = {}, [B1] = { 100 },
           [C0] = { 101, 101, 99 }, [C1] = { 99 } },
  [B1] = { [A0] = { 101, 101, 101 }, [B0] = { 100, 100 }, [A1] = { 101 }, [B1] = {},
           [C0] = { 100, 100, 99 }, [C1] = { 99 } },
  [C0] = { [A0] = { 101 }, [B0] = { 100 }, [A1] = { 101, 101, 101 }, [B1] = { 100, 100, 100 },
           [C0] = {}, [C1] = {} },
  [C1] = { [A0] = { 101, 101, 101 }, [B0] = { 100, 100, 100 }, [A1] = { 101 }, [B1] = { 100 },
           [C0] = {}, [C1] = {} },
}

-- start_idx: 0 normal, 1 GS1 (FNC1 after the start), 2 reader initialisation.
local START_LEN = {
  [0] = { [A0] = 1, [B0] = 1, [A1] = 3, [B1] = 3, [C0] = 1, [C1] = 64 },
  [1] = { [A0] = 2, [B0] = 2, [A1] = 4, [B1] = 4, [C0] = 2, [C1] = 64 },
  [2] = { [A0] = 2, [B0] = 2, [A1] = 4, [B1] = 4, [C0] = 3, [C1] = 64 },
}
local START_SEQ = {
  [0] = { [A0] = { 103 }, [B0] = { 104 }, [A1] = { 103, 101, 101 },
          [B1] = { 104, 100, 100 }, [C0] = { 105 }, [C1] = {} },
  [1] = { [A0] = { 103, 102 }, [B0] = { 104, 102 }, [A1] = { 103, 102, 101, 101 },
          [B1] = { 104, 102, 100, 100 }, [C0] = { 105, 102 }, [C1] = {} },
  [2] = { [A0] = { 103, 96 }, [B0] = { 104, 96 }, [A1] = { 103, 96, 101, 101 },
          [B1] = { 104, 96, 100, 100 }, [C0] = { 104, 96, 99 }, [C1] = {} },
}

local function is_digit(c)
  return c >= 48 and c <= 57
end

local function is_ascii(c)
  return (c & 0x80) == 0
end

-- Map an extended state back to its unextended set. A1 -> A0, B1 -> B0.
local function ab_set(cset)
  if cset > B0 then return cset >> 1 end
  return cset
end

local function cost_ab(cset, ch, mode)
  local mask = ch & 0x60
  local ga = (cset & 1) ~= 0 -- A0 and A1 are odd
  local cost = 1
  if (ga and mask == 0x60) or (not ga and mask == 0) then
    cost = cost + 1
    mode = mode | 0x10
  end
  local extended_state = cset > B0
  if extended_state == is_ascii(ch) then
    -- A0/B0 with a byte above 127, or A1/B1 with an ASCII byte: one FNC4.
    cost = cost + 1
    mode = mode | 0x20
  end
  return cost, mode
end

-- Shortest encoding of source[i..] given the set we are already in.
-- prior_cset 0 means "no start character yet".
local function c128_cost(source, length, i, prior, start_idx, priority, fncs, manuals, costs, modes)
  local ch = source[i]
  local latch_len = prior == 0 and START_LEN[start_idx] or LATCH_LEN[prior]
  local is_fnc1 = ch == 0x1D and fncs[i]
  local next_b = source[i + 1] or 0
  local can_c = is_fnc1 or (is_digit(ch) and is_digit(next_b))
  local manual = manuals[i] or 0
  local manual_c_fail = (not can_c) and manual == C0
  local min_cost = 999999
  local min_mode = 0

  for p = 1, #priority do
    local cset = priority[p]
    if cset >= C0 then
      if can_c and (manual == 0 or manual == C0) then
        local incr = is_fnc1 and 1 or 2
        local mode = prior
        local cost = 1
        if prior ~= cset then
          cost = cost + latch_len[cset]
          mode = cset
        end
        if i + incr < length then
          local memo = costs[i + incr][cset]
          if memo then
            cost = cost + memo
          else
            cost = cost + c128_cost(source, length, i + incr, cset, 0, priority, fncs, manuals, costs, modes)
          end
        end
        if cost < min_cost then
          min_cost = cost
          min_mode = mode
        end
      end
    else
      if manual == 0 or manual == ab_set(cset) or manual_c_fail then
        local mode = cset
        local cost
        if is_fnc1 then
          cost = 1
        else
          cost, mode = cost_ab(cset, ch, mode)
        end
        if prior ~= cset then
          cost = cost + latch_len[cset]
        end
        if i + 1 < length then
          local memo = costs[i + 1][cset]
          if memo then
            cost = cost + memo
          else
            cost = cost + c128_cost(source, length, i + 1, cset, 0, priority, fncs, manuals, costs, modes)
          end
        end
        if cost < min_cost then
          min_cost = cost
          min_mode = mode
        end
      end
    end
  end

  costs[i][prior] = min_cost
  modes[i][prior] = min_mode
  return min_cost
end

local function emit_seq(values, n, seq, len)
  for j = 1, len do
    values[n] = seq[j]
    n = n + 1
  end
  return n
end

-- Returns values (0-based), glyph count, and the set of the last character.
local function set_values(source, length, start_idx, priority, fncs, manuals)
  local costs, modes = {}, {}
  for i = 0, length - 1 do
    costs[i] = {}
    modes[i] = {}
  end
  c128_cost(source, length, 0, 0, start_idx, priority, fncs, manuals, costs, modes)

  local glyph_count = costs[0][0]
  if glyph_count > C128_SYMBOL_MAX then
    return nil, glyph_count
  end

  local values = {}
  local n = 0
  local cset = 0
  local i = 0
  while i < length do
    local ch = source[i]
    local is_fnc1 = ch == 0x1D and fncs[i]
    local mode = modes[i][cset]
    local prior = cset
    cset = mode & 0x0F
    if cset ~= prior then
      if prior == 0 then
        n = emit_seq(values, n, START_SEQ[start_idx][cset], START_LEN[start_idx][cset])
      else
        n = emit_seq(values, n, LATCH_SEQ[prior][cset], LATCH_LEN[prior][cset])
      end
    end
    if mode >= 0x30 then
      values[n] = 100 + (cset & 1) -- FNC4
      values[n + 1] = 98 -- shift
      n = n + 2
    elseif mode >= 0x20 then
      values[n] = 100 + (cset & 1)
      n = n + 1
    elseif mode >= 0x10 then
      values[n] = 98
      n = n + 1
    end
    if is_fnc1 then
      values[n] = 102
      n = n + 1
    elseif cset >= C0 then
      values[n] = (ch - 48) * 10 + source[i + 1] - 48
      n = n + 1
      i = i + 1
    else
      local add96 = (ch & 0x60) == 0 and 96 or 0
      values[n] = (ch & 0x7F) + add96 - 32
      n = n + 1
    end
    i = i + 1
  end

  return values, n, cset
end

local function expand(symbol, values, glyph_count)
  local dest = { C128Table[values[0] + 1] }
  local total = values[0]
  for i = 1, glyph_count - 1 do
    dest[#dest + 1] = C128Table[values[i] + 1]
    total = total + values[i] * i
  end
  total = total % 103
  dest[#dest + 1] = C128Table[total + 1]
  dest[#dest + 1] = "2331112"
  common.expand(symbol, table.concat(dest))
end

local function set_priority(have_a, have_b, have_c, have_extended)
  local p = {}
  if have_c then p[#p + 1] = C0 end
  if have_b or not have_a then p[#p + 1] = B0 end
  if have_a then p[#p + 1] = A0 end
  if have_extended then
    if have_c then p[#p + 1] = C1 end
    if have_b or not have_a then p[#p + 1] = B1 end
    if have_a then p[#p + 1] = A1 end
  end
  return p
end

-- `ab_only` is symbology 60 (Code 128 suppress C). Code set C stays unused,
-- so a run of digits is encoded in set A or B.
local function classify(source, length, fncs, manuals, ab_only)
  local have_a, have_b, have_c, have_extended = false, false, false, false
  local digit = false
  for i = 0, length - 1 do
    local ch = source[i]
    if not (ch == 0x1D and fncs[i]) then
      local mask = ch & 0x60
      local manual = manuals[i] or 0
      if (ch & 0x80) ~= 0 then have_extended = true end
      if mask == 0 or manual == A0 then have_a = true end
      if mask == 0x60 or manual == B0 then have_b = true end
      if not ab_only then
        local prev = digit
        digit = is_digit(ch)
        if prev and digit then have_c = true end
      end
    end
  end
  return have_a, have_b, have_c, have_extended
end

-- `\^A` `\^B` `\^C` pin the following characters. `\^1` is a manual FNC1.
-- `\^^` is a literal backslash and caret. Returns the stripped bytes, the
-- per-character manual set, the FNC1 map, and whether any manual FNC1 occurred.
local function apply_extra_escape(source)
  local manuals, fncs, buf = {}, {}, {}
  local manual = 0
  local have_fnc1 = false
  local j = 0
  local i = 1
  local length = #source
  while i <= length do
    local b = source:byte(i)
    local b2 = source:byte(i + 1)
    local b3 = source:byte(i + 2)
    if b == 0x5C and i + 2 <= length and b2 == 0x5E
        and ((b3 >= 0x40 and b3 <= 0x43) or b3 == 0x31 or b3 == 0x5E) then
      if b3 == 0x5E then
        manuals[j] = manual
        buf[j] = b
        j = j + 1
        manuals[j] = manual
        buf[j] = b2
        j = j + 1
        i = i + 3
      elseif b3 == 0x31 then
        fncs[j] = true
        have_fnc1 = true
        manuals[j] = manual
        buf[j] = 0x1D
        j = j + 1
        i = i + 3
      else
        i = i + 3
        manual = b3 == 0x43 and C0 or (b3 - 0x40)
      end
    else
      manuals[j] = manual
      buf[j] = b
      j = j + 1
      i = i + 1
    end
  end
  if j == length then
    return nil -- nothing was stripped; the caller keeps the original bytes
  end
  if j == 0 then
    return false
  end
  buf[j] = 0 -- NUL so a digit test past the end is false
  return buf, j, manuals, fncs, have_fnc1
end

local function bytes_of(text)
  local src = {}
  for i = 0, #text - 1 do src[i] = text:byte(i + 1) end
  src[#text] = 0
  return src
end

-- backend/common.c `hrt_cpy_iso8859_1()`.
local function hrt_cpy_iso8859_1(symbol, source)
  local out = {}
  local j = 0
  for i = 1, #source do
    local b = source:byte(i)
    if j >= 200 then break end
    if b < 0x80 then
      out[#out + 1] = (b >= 0x20 and b ~= 0x7F) and string.char(b) or " "
      j = j + 1
    elseif b < 0xC0 then
      if b >= 0xA0 then
        if j + 2 >= 200 then break end
        out[#out + 1] = string.char(0xC2, b)
        j = j + 2
      else
        out[#out + 1] = " "
        j = j + 1
      end
    else
      if j + 2 >= 200 then break end
      out[#out + 1] = string.char(0xC3, b - 0x40)
      j = j + 2
    end
  end
  local text = table.concat(out)
  symbol.text = #text == 200 and text:sub(1, 199) or text
end

M.hrt_cpy_iso8859_1 = hrt_cpy_iso8859_1

local function too_long(n, code, limit, what)
  return nil, "Error " .. code .. ": Input " .. what .. " " .. n .. " too long (maximum " .. limit .. ")"
end

function M.code128(symbol, source)
  local length = #source
  if length == 0 then return nil, "Error 842: No input data" end
  if length > C128_MAX then
    return too_long(length, 340, C128_MAX, "length")
  end

  local src = bytes_of(source)
  local manuals, fncs = {}, {}
  local have_fnc1 = false
  local hrt_src = source

  if (symbol.input_mode & common.EXTRA_ESCAPE_MODE) ~= 0 and symbol.symbology == 20 then
    local stripped, n, man, fnc, saw = apply_extra_escape(source)
    if stripped == false then return nil, "Error 842: No input data" end
    if stripped then
      src, length, manuals, fncs, have_fnc1 = stripped, n, man, fnc, saw
      local chars = {}
      for i = 0, n - 1 do
        if not fncs[i] then chars[#chars + 1] = string.char(src[i]) end
      end
      hrt_src = table.concat(chars)
    end
  end

  local ab_only = symbol.symbology == 60
  local have_a, have_b, have_c, have_extended = classify(src, length, fncs, manuals, ab_only)
  local priority = set_priority(have_a, have_b, have_c, have_extended)
  local start_idx = (symbol.output_options & common.READER_INIT) ~= 0 and 2 or 0
  local values, glyph_count = set_values(src, length, start_idx, priority, fncs, manuals)
  if not values then
    return too_long(glyph_count, 341, C128_SYMBOL_MAX, "requires")
  end

  expand(symbol, values, glyph_count)
  hrt_cpy_iso8859_1(symbol, hrt_src)
  return true
end

-- GS1-128. `reduced` still uses `[` as the FNC1 marker, which is what qr.lua
-- reads. The 2.16 Code 128 encoder reads GS (0x1D) instead, so the marker is
-- translated here and nowhere else.
function M.gs1_128(symbol, source, reduced)
  if #source > C128_MAX then
    return too_long(#source, 342, C128_MAX, "length")
  end

  local src, fncs = {}, {}
  local n = 0
  for i = 1, #reduced do
    local b = reduced:byte(i)
    src[n] = b == 0x5B and 0x1D or b
    fncs[n] = true
    n = n + 1
  end
  src[n] = 0

  local priority = set_priority(false, true, true, false)
  local values, glyph_count = set_values(src, n, 1, priority, fncs, {})
  if not values then
    return too_long(glyph_count, 344, C128_SYMBOL_MAX, "requires")
  end

  expand(symbol, values, glyph_count)

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    local min_height = f32(f32(5.8) / f32(1.016))
    local default_height = f32(f32(31.75) / f32(0.495))
    common.set_height(symbol, min_height, default_height, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  if (symbol.output_options & common.READER_INIT) ~= 0 and not symbol.warning then
    symbol.warning = "Warning 845: Cannot use Reader Initialisation in GS1 mode, ignoring"
  end

  local text = source:sub(1, 200):gsub("%[", "("):gsub("%]", ")")
  symbol.text = #text == 200 and text:sub(1, 199) or text
  return true
end

-- backend/gs1.c `gs1_check_digit()`.
local function gs1_check_digit(source, length)
  local count = 0
  local factor = (length & 1) ~= 0 and 3 or 1
  for i = 1, length do
    count = count + factor * (source:byte(i) - 48)
    factor = factor ~ 2
  end
  return string.char(48 + (10 - (count % 10)) % 10)
end

-- backend/code128_based.c `nve18_or_ean14()`, data_len 17.
-- Builds a GS1-128 "(00)" / "[00]" string and encodes that. The delimiter
-- follows --gs1parens; the human-readable line is always parentheses, which
-- gs1_128() does by rewriting brackets. EAN-14 is the same helper with
-- data_len 13 and is not registered.
function M.nve18(symbol, source)
  local data_len = 17
  local length = #source
  if (length == data_len + 4 or length == data_len + 5)
      and (source:sub(1, 4) == "(00)" or source:sub(1, 4) == "[00]") then
    source = source:sub(5)
    length = length - 4
  elseif (length == data_len + 2 or length == data_len + 3)
      and source:sub(1, 2) == "00" then
    source = source:sub(3)
    length = length - 2
  end
  if length > data_len + 1 then
    return nil, string.format(
      "Error 345: Input length %d too long (maximum %d)", length, data_len + 1)
  end
  for i = 1, length do
    local c = source:byte(i)
    if not c or c < 48 or c > 57 then
      return nil, string.format(
        "Error 346: Invalid character at position %d in input (digits only)", i)
    end
  end

  local supplied
  if length == data_len + 1 then
    supplied = source:sub(length, length)
    source = source:sub(1, length - 1)
    length = length - 1
  end
  local digits = string.rep("0", data_len - length) .. source
  local check = gs1_check_digit(digits, data_len)
  if supplied and supplied ~= check then
    return nil, string.format(
      "Error 347: Invalid check digit '%s', expecting '%s'", supplied, check)
  end

  local open_b, close_b = "[", "]"
  if (symbol.input_mode & common.GS1PARENS_MODE) ~= 0 then
    open_b, close_b = "(", ")"
  end
  local equiv = open_b .. "00" .. close_b .. digits .. check
  local reduced, err = gs1.verify(symbol, equiv)
  if not reduced then return nil, err end
  return M.gs1_128(symbol, equiv, reduced)
end

return M
