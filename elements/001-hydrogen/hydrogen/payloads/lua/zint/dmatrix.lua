-- zint/dmatrix.lua -- Data Matrix ECC 200.
--
-- backend/dmatrix.c `zint_datamatrix()`, the ECC 200 path only. ECC 000-140
-- is refused. One segment. Structured Append, Mailmark, and HIBC Data Matrix
-- are not encoded.
--
-- The default encodation is the minimal edge search. `--fast` selects the
-- Annex J look-ahead (`dm_isoenc`). Codeword arrays, the process buffer, and
-- the edge table are 0-based, matching the C.
--
-- GS1 reduction has already happened in the driver. This port's reduced
-- string marks FNC1 with "[", and the C encoder reads GS (0x1D). The marker
-- is translated here, and only while GS1 mode is on, because "[" is a real
-- data character otherwise.
--
-- Reed-Solomon uses GF(0x12d) with the first root at a^1. `reedsol.encode`
-- leaves the remainder reversed; the C 2.16.0 encoder reverses before it
-- returns, and QR undoes the same reversal on the way out. Data Matrix
-- undoes it here, then interleaves. 144x144 uses the de-facto skewed
-- interleave unless `--dmiso144` is set.
--
-- No human-readable text. Every row is one module tall.

local common = require "zint.common"
local reedsol = require "zint.reedsol"

local M = {}

local DM_ASCII   = 1
local DM_C40     = 2
local DM_TEXT    = 3
local DM_X12     = 4
local DM_EDIFACT = 5
local DM_BASE256 = 6
local DM_NUM_MODES = 6

local DM_SQUARE  = 100
local DM_DMRE    = 101
local DM_ISO_144 = 128
local DMSIZESCOUNT = 48
local DMINTSYMBOL144 = 47

local GS = 0x1D

local c40_shift = {
  [0] = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
}

local c40_value = {
  [0] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20,
  21, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 22, 23, 24, 25, 26,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
}

local text_shift = {
  [0] = 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2,
  2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
  3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3
}

local text_value = {
  [0] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19, 20,
  21, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 22, 23, 24, 25, 26,
  0, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 27, 28, 29, 30, 31
}

local matrixbytes = {
  [0] = 3, 5, 5, 8, 10, 12, 16, 18, 18, 22, 22, 24, 30, 32, 32, 36,
  38, 43, 44, 44, 49, 49, 56, 62, 62, 63, 64, 70, 72, 80, 84, 86,
  90, 108, 114, 118, 144, 174, 204, 280, 368, 456, 576, 696, 816, 1050, 1304, 1558
}

local intsymbol = {
  [0] = 0, 1, 3, 5, 7, 9, 12, 15, 18, 23, 31, 34, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 2, 4, 6, 10, 13, 20, 8, 11,
  14, 16, 21, 25, 17, 26, 24, 19, 22, 30, 28, 29, 33, 27, 32, 35
}

local is_dmre = {
  [0] = 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
  1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0,
  1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}

local matrix_h = {
  [0] = 10, 12, 8, 14, 8, 16, 12, 18, 8, 20, 12, 8, 22, 16, 8, 24,
  8, 12, 26, 20, 16, 8, 20, 32, 16, 8, 12, 26, 22, 24, 20, 36,
  26, 24, 40, 26, 44, 48, 52, 64, 72, 80, 88, 96, 104, 120, 132, 144
}

local matrix_w = {
  [0] = 10, 12, 18, 14, 32, 16, 26, 18, 48, 20, 36, 64, 22, 36, 80, 24,
  96, 64, 26, 36, 48, 120, 44, 32, 64, 144, 88, 40, 48, 48, 64, 36,
  48, 64, 40, 64, 44, 48, 52, 64, 72, 80, 88, 96, 104, 120, 132, 144
}

local matrix_fh = {
  [0] = 10, 12, 8, 14, 8, 16, 12, 18, 8, 20, 12, 8, 22, 16, 8, 24,
  8, 12, 26, 20, 16, 8, 20, 16, 16, 8, 12, 26, 22, 24, 20, 18,
  26, 24, 20, 26, 22, 24, 26, 16, 18, 20, 22, 24, 26, 20, 22, 24
}

local matrix_fw = {
  [0] = 10, 12, 18, 14, 16, 16, 26, 18, 24, 20, 18, 16, 22, 18, 20, 24,
  24, 16, 26, 18, 24, 20, 22, 16, 16, 24, 22, 20, 24, 24, 16, 18,
  24, 16, 20, 16, 22, 24, 26, 16, 18, 20, 22, 24, 26, 20, 22, 24
}

local matrix_datablock = {
  [0] = 3, 5, 5, 8, 10, 12, 16, 18, 18, 22, 22, 24, 30, 32, 32, 36,
  38, 43, 44, 44, 49, 49, 56, 62, 62, 63, 64, 70, 72, 80, 84, 86,
  90, 108, 114, 118, 144, 174, 102, 140, 92, 114, 144, 174, 136, 175, 163, 156
}

local matrix_rsblock = {
  [0] = 5, 7, 7, 10, 11, 12, 14, 14, 15, 18, 18, 18, 20, 24, 22, 24,
  28, 27, 28, 28, 28, 32, 34, 36, 36, 36, 36, 38, 38, 41, 42, 42,
  42, 46, 48, 50, 56, 68, 42, 56, 36, 48, 56, 68, 56, 68, 62, 62
}

local function isdigit(b)
  return b >= 48 and b <= 57
end

local function isupper(b)
  return b >= 65 and b <= 90
end

local function is_c40(b)
  if b <= 57 then return b >= 48 or b == 32 end
  return isupper(b)
end

local function is_text(b)
  if b <= 57 then return b >= 48 or b == 32 end
  return b >= 97 and b <= 122
end

local function is_c40text(mode, b)
  if mode == DM_C40 then return is_c40(b) end
  return is_text(b)
end

local function is_x12(b)
  return is_c40(b) or b == 13 or b == 42 or b == 62
end

local function is_edifact(b)
  return b >= 32 and b <= 94
end

local function is_twodigits(src, length, sp)
  return sp + 1 < length and isdigit(src[sp]) and isdigit(src[sp + 1])
end

local function x12_value(b)
  if isdigit(b) then return (b - 48) + 4 end
  if isupper(b) then return (b - 65) + 14 end
  -- CR, asterisk, greater-than, space. The index is the X12 value.
  -- Lua `\ddd` is decimal, so this cannot be the C octal `\015`.
  local set = "\r*> "
  for i = 1, #set do
    if set:byte(i) == b then return i - 1 end
  end
  return -1
