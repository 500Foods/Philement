-- zint/output.lua -- ports the parts of backend/output.c the SVG path reaches:
-- float formatting, colour parsing, quiet zones and row heights.
--
-- output.c in the real thing is shared by every writer -- raster, vector and
-- the text formats -- so most of it is about scaling and rounding for a pixel
-- grid. None of that applies to SVG, which is why so much of the file is
-- absent here. What remains is the part every writer needs: how a number
-- becomes a string, and where the symbol sits inside its own bounding box.

local common = require "zint.common"

local M = {}

local f32, roundf, ctoi = common.stripf, common.roundf, common.ctoi
local format = string.format

-- ------------------------------------------------------------- number output

-- backend/output.c `out_putsf()` -- print with `dp` decimal places, then drop
-- trailing zeroes and a bare decimal point. So 113.34 stays, 100.00 becomes
-- "100", and 2.50 becomes "2.5".
--
-- This is the single most byte-sensitive function in the port: everything
-- upstream exists so that the float handed in here is bit-for-bit what zint
-- would have had. It is also the only place where a difference becomes visible,
-- which makes it a good breakpoint when a case does not match.
--
-- Ported as an explicit right-to-left scan rather than a pattern substitution.
-- A `gsub("%.?0*$", "")` looks equivalent and is not: it would also strip the
-- zeroes off an integer like "100" that never had a decimal point, because zint
-- always prints one at dp > 0 but a future caller with dp = 0 would not.
-- Following the C's own loop removes the question.
function M.putsf(prefix, dp, val)
  local buf = format("%." .. dp .. "f", val)

  -- `keep` is the length to truncate to. It only moves while every character
  -- seen so far has been a strippable zero -- `keep == i` is the test for
  -- "nothing non-zero has been seen to the right of here yet".
  --
  -- C indexes from 0 and tracks an exclusive end; here `keep` is a length,
  -- which is why the comparison is `keep == i` rather than the C's
  -- `end == i + 1`. Getting that translation wrong is silent on most values
  -- and only shows up on ones ending in a zero.
  local keep = #buf
  for i = #buf, 1, -1 do
    local c = buf:sub(i, i)
    if c == "0" then
      if keep == i then keep = i - 1 end
    elseif not c:match("%d") and c ~= "-" then
      -- Anything that is neither a digit nor a minus is the decimal point,
      -- whatever the locale calls it. Reaching it ends the scan either way:
      -- everything to the left is the integer part and is kept.
      if keep == i then keep = i - 1 end
      buf = buf:sub(1, keep)
      break
    end
  end

  return prefix .. buf
end

-- ------------------------------------------------------------------- colours

-- backend/output.c `out_colour_get_rgb()`. Returns red, green, blue, alpha as
-- four integers 0-255.
--
-- Two input syntaxes, told apart by whether there is a comma:
--   "RRGGBB" / "RRGGBBAA"   hex, alpha defaulting to opaque
--   "C,M,Y,K"               four percentages, always opaque
--
-- The task guarantees well-formed values, which matches zint's own
-- precondition -- the CLI validates the string before it ever reaches here, so
-- the C does no checking either and neither does this.
function M.colour_get_rgb(colour)
  if not colour:find(",", 1, true) then
    local function byte_at(i)
      return 16 * ctoi(colour:sub(i, i)) + ctoi(colour:sub(i + 1, i + 1))
    end
    local alpha = 0xFF
    if #colour > 6 then alpha = byte_at(7) end
    return byte_at(1), byte_at(3), byte_at(5), alpha
  end

  -- CMYK. Each channel is the ink *not* laid down, scaled by the black
  -- channel's complement: 0,0,0,100 is pure black, 0,0,0,0 is white.
  local c, m, y, k = colour:match("^([^,]*),([^,]*),([^,]*),([^,]*)$")
  local black = 100 - tonumber(k)

  -- The numerator is integer arithmetic in C -- 255 * 100 * 100 = 2,550,000 at
  -- most, well under the 2^24 a float represents exactly -- so it survives the
  -- trip through float without narrowing. Only the divide needs it.
  local function channel(pct)
    local val = 100 - tonumber(pct)
    return roundf(f32((0xFF * val * black) / 10000.0))
  end

  return channel(c), channel(m), channel(y), 0xFF
end

-- backend/output.c `out_check_colour()`. The CLI rejects a bad colour before
-- a file is written. `colour_get_rgb` above assumes the string has already
-- passed this check.
local function to_int(s)
  local val = 0
  for i = 1, #s do
    local b = s:byte(i)
    if b < 48 or b > 57 then return -1 end
    val = val * 10 + (b - 48)
  end
  return val
