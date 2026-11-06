-- zint/dbar.lua -- GS1 DataBar.
--
-- Expanded (31) and Expanded Stacked (81): backend/rss.c `zint_dbar_exp()` /
-- `zint_dbar_exp_cc()` with cc_rows left at 0, plus the general-purpose
-- compaction those two call (backend/general_field.c).
--
-- Omnidirectional (29) and Stacked Omnidirectional (80): `zint_dbar_omn()` /
-- `zint_dbar_omn_cc()` with cc_rows left at 0. Limited, the non-omnidirectional
-- stacked form, and the composite forms are not encoded here.
--
-- The reduced GS1 string this port already uses marks FNC1 with "[", because
-- that is what gs1.lua and the Code 128 / QR encoders agreed on. The C
-- encoder reads GS (0x1D). The marker is translated at the start of this
-- file and nowhere else.

local common = require "zint.common"
local gs1 = require "zint.gs1"

local M = {}

local BARCODE_DBAR_OMN = 29
local BARCODE_DBAR_EXP = 31
local BARCODE_DBAR_OMNSTK = 80
local BARCODE_DBAR_EXPSTK = 81

local GF_NUMERIC = 110
local GF_ALPHANUMERIC = 97
local GF_ISOIEC = 105
local GS = 0x1D

local ALNUM_PUNCS = "*,-./"
local ISOIEC_PUNCS = "!\"%&'()*+,-./:;<=>?_ "

local function vec(list)
  local t = {}
  for i, v in ipairs(list) do t[i - 1] = v end
  return t
end

local function rows(list)
  local t = {}
  for i, row in ipairs(list) do
    local r = {}
    for j, v in ipairs(row) do r[j - 1] = v end
    t[i - 1] = r
  end
  return t
end

-- backend/rss.h. Transcribed, not recomputed. Finder sequence is declared
-- [18] in the header and initialised with the ten rows a 4..22 symbol
-- character symbol indexes: p = (symbol_chars - 1) / 2 - 1 lands on 0..9.
local G_SUM = vec { 0, 348, 1388, 2948, 3988 }
local T_EVEN = vec { 4, 20, 52, 104, 204 }
local MODULES = vec { 12, 10, 8, 6, 4 }
local WIDEST = vec { 7, 5, 4, 3, 1 }

local CHECKSUM_WEIGHT = rows {
  {   1,   3,   9,  27,  81,  32,  96,  77 },
  {  20,  60, 180, 118, 143,   7,  21,  63 },
  { 189, 145,  13,  39, 117, 140, 209, 205 },
  { 193, 157,  49, 147,  19,  57, 171,  91 },
  {  62, 186, 136, 197, 169,  85,  44, 132 },
  { 185, 133, 188, 142,   4,  12,  36, 108 },
  { 113, 128, 173,  97,  80,  29,  87,  50 },
  { 150,  28,  84,  41, 123, 158,  52, 156 },
  {  46, 138, 203, 187, 139, 206, 196, 166 },
  {  76,  17,  51, 153,  37, 111, 122, 155 },
  {  43, 129, 176, 106, 107, 110, 119, 146 },
  {  16,  48, 144,  10,  30,  90,  59, 177 },
  { 109, 116, 137, 200, 178, 112, 125, 164 },
  {  70, 210, 208, 202, 184, 130, 179, 115 },
  { 134, 191, 151,  31,  93,  68, 204, 190 },
  { 148,  22,  66, 198, 172,  94,  71,   2 },
  {   6,  18,  54, 162,  64, 192, 154,  40 },
  { 120, 149,  25,  75,  14,  42, 126, 167 },
  {  79,  26,  78,  23,  69, 207, 199, 175 },
  { 103,  98,  83,  38, 114, 131, 182, 124 },
  { 161,  61, 183, 127, 170,  88,  53, 159 },
  {  55, 165,  73,   8,  24,  72,   5,  15 },
  {  45, 135, 194, 160,  58, 174, 100,  89 },
}

local FINDER_PATTERN = rows {
  { 1, 8, 4, 1, 1 },
  { 1, 1, 4, 8, 1 },
  { 3, 6, 4, 1, 1 },
  { 1, 1, 4, 6, 3 },
  { 3, 4, 6, 1, 1 },
  { 1, 1, 6, 4, 3 },
  { 3, 2, 8, 1, 1 },
  { 1, 1, 8, 2, 3 },
  { 2, 6, 5, 1, 1 },
  { 1, 1, 5, 6, 2 },
  { 2, 2, 9, 1, 1 },
  { 1, 1, 9, 2, 2 },
}

local FINDER_SEQUENCE = rows {
  { 1,  2, 0, 0, 0,  0,  0,  0,  0,  0,  0 },
  { 1,  4, 3, 0, 0,  0,  0,  0,  0,  0,  0 },
  { 1,  6, 3, 8, 0,  0,  0,  0,  0,  0,  0 },
  { 1, 10, 3, 8, 5,  0,  0,  0,  0,  0,  0 },
  { 1, 10, 3, 8, 7, 12,  0,  0,  0,  0,  0 },
  { 1, 10, 3, 8, 9, 12, 11,  0,  0,  0,  0 },
  { 1,  2, 3, 4, 5,  6,  7,  8,  0,  0,  0 },
  { 1,  2, 3, 4, 5,  6,  7, 10,  9,  0,  0 },
  { 1,  2, 3, 4, 5,  6,  7, 10, 11, 12,  0 },
  { 1,  2, 3, 4, 5,  8,  7, 10,  9, 12, 11 },
}