end

local function err(n, msg)
  return nil, "Error " .. n .. ": " .. msg
end

local function to_bytes(s)
  local t = {}
  for i = 1, #s do t[i - 1] = s:byte(i) end
  return t
end

-- Annex F placement. `places` is 0-based, row-major, length NR * NC.

local function placementbit(array, NR, NC, r, c, p, b)
  if r < 0 then
    r = r + NR
    c = c + 4 - ((NR + 4) % 8)
  end
  if c < 0 then
    c = c + NC
    r = r + 4 - ((NC + 4) % 8)
  end
  if r >= NR then r = r - NR end
  array[r * NC + c] = (p << 3) + b
end

local function placementblock(array, NR, NC, r, c, p)
  placementbit(array, NR, NC, r - 2, c - 2, p, 7)
  placementbit(array, NR, NC, r - 2, c - 1, p, 6)
  placementbit(array, NR, NC, r - 1, c - 2, p, 5)
  placementbit(array, NR, NC, r - 1, c - 1, p, 4)
  placementbit(array, NR, NC, r - 1, c - 0, p, 3)
  placementbit(array, NR, NC, r - 0, c - 2, p, 2)
  placementbit(array, NR, NC, r - 0, c - 1, p, 1)
  placementbit(array, NR, NC, r - 0, c - 0, p, 0)
end

local function placementcornerA(array, NR, NC, p)
  placementbit(array, NR, NC, NR - 1, 0, p, 7)
  placementbit(array, NR, NC, NR - 1, 1, p, 6)
  placementbit(array, NR, NC, NR - 1, 2, p, 5)
  placementbit(array, NR, NC, 0, NC - 2, p, 4)
  placementbit(array, NR, NC, 0, NC - 1, p, 3)
  placementbit(array, NR, NC, 1, NC - 1, p, 2)
  placementbit(array, NR, NC, 2, NC - 1, p, 1)
  placementbit(array, NR, NC, 3, NC - 1, p, 0)
end

local function placementcornerB(array, NR, NC, p)
  placementbit(array, NR, NC, NR - 3, 0, p, 7)
  placementbit(array, NR, NC, NR - 2, 0, p, 6)
  placementbit(array, NR, NC, NR - 1, 0, p, 5)
  placementbit(array, NR, NC, 0, NC - 4, p, 4)
  placementbit(array, NR, NC, 0, NC - 3, p, 3)
  placementbit(array, NR, NC, 0, NC - 2, p, 2)
  placementbit(array, NR, NC, 0, NC - 1, p, 1)
  placementbit(array, NR, NC, 1, NC - 1, p, 0)
end

local function placementcornerC(array, NR, NC, p)
  placementbit(array, NR, NC, NR - 3, 0, p, 7)
  placementbit(array, NR, NC, NR - 2, 0, p, 6)
  placementbit(array, NR, NC, NR - 1, 0, p, 5)
  placementbit(array, NR, NC, 0, NC - 2, p, 4)
  placementbit(array, NR, NC, 0, NC - 1, p, 3)
  placementbit(array, NR, NC, 1, NC - 1, p, 2)
  placementbit(array, NR, NC, 2, NC - 1, p, 1)
  placementbit(array, NR, NC, 3, NC - 1, p, 0)
end

local function placementcornerD(array, NR, NC, p)
  placementbit(array, NR, NC, NR - 1, 0, p, 7)
  placementbit(array, NR, NC, NR - 1, NC - 1, p, 6)
  placementbit(array, NR, NC, 0, NC - 3, p, 5)
  placementbit(array, NR, NC, 0, NC - 2, p, 4)
  placementbit(array, NR, NC, 0, NC - 1, p, 3)
  placementbit(array, NR, NC, 1, NC - 3, p, 2)
  placementbit(array, NR, NC, 1, NC - 2, p, 1)
  placementbit(array, NR, NC, 1, NC - 1, p, 0)
end

local function placement(array, NR, NC)
  local p, r, c = 1, 4, 0
  repeat
    if r == NR and c == 0 then
      placementcornerA(array, NR, NC, p)
      p = p + 1
    end
    if r == NR - 2 and c == 0 and (NC % 4) ~= 0 then
      placementcornerB(array, NR, NC, p)
      p = p + 1
    end
    if r == NR - 2 and c == 0 and (NC % 8) == 4 then
      placementcornerC(array, NR, NC, p)
      p = p + 1
    end
    if r == NR + 4 and c == 2 and (NC % 8) == 0 then
      placementcornerD(array, NR, NC, p)
      p = p + 1
    end
    repeat
      if r < NR and c >= 0 and (array[r * NC + c] or 0) == 0 then
        placementblock(array, NR, NC, r, c, p)
        p = p + 1
      end
      r = r - 2
      c = c + 2
    until not (r >= 0 and c < NC)
    r = r + 1
    c = c + 3
    repeat
      if r >= 0 and c < NC and (array[r * NC + c] or 0) == 0 then
        placementblock(array, NR, NC, r, c, p)
        p = p + 1
      end
      r = r + 2
      c = c - 2
    until not (r < NR and c >= 0)
    r = r + 3
    c = c + 1
  until not (r < NR or c < NC)
  if (array[NR * NC - 1] or 0) == 0 then
    array[NR * NC - 1] = 1
    array[NR * NC - NC - 2] = 1
  end
end

-- ECC. `binary` is 0-based. `reedsol.encode` leaves the remainder reversed,
-- so each block is written from the last remainder byte back to the first.

local function dm_ecc(binary, bytes, datablock, rsblock, skew)
  local blocks = (bytes + 2) // datablock
  local rsblocks = rsblock * blocks
  local rs = reedsol.init_gf(0x12d)
  reedsol.init_code(rs, rsblock, 1)
  for b = 0, blocks - 1 do
    local buf, ecc = {}, {}
    local p, n = 0, b
    while n < bytes do
      buf[p] = binary[n]
      p = p + 1
      n = n + blocks
    end
    reedsol.encode(rs, p, buf, ecc)
    n, p = b, 0
    while n < rsblocks do
      local cw = ecc[rsblock - 1 - p]
      if skew then
        if b < 8 then
          binary[bytes + n + 2] = cw
        else
          binary[bytes + n - 8] = cw
        end
      else
        binary[bytes + n] = cw
      end
      n = n + blocks
      p = p + 1
    end
  end
end

