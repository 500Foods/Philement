-- zint/c25inter.lua -- ports backend/2of5inter.c and the ITF-14 half of
-- backend/2of5inter_based.c.
--
-- Interleaved 2 of 5 (id 3) and ITF-14 (id 89). The Deutsche Post Leitcode
-- and Identcode encoders live in the same C file and are not ported.
--
-- A digit is five elements, each narrow (1) or wide (3). A pair is drawn by
-- taking the bars from the first digit and the spaces from the second, so
-- the input has to be an even number of digits. An odd count grows a leading
-- zero. --vers 1 appends a GS1 check digit and shows it; --vers 2 appends it
-- and leaves it off the human-readable line. ITF-14 always carries its own
-- check digit and ignores --vers.

local common = require "zint.common"

local M = {}
local f32 = common.stripf

-- backend/2of5inter.c `C25InterTable`. Five element widths per digit.
local C25 = {
  ["0"] = "11331", ["1"] = "31113", ["2"] = "13113", ["3"] = "33111", ["4"] = "11313",
  ["5"] = "31311", ["6"] = "13311", ["7"] = "11133", ["8"] = "31131", ["9"] = "13131",
}

-- backend/gs1.c `gs1_check_digit()`. Same weights as the EAN/UPC check.
local function gs1_check_digit(source, length)
  local count = 0
  local factor = (length & 1) ~= 0 and 3 or 1
  for i = 1, length do
    count = count + factor * (source:byte(i) - 48)
    factor = factor ~ 2
  end
  return string.char(48 + (10 - (count % 10)) % 10)
end

local function digits_only(source, err_id)
  for i = 1, #source do
    local c = source:byte(i)
    if not c or c < 48 or c > 57 then
      return nil, string.format(
        "Error %d: Invalid character at position %d in input (digits only)",
        err_id, i)
    end
  end
  return true
end

-- backend/2of5inter.c `zint_c25_inter_common()`.
-- `checkdigit_option` is 1 (show) or 2 (hide). `dont_set_height` lets ITF-14
-- apply its own height after it has forced the bearer box.
local function inter_common(symbol, source, length, checkdigit_option, dont_set_height)
  if length > 125 then
    return nil, string.format("Error 309: Input length %d too long (maximum 125)", length)
  end
  local ok, err = digits_only(source, 310)
  if not ok then return nil, err end

  local have_check = checkdigit_option == 1 or checkdigit_option == 2
  local local_source = source
  local n = length
  -- Odd without a check, or even with one, needs a leading zero so the pairs
  -- come out even. The C writes this as `have_checkdigit == !(length & 1)`.
  if have_check == ((n & 1) == 0) then
    local_source = "0" .. local_source
    n = n + 1
  end
  if have_check then
    local_source = local_source .. gs1_check_digit(local_source, n)
    n = n + 1
  end

  local parts = { "1111" }
  for i = 1, n, 2 do
    local bars = C25[local_source:sub(i, i)]
    local spaces = C25[local_source:sub(i + 1, i + 1)]
    local merge = {}
    for j = 1, 5 do
      merge[#merge + 1] = bars:sub(j, j)
      merge[#merge + 1] = spaces:sub(j, j)
    end
    parts[#parts + 1] = table.concat(merge)
  end
  parts[#parts + 1] = "311"
  common.expand(symbol, table.concat(parts))

  if not dont_set_height then
    if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
      -- ISO/IEC 16390:2007 4.4. Width in modules is (length / 2) * 18 + 9.
      -- The floor is 5mm at the 0.33mm X from Annex D.3.1. Default stays 50
      -- until the minimum exceeds it.
      local min_height_min = f32(f32(5.0) / f32(0.33))
      local pairs = n // 2
      local min_height = f32(f32(f32(f32(18.0) * pairs) + f32(9.0)) * f32(0.15))
      if min_height < min_height_min then min_height = min_height_min end
      local default_height = min_height > f32(50.0) and min_height or f32(50.0)
      common.set_height(symbol, min_height, default_height, 0.0)
    else
      common.set_height(symbol, 0.0, 50.0, 0.0)
    end
  end

  -- Hide the check digit only. A leading zero that was added for parity stays.
  local hrt_n = symbol.option_2 == 2 and (n - 1) or n
  symbol.text = local_source:sub(1, hrt_n)
  return true
end

function M.c25inter(symbol, source)
  return inter_common(symbol, source, #source, symbol.option_2, false)
end

-- backend/2of5inter_based.c `zint_itf14()`.
function M.itf14(symbol, source)
  local length = #source
  if (length == 17 or length == 18)
      and (source:sub(1, 4) == "[01]" or source:sub(1, 4) == "(01)") then
    source = source:sub(5)
    length = length - 4
  elseif (length == 15 or length == 16) and source:sub(1, 2) == "01" then
    source = source:sub(3)
    length = length - 2
  end
  if length > 14 then
    return nil, string.format("Error 311: Input length %d too long (maximum 14)", length)
  end
  local ok, err = digits_only(source, 312)
  if not ok then return nil, err end

  local supplied
  if length == 14 then
    supplied = source:sub(14, 14)
    source = source:sub(1, 13)
    length = 13
  end
  local body = string.rep("0", 13 - length) .. source
  local check = gs1_check_digit(body, 13)
  if supplied and supplied ~= check then
    return nil, string.format(
      "Error 850: Invalid check digit '%s', expecting '%s'", supplied, check)
  end
  body = body .. check

  local encoded, eerr = inter_common(symbol, body, 14, 0, true)
  if not encoded then return nil, eerr end

  -- Bearer bars. A caller who already asked for a box or a bind keeps that
  -- choice, including a border width of zero. Otherwise the box is 5 modules,
  -- which is the GS1 figure (4.83 / 1.016), unless --border already set one.
  local bound = common.BARCODE_BOX | common.BARCODE_BIND | common.BARCODE_BIND_TOP
  if (symbol.output_options & bound) == 0 then
    symbol.output_options = symbol.output_options | common.BARCODE_BOX
    if symbol.border_width == 0 then
      symbol.border_width = 5
    end
  end

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    -- GS1 General Specifications 21.0.1 5.12.3.2, same X as GS1-128.
    -- 5.8 / 1.016 and 31.75 / 0.495. The box is extra, on top of this height.
    local min_height = f32(f32(5.8) / f32(1.016))
    local default_height = f32(f32(31.75) / f32(0.495))
    common.set_height(symbol, min_height, default_height, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  symbol.text = body
  return true
end

return M