end

function M.check_colour(colour, name)
  if not colour:find(",", 1, true) then
    if #colour ~= 6 and #colour ~= 8 then
      return ("Error 880: Malformed %s RGB colour (6 or 8 characters only)"):format(name)
    end
    if colour:find("[^0-9A-Fa-f]") then
      return ("Error 881: Malformed %s RGB colour '%s' (hexadecimal only)"):format(name, colour)
    end
    return nil
  end

  local parts = {}
  local start = 1
  for i = 1, #colour do
    if colour:sub(i, i) == "," then
      parts[#parts + 1] = colour:sub(start, i - 1)
      start = i + 1
    end
  end
  parts[#parts + 1] = colour:sub(start)
  if #parts ~= 4 then
    return ("Error 882: Malformed %s CMYK colour (4 decimal numbers, comma-separated)"):format(name)
  end
  for i = 1, 4 do
    if #parts[i] > 3 then
      return ("Error 883: Malformed %s CMYK colour (3 digit maximum per number)"):format(name)
    end
  end
  local labels = { { 884, "C" }, { 885, "M" }, { 886, "Y" }, { 887, "K" } }
  for i = 1, 4 do
    local val = to_int(parts[i])
    if val < 0 or val > 100 then
      return ("Error %d: Malformed %s CMYK colour %s (decimal 0 to 100 only)"):format(
        labels[i][1], name, labels[i][2])
    end
  end
  return nil
end

-- --------------------------------------------------------------- quiet zones

-- backend/output.c `out_quiet_zones()`, reduced to the five symbologies in
-- scope.
--
-- The C has two switch statements and the split between them is the whole
-- story. The first holds the symbologies whose standards make the quiet zone
-- part of the symbol -- the UPC/EAN family and a handful of others -- and it
-- runs unconditionally. The second covers everything else and sits behind
-- `output_options & BARCODE_QUIET_ZONES`, i.e. --quietzones.
--
-- So Code 39, Code 128, GS1-128 and QR get *nothing* by default. That is
-- surprising if you have only read the standards -- all four of them do
-- specify a quiet zone -- and it is the difference between a 206-wide Code 39
-- and a 246-wide one.
--
-- BARCODE_NO_QUIET_ZONES (--noquietzones) suppresses both, but note what it
-- does *not* do for UPC-A: the `elseif not hide_text` branch puts the same 9
-- modules back, because the outside guard digits are drawn in that space and
-- have to go somewhere. --noquietzones only narrows a UPC-A when --notext is
-- given as well.
local function quiet_zones(symbol, hide_text)
  local left, right, top, bottom = 0.0, 0.0, 0.0, 0.0
  local opts = symbol.output_options
  local no_qz = (opts & common.BARCODE_NO_QUIET_ZONES) ~= 0

  -- First switch: always applies. comp_xoffset is zero throughout -- it is
  -- non-zero only for composite symbologies, which this port does not encode --
  -- so the `comp_xoffset >= 8 ? 1 : 9 - comp_xoffset` forms collapse to 9, and
  -- `9 - (comp_xoffset != 0)` to 9.
  if common.is_upcean(symbol.symbology) then
    -- GS1 General Specifications 21.0.1 Section 5.2.3.4. comp_xoffset is zero
    -- without a composite, so the C's `11 - comp_xoffset` forms are 11 and 7.
    if common.is_ean(symbol.symbology) then
      local n = #symbol.text
      if n == 13 then
        if not no_qz then
          left, right = 11.0, 7.0
        elseif not hide_text then
          left = 11.0
        end
      elseif n == 16 or n == 19 then
        if not no_qz then
          left, right = 11.0, 5.0
        elseif not hide_text then
          left = 11.0
        end
      elseif n == 2 or n == 5 then
        if not no_qz then right = 5.0 end
      elseif n == 8 then
        if not no_qz then left, right = 7.0, 7.0 end
      else
        if not no_qz then left, right = 7.0, 5.0 end
      end
    elseif symbol.symbology == 37 or symbol.symbology == 38 then
      -- UPC-E. Right zone is 7, or 5 once an add-on pushes the text past 8.
      -- The outside digits still need the left 9, and the right 7, when
      -- --noquietzones is combined with text and there is no add-on.
      if not no_qz then
        left = 9.0
        right = #symbol.text > 8 and 5.0 or 7.0
      elseif not hide_text then
        left = 9.0
        if #symbol.text <= 8 then right = 7.0 end
      end
    else
      -- UPC-A. The right zone narrows when an add-on is present. A plain
      -- UPC-A has 12 characters of human-readable text. --noquietzones puts
      -- the same 9 modules back while the outside digits are drawn, and only
      -- narrows the symbol when --notext is given as well.
      if not no_qz then
        left = 9.0
        right = #symbol.text > 12 and 5.0 or 9.0
      elseif not hide_text then
        left = 9.0
        if #symbol.text <= 12 then right = 9.0 end
      end
    end
    return left, right, top, bottom -- C: done = 1
  end

  -- ITF-14. GS1 General Specifications 21.0.1 5.3.2.2. The quiet zone is part
  -- of the symbol, same as UPC/EAN, so it is drawn unless --noquietzones.
  if symbol.symbology == 89 then
    if not no_qz then left, right = 10.0, 10.0 end
    return left, right, top, bottom
  end

  -- GS1 DataBar. General Specifications 21.0.1 Section 5.5.1.1: no quiet
  -- zone. This is the first switch, so --quietzones does not add one.
  -- 30 and 79 are named now so the rule is in one place; Limited and
  -- Stacked are not encoded. Omnidirectional is 29 and 80, Expanded is
  -- 31 and 81.
  if symbol.symbology == 29 or symbol.symbology == 30 or symbol.symbology == 31
      or symbol.symbology == 79 or symbol.symbology == 80 or symbol.symbology == 81 then
    return left, right, top, bottom
  end

  -- Second switch: only with --quietzones, and --noquietzones vetoes it.
  if (opts & common.BARCODE_QUIET_ZONES) == 0 or no_qz then
    return left, right, top, bottom
  end

  if symbol.symbology == 8 then       -- CODE39: ISO/IEC 16388:2007 4.4 (d)
    left, right = 10.0, 10.0
  elseif symbol.symbology == 3 then   -- C25INTER: ISO/IEC 16390:2007 4.4, 10X
    left, right = 10.0, 10.0
  elseif symbol.symbology == 16 then  -- GS1-128: GS1 Gen Specs 21.0.1 5.4.4.2
    left, right = 10.0, 10.0
  elseif symbol.symbology == 20 or symbol.symbology == 60 or symbol.symbology == 75 then
    -- CODE128, CODE128AB, and NVE-18: ISO/IEC 15417:2007 4.4.2
    left, right = 10.0, 10.0
  elseif symbol.symbology == 58 then  -- QRCODE: ISO/IEC 18004:2015 Section 9.1
    left, right, top, bottom = 4.0, 4.0, 4.0, 4.0
  elseif symbol.symbology == 71 then  -- DATAMATRIX: ISO/IEC 16022:2006 7.1, 1X
    left, right, top, bottom = 1.0, 1.0, 1.0, 1.0
  elseif symbol.symbology == 55 or symbol.symbology == 56 then
    -- PDF417 and Compact PDF417: ISO/IEC 15438:2015 5.8.3
    left, right, top, bottom = 2.0, 2.0, 2.0, 2.0
  elseif symbol.symbology == 57 then  -- MAXICODE: ISO/IEC 16023:2000 4.11.5
    left, right, top, bottom = 1.0, 1.0, 1.0, 1.0
  elseif symbol.symbology == 85 then
    -- USPS Intelligent Mail: USPS-B-3200 2.3.2, X max = 1/39
    left = common.stripf(4.875)
    right = left
    top = common.stripf(1.01400006)
    bottom = top
    -- Aztec (92) specifies no quiet zone even with --quietzones, so it is
    -- absent here and stays at zero.
  end

  return left, right, top, bottom
end

-- backend/output.c `out_set_whitespace_offsets()`. Works out how far the symbol
-- proper is inset from the edge of the image on each side.
--
-- Three things stack into each offset: the quiet zone, --whitesp/--vwhitesp,
-- and the border. Note how unevenly the border enters: it widens the left and
-- right only under --box (a bind draws over the full width, so it needs no
-- horizontal room), and it lifts the bottom only when the bottom bar is
-- actually drawn -- which --bindtop suppresses.
--
-- The fifth return, qz_right, is what the UPC/EAN text placement needs; nothing
-- else uses it.
function M.set_whitespace_offsets(symbol, hide_text)
  local qz_left, qz_right, qz_top, qz_bottom = quiet_zones(symbol, hide_text)
  local opts = symbol.output_options

  local xoffset = symbol.whitespace_width + qz_left
  local roffset = symbol.whitespace_width + qz_right
  if (opts & common.BARCODE_BOX) ~= 0 then
    xoffset = xoffset + symbol.border_width
    roffset = roffset + symbol.border_width
  end

  local yoffset = symbol.whitespace_height + qz_top
  local boffset = symbol.whitespace_height + qz_bottom
  local BIND_ANY = common.BARCODE_BOX | common.BARCODE_BIND | common.BARCODE_BIND_TOP
  if (opts & BIND_ANY) ~= 0 then
    yoffset = yoffset + symbol.border_width
    if (opts & common.BARCODE_BIND_TOP) == 0 then
      boffset = boffset + symbol.border_width
    end
  end

  return xoffset, yoffset, roffset, boffset, qz_right
end

-- ---------------------------------------------------------------- UPC/EAN

-- backend/output.c `out_process_upcean()`. Pulls three things out of a UPC/EAN
-- symbol that the layout needs and cannot get any other way: which member of
-- the family it is, how wide the main symbol is without its add-on, and the
-- add-on's own text.
--
-- Returns upceanflag, main_width, addon, addon_len, addon_gap.
--
-- The "flag" is the digit count of the family member -- 12 for UPC-A, 13 for
-- EAN-13, 6 for UPC-E and so on -- and vector.c branches on it numerically
-- (`>= 6` means "a real symbol rather than a standalone add-on"). 2, 5, 6,
-- 8, 12, and 13 are reachable here. 2 and 5 are the standalone add-ons.
--
-- Note the add-on is scraped back out of the human-readable text rather than
-- passed down from the encoder, and the scan starts at offset 6 because no
-- member of the family has a body shorter than that. With --notext the digits
-- are replaced by spaces rather than skipped, which keeps the add-on's width
-- the same; nothing reads them in that case, but the C does it and so does this.
function M.process_upcean(symbol)
  local text = symbol.text
  local addon = {}
  local latch = false

  local i = 7 -- C's `i = 6`, 0-based
  while i <= #text and #addon < 5 do
    if latch then
      addon[#addon + 1] = symbol.show_hrt ~= 0 and text:sub(i, i) or " "
    elseif text:sub(i, i) == "+" then
      latch = true
    end
    i = i + 1
  end
  addon = table.concat(addon)

  local addon_len, addon_gap = 0, 0
  if latch then
    addon_len = #addon
    -- UPC-A's minimum add-on gap is 9 modules against 7 for the rest of the
    -- family. option_2 is --addongap, which this port does not accept, so the
    -- conditional always takes the default.
    local o2 = symbol.option_2
    if symbol.symbology == 34 or symbol.symbology == 35 then
      addon_gap = (o2 >= 9 and o2 <= 12) and o2 or 9
    else
      addon_gap = (o2 >= 7 and o2 <= 12) and o2 or 7
    end
  end

  -- Main width is the body, not symbol.width: the add-on and its gap sit past
  -- it. comp_xoffset is zero without a composite. EAN-13 and UPC-A are both
  -- 95 modules; the flag is what tells the text layout which slots to use.
  local flag, main_width
  if common.is_ean(symbol.symbology) then
    local n = #text
    if n == 13 or n == 16 or n == 19 then
      flag, main_width = 13, 95
    elseif n == 2 then
      flag, main_width = 2, symbol.width
    elseif n == 5 then
      flag, main_width = 5, symbol.width
    else
      flag, main_width = 8, 68
    end
  elseif symbol.symbology == 37 or symbol.symbology == 38 then
    flag, main_width = 6, 51
  else
    flag, main_width = 12, 95
  end
  return flag, main_width, addon, addon_len, addon_gap
end

-- ---------------------------------------------------------------- row height

-- backend/output.c `out_large_bar_height()`. Every row the encoder left at zero
-- is a "large bar" and they share out whatever height is left over.
--
-- The C takes a scale-and-round argument, `si`, used by the raster writers to
-- snap row heights onto whole pixels. The vector path passes zero, so that
-- whole branch is gone from here.
--
-- Note the write-back to symbol.height. It looks redundant -- the rows already
-- sum to it -- but it is not, quite: the division rounds, so N * (h / N) can
-- come back a hair off h, and every later use of symbol.height has to be the
-- value that matches the rows actually drawn.
function M.large_bar_height(symbol)
  local fixed_height = 0.0
  local zero_count = 0

  for i = 0, symbol.rows - 1 do
    local rh = symbol.row_height[i] or 0.0
    if rh ~= 0 then
      fixed_height = f32(fixed_height + rh)
    else
      zero_count = zero_count + 1
    end
  end

  local large_bar_height = 0.0 -- never read when zero_count is zero
  if zero_count > 0 then
    large_bar_height = f32((symbol.height - fixed_height) / zero_count)
    symbol.height = f32(f32(large_bar_height * zero_count) + fixed_height)
  end

  return large_bar_height
end

return M