local function get_symbolsize(symbol, minimum)
  local opt2 = symbol.option_2 or 0
  if opt2 >= 1 and opt2 <= DMSIZESCOUNT then
    return intsymbol[opt2 - 1]
  end
  if minimum > 1304 then
    if minimum <= 1558 then return DMSIZESCOUNT - 1 end
    return 0
  end
  local i = minimum >= 62 and 23 or 0
  while minimum > matrixbytes[i] do i = i + 1 end
  local low = (symbol.option_3 or 0) & 0x7F
  if low == DM_DMRE then return i end
  if low == DM_SQUARE then
    while matrix_h[i] ~= matrix_w[i] do i = i + 1 end
    return i
  end
  while is_dmre[i] ~= 0 do i = i + 1 end
  return i
end

local function codewords_remaining(symbol, tp, process_p)
  local symbolsize = get_symbolsize(symbol, tp + process_p)
  return matrixbytes[symbolsize] - tp
end

local function c40text_cnt(mode, gs1, input)
  if gs1 ~= 0 and input == GS then return 2 end
  local cnt = 1
  if (input & 0x80) ~= 0 then
    cnt = cnt + 2
    input = input - 128
  end
  if mode == DM_C40 and c40_shift[input] ~= 0 then cnt = cnt + 1 end
  if mode == DM_TEXT and text_shift[input] ~= 0 then cnt = cnt + 1 end
  return cnt
end

