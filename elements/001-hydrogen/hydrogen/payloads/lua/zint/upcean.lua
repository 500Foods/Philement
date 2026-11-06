-- zint/upcean.lua -- ports backend/upcean.c for UPC-A, UPC-E, EAN-13, and EAN-8.
--
-- upcean.c handles the whole EAN/UPC family in one file: EAN-13, EAN-8, UPC-A,
-- UPC-E, ISBN, and the two- and five-digit add-ons that can hang off any of
-- them. Encoded here: UPC-A (34 and 35), UPC-E (37 and 38), EAN-13 (15),
-- EAN-8 (10), ISBN (69), and the standalone EAN-2 (11) and EAN-5 (12)
-- add-ons. EAN-13 and EAN-8 have no separate check-digit id. UPC-E does:
-- 37 computes the check, 38 requires it, and an 8-digit 37 still checks the
-- eighth. ISBN is drawn as EAN-13. A 10-digit ISBN or a 9-digit SBN is
-- turned into 978 plus the body, and the EAN-13 check digit replaces the
-- ISBN check digit.
--
-- Two things about UPC-A make it unlike the four symbologies already here.
--
-- The first is that the symbol has a fixed width. Code 39 and Code 128 grow
-- with the data; UPC-A is always 95 modules, always twelve digits, and the
-- encoder's job is mostly validation. That makes the encoder small and pushes
-- the interesting work into vector.c, which has an entire second layout path
-- for this family -- guard bars that hang below the others, and a text line
-- broken into four separately-positioned pieces rather than one centred string.
--
-- The second is the check digit. UPC-A is the first symbology here where the
-- data can be *wrong* in a way zint detects: pass twelve digits with a bad
-- twelfth and it refuses rather than encoding what you gave it.

local common = require "zint.common"

local M = {}

local f32, ctoi = common.stripf, common.ctoi

-- ------------------------------------------------------------------- tables

-- backend/upcean.c EANsetA -- "Representation set A and C (EN Table 1)".
--
-- Each entry is the four element widths of one digit, in modules, alternating
-- space/bar/space/bar or bar/space/bar/space depending on where the digit
-- lands. Every row sums to 7, which is what makes the symbol width fixed.
--
-- Stored as strings rather than tables of characters because that is what
-- common.expand consumes: the whole symbol is built by concatenating these into
-- one run-length string.
local EANsetA = {
  [0] = "3211", "2221", "2122", "1411", "1132",
        "1231", "1114", "1312", "1213", "3112",
}

-- backend/upcean.c EANsetB -- "Representation set B (EN Table 1)". The mirror
-- of set A, read right to left, which inverts bar and space. UPC-A proper never
-- uses it -- upca_draw is set A throughout -- but the add-ons do, and their
-- parity pattern is what carries the add-on's own check information.
local EANsetB = {
  [0] = "1123", "1222", "2212", "1141", "2311",
        "1321", "4111", "2131", "3121", "2113",
}

-- backend/upcean.c EAN2Parity -- "Number sets for 2-digit add-on (EN Table 6)".
local EAN2Parity = { [0] = "AA", "AB", "BA", "BB" }

-- backend/upcean.c EAN5Parity -- "Number set for 5-digit add-on (EN Table 7)".
local EAN5Parity = {
  [0] = "BBAAA", "BABAA", "BAABA", "BAAAB", "ABBAA",
        "AABBA", "AAABB", "ABABA", "ABAAB", "AABAB",
}

-- backend/upcean.c UPCParity0 -- "Number set for UPC-E Symbol (Table 4)".
-- Indexed by the check digit of the expanded UPC-A. Number system 0.
local UPCParity0 = {
  [0] = "BBBAAA", "BBABAA", "BBAABA", "BBAAAB", "BABBAA",
        "BAABBA", "BAAABB", "BABABA", "BABAAB", "BAABAB",
}

-- backend/upcean.c UPCParity1 -- number system 1, Table 4 inverted.
local UPCParity1 = {
  [0] = "AAABBB", "AABABB", "AABBAB", "AABBBA", "ABAABB",
        "ABBAAB", "ABBBAA", "ABABAB", "ABABBA", "ABBABA",
}

-- backend/upcean.c EAN13Parity -- "Left half of EAN-13 symbol (Table 3)".
-- The first digit is not drawn. It selects which of the next five left-half
-- digits come from set B. The sixth left-half digit, and the whole right
-- half, stay on set A. A leading zero is all A, which is why a UPC-A drawn
-- as an EAN-13 matches UPC-A's bars.
local EAN13Parity = {
  [0] = "AAAAA", "ABABB", "ABBAB", "ABBBA", "BAABB",
        "BBAAB", "BBBAA", "BABAB", "BABBA", "BBABA",
}

-- ------------------------------------------------------------- check digit