local WEIGHT_ROWS = rows {
  { 0,  1,  2, 0, 0,  0, 0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  5,  6, 3, 4,  0, 0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  9, 10, 3, 4, 13, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0, 17, 18, 3, 4, 13, 14,  7,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0, 17, 18, 3, 4, 13, 14, 11, 12, 21, 22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0, 17, 18, 3, 4, 13, 14, 15, 16, 21, 22, 19, 20,  0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  1,  2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  0,  0,  0,  0,  0,  0 },
  { 0,  1,  2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12, 17, 18, 15, 16,  0,  0,  0,  0 },
  { 0,  1,  2, 3, 4,  5,  6,  7,  8,  9, 10, 11, 12, 17, 18, 19, 20, 21, 22,  0,  0 },
  { 0,  1,  2, 3, 4,  5,  6,  7,  8, 13, 14, 11, 12, 17, 18, 15, 16, 21, 22, 19, 20 },
}

local ALNUM_SET = {}
for i = 1, #ALNUM_PUNCS do ALNUM_SET[ALNUM_PUNCS:byte(i)] = true end
local ISOIEC_SET = {}
for i = 1, #ISOIEC_PUNCS do ISOIEC_SET[ISOIEC_PUNCS:byte(i)] = true end
for b = 97, 122 do ISOIEC_SET[b] = true end

-- backend/common.c `bin_append_posn()`. `binary` is a 0-based table of "0"/"1".
local function bin_append(arg, length, binary, bp)
  local last = length - 1
  for i = 0, length - 1 do
    binary[bp + i] = ((arg >> (last - i)) & 1) == 1 and "1" or "0"
  end
  return bp + length
end

local function posn(set, b)
  local i = set:find(string.char(b), 1, true)
  if not i then return -1 end
  return i - 1
end

local function ch(s, i)
  return s:sub(i + 1, i + 1)
end

-- backend/common.c `to_int()`. `pos` is 0-based.
local function to_int(s, pos, len)
  local field = s:sub(pos + 1, pos + len)
  if #field ~= len or field:find("%D") then return -1 end
  return tonumber(field)
end

local function is_digit(b)
  return b >= 48 and b <= 57
end

-- backend/general_field.c `gf_type()`. FNC1 counts as numeric.
local function gf_type(field, i)
  local b = field:byte(i + 1)
  if not b then return 0 end
  if b == GS or is_digit(b) then return GF_NUMERIC end
  if (b >= 65 and b <= 90) or ALNUM_SET[b] then return GF_ALPHANUMERIC end
  if ISOIEC_SET[b] then return GF_ISOIEC end
  return 0
end

local function gf_next(field, i, len, num, typ, type2)
  if i + num > len then return false end
  while i < len and num ~= 0 do
    local type_i = gf_type(field, i)
    if (type_i ~= typ and type2 == 0) or (type_i ~= typ and type_i ~= type2) then
      return false
    end
    i = i + 1
    num = num - 1
  end
  return num == 0
end

local function gf_next_terminate(field, i, len, num, max_num, typ)
  if i + max_num < len then return false end
  while i < len do
    if gf_type(field, i) ~= typ then return false end
    i = i + 1
    num = num - 1
  end
  return i == len and num <= 0
end

local function gf_next_none(field, i, len, num, typ)
  while i < len and num ~= 0 do
    if gf_type(field, i) == typ then return false end
    i = i + 1
    num = num - 1
  end
  return num == 0 or i == len
end

-- backend/general_field.c `zint_general_field_encode()`.
-- Returns mode, last_digit (a one-character string, or nil), bp. Nil mode
-- means the character is outside CSET 82 plus space.
local function general_field_encode(field, mode, binary, bp)
  local len = #field
  local last_digit = nil
  local i = 0
  while i < len do
    local typ = gf_type(field, i)
    if typ == 0 then return nil end
    local b = field:byte(i + 1)
    if mode == GF_NUMERIC then
      if i < len - 1 then
        if typ ~= GF_NUMERIC or gf_type(field, i + 1) ~= GF_NUMERIC then
          bp = bin_append(0, 4, binary, bp)
          mode = GF_ALPHANUMERIC
        else
          local b2 = field:byte(i + 2)
          local d1 = b == GS and 10 or (b - 48)
          local d2 = b2 == GS and 10 or (b2 - 48)
          bp = bin_append((11 * d1) + d2 + 8, 7, binary, bp)
          i = i + 2
        end
      else
        if typ ~= GF_NUMERIC then
          bp = bin_append(0, 4, binary, bp)
          mode = GF_ALPHANUMERIC
        else
          last_digit = field:sub(i + 1, i + 1)
          i = i + 1
        end
      end
    elseif mode == GF_ALPHANUMERIC then
      if b == GS then
        bp = bin_append(15, 5, binary, bp)
        mode = GF_NUMERIC
        i = i + 1
      elseif typ == GF_ISOIEC then
        bp = bin_append(4, 5, binary, bp)
        mode = GF_ISOIEC
      elseif gf_next(field, i, len, 6, GF_NUMERIC, 0) then
        bp = bin_append(0, 3, binary, bp)
        mode = GF_NUMERIC
      elseif gf_next_terminate(field, i, len, 4, 5, GF_NUMERIC) then
        bp = bin_append(0, 3, binary, bp)
        mode = GF_NUMERIC
      elseif is_digit(b) then
        bp = bin_append(b - 43, 5, binary, bp)
        i = i + 1
      elseif b >= 65 and b <= 90 then
        bp = bin_append(b - 33, 6, binary, bp)
        i = i + 1
      else
        bp = bin_append(posn(ALNUM_PUNCS, b) + 58, 6, binary, bp)
        i = i + 1
      end
    else
      if b == GS then
        bp = bin_append(15, 5, binary, bp)
        mode = GF_NUMERIC
        i = i + 1
      else
        local next_10 = gf_next_none(field, i, len, 10, GF_ISOIEC)
        if next_10 and gf_next(field, i, len, 4, GF_NUMERIC, 0) then
          bp = bin_append(0, 3, binary, bp)
          mode = GF_NUMERIC
        elseif next_10 and gf_next(field, i, len, 5, GF_ALPHANUMERIC, GF_NUMERIC) then
          bp = bin_append(4, 5, binary, bp)
          mode = GF_ALPHANUMERIC
        elseif is_digit(b) then
          bp = bin_append(b - 43, 5, binary, bp)
          i = i + 1
        elseif b >= 65 and b <= 90 then
          bp = bin_append(b - 1, 7, binary, bp)
          i = i + 1
        elseif b >= 97 and b <= 122 then
          bp = bin_append(b - 7, 7, binary, bp)
          i = i + 1
        else
          bp = bin_append(posn(ISOIEC_PUNCS, b) + 232, 8, binary, bp)
          i = i + 1
        end
      end
    end
  end
  return mode, last_digit, bp
