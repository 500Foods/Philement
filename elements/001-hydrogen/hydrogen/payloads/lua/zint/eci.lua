-- zint/eci.lua -- ports the parts of backend/eci.c the five symbologies reach.
--
-- Linear symbols fold UTF-8 to ISO/IEC 8859-1 before the encoder sees it.
-- That is `utf8_to_latin1`, the ECI 0 / 1 / 3 branch. QR does not: it receives
-- the UTF-8 and folds it itself. When Latin-1 fails, QR tries Shift JIS
-- (`sjis_utf8`) and, if that fails too, the driver calls `get_best_eci` and
-- encodes again. `get_best_eci` tries the single-byte sets in zint/eci_sb.lua
-- and otherwise returns 26, which means "leave the bytes as UTF-8 and say so
-- in an ECI header". Big5, GB 2312, and KS X 1001 are not on that path.
--
-- `--esc` makes a non-ASCII byte reachable in one keystroke (`é`). Skipping
-- the Latin-1 fold encodes "cafe\xc3\xa9" as five characters where zint encodes
-- four, which is a different barcode, not a rounding difference.

local common = require "zint.common"

local M = {}

-- backend/eci.c `utf8_to_eci()`, the `eci == 0 || eci == 3` branch. Returns the
-- converted bytes, or nil for ZINT_ERROR_INVALID_DATA.
--
-- Two ways to fail, and they are different failures wearing the same return
-- code: the input is not valid UTF-8, or it is valid UTF-8 for a codepoint that
-- ISO 8859-1 has no room for. The second test looks lopsided -- everything below
-- 0x80 passes, 0x80 to 0x9f is rejected, 0xa0 to 0xff passes, 0x100 up is
-- rejected -- because ISO 8859-1 leaves 0x80..0x9f as unassigned C1 control
-- codes. Latin-1 as most people mean it (Windows-1252, with quotes and dashes
-- in that gap) is a different ECI, number 23.
function M.utf8_to_latin1(source)
  local out = {}
  local state, codepoint = 0, 0
  local pos = 1
  local len = #source

  while pos <= len do
    -- The C's inner do/while: keep feeding bytes until the DFA either completes
    -- a codepoint (state 0) or gives up (state 12). Running off the end of the
    -- input mid-sequence leaves a state that is neither, which the test below
    -- catches -- a truncated multi-byte sequence is invalid, not ignorable.
    repeat
      state, codepoint = common.decode_utf8(state, codepoint, source:byte(pos))
      pos = pos + 1
    until pos > len or state == 0 or state == 12

    if state ~= 0 then return nil end
    if codepoint >= 0x80 and (codepoint < 0xa0 or codepoint >= 0x100) then
      return nil
    end
    out[#out + 1] = string.char(codepoint)
  end

  return table.concat(out)
end

-- backend/eci.c `zint_is_eci_convertible`. ECI 26 is UTF-8, so there is nothing
-- to convert. The other exclusions are the unassigned numbers above 35, apart
-- from the two late assignments 170 and 899.
function M.is_eci_convertible(eci)
  if eci == 26 or (eci > 35 and eci ~= 170 and eci ~= 899) then
    return false
  end
  return true
end

local function map_utf8(source, map)
  local out = {}
  local state, codepoint = 0, 0
  local pos, len = 1, #source
  while pos <= len do
    repeat
      state, codepoint = common.decode_utf8(state, codepoint, source:byte(pos))
      pos = pos + 1
    until pos > len or state == 0 or state == 12
    if state ~= 0 then return nil end
    local byte = map(codepoint)
    if byte == nil then return nil end
    out[#out + 1] = string.char(byte)
  end
  return table.concat(out)
end

-- backend/eci.c `zint_utf8_to_eci`, for the ECIs this port can be asked for.
-- Returns the converted bytes, or nil for ZINT_ERROR_INVALID_DATA. ECI 20 and
-- the multibyte CJK sets are absent: `get_best_eci` skips 20, and a caller that
-- does land on 26 takes the raw-byte arm of `sjis_utf8_to_eci` instead.
function M.utf8_to_eci(eci, source)
  if eci == 0 or eci == 1 or eci == 3 then
    return M.utf8_to_latin1(source)
  end
  local sb = require "zint.eci_sb"
  if not sb.has(eci) then return nil end
  return map_utf8(source, function(u) return sb.byte(eci, u) end)
end

-- backend/eci.c `zint_get_best_eci`. Lowest single-byte ECI that can hold the
-- string. 14 and 19 are reserved, and 20 is Shift JIS, which QR has already
-- tried by the time this runs. If none of them fit, 26 means UTF-8.
function M.get_best_eci(source)
  local eci = 3
  repeat
    if eci == 14 then
      eci = 15
    elseif eci == 19 then
      eci = 21
    end
    if M.utf8_to_eci(eci, source) then return eci end
    eci = eci + 1
  until eci >= 25
  return 26
end

local function bytes_to_ddata(text)
  local ddata = {}
  for i = 1, #text do
    ddata[i - 1] = text:byte(i)
  end
  return ddata, #text
end

-- backend/eci.c `zint_sjis_utf8_to_eci` with `full_multibyte` off. A convertible
-- ECI is folded and then each byte is widened to an int. ECI 26 is not
-- convertible: the UTF-8 bytes are widened as they stand, and the QR encoder
-- emits an ECI header so a reader knows that.
function M.sjis_utf8_to_eci(eci, source)
  if M.is_eci_convertible(eci) then
    local converted = M.utf8_to_eci(eci, source)
    if not converted then return nil end
    return bytes_to_ddata(converted)
  end
  return bytes_to_ddata(source)
end

-- backend/eci.c `zint_sjis_utf8`. One ddata entry per Unicode scalar, which is
-- why a successful call shortens a multi-byte UTF-8 string. Values above 255
-- are Shift JIS double-byte characters. 4-byte UTF-8 is rejected here and only
-- here; the automatic-ECI retry then keeps those bytes under ECI 26.
function M.sjis_utf8(source)
  local sjis = require "zint.sjis"
  local ddata = {}
  local n = 0
  local state, codepoint = 0, 0
  local pos, len = 1, #source
  while pos <= len do
    repeat
      state, codepoint = common.decode_utf8(state, codepoint, source:byte(pos))
      pos = pos + 1
    until pos > len or state == 0 or state == 12
    if state ~= 0 then
      return nil, "Error 240: Corrupt Unicode data"
    end
    if codepoint > 0xFFFF then
      return nil, "Error 242: Unicode sequences of more than 3 bytes not supported"
    end
    local d = sjis.to_int(codepoint)
    if d == nil then
      return nil, "Error 800: Invalid character in input"
    end
    ddata[n] = d
    n = n + 1
  end
  return ddata, n
end

return M