-- backend/gs1.c `gs1_check_digit()` -- the standard GS1 modulo-10 check
-- (GS1 General Specifications 7.9.1).
--
-- Digits are weighted 3 and 1 alternately, but which weight the *first* digit
-- gets depends on the length: `factor = length & 1 ? 3 : 1`. Eleven digits is
-- odd, so a UPC-A body starts on 3. Getting that backwards produces a check
-- digit that is right half the time, which is a miserable thing to debug.
--
-- Lives here rather than in gs1.lua because gs1.lua is the AI parser and this
-- is arithmetic on a digit string; the C keeps them together only because they
-- share a header.
local function gs1_check_digit(source, length)
  local count = 0
  local factor = (length & 1) ~= 0 and 3 or 1

  for i = 1, length do
    count = count + factor * ctoi(source:sub(i, i))
    factor = factor ~ 2 -- toggles 1 and 3
  end

  return string.char(48 + (10 - (count % 10)) % 10)
end

-- ------------------------------------------------------------------ drawing

-- backend/upcean.c `upca_draw()`. Builds the run-length string for the whole
-- symbol: guard, six digits, centre guard, six digits, guard.
--
-- The comment in the C -- "also inverts right hand characters" -- is about the
-- physical symbol rather than this code. The centre guard is five elements, an
-- odd count, so everything after it lands on the opposite bar/space phase from
-- everything before. The same EANsetA row therefore draws as bars on the left
-- and spaces on the right, which is what lets a scanner tell which way up the
-- label is. Nothing here has to do anything to make that happen; it falls out
-- of the element count.
local function upca_draw(source, length)
  local half_way = length // 2
  local d = { "111" } -- start guard

  for i = 1, length do
    if i - 1 == half_way then
      d[#d + 1] = "11111" -- centre guard, separating manufacturer from product
    end
    d[#d + 1] = EANsetA[ctoi(source:sub(i, i))]
  end

  d[#d + 1] = "111" -- stop guard
  return table.concat(d)
end

-- backend/upcean.c `ean_add_on()`. Appends a two- or five-digit add-on to a
-- symbol already built by upca_draw.
--
-- The add-on has no check digit of its own. Instead the *parity* of its digits
-- -- which of set A and set B each is drawn from -- encodes a checksum, which
-- is why the tables above are indexed by a computed value rather than by
-- position. For EAN-2 that value is the two digits read as a number mod 4; for
-- EAN-5 it is a weighted sum mod 10.
--
-- `addon_gap` is the space between the main symbol and the add-on, in modules;
-- it comes out as a single run-length digit, so it has to be 9-12 and is
-- clamped to that by the caller.
local function ean_add_on(source, length, dest, addon_gap)
  local d = { dest }

  if addon_gap ~= 0 then
    d[#d + 1] = string.char(48 + addon_gap)
  end

  d[#d + 1] = "112" -- add-on start character

  local parity
  if length == 2 then
    local code_value = 10 * ctoi(source:sub(1, 1)) + ctoi(source:sub(2, 2))
    parity = EAN2Parity[code_value % 4]
  else
    -- The C reads six values out of a five-digit string here -- values[5] picks
    -- up the terminating NUL, for which ctoi returns -1 -- and then never uses
    -- it. Only indices 0..4 reach the sum, so the loop below stops at five.
    local v = {}
    for i = 1, 5 do v[i] = ctoi(source:sub(i, i)) end
    local parity_sum = 3 * (v[1] + v[3] + v[5]) + 9 * (v[2] + v[4])
    parity = EAN5Parity[parity_sum % 10]
  end

  for i = 1, length do
    local digit = ctoi(source:sub(i, i))
    d[#d + 1] = (parity:sub(i, i) == "A") and EANsetA[digit] or EANsetB[digit]
    if i ~= length then
      d[#d + 1] = "11" -- glyph separator
    end
  end

  return table.concat(d)
end

-- --------------------------------------------------------------- UPC-A proper

-- BS EN 797:1996 4.5.1 nominal dimensions, 22.85mm at an 0.33mm X dimension.
-- Same as the minimum in GS1 General Specifications 21.0.1 5.12.3.1.
--
-- Narrowed exactly where the C narrows: both literals are floats, the division
-- happens in float, and stripf() then re-narrows the result. Writing this as
-- f32(22.85 / 0.33) instead -- one narrowing rather than three -- gives
-- 69.2424240112 where zint has 69.2424240112, which happens to agree; the
-- bracketing is kept because agreeing by luck is not the same as being right.
local COMPLIANT_HEIGHT = f32(f32(22.85) / f32(0.33))

-- ISO/IEC 15420:2009 4.3.3 bar height EAN-8, 18.23mm / 0.33mm. Same narrowing
-- as the UPC-A constant above. The C writes the result as the literal
-- 55.242424f; the division is what produces it.
local EAN8_COMPLIANT_HEIGHT = f32(f32(18.23) / f32(0.33))

-- backend/upcean.c `upca_cc()`, without the composite arm.
--
-- `symbol.text` is set here, to the full twelve digits including the check --
-- so the human-readable line shows the digit zint computed even when the caller
-- supplied only eleven. vector.lua then slices that string into its four
-- pieces, which is why the length has to be exactly twelve by the time this
-- returns.
local function upca(symbol, source, length)
  local gtin = source

  if length == 11 then
    -- Eleven digits: zint supplies the twelfth.
    gtin = gtin .. gs1_check_digit(gtin, 11)
    length = 12
  else
    -- Twelve digits: the twelfth must be the one zint would have computed.
    -- Note the check is calculated over the first eleven characters of the
    -- twelve-digit string, not over a separate body.
    local expected = gs1_check_digit(gtin, 11)
    local given = gtin:sub(length, length)
    if given ~= expected then
      return nil, string.format(
        "Error 270: Invalid check digit '%s', expecting '%s'", given, expected)
    end
  end

  symbol.text = gtin

  local dest = upca_draw(gtin, length)

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    common.set_height(symbol, COMPLIANT_HEIGHT, COMPLIANT_HEIGHT, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  return dest
end

-- backend/upcean.c `ean_leading_zeroes()`, reduced to the UPC-A arms.
--
-- Splits the input at a "+" or a space into the main body and the add-on, then pads each
-- with leading zeroes up to the length its symbology wants. UPC-A wants eleven
-- (twelve for UPCA_CHK, where the check digit is part of the input); an add-on
-- is padded to two or five.
--
-- This is why "00" .. nine digits and eleven digits both work, and also why a
-- three-digit input is accepted: it becomes 00000000123. The padding is silent
-- -- no warning -- so a caller who miscounts gets a valid symbol for a
-- different number.
--
-- Returns nil plus a flag when a part is too long for any padding to help. The
-- flag says *which* part overflowed, because the caller uses it to pick the
-- message. The C computes it as `second_len > 5 ? with_addon : 0`, which is a
-- longer way of writing `second_len > 5`: a second part can only be non-empty
-- if there was a "+" to begin with.
local function ean_leading_zeroes(symbol, source)
  local sep = source:find("[+ ]")
  local with_addon = sep ~= nil

  local first_part = with_addon and source:sub(1, sep - 1) or source
  local second_part = with_addon and source:sub(sep + 1) or ""

  -- The C counts characters rather than splitting, which means a *second* "+"
  -- would land in second_part's count. It cannot get here -- eanx_cc rejects
  -- multiple pluses first -- so a plain split is equivalent.
  if #first_part > 13 or #second_part > 5 then
    return nil, #second_part > 5 -- true: the add-on overflowed, not the body
  end

  local zfirst_len = (symbol.symbology == 35) and 12 or 11 -- UPCA_CHK / UPCA
  local zsecond_len
  if #second_part == 0 then
    zsecond_len = 0
  elseif #second_part <= 2 then
    zsecond_len = 2
  else
    zsecond_len = 5
  end

  -- string.rep with a negative count gives "", matching the C's for-loop that
  -- simply does not run when the target is shorter than the input.
  first_part = string.rep("0", zfirst_len - #first_part) .. first_part
  if with_addon then
    second_part = string.rep("0", zsecond_len - #second_part) .. second_part
  end

  return first_part, second_part
end

-- backend/upcean.c `eanx_cc()`, reduced to the UPC-A arms.
--
-- The order of the checks is the C's. It matters in one place: the "one +
-- only" test runs before the padding, so "1+2+3" is rejected as malformed
-- rather than silently padded into something.
function M.upca(symbol, source)
  if #source > 19 then
    return nil, "Error 283: Input too long (19 character maximum)"
  end

  -- SODIUM "0123456789+ ". A space is an add-on separator, the same as "+".
  local bad = source:find("[^0-9+ ]")
  if bad then
    return nil, string.format(
      "Error 284: Invalid character at position %d in input (digits and \"+\" or space only)",
      bad)
  end

  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end

  local first_part, second_part = ean_leading_zeroes(symbol, source)
  if not first_part then
    -- On failure the second return is the "which part" flag, not a string.
    return nil, "Error 294: Input too long (" ..
      (second_part and "5 character maximum for add-on" or "13 character maximum")
      .. ")"
  end

  -- Gap between symbol and add-on. UPC-A's minimum is 9 modules where the rest
  -- of the family uses 7, because UPC-A's right quiet zone is wider.
  -- --addongap (option_2) is not among the options this port accepts, so the
  -- default is what always applies.
  local addon_gap = 0
  if #second_part > 0 then
    local o2 = symbol.option_2
    addon_gap = (o2 >= 9 and o2 <= 12) and o2 or 9
  end

  if #first_part ~= 11 and #first_part ~= 12 then
    return nil, "Error 288: Input wrong length (12 character maximum)"
  end

  local dest, err = upca(symbol, first_part, #first_part)
  if not dest then return nil, err end

  if #second_part == 2 or #second_part == 5 then
    dest = ean_add_on(second_part, #second_part, dest, addon_gap)
    symbol.text = symbol.text .. "+" .. second_part
  elseif #second_part ~= 0 then
    return nil, "Error 292: Add-on data wrong length (2 or 5 characters only)"
  end

  common.expand(symbol, dest)

  return true
end

-- -------------------------------------------------------------- EAN-13 proper

-- backend/upcean.c `ean13_cc()`, without the composite arm.
--
-- The run is the same shape as UPC-A -- start, six digits, centre, six digits,
-- stop, 95 modules -- but the thirteen-digit string contributes only twelve
-- patterns. The first character selects EAN13Parity and is drawn later as
-- text in the left quiet zone. Parity applies to patterns 2..6 (C indices
-- i = 2..6). The centre guard is inserted when i == 7, before the seventh
-- encoded digit.
local function ean13_draw(gtin)
  local parity = EAN13Parity[ctoi(gtin:sub(1, 1))]
  local d = { "111" }

  for i = 1, 12 do
    if i == 7 then
      d[#d + 1] = "11111"
    end
    local digit = ctoi(gtin:sub(i + 1, i + 1))
    local use_b = i > 1 and i < 7 and parity:sub(i - 1, i - 1) == "B"
    d[#d + 1] = use_b and EANsetB[digit] or EANsetA[digit]
  end

  d[#d + 1] = "111"
  return table.concat(d)
end

local function ean13_body(symbol, source, length)
  symbol.text = source

  if length == 12 then
    symbol.text = symbol.text .. gs1_check_digit(symbol.text, 12)
  else
    local expected = gs1_check_digit(symbol.text, 12)
    local given = source:sub(length, length)
    if given ~= expected then
      return nil, string.format(
        "Error 275: Invalid check digit '%s', expecting '%s'", given, expected)
    end
  end

  local dest = ean13_draw(symbol.text)

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    common.set_height(symbol, COMPLIANT_HEIGHT, COMPLIANT_HEIGHT, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  return dest
end

-- backend/upcean.c `zint_ean_leading_zeroes()` for BARCODE_EAN13.
--
-- Two input shapes. A 13-, 15-, or 18-digit string whose thirteenth digit is
-- already the right check digit is a GTIN-13 with the add-on concatenated and
-- no separator: 15 is a two-digit add-on, 18 a five-digit one. Everything else
-- splits on the first "+" or space, then pads the body out to 12 (check still
-- to be computed) or leaves a 13-digit body alone, and pads the add-on out to
-- 2 or 5.
local function ean13_parts(source)
  local length = #source
  if (length == 13 or length == 15 or length == 18)
      and not source:find("[^0-9]")
      and gs1_check_digit(source, 12) == source:sub(13, 13) then
    local second = length > 13 and source:sub(14) or ""
    return source:sub(1, 13), second
  end

  local sep = source:find("[+ ]")
  local with_addon = sep ~= nil
  local first_part = with_addon and source:sub(1, sep - 1) or source
  local second_part = with_addon and source:sub(sep + 1) or ""
  local first_len, second_len = #first_part, #second_part
  local max = 13

  if first_len > max or second_len > 5 then
    if first_len > max then
      if second_len == 0 then
        return nil, string.format(
          "Error 294: Input length %d too long (maximum %d)", first_len, max)
      end
      return nil, string.format(
        "Error 298: Input EAN length %d too long (maximum %d)", first_len, max)
    end
    return nil, string.format(
      "Error 297: Input add-on length %d too long (maximum 5)", second_len)
  end

  local zfirst_len = first_len <= 12 and 12 or 13
  local zsecond_len = 0
  if second_len == 0 then
    zsecond_len = 0
  elseif second_len <= 2 then
    zsecond_len = 2
  else
    zsecond_len = 5
  end

  first_part = string.rep("0", zfirst_len - first_len) .. first_part
  if with_addon and zsecond_len > 0 then
    second_part = string.rep("0", zsecond_len - second_len) .. second_part
  else
    second_part = ""
  end

  return first_part, second_part
end

-- backend/upcean.c `eanx_cc()` for BARCODE_EAN13. The add-on gap defaults to
-- 7 modules. UPC-A's 9 is the wider right quiet zone, and EAN-13 does not
-- share it. option_2 is --addongap, which the CLI does not expose; the
-- library still honours a value in 7..12.
function M.ean13(symbol, source)
  if #source > 19 then
    return nil, string.format(
      "Error 283: Input length %d too long (maximum 19)", #source)
  end

  local bad = source:find("[^0-9+ ]")
  if bad then
    return nil, string.format(
      "Error 284: Invalid character at position %d in input (digits and \"+\" or space only)",
      bad)
  end

  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end

  local first_part, second_part = ean13_parts(source)
  if not first_part then return nil, second_part end

  local addon_gap = 0
  if #second_part > 0 then
    local o2 = symbol.option_2
    addon_gap = (o2 >= 7 and o2 <= 12) and o2 or 7
  end

  local dest, err = ean13_body(symbol, first_part, #first_part)
  if not dest then return nil, err end

  if #second_part > 0 then
    dest = ean_add_on(second_part, #second_part, dest, addon_gap)
    symbol.text = symbol.text .. "+" .. second_part
  end

  common.expand(symbol, dest)

  return true
end

-- --------------------------------------------------------------- EAN-8 proper

-- backend/upcean.c `ean8_cc()`, without the composite arm. The pattern is
-- UPC-A's with eight digits instead of twelve: four, centre guard, four.
local function ean8_body(symbol, source, length)
  symbol.text = source

  if length == 7 then
    symbol.text = symbol.text .. gs1_check_digit(symbol.text, 7)
  else
    local expected = gs1_check_digit(symbol.text, 7)
    local given = source:sub(length, length)
    if given ~= expected then
      return nil, string.format(
        "Error 276: Invalid EAN-8 check digit '%s', expecting '%s'", given, expected)
    end
  end

  local dest = upca_draw(symbol.text, #symbol.text)

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    common.set_height(symbol, EAN8_COMPLIANT_HEIGHT, EAN8_COMPLIANT_HEIGHT, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  return dest
end

-- backend/upcean.c `zint_ean_leading_zeroes()` for BARCODE_EAN8.
--
-- A 13-, 15-, or 18-digit GTIN whose check digit is right and whose first five
-- digits are zero is an EAN-8 stuffed into a GTIN-13. The body is the next
-- eight digits. Anything else pads a body of at most eight out to 7, or leaves
-- an 8-digit body alone.
local function ean8_parts(source)
  local length = #source
  if (length == 13 or length == 15 or length == 18)
      and not source:find("[^0-9]")
      and source:sub(1, 5) == "00000"
      and gs1_check_digit(source, 12) == source:sub(13, 13) then
    local second = length > 13 and source:sub(14) or ""
    return source:sub(6, 13), second
  end

  local sep = source:find("[+ ]")
  local with_addon = sep ~= nil
  local first_part = with_addon and source:sub(1, sep - 1) or source
  local second_part = with_addon and source:sub(sep + 1) or ""
  local first_len, second_len = #first_part, #second_part
  local max = 8

  if first_len > max or second_len > 5 then
    if first_len > max then
      if second_len == 0 then
        return nil, string.format(
          "Error 294: Input length %d too long (maximum %d)", first_len, max)
      end
      return nil, string.format(
        "Error 298: Input EAN length %d too long (maximum %d)", first_len, max)
    end
    return nil, string.format(
      "Error 297: Input add-on length %d too long (maximum 5)", second_len)
  end

  local zfirst_len = first_len <= 7 and 7 or 8
  local zsecond_len = 0
  if second_len == 0 then
    zsecond_len = 0
  elseif second_len <= 2 then
    zsecond_len = 2
  else
    zsecond_len = 5
  end

  first_part = string.rep("0", zfirst_len - first_len) .. first_part
  if with_addon and zsecond_len > 0 then
    second_part = string.rep("0", zsecond_len - second_len) .. second_part
  else
    second_part = ""
  end

  return first_part, second_part
end

-- backend/upcean.c `eanx_cc()` for BARCODE_EAN8. An add-on is legal and drawn,
-- and it is also Warning 292: EAN-8 with an add-on is non-standard. That
-- warning replaces a compliant-height warning, which is the order the C
-- assigns the one errtxt buffer.
function M.ean8(symbol, source)
  if #source > 19 then
    return nil, string.format(
      "Error 283: Input length %d too long (maximum 19)", #source)
  end

  local bad = source:find("[^0-9+ ]")
  if bad then
    return nil, string.format(
      "Error 284: Invalid character at position %d in input (digits and \"+\" or space only)",
      bad)
  end

  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end

  local first_part, second_part = ean8_parts(source)
  if not first_part then return nil, second_part end

  local addon_gap = 0
  if #second_part > 0 then
    local o2 = symbol.option_2
    addon_gap = (o2 >= 7 and o2 <= 12) and o2 or 7
  end

  local dest, err = ean8_body(symbol, first_part, #first_part)
  if not dest then return nil, err end

  if #second_part > 0 then
    dest = ean_add_on(second_part, #second_part, dest, addon_gap)
    symbol.text = symbol.text .. "+" .. second_part
    symbol.warning = "Warning 292: EAN-8 with add-on is non-standard"
  end

  common.expand(symbol, dest)

  return true
end

-- --------------------------------------------------------------- UPC-E proper

-- backend/upcean.c `ean_is_upce()`. A GTIN-13 that is a zero-compressed UPC-A
-- converts to an 8-digit UPC-E (number system, six digits, check). The four
-- rules are BS 797 Table 5, and the first one that matches wins. Returns nil
-- when the GTIN is not UPC-E compatible.
local function ean_is_upce(source)
  if source:sub(1, 1) ~= "0" or source:sub(2, 2) > "1" then
    return nil
  end
  local ns = source:sub(2, 2)
  local check = source:sub(13, 13)

  -- Last of the manufacturer block is 5-9, and four zeroes follow a non-zero.
  if source:sub(12, 12) >= "5" and source:sub(7, 7) ~= "0"
      and source:sub(8, 11) == "0000" then
    return ns .. source:sub(3, 7) .. source:sub(12, 12) .. check
  end
  -- Five trailing zeroes, and the fifth digit is not zero. Marker digit 4.
  if source:sub(6, 6) ~= "0" and source:sub(7, 11) == "00000" then
    return ns .. source:sub(3, 6) .. source:sub(12, 12) .. "4" .. check
  end
  -- Four zeroes after a 0, 1, or 2 in the fourth digit.
  if source:sub(5, 5) <= "2" and source:sub(6, 9) == "0000" then
    return ns .. source:sub(3, 4) .. source:sub(10, 12) .. source:sub(5, 5) .. check
  end
  -- Five zeroes after a 3-9 in the fourth digit. Marker digit 3.
  if source:sub(5, 5) >= "3" and source:sub(6, 10) == "00000" then
    return ns .. source:sub(3, 5) .. source:sub(11, 12) .. "3" .. check
  end
  return nil
end

-- Expand six compressed digits to the 11-digit UPC-A body the check digit is
-- calculated on. `emode` is the sixth digit and selects the rule. The number
-- system is the first digit of the equivalent, and it is 0 unless the caller
-- is in number system 1.
local function upce_expand(digits, num_system, ns_char)
  local eq = {}
  for i = 1, 11 do eq[i] = "0" end
  if num_system == 1 then eq[1] = ns_char end
  eq[2] = digits:sub(1, 1)
  eq[3] = digits:sub(2, 2)
  local emode = digits:sub(6, 6)
  local d3, d4, d5 = digits:sub(3, 3), digits:sub(4, 4), digits:sub(5, 5)

  if emode == "0" or emode == "1" or emode == "2" then
    eq[4] = emode
    eq[9], eq[10], eq[11] = d3, d4, d5
  elseif emode == "3" then
    if d3 == "0" or d3 == "1" or d3 == "2" then
      return nil, string.format(
        'Error 271: For this UPC-E zero suppression, 3rd character cannot be "0", "1" or "2" (%s)',
        digits)
    end
    eq[4] = d3
    eq[10], eq[11] = d4, d5
  elseif emode == "4" then
    if d4 == "0" then
      return nil, string.format(
        'Error 272: For this UPC-E zero suppression, 4th character cannot be "0" (%s)',
        digits)
    end
    eq[4], eq[5] = d3, d4
    eq[11] = d5
  else
    if d5 == "0" then
      return nil, string.format(
        'Error 273: For this UPC-E zero suppression, 5th character cannot be "0" (%s)',
        digits)
    end
    eq[4], eq[5], eq[6] = d3, d4, d5
    eq[11] = emode
  end
  return table.concat(eq)
end

local function upce_draw(digits, parity)
  local d = { "111" }
  for i = 1, 6 do
    local digit = ctoi(digits:sub(i, i))
    d[#d + 1] = (parity:sub(i, i) == "A") and EANsetA[digit] or EANsetB[digit]
  end
  d[#d + 1] = "111111"
  return table.concat(d)
end

-- backend/upcean.c `upce_cc()`, without the composite arm.
--
-- UPC-E stores six digits and rebuilds a UPC-A to find the check digit. The
-- number system is 0 or 1; anything else is forced to 0 and warns 851. A
-- compliant-height warning replaces that one, because errtxt holds a single
-- message and the C assigns the height warning last.
local function upce_body(symbol, source, length)
  local src_check
  if length == 8 or symbol.symbology == 38 then
    src_check = source:sub(length, length)
    source = source:sub(1, length - 1)
    length = length - 1
  end

  local hrt, digits, num_system
  local ignored
  if length == 7 then
    local ns = source:sub(1, 1)
    if ns == "0" or ns == "1" then
      hrt = source
      if ns == "1" then num_system = 1 else num_system = 0 end
    else
      hrt = "0" .. source:sub(2)
      num_system = 0
      ignored = "Warning 851: Ignoring first digit which is not '0' or '1'"
    end
    digits = hrt:sub(2)
  else
    hrt = "0" .. source
    digits = source
    num_system = 0
  end

  local equivalent, err = upce_expand(digits, num_system, hrt:sub(1, 1))
  if not equivalent then return nil, err end

  local check = gs1_check_digit(equivalent, 11)
  if src_check and src_check ~= check then
    return nil, string.format(
      "Error 274: Invalid check digit '%s', expecting '%s'", src_check, check)
  end

  local parity = (num_system == 1 and UPCParity1 or UPCParity0)[ctoi(check)]
  local dest = upce_draw(digits, parity)
  symbol.text = hrt .. check

  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    common.set_height(symbol, COMPLIANT_HEIGHT, COMPLIANT_HEIGHT, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end
  if ignored and not symbol.warning then
    symbol.warning = ignored
  end

  return dest
end

-- backend/upcean.c `zint_ean_leading_zeroes()` for UPC-E and UPC-E with check.
-- A compressible GTIN-13 (length 13, 15, or 18, check digit already right)
-- becomes the 8-digit UPC-E. Otherwise the body pads to 6 (or 7 when a
-- 7-digit number system is present). UPC-E with check pads to 7, or stays at
-- 8. Longer bodies are left alone so the caller can report Error 290.
local function upce_parts(symbol, source)
  local length = #source
  if (length == 13 or length == 15 or length == 18)
      and not source:find("[^0-9]")
      and gs1_check_digit(source, 12) == source:sub(13, 13) then
    local compressed = ean_is_upce(source)
    if compressed then
      local second = length > 13 and source:sub(14) or ""
      return compressed, second
    end
  end

  local sep = source:find("[+ ]")
  local with_addon = sep ~= nil
  local first_part = with_addon and source:sub(1, sep - 1) or source
  local second_part = with_addon and source:sub(sep + 1) or ""
  local first_len, second_len = #first_part, #second_part
  local max = 13

  if first_len > max or second_len > 5 then
    if first_len > max then
      if second_len == 0 then
        return nil, string.format(
          "Error 294: Input length %d too long (maximum %d)", first_len, max)
      end
      return nil, string.format(
        "Error 298: Input EAN length %d too long (maximum %d)", first_len, max)
    end
    return nil, string.format(
      "Error 297: Input add-on length %d too long (maximum 5)", second_len)
  end

  local zfirst_len = 0
  if symbol.symbology == 38 then
    if first_len == 8 then
      zfirst_len = 8
    elseif first_len <= 7 then
      zfirst_len = 7
    end
  else
    if first_len == 7 then
      zfirst_len = 7
    elseif first_len <= 6 then
      zfirst_len = 6
    end
  end

  if zfirst_len > first_len then
    first_part = string.rep("0", zfirst_len - first_len) .. first_part
  end
  local zsecond_len = 0
  if second_len == 0 then
    zsecond_len = 0
  elseif second_len <= 2 then
    zsecond_len = 2
  else
    zsecond_len = 5
  end
  if with_addon and zsecond_len > 0 then
    second_part = string.rep("0", zsecond_len - second_len) .. second_part
  else
    second_part = ""
  end

  return first_part, second_part
end

-- backend/upcean.c `eanx_cc()` for BARCODE_UPCE and BARCODE_UPCE_CHK.
function M.upce(symbol, source)
  if #source > 19 then
    return nil, string.format(
      "Error 283: Input length %d too long (maximum 19)", #source)
  end

  local bad = source:find("[^0-9+ ]")
  if bad then
    return nil, string.format(
      "Error 284: Invalid character at position %d in input (digits and \"+\" or space only)",
      bad)
  end

  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end

  local first_part, second_part = upce_parts(symbol, source)
  if not first_part then return nil, second_part end

  if #first_part > 8 then
    return nil, string.format(
      "Error 290: Input length %d too long (maximum 8)", #first_part)
  end

  local addon_gap = 0
  if #second_part > 0 then
    local o2 = symbol.option_2
    addon_gap = (o2 >= 7 and o2 <= 12) and o2 or 7
  end

  local dest, err = upce_body(symbol, first_part, #first_part)
  if not dest then return nil, err end

  if #second_part > 0 then
    dest = ean_add_on(second_part, #second_part, dest, addon_gap)
    symbol.text = symbol.text .. "+" .. second_part
  end

  common.expand(symbol, dest)

  return true
end

-- ------------------------------------------------- standalone add-ons, ISBN

-- 21.9mm from GS1 General Specifications 5.2.6.6, Figure 5.2.6.6-6.
local ADDON_COMPLIANT_HEIGHT = f32(f32(21.9) / f32(0.33))

local function too_long(n)
  return nil, string.format("Error 283: Input length %d too long (maximum 19)", n)
end

-- Split on the first "+" or space and pad. `max` is the body's ceiling:
-- 2, 5, or 13. An add-on symbology always reports a long body as Error 294,
-- even when an add-on is also present.
local function split_pad(source, max, addon_symbology)
  local sep = source:find("[+ ]")
  local first = sep and source:sub(1, sep - 1) or source
  local second = sep and source:sub(sep + 1) or ""
  local first_len, second_len = #first, #second

  if first_len > max or second_len > 5 then
    if first_len > max then
      if second_len == 0 or addon_symbology then
        return nil, nil, string.format(
          "Error 294: Input length %d too long (maximum %d)", first_len, max)
      end
      return nil, nil, string.format(
        "Error 298: Input EAN length %d too long (maximum %d)", first_len, max)
    end
    return nil, nil, string.format(
      "Error 297: Input add-on length %d too long (maximum 5)", second_len)
  end

  local zfirst = max
  if not addon_symbology then
    zfirst = first_len <= 9 and 9 or first_len
  end
  local zsecond = 0
  if second_len == 0 then
    zsecond = 0
  elseif second_len <= 2 then
    zsecond = 2
  else
    zsecond = 5
  end

  first = string.rep("0", zfirst - first_len) .. first
  if sep and zsecond > 0 then
    second = string.rep("0", zsecond - second_len) .. second
  else
    second = ""
  end
  return first, second
end

local function addon_gap_of(symbol)
  local o2 = symbol.option_2 or 0
  return (o2 >= 7 and o2 <= 12) and o2 or 7
end

local function finish_addon(symbol, first, second)
  local dest = ean_add_on(first, #first, "", 0)
  symbol.text = first
  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    common.set_height(symbol, ADDON_COMPLIANT_HEIGHT, ADDON_COMPLIANT_HEIGHT, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end
  if #second > 0 then
    dest = ean_add_on(second, #second, dest, addon_gap_of(symbol))
    symbol.text = symbol.text .. "+" .. second
  end
  common.expand(symbol, dest)
  return true
end

local function standalone(symbol, source, width)
  if #source > 19 then return too_long(#source) end
  local bad = source:find("[^0-9]")
  if bad then
    return nil, string.format(
      "Error 852: Invalid character at position %d in input (digits only)", bad)
  end
  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end
  local first, second, err = split_pad(source, width, true)
  if not first then return nil, err end
  return finish_addon(symbol, first, second)
end

function M.ean2(symbol, source)
  return standalone(symbol, source, 2)
end

function M.ean5(symbol, source)
  return standalone(symbol, source, 5)
end

-- ISBN-10 / SBN check. Weights are 1..9 over the nine digits that precede
-- the check character. 10 is written "X".
local function isbnx_check(source)
  local sum = 0
  for i = 1, 9 do
    sum = sum + ctoi(source:sub(i, i)) * i
  end
  local check = sum % 11
  if check == 10 then return "X" end
  return string.char(48 + check)
end

-- Turn a padded ISBN or SBN into the 12-digit EAN-13 body. The EAN check
-- digit is added by ean13_body.
local function isbn_to_ean12(source)
  source = source:upper()
  if source:find("[^0-9X]") then
    return nil, "Error 277: Invalid character in input (digits and \"X\" only)"
  end
  local length = #source
  if length ~= 9 and length ~= 10 and length ~= 13 then
    return nil, string.format(
      "Error 278: Input length %d wrong (9, 10, or 13 characters required)", length)
  end

  if length == 13 then
    if source:sub(1, 2) ~= "97" or (source:sub(3, 3) ~= "8" and source:sub(3, 3) ~= "9") then
      return nil, "Error 279: Invalid ISBN (must begin with \"978\" or \"979\")"
    end
    if source:find("X", 1, true) then
      return nil, "Error 282: Invalid character in input, \"X\" not allowed in ISBN-13"
    end
    local check = gs1_check_digit(source, 12)
    if source:sub(13, 13) ~= check then
      return nil, string.format(
        "Error 280: Invalid ISBN check digit '%s', expecting '%s'",
        source:sub(13, 13), check)
    end
    return source:sub(1, 12)
  end

  local original = length
  if length == 9 then source = "0" .. source end
  if source:sub(1, 9):find("[^0-9]") then
    return nil, "Error 296: Invalid character in input, \"X\" allowed in last position only"
  end
  local check = isbnx_check(source)
  local given = source:sub(10, 10)
  if check ~= given then
    return nil, string.format(
      "Error 281: Invalid %s check digit '%s', expecting '%s'",
      original == 9 and "SBN" or "ISBN", given, check)
  end
  return "978" .. source:sub(1, 9)
end

function M.isbn(symbol, source)
  if #source > 19 then return too_long(#source) end
  local bad = source:find("[^0-9Xx+ ]")
  if bad then
    return nil, string.format(
      "Error 285: Invalid character at position %d in input (digits, \"X\" and \"+\" or space only)",
      bad)
  end
  local _, sep_count = source:gsub("[+ ]", "")
  if sep_count > 1 then
    return nil, "Error 293: Invalid add-on data (one \"+\" or space only)"
  end

  local first, second, err = split_pad(source, 13, false)
  if not first then return nil, err end

  local body, berr = isbn_to_ean12(first)
  if not body then return nil, berr end

  local dest, derr = ean13_body(symbol, body, #body)
  if not dest then return nil, derr end

  if #second > 0 then
    if second:find("[^0-9]") then
      return nil, "Error 295: Invalid add-on data (digits only)"
    end
    dest = ean_add_on(second, #second, dest, addon_gap_of(symbol))
    symbol.text = symbol.text .. "+" .. second
  end

  common.expand(symbol, dest)
  return true
end

return M