end

-- backend/rss.c `dbar_combins()`.
local function dbar_combins(n, r)
  local min_denom, max_denom
  if n - r > r then
    min_denom, max_denom = r, n - r
  else
    min_denom, max_denom = n - r, r
  end
  local val, j = 1, 1
  for i = n, max_denom + 1, -1 do
    val = val * i
    if j <= min_denom then
      val = val // j
      j = j + 1
    end
  end
  for k = j, min_denom do
    val = val // k
  end
  return val
end

-- backend/rss.c `dbar_getWidths()`. `noNarrow` is the C's inverted flag:
-- true skips patterns that have no one-module element.
local function dbar_get_widths(val, n, elements, max_width, no_narrow)
  local widths = {}
  local narrow_mask = 0
  local bar = 0
  while bar < elements - 1 do
    local elm_width = 1
    narrow_mask = narrow_mask | (1 << bar)
    local sub_val
    while true do
      sub_val = dbar_combins(n - elm_width - 1, elements - bar - 2)
      if no_narrow and narrow_mask == 0
          and (n - elm_width - (elements - bar - 1) >= elements - bar - 1) then
        sub_val = sub_val - dbar_combins(n - elm_width - (elements - bar), elements - bar - 2)
      end
      if elements - bar - 1 > 1 then
        local less_val = 0
        local mxw = n - elm_width - (elements - bar - 2)
        while mxw > max_width do
          less_val = less_val + dbar_combins(n - elm_width - mxw - 1, elements - bar - 3)
          mxw = mxw - 1
        end
        sub_val = sub_val - less_val * (elements - 1 - bar)
      elseif n - elm_width > max_width then
        sub_val = sub_val - 1
      end
      val = val - sub_val
      if val < 0 then break end
      elm_width = elm_width + 1
      narrow_mask = narrow_mask & ~(1 << bar)
    end
    val = val + sub_val
    n = n - elm_width
    widths[bar] = elm_width
    bar = bar + 1
  end
  widths[bar] = n
  return widths
end

local function dbar_widths(v_odd, v_even, n_odd, n_even, elements, max_width, no_narrow)
  local odd_w = dbar_get_widths(v_odd, n_odd, elements, max_width, no_narrow)
  local even_w = dbar_get_widths(v_even, n_even, elements, 9 - max_width, not no_narrow)
  local ret = {}
  for i = 0, elements - 1 do
    ret[i * 2] = odd_w[i]
    ret[i * 2 + 1] = even_w[i]
  end
  return ret
end

local function dbar_exp_date(source, length, position)
  if position + 6 > length then return -1 end
  local yy = to_int(source, position, 2)
  local mm = to_int(source, position + 2, 2)
  local dd = to_int(source, position + 4, 2)
  if yy < 0 or mm <= 0 or mm > 12 or dd < 0 or dd > 31 then return -1 end
  return yy * 384 + (mm - 1) * 32 + dd
end

local function dbar_exp_group(val)
  local i = 0
  while i < 4 do
    if val < G_SUM[i + 1] then return i end
    i = i + 1
  end
  return i
end

