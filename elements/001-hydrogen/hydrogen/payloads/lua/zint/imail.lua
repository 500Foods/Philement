-- zint/imail.lua -- ports backend/imail.c `zint_usps_imail` and
-- backend/postal.c `zint_daft_set_height`.
--
-- USPS Intelligent Mail (OneCode), symbology 85. Input is a 20-digit
-- tracking code, an optional "-", then a ZIP of length 0, 5, 9 or 11.
-- The second tracking digit is 0..4. No human-readable line: the encoder
-- does not set symbol.text. 65 bars, one every other column, three rows
-- (ascender, tracker, descender).

local common = require "zint.common"
local large = require "zint.large"
local tabs = require "zint.imail_tabs"

local M = {}
local f32 = common.stripf

local AppxD_I = tabs.AppxD_I
local AppxD_II = tabs.AppxD_II
local AppxD_IV = tabs.AppxD_IV

-- C string up to the first NUL. The split loop writes NULs the same way
-- imail.c does, including into the middle of the tracker.
local function cstr(arr)
  local parts = {}
  local i = 0
  while arr[i] and arr[i] ~= 0 do
    parts[#parts + 1] = string.char(arr[i])
    i = i + 1
  end
  return table.concat(parts)
end

-- backend/common.c `z_not_sane(SODIUM_MNS_F)`. Digits and "-". 1-based
-- position of the first reject, or 0.
local function not_sane(source, length)
  for i = 1, length do
    local c = source:byte(i)
    if not c or not ((c >= 48 and c <= 57) or c == 45) then
      return i
    end
  end
  return 0
end

-- The function "USPS_MSB_Math_CRC11GenerateFrameCheckSequence"
-- is Copyright (C) 2006 United States Postal Service.
--
-- 13 bytes holding 102 bits, right justified (the top two bits of the
-- first byte are zero). Returns the 11-bit frame check sequence.
local function crc11(byte_array)
  local poly = 0x0F35
  local fcs = 0x07FF
  local data = byte_array[0] << 5
  for _ = 2, 7 do
    if ((fcs ~ data) & 0x400) ~= 0 then
      fcs = (fcs << 1) ~ poly
    else
      fcs = fcs << 1
    end
    fcs = fcs & 0x7FF
    data = data << 1
  end
  for i = 1, 12 do
    data = byte_array[i] << 3
    for _ = 0, 7 do
      if ((fcs ~ data) & 0x0400) ~= 0 then
        fcs = (fcs << 1) ~ poly
      else
        fcs = fcs << 1
      end
      fcs = fcs & 0x7FF
      data = data << 1
    end
  end
  return fcs
end

-- backend/postal.c `zint_daft_set_height`. Not common.set_height.
-- row_height[0] and [1] are already set. A non-zero symbol.height is
-- re-split so the tracker (row 1) keeps its share of the total; row 2
-- is a copy of row 0. Every arithmetic step is narrowed to float32.
local function daft_set_height(symbol, min_height, max_height)
  if symbol.height ~= 0 then
    local rh0 = symbol.row_height[0]
    local rh1 = symbol.row_height[1]
    local height = f32(symbol.height)
    local t_ratio = f32(rh1 / f32(f32(rh0 * 2) + rh1))
    symbol.row_height[1] = f32(height * t_ratio)
    if symbol.row_height[1] < 0.5 then
      symbol.row_height[1] = 0.5
      symbol.row_height[0] = f32(f32(0.25 / t_ratio) - 0.25)
    else
      symbol.row_height[0] = f32(f32(height - symbol.row_height[1]) / 2.0)
    end
    if symbol.row_height[0] < 0.5 then
      symbol.row_height[0] = 0.5
      symbol.row_height[1] = f32(t_ratio / f32(1.0 - t_ratio))
    end
  end
  symbol.row_height[2] = symbol.row_height[0]
  symbol.height = f32(f32(symbol.row_height[0] + symbol.row_height[1]) + symbol.row_height[2])

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    if (min_height ~= 0 and symbol.height < min_height)
        or (max_height ~= 0 and symbol.height > max_height) then
      symbol.warning = "Warning 499: Height not compliant with standards"
    end
  end
end

-- backend/imail.c `zint_usps_imail`.
function M.imail(symbol, source)
  local length = #source
  if length > 32 then
    return nil, string.format("Error 450: Input length %d too long (maximum 32)", length)
  end
  local bad = not_sane(source, length)
  if bad ~= 0 then
    return nil, string.format(
      "Error 451: Invalid character at position %d in input (digits and \"-\" only)", bad)
  end

  -- A '-' ends the tracker and starts the ZIP. Another '-' writes a NUL
  -- at tracker[zip_so_far] (which truncates the tracker) and restarts the ZIP.
  local tracker_b = {}
  local zip_b = {}
  local read = 0
  local j = 0
  for i = 1, length do
    local c = source:byte(i)
    if c == 45 then
      tracker_b[read] = 0
      j = 1
      read = 0
    elseif j == 0 then
      tracker_b[read] = c
      read = read + 1
    else
      zip_b[read] = c
      read = read + 1
    end
  end
  if j == 0 then
    tracker_b[read] = 0
  else
    zip_b[read] = 0
  end
  local tracker = cstr(tracker_b)
  local zip = cstr(zip_b)

  if #tracker ~= 20 then
    return nil, "Error 452: Invalid length for tracking code (20 characters required)"
  end
  if tracker:byte(2) > string.byte("4") then
    return nil, "Error 454: Barcode Identifier (second character) out of range (0 to 4)"
  end

  local zip_len = #zip
  if zip_len ~= 0 and zip_len ~= 5 and zip_len ~= 9 and zip_len ~= 11 then
    return nil, "Error 453: Invalid length for ZIP code (5, 9 or 11 characters required)"
  end

  -- Step 1. Routing code, then its length weight, then the tracking digits.
  local accum = large.load_str_u64(zip, zip_len)
  if zip_len > 9 then
    large.add_u64(accum, 1000100001)
  elseif zip_len > 5 then
    large.add_u64(accum, 100001)
  elseif zip_len > 0 then
    large.add_u64(accum, 1)
  end

  large.mul_u64(accum, 10)
  large.add_u64(accum, tracker:byte(1) - 48)
  large.mul_u64(accum, 5)
  large.add_u64(accum, tracker:byte(2) - 48)
  for i = 3, 20 do
    large.mul_u64(accum, 10)
    large.add_u64(accum, tracker:byte(i) - 48)
  end

  -- Step 2. CRC over the top 102 bits. Bits 102 and 103 are cleared so the
  -- leading byte's top two bits are zero, as the USPS routine requires.
  local byte_reg = large.copy(accum)
  large.unset_bit(byte_reg, 102)
  large.unset_bit(byte_reg, 103)
  local byte_array = large.uchar_array(byte_reg, 13, 8)
  local usps_crc = crc11(byte_array)

  -- Step 3. Base 636 for codeword J, then base 1365 for I..B. A is what's left.
  local codeword = {}
  codeword[9] = large.div_u64(accum, 636)
  for k = 8, 1, -1 do
    codeword[k] = large.div_u64(accum, 1365)
  end
  codeword[0] = accum.lo

  -- Step 4.
  codeword[9] = codeword[9] * 2
  if usps_crc >= 1024 then
    codeword[0] = codeword[0] + 659
  end

  -- Step 5. Characters, then invert the ones selected by the CRC bits.
  local characters = {}
  for i = 0, 9 do
    if codeword[i] < 1287 then
      characters[i] = AppxD_I[codeword[i]]
    else
      characters[i] = AppxD_II[codeword[i] - 1287]
    end
  end
  for i = 0, 9 do
    if (usps_crc & (1 << i)) ~= 0 then
      characters[i] = 0x1FFF - characters[i]
    end
  end

  -- Step 6. 130 bits, mapped onto the 65 bars. A cleared bit in the
  -- low half is an ascender (row 0); a cleared bit in the high half is
  -- a descender (row 2).
  local bar_map = {}
  for i = 0, 9 do
    local ch = characters[i]
    for b = 0, 12 do
      bar_map[AppxD_IV[13 * i + b] - 1] = (ch >> b) & 1
    end
  end

  read = 0
  for i = 0, 64 do
    local n = 0
    if bar_map[i] == 0 then n = n + 1 end
    if bar_map[i + 65] == 0 then n = n + 2 end
    -- '0' full, '1' ascender, '2' descender, '3' tracker only.
    if n == 0 or n == 1 then common.set_module(symbol, 0, read) end
    common.set_module(symbol, 1, read)
    if n == 0 or n == 2 then common.set_module(symbol, 2, read) end
    read = read + 2
  end

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    -- USPS-B-3200 2.3.1, bar pitch as X (1/43 inch).
    symbol.row_height[0] = f32(2.0855) -- 0.0485 * 43
    symbol.row_height[1] = f32(2.06399989) -- 0.048 * 43
    daft_set_height(symbol, f32(4.875), f32(7.75500011))
  else
    symbol.row_height[0] = 3
    symbol.row_height[1] = 2
    daft_set_height(symbol, 0, 0)
  end
  symbol.rows = 3
  symbol.width = read - 1

  return true
end

return M