local function update_b256_field_length(target, tp, b256_start)
  local count = tp - (b256_start + 1)
  if count <= 249 then
    target[b256_start] = count
  else
    for j = count - 1, 0, -1 do
      target[b256_start + 2 + j] = target[b256_start + 1 + j]
    end
    target[b256_start] = 249 + (count // 250)
    target[b256_start + 1] = count % 250
    tp = tp + 1
  end
  return tp
end

local function randomise_255(target, start_i, tp)
  for i = start_i, tp - 1 do
    local prn = (149 * (i + 1)) % 255 + 1
    target[i] = (target[i] + prn) & 0xFF
  end
end

local function switch_mode(next_mode, target, tp, b256_start)
  if next_mode == DM_C40 then
    target[tp] = 230
    tp = tp + 1
  elseif next_mode == DM_TEXT then
    target[tp] = 239
    tp = tp + 1
  elseif next_mode == DM_X12 then
    target[tp] = 238
    tp = tp + 1
  elseif next_mode == DM_EDIFACT then
    target[tp] = 240
    tp = tp + 1
  elseif next_mode == DM_BASE256 then
    target[tp] = 231
    tp = tp + 1
    b256_start = tp
    target[tp] = 0
    tp = tp + 1
  end
  return tp, b256_start
end

local function ctx_buffer_xfer(buf, process_p, target, tp)
  local process_e = (process_p // 3) * 3
  local i = 0
  while i < process_e do
    local iv = 1600 * buf[i] + 40 * buf[i + 1] + buf[i + 2] + 1
    target[tp] = (iv >> 8) & 0xFF
    target[tp + 1] = iv & 0xFF
    tp = tp + 2
    i = i + 3
  end
  process_p = process_p - process_e
  if process_p ~= 0 then
    for j = 0, process_p - 1 do
      buf[j] = buf[process_e + j]
    end
  end
  return process_p, tp
end

local function edi_buffer_xfer(buf, process_p, target, tp, empty)
  local process_e = (process_p // 4) * 4
  local i = 0
  while i < process_e do
    target[tp] = ((buf[i] << 2) | ((buf[i + 1] & 0x30) >> 4)) & 0xFF
    target[tp + 1] = (((buf[i + 1] & 0x0F) << 4) | ((buf[i + 2] & 0x3C) >> 2)) & 0xFF
    target[tp + 2] = (((buf[i + 2] & 0x03) << 6) | buf[i + 3]) & 0xFF
    tp = tp + 3
    i = i + 4
  end
  process_p = process_p - process_e
  if process_p ~= 0 then
    for j = 0, process_p - 1 do
      buf[j] = buf[i + j]
    end
    if empty then
      if process_p == 3 then
        target[tp] = ((buf[0] << 2) | ((buf[1] & 0x30) >> 4)) & 0xFF
        target[tp + 1] = (((buf[1] & 0x0F) << 4) | ((buf[2] & 0x3C) >> 2)) & 0xFF
        target[tp + 2] = ((buf[2] & 0x03) << 6) & 0xFF
        tp = tp + 3
      elseif process_p == 2 then
        target[tp] = ((buf[0] << 2) | ((buf[1] & 0x30) >> 4)) & 0xFF
        target[tp + 1] = ((buf[1] & 0x0F) << 4) & 0xFF
        tp = tp + 2
      else
        target[tp] = (buf[0] << 2) & 0xFF
        tp = tp + 1
      end
      process_p = 0
    end
  end
  return process_p, tp
end

local function push_c40text(buf, process_p, src_b, mode, gs1)
  local ct_shift = mode == DM_C40 and c40_shift or text_shift
  local ct_value = mode == DM_C40 and c40_value or text_value
  local shift, value
  if (src_b & 0x80) ~= 0 then
    buf[process_p] = 1
    buf[process_p + 1] = 30
    process_p = process_p + 2
    local low = src_b - 128
    shift = ct_shift[low]
    value = ct_value[low]
  elseif gs1 ~= 0 and src_b == GS then
    if gs1 == 2 then
      shift = ct_shift[29]
      value = ct_value[29]
    else
      shift = 2
      value = 27
    end
  else
    shift = ct_shift[src_b]
    value = ct_value[src_b]
  end
  if shift ~= 0 then
    buf[process_p] = shift - 1
    process_p = process_p + 1
  end
  buf[process_p] = value
  return process_p + 1
end

local function append_ascii_run(target, tp, src, length, sp, gs1)
  while sp < length do
    if is_twodigits(src, length, sp) then
      target[tp] = 10 * (src[sp] - 48) + (src[sp + 1] - 48) + 130
      tp = tp + 1
      sp = sp + 2
    elseif (src[sp] & 0x80) ~= 0 then
      target[tp] = 235
      target[tp + 1] = (src[sp] - 128) + 1
      tp = tp + 2
      sp = sp + 1
    elseif gs1 ~= 0 and src[sp] == GS then
      target[tp] = gs1 == 2 and 30 or 232
      tp = tp + 1
      sp = sp + 1
    else
      target[tp] = src[sp] + 1
      tp = tp + 1
      sp = sp + 1
    end
  end
  return tp
end

-- Annex J look-ahead. Counts are scaled by 12 so the comparisons stay integral.
-- DM_MULT_3 is 26 in the C, not 36. That adjustment is load-bearing.

local DM_MULT_1_DIV_2  = 6
local DM_MULT_2_DIV_3  = 8
local DM_MULT_3_DIV_4  = 9
local DM_MULT_1        = 12
local DM_MULT_4_DIV_3  = 16
local DM_MULT_2        = 24
local DM_MULT_8_DIV_3  = 32
local DM_MULT_3        = 26
local DM_MULT_13_DIV_4 = 39
local DM_MULT_10_DIV_3 = 40
local DM_MULT_4        = 48
local DM_MULT_17_DIV_4 = 51
local DM_MULT_13_DIV_3 = 52

local function mult_ceil(n)
  return ((n + 11) // 12) * 12
end

local function text_sp_cnt(src, position, length, sp)
  local cnt, i = 0, sp
  while i < length and is_text(src[i]) do
    i = i + 1
    cnt = cnt + 1
  end
  i = sp - 1
  while i >= position and is_text(src[i]) do
    i = i - 1
    cnt = cnt + 1
  end
  return cnt
end

local function substep_r_6_2_1(src, length, sp)
  local i = sp
  while i < length and is_x12(src[i]) do
    local c = src[i]
    if c == 13 or c == 42 or c == 62 then return true end
    i = i + 1
  end
  return false
end

local function look_ahead(src, length, position, current_mode, mode_arg, gs1)
  local ascii_count, c40_count, text_count, x12_count, edf_count, b256_count
  if current_mode == DM_ASCII or current_mode == DM_BASE256 then
    ascii_count, c40_count, text_count = 0, DM_MULT_1, DM_MULT_1
    x12_count, edf_count, b256_count = DM_MULT_1, DM_MULT_1, DM_MULT_2
  else
    ascii_count, c40_count, text_count = DM_MULT_1, DM_MULT_2, DM_MULT_2
    x12_count, edf_count, b256_count = DM_MULT_2, DM_MULT_2, DM_MULT_3
  end
  if current_mode == DM_C40 then c40_count = 0
  elseif current_mode == DM_TEXT then text_count = 0
  elseif current_mode == DM_X12 then x12_count = 0
  elseif current_mode == DM_EDIFACT then edf_count = 0
  elseif current_mode == DM_BASE256 then
    b256_count = mode_arg == 249 and DM_MULT_1 or 0
  end

  for sp = position, length - 1 do
    local c = src[sp]
    local extended = (c & 0x80) ~= 0
    if isdigit(c) then
      ascii_count = ascii_count + DM_MULT_1_DIV_2
    elseif extended then
      ascii_count = mult_ceil(ascii_count) + DM_MULT_2
    else
      ascii_count = mult_ceil(ascii_count) + DM_MULT_1
    end
    if is_c40(c) then
      c40_count = c40_count + DM_MULT_2_DIV_3
    elseif extended then
      c40_count = c40_count + DM_MULT_8_DIV_3
    else
      c40_count = c40_count + DM_MULT_4_DIV_3
    end
    if is_text(c) then
      text_count = text_count + DM_MULT_2_DIV_3
    elseif extended then
      text_count = text_count + DM_MULT_8_DIV_3
    else
      text_count = text_count + DM_MULT_4_DIV_3
    end
    if is_x12(c) then
      x12_count = x12_count + DM_MULT_2_DIV_3
    elseif extended then
      x12_count = x12_count + DM_MULT_13_DIV_3
    else
      x12_count = x12_count + DM_MULT_10_DIV_3
    end
    if is_edifact(c) then
      edf_count = edf_count + DM_MULT_3_DIV_4
    elseif extended then
      edf_count = edf_count + DM_MULT_17_DIV_4
    else
      edf_count = edf_count + DM_MULT_13_DIV_4
    end
    if gs1 == 1 and c == GS then
      b256_count = b256_count + DM_MULT_4
    else
      b256_count = b256_count + DM_MULT_1
    end

    if sp >= position + 3 then
      local cnt = ascii_count + DM_MULT_1
      if cnt < b256_count and cnt <= edf_count and cnt <= text_count
          and cnt <= x12_count and cnt <= c40_count then
        return DM_ASCII
      end
      cnt = b256_count + DM_MULT_1
      if cnt <= ascii_count or (cnt < edf_count and cnt < text_count
          and cnt < x12_count and cnt < c40_count) then
        return DM_BASE256
      end
      cnt = edf_count + DM_MULT_1
      if cnt < ascii_count and cnt < b256_count and cnt < text_count
          and cnt < x12_count and cnt < c40_count then
        return DM_EDIFACT
      end
      cnt = text_count + DM_MULT_1
      if cnt < ascii_count and cnt < b256_count and cnt < edf_count
          and cnt < x12_count and cnt < c40_count then
        if current_mode == DM_BASE256 and position + 6 < length then
          if text_sp_cnt(src, position, length, sp) >= 12 then return DM_TEXT end
        else
          return DM_TEXT
        end
      end
      cnt = x12_count + DM_MULT_1
      if cnt < ascii_count and cnt < b256_count and cnt < edf_count
          and cnt < text_count and cnt < c40_count then
        return DM_X12
      end
      cnt = c40_count + DM_MULT_1
      if cnt < ascii_count and cnt < b256_count and cnt < edf_count
          and cnt < text_count then
        if c40_count < x12_count then return DM_C40 end
        if c40_count == x12_count then
          if substep_r_6_2_1(src, length, sp) then return DM_X12 end
          return DM_C40
        end
      end
    end
  end

  local ascii_rnded = mult_ceil(ascii_count)
  local b256_rnded = mult_ceil(b256_count)
  local edf_rnded = mult_ceil(edf_count)
  local text_rnded = mult_ceil(text_count)
  local x12_rnded = mult_ceil(x12_count)
  local c40_rnded = mult_ceil(c40_count)
  if ascii_rnded <= b256_rnded and ascii_rnded <= edf_rnded
      and ascii_rnded <= text_rnded and ascii_rnded <= x12_rnded
      and ascii_rnded <= c40_rnded then
    return DM_ASCII
  end
  if b256_rnded < ascii_rnded and b256_rnded < edf_rnded
      and b256_rnded < text_rnded and b256_rnded < x12_rnded
      and b256_rnded < c40_rnded then
    return DM_BASE256
  end
  if edf_rnded < ascii_rnded and edf_rnded < b256_rnded
      and edf_rnded < text_rnded and edf_rnded <= x12_rnded
      and edf_rnded < c40_rnded then
    return DM_EDIFACT
  end
  if text_rnded < ascii_rnded and text_rnded < b256_rnded
      and text_rnded < edf_rnded and text_rnded < x12_rnded
      and text_rnded < c40_rnded then
    return DM_TEXT
  end
  if x12_rnded < ascii_rnded and x12_rnded < b256_rnded
      and x12_rnded <= edf_rnded and x12_rnded < text_rnded
      and x12_rnded < c40_rnded then
    return DM_X12
  end
  return DM_C40
end

-- Minimal encodation. Edges are indexed by vertex * 6 + endMode - 1.
-- A previous index of 0 means "no previous edge". Vertex 0 is never the
-- end of an edge, so 0 stays a free sentinel without a 16-bit cap.

local function last_ascii(src, length, from)
  local remain = length - from
  if remain > 4 or from >= length then return 0 end
  if remain == 1 then
    if (src[from] & 0x80) ~= 0 then return 0 end
    return 1
  end
  if remain == 2 then
    if (src[from] & 0x80) ~= 0 or (src[from + 1] & 0x80) ~= 0 then return 0 end
    if isdigit(src[from]) and isdigit(src[from + 1]) then return 1 end
    return 2
  end
  if remain == 3 then
    if isdigit(src[from]) and isdigit(src[from + 1]) and (src[from + 2] & 0x80) == 0 then
      return 2
    end
    if isdigit(src[from + 1]) and isdigit(src[from + 2]) and (src[from] & 0x80) == 0 then
      return 2
    end
    return 0
  end
  if isdigit(src[from]) and isdigit(src[from + 1]) and isdigit(src[from + 2])
      and isdigit(src[from + 3]) then
    return 2
  end
  return 0
end

local function get_end_mode(symbol, src, length, last_seg, mode, from, len, size)
  if mode == DM_EDIFACT then
    if len < 4 then return DM_ASCII end
    if last_seg then
      local la = last_ascii(src, length, from + len)
      if la ~= 0 then
        local symbols_left = codewords_remaining(symbol, size + la, 0)
        if symbols_left <= 2 - la then return DM_ASCII end
      end
    end
  end
  return mode
end

local function number_of_c40_words(src, length, from, mode)
  local thirds = 0
  for i = from, length - 1 do
    local ci = src[i]
    if is_c40text(mode, ci) then
      thirds = thirds + 1
    elseif (ci & 0x80) == 0 then
      thirds = thirds + 2
    elseif is_c40text(mode, ci & 0x7F) then
      thirds = thirds + 3
    else
      thirds = thirds + 4
    end
    local remainder = thirds % 3
    if remainder == 0 or (remainder == 2 and i + 1 == length) then
      return ((thirds + 2) // 3) * 2, i - from + 1
    end
  end
  return 0, 0
end

local function new_edge(symbol, src, length, last_seg, edges, mode, from, len, prev_index, cwds)
  local edge = {
    mode = mode, endMode = mode, from = from, len = len,
    bytes = 0, previous = 0, size = 0,
  }
  local previous_mode, size
  if prev_index ~= 0 then
    local previous = edges[prev_index]
    previous_mode = previous.endMode
    edge.previous = prev_index
    size = previous.size
  else
    previous_mode = DM_ASCII
    size = 0
  end

  if mode == DM_ASCII then
    size = size + 1
    if (src[from] & 0x80) ~= 0 then size = size + 1 end
    if previous_mode ~= DM_ASCII and previous_mode ~= DM_BASE256 then
      size = size + 1
    end
  elseif mode == DM_BASE256 then
    size = size + 1
    if previous_mode ~= DM_BASE256 then
      size = size + 2
      if previous_mode ~= DM_ASCII then size = size + 1 end
      edge.bytes = 1
    else
      edge.bytes = 1 + edges[prev_index].bytes
      if edge.bytes == 250 then size = size + 1 end
    end
  elseif mode == DM_C40 or mode == DM_TEXT then
    size = size + cwds
    if previous_mode ~= mode then
      size = size + 1
      if previous_mode ~= DM_ASCII and previous_mode ~= DM_BASE256 then
        size = size + 1
      end
    end
    if last_seg and from + len + 2 >= length then
      local la = last_ascii(src, length, from + len)
      local symbols_left = codewords_remaining(symbol, size + la, 0)
      if symbols_left > 0 then size = size + 1 end
    end
  elseif mode == DM_X12 then
    size = size + 2
    if previous_mode ~= DM_X12 then
      size = size + 1
      if previous_mode ~= DM_ASCII and previous_mode ~= DM_BASE256 then
        size = size + 1
      end
    end
    if last_seg and from + len + 2 >= length then
      local la = last_ascii(src, length, from + len)
      if la == 2 then
        size = size + 1
      else
        local symbols_left = codewords_remaining(symbol, size + la, 0)
        if symbols_left > 0 then size = size + 1 end
      end
    end
  elseif mode == DM_EDIFACT then
    size = size + 3
    if previous_mode ~= DM_EDIFACT then
      size = size + 1
      if previous_mode ~= DM_ASCII and previous_mode ~= DM_BASE256 then
        size = size + 1
      end
    end
    edge.endMode = get_end_mode(symbol, src, length, last_seg, mode, from, len, size)
  end
  edge.size = size
  return edge
end

local function add_edge(symbol, src, length, last_seg, edges, mode, from, len, prev_index, cwds)
  local edge = new_edge(symbol, src, length, last_seg, edges, mode, from, len, prev_index, cwds)
  local v_ij = (from + len) * DM_NUM_MODES + edge.endMode - 1
  local existing = edges[v_ij]
  if not existing or existing.mode == 0 or existing.size > edge.size then
    edges[v_ij] = edge
  end
end

local function add_edges(symbol, src, length, last_seg, edges, from, prev_index, gs1)
  local blocked = prev_index ~= 0 and edges[prev_index].endMode == DM_EDIFACT
  if not blocked then
    if isdigit(src[from]) and from + 1 < length and isdigit(src[from + 1]) then
      add_edge(symbol, src, length, last_seg, edges, DM_ASCII, from, 2, prev_index, 0)
      if prev_index ~= 0 and edges[prev_index].mode == DM_ASCII then return end
    else
      add_edge(symbol, src, length, last_seg, edges, DM_ASCII, from, 1, prev_index, 0)
    end
    for _, mode in ipairs({ DM_C40, DM_TEXT }) do
      local cwds, len = number_of_c40_words(src, length, from, mode)
      if cwds ~= 0 then
        add_edge(symbol, src, length, last_seg, edges, mode, from, len, prev_index, cwds)
      end
    end
    if from + 2 < length and is_x12(src[from]) and is_x12(src[from + 1])
        and is_x12(src[from + 2]) then
      add_edge(symbol, src, length, last_seg, edges, DM_X12, from, 3, prev_index, 0)
    end
    if not (gs1 == 1 and src[from] == GS) then
      add_edge(symbol, src, length, last_seg, edges, DM_BASE256, from, 1, prev_index, 0)
    end
  end
  if is_edifact(src[from]) then
    local i, pos = 1, from + 1
    while i < 4 and pos < length and is_edifact(src[pos]) do
      add_edge(symbol, src, length, last_seg, edges, DM_EDIFACT, from, i + 1, prev_index, 0)
      i = i + 1
      pos = pos + 1
    end
  end
end

local function define_modes(symbol, src, length, last_seg, gs1)
  local edges = {}
  add_edges(symbol, src, length, last_seg, edges, 0, 0, gs1)
  for i = 1, length - 1 do
    local v_i = i * DM_NUM_MODES
    for j = 0, DM_NUM_MODES - 1 do
      local e = edges[v_i + j]
      if e and e.mode ~= 0 then
        add_edges(symbol, src, length, last_seg, edges, i, v_i + j, gs1)
      end
    end
  end

  local v_i = length * DM_NUM_MODES
  local minimal_j, minimal_size = -1, math.maxinteger
  for j = 0, DM_NUM_MODES - 1 do
    local e = edges[v_i + j]
    if e and e.mode ~= 0 and e.size < minimal_size then
      minimal_size = e.size
      minimal_j = j
    end
  end
  if minimal_j < 0 then
    return nil, "Error 728: Insufficient memory for mode buffers"
  end

  local modes = {}
  local edge = edges[v_i + minimal_j]
  local mode_len, mode_end = 0, length
  while edge do
    local current = edge.mode
    mode_len = mode_len + edge.len
    if edge.previous ~= 0 then
      edge = edges[edge.previous]
    else
      edge = nil
    end
    if not edge or edge.mode ~= current then
      for i = mode_end - mode_len, mode_end - 1 do
        modes[i] = current
      end
      mode_end = mode_end - mode_len
      mode_len = 0
    end
  end
  return modes
end

local function too_many(n)
  return err(n, "Input too long, requires too many codewords (maximum 1558)")
end

local function minimalenc(symbol, src, length, last_seg, st, target, gs1)
  local modes, merr = define_modes(symbol, src, length, last_seg, gs1)
  if not modes then return merr end
  local sp, tp = st.sp, st.tp
  local process_p, b256_start = st.process_p, st.b256_start
  local current_mode = st.current_mode
  local buf = st.buf

  while sp < length do
    if modes[sp] ~= current_mode then
      if current_mode == DM_C40 or current_mode == DM_TEXT or current_mode == DM_X12 then
        process_p = 0
        target[tp] = 254
        tp = tp + 1
      elseif current_mode == DM_EDIFACT then
        if last_seg then
          local la = last_ascii(src, length, sp)
          if la == 0 then
            buf[process_p] = 31
            process_p = process_p + 1
          else
            local symbols_left = codewords_remaining(symbol, tp + la, process_p)
            if symbols_left > 2 - la then
              buf[process_p] = 31
              process_p = process_p + 1
            end
          end
        end
        process_p, tp = edi_buffer_xfer(buf, process_p, target, tp, true)
      elseif current_mode == DM_BASE256 then
        tp = update_b256_field_length(target, tp, b256_start)
        randomise_255(target, b256_start, tp)
      end
      tp, b256_start = switch_mode(modes[sp], target, tp, b256_start)
    end

    current_mode = modes[sp]
    if current_mode == DM_ASCII then
      if is_twodigits(src, length, sp) then
        target[tp] = 10 * (src[sp] - 48) + (src[sp + 1] - 48) + 130
        tp = tp + 1
        sp = sp + 2
      elseif (src[sp] & 0x80) ~= 0 then
        target[tp] = 235
        target[tp + 1] = (src[sp] - 128) + 1
        tp = tp + 2
        sp = sp + 1
      elseif gs1 ~= 0 and src[sp] == GS then
        target[tp] = gs1 == 2 and 30 or 232
        tp = tp + 1
        sp = sp + 1
      else
        target[tp] = src[sp] + 1
        tp = tp + 1
        sp = sp + 1
      end
    elseif current_mode == DM_C40 or current_mode == DM_TEXT then
      process_p = push_c40text(buf, process_p, src[sp], current_mode, gs1)
      if process_p >= 3 then
        process_p, tp = ctx_buffer_xfer(buf, process_p, target, tp)
      end
      sp = sp + 1
    elseif current_mode == DM_X12 then
      buf[process_p] = x12_value(src[sp])
      process_p = process_p + 1
      if process_p >= 3 then
        process_p, tp = ctx_buffer_xfer(buf, process_p, target, tp)
      end
      sp = sp + 1
    elseif current_mode == DM_EDIFACT then
      local value = src[sp]
      if value >= 64 then value = value - 64 end
      buf[process_p] = value
      process_p = process_p + 1
      sp = sp + 1
      if process_p >= 4 then
        process_p, tp = edi_buffer_xfer(buf, process_p, target, tp, false)
      end
    elseif current_mode == DM_BASE256 then
      target[tp] = src[sp]
      tp = tp + 1
      sp = sp + 1
    end
    if tp > 1558 then return too_many(729) end
  end

  st.sp, st.tp, st.process_p = sp, tp, process_p
  st.b256_start, st.current_mode = b256_start, current_mode
  return nil
end

local function isoenc(symbol, src, length, st, target, gs1)
  local sp, tp = st.sp, st.tp
  local process_p, b256_start = st.process_p, st.b256_start
  local current_mode = st.current_mode
  local buf = st.buf
  local next_mode = DM_ASCII
  local not_first = 0

  while sp < length do
    current_mode = next_mode
    if current_mode == DM_ASCII then
      next_mode = DM_ASCII
      if is_twodigits(src, length, sp) then
        target[tp] = 10 * (src[sp] - 48) + (src[sp + 1] - 48) + 130
        tp = tp + 1
        sp = sp + 2
      else
        next_mode = look_ahead(src, length, sp, current_mode, 0, gs1)
        if next_mode ~= DM_ASCII then
          tp, b256_start = switch_mode(next_mode, target, tp, b256_start)
          not_first = 0
        elseif (src[sp] & 0x80) ~= 0 then
          target[tp] = 235
          target[tp + 1] = (src[sp] - 128) + 1
          tp = tp + 2
          sp = sp + 1
        elseif gs1 ~= 0 and src[sp] == GS then
          target[tp] = gs1 == 2 and 30 or 232
          tp = tp + 1
          sp = sp + 1
        else
          target[tp] = src[sp] + 1
          tp = tp + 1
          sp = sp + 1
        end
      end
    elseif current_mode == DM_C40 or current_mode == DM_TEXT then
      next_mode = current_mode
      if process_p == 0 and not_first ~= 0 then
        next_mode = look_ahead(src, length, sp, current_mode, process_p, gs1)
      end
      if next_mode ~= current_mode then
        target[tp] = 254
        tp = tp + 1
        next_mode = DM_ASCII
      else
        process_p = push_c40text(buf, process_p, src[sp], current_mode, gs1)
        if process_p >= 3 then
          process_p, tp = ctx_buffer_xfer(buf, process_p, target, tp)
        end
        sp = sp + 1
        not_first = 1
      end
    elseif current_mode == DM_X12 then
      if not is_x12(src[sp]) then
        next_mode = DM_ASCII
      else
        next_mode = DM_X12
        if process_p == 0 and not_first ~= 0 then
          next_mode = look_ahead(src, length, sp, current_mode, process_p, gs1)
        end
      end
      if next_mode ~= DM_X12 then
        sp = sp - process_p
        process_p = 0
        target[tp] = 254
        tp = tp + 1
        next_mode = DM_ASCII
      else
        buf[process_p] = x12_value(src[sp])
        process_p = process_p + 1
        if process_p >= 3 then
          process_p, tp = ctx_buffer_xfer(buf, process_p, target, tp)
        end
        sp = sp + 1
        not_first = 1
      end
    elseif current_mode == DM_EDIFACT then
      if not is_edifact(src[sp]) then
        next_mode = DM_ASCII
      else
        next_mode = DM_EDIFACT
        if process_p == 3 then
          next_mode = look_ahead(src, length, sp, current_mode, process_p, gs1)
        end
      end
      if next_mode ~= DM_EDIFACT then
        buf[process_p] = 31
        process_p = process_p + 1
        process_p, tp = edi_buffer_xfer(buf, process_p, target, tp, true)
        next_mode = DM_ASCII
      else
        local value = src[sp]
        if value >= 64 then value = value - 64 end
        buf[process_p] = value
        process_p = process_p + 1
        sp = sp + 1
        not_first = 1
        if process_p >= 4 then
          process_p, tp = edi_buffer_xfer(buf, process_p, target, tp, false)
        end
      end
    elseif current_mode == DM_BASE256 then
      if gs1 == 1 and src[sp] == GS then
        next_mode = DM_ASCII
      else
        next_mode = DM_BASE256
        if not_first ~= 0 then
          next_mode = look_ahead(src, length, sp, current_mode, tp - (b256_start + 1), gs1)
        end
      end
      if next_mode ~= DM_BASE256 then
        tp = update_b256_field_length(target, tp, b256_start)
        randomise_255(target, b256_start, tp)
        tp, b256_start = switch_mode(next_mode, target, tp, b256_start)
        not_first = 0
      else
        if gs1 == 2 and src[sp] == GS then
          target[tp] = 29
        else
          target[tp] = src[sp]
        end
        tp = tp + 1
        sp = sp + 1
        not_first = 1
      end
    end
    if tp > 1558 then return too_many(520) end
  end

  st.sp, st.tp, st.process_p = sp, tp, process_p
  st.b256_start, st.current_mode = b256_start, current_mode
  return nil
end

local function append_eci(target, tp, eci)
  if eci <= 0 then return tp end
  target[tp] = 241
  tp = tp + 1
  if eci <= 126 then
    target[tp] = eci + 1
    return tp + 1
  elseif eci <= 16382 then
    target[tp] = ((eci - 127) // 254) + 128
    target[tp + 1] = ((eci - 127) % 254) + 1
    return tp + 2
  end
  target[tp] = ((eci - 16383) // 64516) + 192
  target[tp + 1] = (((eci - 16383) // 254) % 254) + 1
  target[tp + 2] = ((eci - 16383) % 254) + 1
  return tp + 3
end

local function eod_unlatch(symbol, src, length, last_seg, st, target, gs1)
  local tp, process_p = st.tp, st.process_p
  local current_mode = st.current_mode
  local buf = st.buf
  local symbols_left = last_seg and codewords_remaining(symbol, tp, process_p) or 3

  if current_mode == DM_C40 or current_mode == DM_TEXT then
    if process_p == 0 then
      if symbols_left > 0 then
        target[tp] = 254
        tp = tp + 1
      end
    elseif process_p == 2 and symbols_left == 2 then
      buf[process_p] = 0
      process_p = process_p + 1
      process_p, tp = ctx_buffer_xfer(buf, process_p, target, tp)
    elseif process_p == 1 and symbols_left <= 2
        and is_c40text(current_mode, src[length - 1]) then
      if symbols_left > 1 then
        target[tp] = 254
        tp = tp + 1
      end
      target[tp] = src[length - 1] + 1
      tp = tp + 1
    else
      local sp = st.sp
      local total = 0
      while sp > 0 and process_p % 3 ~= 0 do
        sp = sp - 1
        local cnt = c40text_cnt(current_mode, gs1, src[sp])
        total = total + cnt
        process_p = process_p - cnt
      end
      tp = tp - (total // 3) * 2
      target[tp] = 254
      tp = tp + 1
      tp = append_ascii_run(target, tp, src, length, sp, gs1)
    end
  elseif current_mode == DM_X12 then
    if symbols_left == 1 and process_p == 1 then
      target[tp] = src[length - 1] + 1
      tp = tp + 1
    else
      if symbols_left > 0 then
        target[tp] = 254
        tp = tp + 1
      end
      if process_p == 1 then
        target[tp] = src[length - 1] + 1
        tp = tp + 1
      elseif process_p == 2 then
        target[tp] = src[length - 2] + 1
        target[tp + 1] = src[length - 1] + 1
        tp = tp + 2
      end
    end
  elseif current_mode == DM_EDIFACT then
    if symbols_left <= 2 and process_p <= symbols_left then
      if process_p == 1 then
        target[tp] = src[length - 1] + 1
        tp = tp + 1
      elseif process_p == 2 then
        target[tp] = src[length - 2] + 1
        target[tp + 1] = src[length - 1] + 1
        tp = tp + 2
      end
    else
      if process_p <= 3 then
        buf[process_p] = 31
        process_p = process_p + 1
      end
      process_p, tp = edi_buffer_xfer(buf, process_p, target, tp, true)
    end
  elseif current_mode == DM_BASE256 then
    if symbols_left > 0 then
      tp = update_b256_field_length(target, tp, st.b256_start)
    end
    randomise_255(target, st.b256_start, tp)
  end
  st.tp = tp
  return nil
end

local function dm_encode(symbol, src, length, eci, last_seg, gs1, target, tp)
  local st = {
    sp = 0, tp = tp, process_p = 0, b256_start = 0,
    current_mode = DM_ASCII, buf = {},
  }
  st.tp = append_eci(target, st.tp, eci)
  local enc_err
  if (symbol.input_mode & common.FAST_MODE) ~= 0 then
    enc_err = isoenc(symbol, src, length, st, target, gs1)
  else
    enc_err = minimalenc(symbol, src, length, last_seg, st, target, gs1)
  end
  if enc_err then return nil, enc_err end
  local eod_err = eod_unlatch(symbol, src, length, last_seg, st, target, gs1)
  if eod_err then return nil, eod_err end
  return st.tp
end

local function macro_codeword(src, length)
  if length < 9 then return 0 end
  if src[0] == 0x5B and src[1] == 0x29 and src[2] == 0x3E
      and src[3] == 0x1E and src[4] == 0x30
      and (src[5] == 0x35 or src[5] == 0x36)
      and src[6] == GS
      and src[length - 1] == 0x04
      and src[length - 2] == 0x1E then
    return src[5] == 0x35 and 236 or 237
  end
  return 0
end

local function slice_bytes(src, from, length)
  local out = {}
  for i = 0, length - 1 do out[i] = src[from + i] end
  return out
end

local function add_tail(target, tp, tail_length)
  target[tp] = 129
  tp = tp + 1
  for _ = 1, tail_length - 1 do
    local prn = (149 * (tp + 1)) % 253 + 1
    local temp = 129 + prn
    target[tp] = temp <= 254 and temp or (temp - 254)
    tp = tp + 1
  end
end

local function ecc200(symbol, source)
  if #source > 3116 then
    return err(719, string.format("Input length %d too long (maximum 3116)", #source))
  end

  local gs1 = 0
  if (symbol.input_mode & 0x07) == common.GS1_MODE then
    gs1 = (symbol.output_options & common.GS1_GS_SEPARATOR) ~= 0 and 2 or 1
    source = source:gsub("%[", "\29")
  end

  local src = to_bytes(source)
  local length = #source
  local target = {}
  local tp = 0

  if gs1 ~= 0 then
    target[tp] = 232
    tp = tp + 1
  end
  if (symbol.output_options & common.READER_INIT) ~= 0 then
    if gs1 ~= 0 then
      return err(521, "Cannot use Reader Initialisation in GS1 mode")
    end
    target[tp] = 234
    tp = tp + 1
  end

  local macro = 0
  if tp == 0 then macro = macro_codeword(src, length) end
  if macro ~= 0 then
    target[tp] = macro
    tp = tp + 1
    src = slice_bytes(src, 7, length - 9)
    length = length - 9
  end

  local binlen, eerr = dm_encode(symbol, src, length, symbol.eci or 0, true, gs1, target, tp)
  if not binlen then return nil, eerr end

  local symbolsize = get_symbolsize(symbol, binlen)
  if binlen > matrixbytes[symbolsize] then
    local opt2 = symbol.option_2 or 0
    if opt2 >= 1 and opt2 <= DMSIZESCOUNT then
      return err(522, string.format(
        "Input too long for Version %d, requires %d codewords (maximum %d)",
        opt2, binlen, matrixbytes[symbolsize]))
    end
    return err(523, string.format(
      "Input too long, requires %d codewords (maximum 1558)", binlen))
  end

  local opt2 = symbol.option_2 or 0
  if opt2 < 1 or opt2 > DMSIZESCOUNT then
    local i = 0
    while i < DMSIZESCOUNT and symbolsize ~= intsymbol[i] do i = i + 1 end
    symbol.option_2 = i + 1
  end

  local H = matrix_h[symbolsize]
  local W = matrix_w[symbolsize]
  local FH = matrix_fh[symbolsize]
  local FW = matrix_fw[symbolsize]
  local bytes = matrixbytes[symbolsize]
  local datablock = matrix_datablock[symbolsize]
  local rsblock = matrix_rsblock[symbolsize]
  local taillength = bytes - binlen
  if taillength ~= 0 then add_tail(target, binlen, taillength) end

  local skew = symbolsize == DMINTSYMBOL144 and ((symbol.option_3 or 0) & DM_ISO_144) == 0
  dm_ecc(target, bytes, datablock, rsblock, skew)

  local NC = W - 2 * (W // FW)
  local NR = H - 2 * (H // FH)
  local places = {}
  placement(places, NR, NC)
  for y = 0, H - 1, FH do
    for x = 0, W - 1 do
      common.set_module(symbol, (H - y) - 1, x)
    end
    for x = 0, W - 1, 2 do
      common.set_module(symbol, y, x)
    end
  end
  for x = 0, W - 1, FW do
    for y = 0, H - 1 do
      common.set_module(symbol, (H - y) - 1, x)
    end
    for y = 0, H - 1, 2 do
      common.set_module(symbol, (H - y) - 1, x + FW - 1)
    end
  end
  for y = 0, NR - 1 do
    for x = 0, NC - 1 do
      local v = places[(NR - y - 1) * NC + x] or 0
      if v == 1 or (v > 7 and (target[(v >> 3) - 1] & (1 << (v & 7))) ~= 0) then
        common.set_module(symbol,
          H - (1 + y + 2 * (y // (FH - 2))) - 1,
          1 + x + 2 * (x // (FW - 2)))
      end
    end
  end
  for y = 0, H - 1 do symbol.row_height[y] = 1 end
  symbol.height = H
  symbol.rows = H
  symbol.width = W
  return true
end

function M.datamatrix(symbol, source)
  -- option_1 defaults to -1. Anything above 1 is an old ECC request.
  if (symbol.option_1 or 0) > 1 then
    return err(524, "Older Data Matrix standards are no longer supported")
  end
  return ecc200(symbol, source)
end

return M