-- Symbol-character count, including the stacked "not a single character on
-- the last row" bump. Returns symbol_characters, remainder, characters_per_row,
-- min_cols.
local function fit_symbols(bp, cols_per_row, max_rows, characters_per_row, min_cols)
  local remainder = 12 - (bp % 12)
  if remainder == 12 then remainder = 0 end
  local symbol_characters = ((bp + remainder) // 12) + 1
  if max_rows ~= 0 then
    min_cols = ((symbol_characters + 1) // 2 + max_rows - 1) // max_rows
    if min_cols > cols_per_row then
      characters_per_row = min_cols * 2
    end
  end
  if characters_per_row ~= 0 and (symbol_characters % characters_per_row) == 1 then
    symbol_characters = symbol_characters + 1
  end
  if symbol_characters < 4 then symbol_characters = 4 end
  remainder = (12 * (symbol_characters - 1)) - bp
  return symbol_characters, remainder, characters_per_row, min_cols
end

-- backend/rss.c `dbar_exp_binary_string()`.
-- Returns bp, cols_per_row, or nil plus a message.
local function dbar_exp_binary_string(source, cols_per_row, max_rows, binary, bp)
  local length = #source
  if length > 77 then
    return nil, string.format(
      "Error 378: Processed input length %d too long (maximum 77)", length)
  end

  local encoding_method
  if length >= 16 and ch(source, 0) == "0" and ch(source, 1) == "1" then
    encoding_method = 1
  else
    encoding_method = 2
  end

  if length >= 20 and encoding_method == 1 and ch(source, 2) == "9" and ch(source, 16) == "3" then
    if length >= 26 and ch(source, 17) == "1" and ch(source, 18) == "0" then
      local weight = to_int(source, 20, 6)
      if weight >= 0 and weight <= 99999 then
        if length == 26 then
          if ch(source, 19) == "3" and weight <= 32767 then
            encoding_method = 3
          else
            encoding_method = 7
          end
        elseif length == 34 and ch(source, 26) == "1"
            and (ch(source, 27) == "1" or ch(source, 27) == "3"
              or ch(source, 27) == "5" or ch(source, 27) == "7")
            and dbar_exp_date(source, length, 28) >= 0 then
          encoding_method = 6 + (ch(source, 27):byte() - 48)
        end
      end
    elseif length >= 26 and ch(source, 17) == "2" and ch(source, 18) == "0" then
      local weight = to_int(source, 20, 6)
      if weight >= 0 and weight <= 99999 then
        if length == 26 then
          if (ch(source, 19) == "2" and weight <= 9999)
              or (ch(source, 19) == "3" and weight <= 22767) then
            encoding_method = 4
          else
            encoding_method = 8
          end
        elseif length == 34 and ch(source, 26) == "1"
            and (ch(source, 27) == "1" or ch(source, 27) == "3"
              or ch(source, 27) == "5" or ch(source, 27) == "7")
            and dbar_exp_date(source, length, 28) >= 0 then
          encoding_method = 7 + (ch(source, 27):byte() - 48)
        end
      end
    elseif ch(source, 17) == "9" and ch(source, 19) >= "0" and ch(source, 19) <= "3" then
      if ch(source, 18) == "2" then
        encoding_method = 5
      elseif ch(source, 18) == "3" and to_int(source, 20, 3) >= 0 then
        encoding_method = 6
      end
    end
  end

  local read_posn
  if encoding_method == 1 then
    bp = bin_append(4, 3, binary, bp)
    read_posn = 16
  elseif encoding_method == 2 then
    bp = bin_append(0, 4, binary, bp)
    read_posn = 0
  elseif encoding_method == 3 or encoding_method == 4 then
    bp = bin_append(4 + (encoding_method - 3), 4, binary, bp)
    read_posn = 26
  elseif encoding_method == 5 then
    bp = bin_append(0x30, 7, binary, bp)
    read_posn = 20
  elseif encoding_method == 6 then
    bp = bin_append(0x34, 7, binary, bp)
    read_posn = 23
  else
    bp = bin_append(56 + (encoding_method - 7), 7, binary, bp)
    read_posn = length
  end

  for i = 0, read_posn - 1 do
    local b = source:byte(i + 1)
    if not b or (not is_digit(b) and b ~= GS) then
      return nil, "Error 385: Invalid character in Compressed Field data (digits only)"
    end
  end

  if encoding_method == 1 then
    bp = bin_append(source:byte(3) - 48, 4, binary, bp)
    for i = 3, 14, 3 do
      bp = bin_append(to_int(source, i, 3), 10, binary, bp)
    end
  elseif encoding_method == 3 or encoding_method == 4 then
    for i = 3, 14, 3 do
      bp = bin_append(to_int(source, i, 3), 10, binary, bp)
    end
    if encoding_method == 4 and ch(source, 19) == "3" then
      bp = bin_append(to_int(source, 20, 6) + 10000, 15, binary, bp)
    else
      bp = bin_append(to_int(source, 20, 6), 15, binary, bp)
    end
  elseif encoding_method == 5 or encoding_method == 6 then
    for i = 3, 14, 3 do
      bp = bin_append(to_int(source, i, 3), 10, binary, bp)
    end
    bp = bin_append(source:byte(20) - 48, 2, binary, bp)
    if encoding_method == 6 then
      bp = bin_append(to_int(source, 20, 3), 10, binary, bp)
    end
  elseif encoding_method >= 7 and encoding_method <= 14 then
    for i = 3, 14, 3 do
      bp = bin_append(to_int(source, i, 3), 10, binary, bp)
    end
    -- Leading zero of the six-digit weight is dropped: source[19] then source[21..25].
    local weight_str = ch(source, 19) .. source:sub(22, 26)
    bp = bin_append(to_int(weight_str, 0, 6), 20, binary, bp)
    local group_val
    if length == 34 then
      group_val = dbar_exp_date(source, length, 28)
    else
      group_val = 38400
    end
    bp = bin_append(group_val, 16, binary, bp)
  end

  local general = source:sub(read_posn + 1)
  local mode = GF_NUMERIC
  local last_digit = nil
  if #general ~= 0 then
    local new_mode, digit, new_bp = general_field_encode(general, mode, binary, bp)
    if not new_mode then
      return nil, "Error 386: Invalid character in General Field data"
    end
    mode, last_digit, bp = new_mode, digit, new_bp
  end

  local characters_per_row = cols_per_row * 2
  local min_cols = 0
  local symbol_characters, remainder
  symbol_characters, remainder, characters_per_row, min_cols =
    fit_symbols(bp, cols_per_row, max_rows, characters_per_row, min_cols)

  if last_digit then
    if remainder >= 4 and remainder <= 6 then
      bp = bin_append(last_digit:byte() - 48 + 1, 4, binary, bp)
    else
      bp = bin_append((last_digit:byte() - 48) * 11 + 10 + 8, 7, binary, bp)
    end
    symbol_characters, remainder, characters_per_row, min_cols =
      fit_symbols(bp, cols_per_row, max_rows, characters_per_row, min_cols)
  end

  if bp > 252 then
    return nil, string.format(
      "Error 387: Input too long, requires %d symbol characters (maximum 21)",
      (bp + 11) // 12)
  end

  if min_cols > cols_per_row then
    cols_per_row = min_cols
  end

  local pad = remainder
  if mode == GF_NUMERIC then
    bp = bin_append(0, 4, binary, bp)
    pad = pad - 4
  end
  while pad > 0 do
    bp = bin_append(4, 5, binary, bp)
    pad = pad - 5
  end

  if encoding_method == 1 or encoding_method == 2
      or encoding_method == 5 or encoding_method == 6 then
    local bit1 = (symbol_characters & 1) ~= 0 and "1" or "0"
    local bit2 = symbol_characters > 14 and "1" or "0"
    if encoding_method == 1 then
      binary[2], binary[3] = bit1, bit2
    elseif encoding_method == 2 then
      binary[3], binary[4] = bit1, bit2
    else
      binary[6], binary[7] = bit1, bit2
    end
  end

  return bp, cols_per_row
end

local function unset_module(symbol, y, x)
  local row = symbol.encoded[y]
  if row then row[x] = nil end
end

-- backend/rss.c `dbar_expand()`. `latch` true draws a bar.
local function dbar_expand(symbol, writer, latch, widths, start_i, end_i)
  for i = start_i, end_i - 1 do
    local width = widths[i] or 0
    for _ = 1, width do
      if latch then common.set_module(symbol, symbol.rows, writer) end
      writer = writer + 1
    end
    latch = not latch
  end
  return writer
end

-- backend/rss.c `dbar_exp_separator()`. `v2` is `{ latch = bool }` or nil.
-- The latch is written back only for the separator below a row (above_below -1).
local function dbar_exp_separator(symbol, width, cols, separator_row, above_below,
    special_case_row, left_to_right, odd_last_row, v2)
  local module_row = separator_row + above_below
  local v2_latch = v2 and v2.latch or false
  local space_latch = false

  local j = 4 + special_case_row
  width = width - 4
  while j < width do
    if common.module_is_set(symbol, module_row, j) == 1 then
      unset_module(symbol, separator_row, j)
    else
      common.set_module(symbol, separator_row, j)
    end
    j = j + 1
  end

  for col = 0, cols - 1 do
    local k = (49 * col) + 19 + special_case_row
    local function adjust(i)
      local x = i + k
      if common.module_is_set(symbol, module_row, x) == 1 then
        unset_module(symbol, separator_row, x)
        space_latch = false
      else
        if space_latch then
          unset_module(symbol, separator_row, x)
        else
          common.set_module(symbol, separator_row, x)
        end
        space_latch = not space_latch
      end
    end
    if left_to_right then
      local i_start = v2_latch and 2 or 0
      local i_end = v2_latch and 15 or 13
      for i = i_start, i_end - 1 do adjust(i) end
    else
      if odd_last_row then k = k - 17 end
      local i_start = v2_latch and 14 or 12
      local i_end = v2_latch and 2 or 0
      for i = i_start, i_end, -1 do adjust(i) end
    end
    v2_latch = not v2_latch
  end

  if v2 and above_below == -1 then v2.latch = v2_latch end
end

-- backend/common.c `z_hrt_conv_gs1_brackets_nochk()`.
local function hrt_brackets(source)
  local out = {}
  local level = 0
  for i = 1, #source do
    local c = source:sub(i, i)
    if c == "[" then
      out[i] = "("
      level = level + 1
    elseif c == "]" and level > 0 then
      out[i] = ")"
      level = level - 1
    else
      out[i] = c
    end
  end
  return table.concat(out)
end

local function apply_height(symbol, stack_rows)
  local min_h, default_h, no_errtxt
  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    min_h = 34.0
    default_h = 34.0 * stack_rows
    no_errtxt = symbol.warning ~= nil
  else
    min_h = 0.0
    default_h = 34.0 * stack_rows
    no_errtxt = true
  end
  local saved = symbol.warning
  common.set_height(symbol, min_h, default_h, 0.0)
  if no_errtxt then symbol.warning = saved end
end

-- Both ids. The symbol's symbology selects the linear or stacked layout.
function M.expanded(symbol, source)
  local reduced, err = gs1.verify(symbol, source)
  if not reduced then return nil, err end
  -- "[" is this port's FNC1. The C reduced string uses GS.
  reduced = reduced:gsub("%[", "\29")

  if symbol.symbology ~= BARCODE_DBAR_EXP then
    symbol.rows = 0
  end

  local binary = {}
  local bp = 0
  binary[bp] = "0" -- linkage flag. Composites, which would set it, are not encoded.
  bp = 1

  local cols_per_row = 0
  local max_rows = 0
  if symbol.symbology == BARCODE_DBAR_EXPSTK then
    cols_per_row = 2
    if symbol.option_2 >= 1 and symbol.option_2 <= 11 then
      cols_per_row = symbol.option_2
    elseif symbol.option_3 >= 2 and symbol.option_3 <= 11 then
      max_rows = symbol.option_3
    end
  end

  local new_bp, new_cols = dbar_exp_binary_string(reduced, cols_per_row, max_rows, binary, bp)
  if not new_bp then return nil, new_cols end
  bp, cols_per_row = new_bp, new_cols

  if symbol.symbology == BARCODE_DBAR_EXPSTK then
    symbol.option_2 = cols_per_row
    symbol.option_3 = max_rows
  end

  local data_chars = bp // 12
  local symbol_chars = data_chars + 1
  local char_widths = {}
  for i = 0, data_chars - 1 do
    local vs = 0
    local k = i * 12
    for j = 0, 11 do
      if binary[k + j] == "1" then vs = vs | (0x800 >> j) end
    end
    local group = dbar_exp_group(vs)
    local gsum = G_SUM[group]
    local odd = (vs - gsum) // T_EVEN[group]
    local even = (vs - gsum) % T_EVEN[group]
    char_widths[i] = dbar_widths(odd, even, MODULES[group], 17 - MODULES[group],
      4, WIDEST[group], true)
  end

  local checksum = 0
  local wrow = WEIGHT_ROWS[(data_chars - 2) // 2]
  for i = 0, data_chars - 1 do
    local weights = CHECKSUM_WEIGHT[wrow[i]]
    local cw = char_widths[i]
    for j = 0, 7 do
      checksum = checksum + cw[j] * weights[j]
    end
  end
  local check_char = 211 * (symbol_chars - 4) + (checksum % 211)
  local group = dbar_exp_group(check_char)
  local gsum = G_SUM[group]
  local odd = (check_char - gsum) // T_EVEN[group]
  local even = (check_char - gsum) % T_EVEN[group]
  local check_widths = dbar_widths(odd, even, MODULES[group], 17 - MODULES[group],
    4, WIDEST[group], true)

  local codeblocks = (symbol_chars + 1) // 2
  local pattern_width = codeblocks * 5 + symbol_chars * 8 + 4
  local elements = {}
  local p = (symbol_chars - 1) // 2 - 1
  for i = 0, codeblocks - 1 do
    local k = FINDER_SEQUENCE[p][i] - 1
    local pat = FINDER_PATTERN[k]
    for j = 0, 4 do
      elements[(21 * i) + j + 10] = pat[j]
    end
  end
  for i = 0, 7 do
    elements[i + 2] = check_widths[i]
  end
  for i = 1, data_chars - 1, 2 do
    local k = ((i - 1) // 2) * 21 + 23
    local cw = char_widths[i]
    for j = 0, 7 do elements[k + j] = cw[j] end
  end
  for i = 0, data_chars - 1, 2 do
    local k = (i // 2) * 21 + 15
    local cw = char_widths[i]
    for j = 0, 7 do elements[k + j] = cw[7 - j] end
  end

  local stack_rows = 1
  if symbol.symbology == BARCODE_DBAR_EXP then
    elements[0] = 1
    elements[1] = 1
    elements[pattern_width - 2] = 1
    elements[pattern_width - 1] = 1
    local writer = dbar_expand(symbol, 0, false, elements, 0, pattern_width)
    if symbol.width < writer then symbol.width = writer end
    symbol.rows = symbol.rows + 1
    if (symbol.input_mode & common.GS1PARENS_MODE) ~= 0 then
      symbol.text = source
    else
      symbol.text = hrt_brackets(source)
    end
  else
    stack_rows = codeblocks // cols_per_row
    if codeblocks % cols_per_row > 0 then stack_rows = stack_rows + 1 end
    local current_block = 0
    local v2 = { latch = false }
    for current_row = 1, stack_rows do
      local elements_in_sub = 2
      local sub = {}
      local special_case_row = 0
      local num_columns = cols_per_row
      if current_row * cols_per_row > codeblocks then
        num_columns = codeblocks - current_block
      end
      sub[0] = 1
      sub[1] = 1
      if current_row == stack_rows and num_columns ~= cols_per_row
          and (current_row % 2 == 0) and (cols_per_row % 2 == 0)
          and (num_columns % 2 == 1) then
        special_case_row = 1
        sub[0] = 2
      end
      local left_to_right = (cols_per_row % 2 == 1) or (current_row % 2 == 1)
        or (special_case_row == 1)

      local reader = 0
      repeat
        local i0 = 2 + (current_block * 21)
        for j = 0, 20 do
          if i0 + j < pattern_width then
            local value = elements[i0 + j] or 0
            if left_to_right then
              sub[j + (reader * 21) + 2] = value
            else
              sub[(20 - j) + (num_columns - 1 - reader) * 21 + 2] = value
            end
          end
          elements_in_sub = elements_in_sub + 1
        end
        reader = reader + 1
        current_block = current_block + 1
      until not (reader < cols_per_row and current_block < codeblocks)

      sub[elements_in_sub] = 1
      elements_in_sub = elements_in_sub + 1
      sub[elements_in_sub] = 1
      elements_in_sub = elements_in_sub + 1

      local latch = (current_row % 2 == 0) and (special_case_row == 0)
      local writer = dbar_expand(symbol, 0, latch, sub, 0, elements_in_sub)
      if symbol.width < writer then symbol.width = writer end

      if current_row ~= 1 then
        local odd_last_row = (current_row == stack_rows) and (data_chars % 2 == 0)
        local sj = 5
        while sj < (49 * cols_per_row) do
          common.set_module(symbol, symbol.rows - 2, sj)
          sj = sj + 2
        end
        symbol.row_height[symbol.rows - 2] = 1
        dbar_exp_separator(symbol, writer, reader, symbol.rows - 1, 1,
          special_case_row, left_to_right, odd_last_row, v2)
        symbol.row_height[symbol.rows - 1] = 1
      end
      if current_row ~= stack_rows then
        dbar_exp_separator(symbol, writer, reader, symbol.rows + 1, -1,
          0, left_to_right, false, v2)
        symbol.row_height[symbol.rows + 1] = 1
      end
      symbol.rows = symbol.rows + 4
    end
    symbol.rows = symbol.rows - 3
  end

  apply_height(symbol, stack_rows)
  return true
end

-- ---------------------------------------------------------------- omn

-- backend/rss.h, DataBar Omnidirectional tables.
local OMN_G_SUM = vec { 0, 161, 961, 2015, 2715, 0, 336, 1036, 1516 }
local OMN_T_EVEN_ODD = vec { 1, 10, 34, 70, 126, 4, 20, 48, 81 }
local OMN_MODULES = vec {
  12, 10, 8, 6, 4, -- outside odd
   5,  7, 9, 11,   -- inside odd
   4,  6, 8, 10, 12, -- outside even
  10,  8, 6,  4,   -- inside even
}
local OMN_WIDEST = vec { 8, 6, 4, 3, 1, 2, 4, 6, 8 }
local OMN_FINDER = rows {
  { 3, 8, 2, 1, 1 },
  { 3, 5, 5, 1, 1 },
  { 3, 3, 7, 1, 1 },
  { 3, 1, 9, 1, 1 },
  { 2, 7, 4, 1, 1 },
  { 2, 5, 6, 1, 1 },
  { 2, 3, 8, 1, 1 },
  { 1, 5, 7, 1, 1 },
  { 1, 3, 9, 1, 1 },
}
local OMN_CHECKSUM = rows {
  {  1,  3,  9, 27,  2,  6, 18, 54 },
  {  4, 12, 36, 29,  8, 24, 72, 58 },
  { 16, 48, 65, 37, 32, 17, 51, 74 },
  { 64, 34, 23, 69, 49, 68, 46, 59 },
}

-- backend/gs1.c `gs1_check_digit()`. Local copy; the other encoders keep theirs.
local function gs1_check_digit(source, length)
  local count = 0
  local factor = (length & 1) ~= 0 and 3 or 1
  for i = 1, length do
    count = count + factor * (source:byte(i) - 48)
    factor = factor ~ 2
  end
  return string.char(48 + (10 - (count % 10)) % 10)
end

-- backend/rss.c `dbar_gtin14()`. Thirteen digits, zero padded, plus the check.
local function gtin14(source)
  local buf = string.rep("0", 13 - #source) .. source
  return buf .. gs1_check_digit(buf, 13)
end

-- backend/common.c `z_not_sane(NEON_F)`. 1-based position, or 0.
local function not_sane_digits(source)
  for i = 1, #source do
    local b = source:byte(i)
    if b < 48 or b > 57 then return i end
  end
  return 0
end

-- backend/rss.c `dbar_omn_group()`. Outside groups are 0..4, inside are 5..8.
local function dbar_omn_group(val, outside)
  local i = outside and 0 or 5
  local last = 8 >> (outside and 1 or 0)
  while i < last do
    if val < OMN_G_SUM[i + 1] then return i end
    i = i + 1
  end
  return i
end

-- backend/rss.c `dbar_omn_finder_adjust()`.
local function dbar_omn_finder_adjust(symbol, separator_row, above_below, finder_start)
  local module_row = separator_row + above_below
  local latch = true
  for i = finder_start, finder_start + 12 do
    if common.module_is_set(symbol, module_row, i) == 0 then
      if latch then
        common.set_module(symbol, separator_row, i)
        latch = false
      else
        unset_module(symbol, separator_row, i)
        latch = true
      end
    else
      unset_module(symbol, separator_row, i)
      latch = true
    end
  end
end

-- backend/rss.c `dbar_omn_separator()`. A finder start of 0 means "no finder".
-- 0 is truthy in Lua, so that test is written `~= 0`. `bottom_finder_value_3`
-- is a real boolean for the same reason.
local function dbar_omn_separator(symbol, width, separator_row, above_below,
    finder_start, finder2_start, bottom_finder_value_3)
  local module_row = separator_row + above_below
  local limit = width - 4
  for i = 4, limit - 1 do
    if common.module_is_set(symbol, module_row, i) == 0 then
      common.set_module(symbol, separator_row, i)
    end
  end
  if bottom_finder_value_3 then
    local finder_value_3_set = finder_start + 10
    for i = finder_start, finder_start + 12 do
      if i == finder_value_3_set then
        common.set_module(symbol, separator_row, i)
      else
        unset_module(symbol, separator_row, i)
      end
    end
  else
    if finder_start ~= 0 then
      dbar_omn_finder_adjust(symbol, separator_row, above_below, finder_start)
    end
    if finder2_start ~= 0 then
      dbar_omn_finder_adjust(symbol, separator_row, above_below, finder2_start)
    end
  end
end

-- GS1 DataBar Omnidirectional (29) and Stacked Omnidirectional (80).
-- Digits, not an element string. An optional "(01)", "[01]", or "01" prefix
-- is accepted only at the lengths the C checks. --gs1 is refused by the
-- driver: these two are not gs1-compliant in library.c.
function M.omn(symbol, source)
  local n = #source
  if (n == 17 or n == 18) and (source:sub(1, 4) == "[01]" or source:sub(1, 4) == "(01)") then
    source = source:sub(5)
  elseif (n == 15 or n == 16) and source:sub(1, 2) == "01" then
    source = source:sub(3)
  end

  if #source > 14 then
    return nil, string.format(
      "Error 380: Input length %d too long (maximum 14)", #source)
  end
  local bad = not_sane_digits(source)
  if bad ~= 0 then
    return nil, string.format(
      "Error 381: Invalid character at position %d in input (digits only)", bad)
  end
  if #source == 14 then
    local expect = gs1_check_digit(source, 13)
    if source:sub(14, 14) ~= expect then
      return nil, string.format(
        "Error 388: Invalid check digit '%s', expecting '%s'",
        source:sub(14, 14), expect)
    end
    source = source:sub(1, 13)
  end

  if symbol.symbology ~= BARCODE_DBAR_OMN then
    symbol.rows = 0
  end

  local val = 0
  for i = 1, #source do
    val = val * 10 + source:byte(i) - 48
  end
  local left_pair = val // 4537077
  local right_pair = val % 4537077
  local data_character = {
    [0] = left_pair // 1597,
    [1] = left_pair % 1597,
    [2] = right_pair // 1597,
    [3] = right_pair % 1597,
  }

  local data_widths = {}
  for i = 0, 3 do
    local outside = (i & 1) == 0
    local group = dbar_omn_group(data_character[i], outside)
    local v = data_character[i] - OMN_G_SUM[group]
    local divisor = OMN_T_EVEN_ODD[group]
    local v_div = v // divisor
    local v_mod = v % divisor
    data_widths[i] = dbar_widths(
      outside and v_div or v_mod,
      (not outside) and v_div or v_mod,
      OMN_MODULES[group], OMN_MODULES[group + 9],
      4, OMN_WIDEST[group], not outside)
  end

  local checksum = 0
  for i = 0, 3 do
    local weights = OMN_CHECKSUM[i]
    local cw = data_widths[i]
    for j = 0, 7 do
      checksum = checksum + weights[j] * cw[j]
    end
  end
  checksum = checksum % 79
  if checksum >= 8 then checksum = checksum + 1 end
  if checksum >= 72 then checksum = checksum + 1 end
  local c_left = checksum // 9
  local c_right = checksum % 9

  local total = {}
  total[0], total[1], total[44], total[45] = 1, 1, 1, 1
  for i = 0, 7 do
    total[i + 2] = data_widths[0][i]
    total[i + 15] = data_widths[1][7 - i]
    total[i + 23] = data_widths[3][i]
    total[i + 36] = data_widths[2][7 - i]
  end
  local finder_l = OMN_FINDER[c_left]
  local finder_r = OMN_FINDER[c_right]
  for i = 0, 4 do
    total[i + 10] = finder_l[i]
    total[i + 31] = finder_r[4 - i]
  end

  local compliant = (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0
  if symbol.symbology == BARCODE_DBAR_OMN then
    local writer = dbar_expand(symbol, 0, false, total, 0, 46)
    if symbol.width < writer then symbol.width = writer end
    symbol.rows = symbol.rows + 1
    symbol.text = "(01)" .. gtin14(source)
    if compliant then
      common.set_height(symbol, 13.0, 33.0, 0.0)
    else
      common.set_height(symbol, 0.0, 50.0, 0.0)
    end
  else
    local writer = dbar_expand(symbol, 0, false, total, 0, 23)
    common.set_module(symbol, symbol.rows, writer)
    unset_module(symbol, symbol.rows, writer + 1)

    symbol.rows = symbol.rows + 4
    common.set_module(symbol, symbol.rows, 0)
    unset_module(symbol, symbol.rows, 1)
    dbar_expand(symbol, 2, true, total, 23, 46)

    for i = 5, 45, 2 do
      common.set_module(symbol, symbol.rows - 2, i)
    end
    symbol.row_height[symbol.rows - 2] = 1

    dbar_omn_separator(symbol, 50, symbol.rows - 3, -1, 18, 0, false)
    symbol.row_height[symbol.rows - 3] = 1

    dbar_omn_separator(symbol, 50, symbol.rows - 1, 1, 19, 0, c_right == 3)
    symbol.row_height[symbol.rows - 1] = 1
    if symbol.width < 50 then symbol.width = 50 end
    symbol.rows = symbol.rows + 1

    if compliant then
      common.set_height(symbol, 33.0, 66.0, 0.0)
    else
      common.set_height(symbol, 0.0, 66.0, 0.0)
    end
  end
  return true
end

return M
