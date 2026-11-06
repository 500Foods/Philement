-- zint/aztec.lua -- Aztec Code.
--
-- backend/aztec.c `zint_aztec()`. One segment. Aztec Rune is not encoded.
-- Structured Append is not encoded: `symbol.structapp` is absent, so the C's
-- `symbol->structapp.count` is 0 and that block does not run.
--
-- GS1 reduction has already happened. This port's reduced string marks FNC1
-- with "[", and the C encoder reads GS (byte 29). The marker is translated
-- here, and only while GS1 mode is on, because "[" is punct data otherwise.
--
-- Reed-Solomon stays in the order `encode_uint` / `uint_encode` / `encode_c`
-- already return. Those reverse the remainder the way `zint_rs_encode_uint`,
-- `zint_rs_uint_encode` and `zint_rs_encode` do. Do not reverse again.
--
-- After the check symbols are appended the bit string is inverted, then
-- plotted as a spiral. Both steps are required. The grid is modules; each
-- row is one module tall and `symbol.height` is the module count, which is
-- what keeps the driver from stretching the symbol.

local common = require "zint.common"
local reedsol = require "zint.reedsol"
local tabs = require "zint.aztec_tabs"

local M = {}

local AZTEC_BIN_CAPACITY = 19932 -- AZTEC_MAX_CAPACITY less 3 * 12
local AZTEC_MAP_POSN_MAX = 20039
local AZTEC_MAP_SIZE = 22801

local DOT, COMMA, SP = 46, 44, 32
local CR, LF, COLON = 13, 10, 58
local GS = 29
local MARK_A, MARK_B, MARK_C, MARK_D = 97, 98, 99, 100 -- "a" "b" "c" "d"

local stripf = common.stripf
local format = string.format

local function fill_mode(t, at, n, ch)
  for k = 0, n - 1 do
    t[at + k] = ch
  end
end

-- Count consecutive (. SP) or (, SP) pairs.
local function az_count_doubles(source, position, length)
  local i = position
  while i + 1 < length and (source[i] == DOT or source[i] == COMMA) and source[i + 1] == SP do
    i = i + 2
  end
  return (i - position) >> 1
end

local function az_count_dotcomma(source, position, length)
  local i = position
  while i < length and (source[i] == DOT or source[i] == COMMA) do
    i = i + 1
  end
  return i - position
end

local function az_count_chr(source, position, length, chr)
  local i = position
  while i < length and source[i] == chr do
    i = i + 1
  end
  return i - position
end

-- Mode after the run that starts at i, or "E" at the end of the string.
local function az_get_next_mode(encode_mode, length, i)
  local current = encode_mode[i]
  repeat
    i = i + 1
  until i >= length or encode_mode[i] ~= current
  if i >= length then return "E" end
  return encode_mode[i]
end

-- backend/common.c `z_bin_append_posn()`. MSB first. `binary` is 0-based.
local function bin_append(arg, length, binary, bin_posn)
  local last = length - 1
  for i = 0, length - 1 do
    binary[bin_posn + i] = ((arg >> (last - i)) & 1) ~= 0 and "1" or "0"
  end
  return bin_posn + length
end

-- Same append, except a write past AZTEC_BIN_CAPACITY fails with 0.
local function az_bin_append(arg, length, binary, bin_posn)
  if bin_posn + length > AZTEC_BIN_CAPACITY then return 0 end
  return bin_append(arg, length, binary, bin_posn)
end

-- Decide modes and append the data bits. Returns the new bit length, or nil
-- when the string does not fit in AZTEC_BIN_CAPACITY. That is the only failure
-- `aztec_text_process()` reports.
local function text_process(source, length, bp, binary, gs1, gs1_bp, eci, initial_mode)
  local encode_mode = {}
  local reduced_source = {}
  local reduced_encode_mode = {}
  local current_mode = initial_mode

  local function app(val, n)
    local nbp = az_bin_append(val, n, binary, bp)
    if nbp == 0 then return false end
    bp = nbp
    return true
  end

  for i = 0, length - 1 do
    local b = source[i]
    if b > 127 then
      encode_mode[i] = "B"
    elseif gs1 and b == GS then
      encode_mode[i] = "P"
    else
      encode_mode[i] = tabs.AztecModes[b]
    end
  end

  -- Pairs that share one codeword: CR LF, : SP, and sometimes . SP / , SP.
  for i = 0, length - 2 do
    if source[i] == CR and source[i + 1] == LF then
      encode_mode[i] = "P"
      encode_mode[i + 1] = "P"
    elseif source[i] == COLON and source[i + 1] == SP then
      encode_mode[i + 1] = "P"
    elseif (source[i] == DOT or source[i] == COMMA) and source[i + 1] == SP
        and encode_mode[i] == "X" then
      local count = az_count_doubles(source, i, length)
      local next_mode = az_get_next_mode(encode_mode, length, i)
      if current_mode == "U" then
        if next_mode == "D" and count <= 5 then
          fill_mode(encode_mode, i, 2 * count, "D")
        end
      elseif current_mode == "L" then
        if next_mode == "D" and count <= 4 then
          fill_mode(encode_mode, i, 2 * count, "D")
        end
      elseif current_mode == "M" then
        if next_mode == "D" and count == 1 then
          encode_mode[i] = "D"
          encode_mode[i + 1] = "D"
        end
      elseif current_mode == "D" then
        if next_mode ~= "D" and count <= 4 then
          fill_mode(encode_mode, i, 2 * count, "D")
        elseif next_mode == "D" and count <= 7 then
          fill_mode(encode_mode, i, 2 * count, "D")
        end
      end
      if encode_mode[i] == "X" then
        encode_mode[i] = "P"
        encode_mode[i + 1] = "P"
      end
    end
    if encode_mode[i] ~= "X" and encode_mode[i] ~= "B" then
      current_mode = encode_mode[i]
    end
  end

  -- Collapse each kept pair to one marker. "a" CR LF, "b" . SP, "c" , SP, "d" : SP.
  local i, j = 0, 0
  while i < length do
    reduced_encode_mode[j] = encode_mode[i]
    if i + 1 < length then
      if source[i] == CR and source[i + 1] == LF then
        reduced_source[j] = MARK_A
        i = i + 2
      elseif source[i] == DOT and source[i + 1] == SP and encode_mode[i] == "P" then
        reduced_source[j] = MARK_B
        i = i + 2
      elseif source[i] == COMMA and source[i + 1] == SP and encode_mode[i] == "P" then
        reduced_source[j] = MARK_C
        i = i + 2
      elseif source[i] == COLON and source[i + 1] == SP then
        reduced_source[j] = MARK_D
        i = i + 2
      else
        reduced_source[j] = source[i]
        i = i + 1
      end
    else
      reduced_source[j] = source[i]
      i = i + 1
    end
    j = j + 1
  end
  local reduced_length = j

  current_mode = initial_mode
  for ri = 0, reduced_length - 1 do
    local ch = reduced_source[ri]
    if ch == CR then
      local count = az_count_chr(reduced_source, ri, reduced_length, CR)
      local next_mode = az_get_next_mode(reduced_encode_mode, reduced_length, ri)
      if current_mode == "U" and (next_mode == "U" or next_mode == "B") and count == 1 then
        reduced_encode_mode[ri] = "P"
      elseif current_mode == "L" and (next_mode == "L" or next_mode == "B") and count == 1 then
        reduced_encode_mode[ri] = "P"
      elseif current_mode == "P" or next_mode == "P" then
        reduced_encode_mode[ri] = "P"
      end
      if current_mode == "D" then
        if (next_mode == "E" or next_mode == "U" or next_mode == "D" or next_mode == "B")
            and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "P")
        elseif next_mode == "L" and count == 1 then
          reduced_encode_mode[ri] = "P"
        end
      end
      if reduced_encode_mode[ri] == "X" then
        reduced_encode_mode[ri] = "M"
      end
    elseif ch == DOT or ch == COMMA then
      local count = az_count_dotcomma(reduced_source, ri, reduced_length)
      local next_mode = az_get_next_mode(reduced_encode_mode, reduced_length, ri)
      if current_mode == "U" then
        if (next_mode == "U" or next_mode == "L" or next_mode == "M" or next_mode == "B")
            and count == 1 then
          reduced_encode_mode[ri] = "P"
        end
      elseif current_mode == "L" then
        if next_mode == "L" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "P")
        elseif (next_mode == "M" or next_mode == "B") and count == 1 then
          reduced_encode_mode[ri] = "P"
        end
      elseif current_mode == "M" then
        if (next_mode == "E" or next_mode == "U" or next_mode == "L" or next_mode == "M")
            and count <= 4 then
          fill_mode(reduced_encode_mode, ri, count, "P")
        elseif next_mode == "B" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "P")
        end
      elseif current_mode == "P" and next_mode ~= "D" and count <= 9 then
        fill_mode(reduced_encode_mode, ri, count, "P")
      end
      if reduced_encode_mode[ri] == "X" then
        reduced_encode_mode[ri] = "D"
      end
    elseif ch == SP then
      local count = az_count_chr(reduced_source, ri, reduced_length, SP)
      local next_mode = az_get_next_mode(reduced_encode_mode, reduced_length, ri)
      if current_mode == "U" then
        if next_mode == "E" and count <= 5 then
          fill_mode(reduced_encode_mode, ri, count, "U")
        elseif (next_mode == "U" or next_mode == "L" or next_mode == "M"
            or next_mode == "P" or next_mode == "B") and count <= 9 then
          fill_mode(reduced_encode_mode, ri, count, "U")
        end
      elseif current_mode == "L" then
        if next_mode == "E" and count <= 5 then
          fill_mode(reduced_encode_mode, ri, count, "L")
        elseif next_mode == "U" and count == 1 then
          reduced_encode_mode[ri] = "L"
        elseif next_mode == "L" and count <= 14 then
          fill_mode(reduced_encode_mode, ri, count, "L")
        elseif (next_mode == "M" or next_mode == "P" or next_mode == "B") and count <= 9 then
          fill_mode(reduced_encode_mode, ri, count, "L")
        end
      elseif current_mode == "M" then
        if (next_mode == "E" or next_mode == "U") and count <= 9 then
          fill_mode(reduced_encode_mode, ri, count, "M")
        elseif (next_mode == "L" or next_mode == "B") and count <= 14 then
          fill_mode(reduced_encode_mode, ri, count, "M")
        elseif (next_mode == "M" or next_mode == "P") and count <= 19 then
          fill_mode(reduced_encode_mode, ri, count, "M")
        end
      elseif current_mode == "P" then
        if next_mode == "E" and count <= 5 then
          fill_mode(reduced_encode_mode, ri, count, "U")
        elseif (next_mode == "U" or next_mode == "L" or next_mode == "M"
            or next_mode == "P" or next_mode == "B") and count <= 9 then
          fill_mode(reduced_encode_mode, ri, count, "U")
        end
      end
      if reduced_encode_mode[ri] == "X" then
        reduced_encode_mode[ri] = "D"
      end
    end
    if reduced_encode_mode[ri] ~= "B" then
      current_mode = reduced_encode_mode[ri]
    end
  end

  -- Punct shift and upper shift, where a latch would cost more than it saves.
  current_mode = initial_mode
  for ri = 0, reduced_length - 1 do
    if reduced_encode_mode[ri] ~= current_mode then
      local count = 0
      while ri + count < reduced_length
          and reduced_encode_mode[ri + count] == reduced_encode_mode[ri] do
        count = count + 1
      end
      local next_mode = az_get_next_mode(reduced_encode_mode, reduced_length, ri)
      if reduced_encode_mode[ri] == "P" then
        if current_mode == "U" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "p")
        elseif current_mode == "L" and next_mode ~= "U" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "p")
        elseif current_mode == "L" and next_mode == "U" and count == 1 then
          reduced_encode_mode[ri] = "p"
        elseif current_mode == "M" and next_mode ~= "M" and count == 1 then
          reduced_encode_mode[ri] = "p"
        elseif current_mode == "M" and next_mode == "M" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "p")
        elseif current_mode == "D" and next_mode ~= "D" and count <= 3 then
          fill_mode(reduced_encode_mode, ri, count, "p")
        elseif current_mode == "D" and next_mode == "D" and count <= 6 then
          fill_mode(reduced_encode_mode, ri, count, "p")
        end
      elseif reduced_encode_mode[ri] == "U" then
        if current_mode == "L" and (next_mode == "L" or next_mode == "M") and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "u")
        elseif current_mode == "L"
            and (next_mode == "E" or next_mode == "D" or next_mode == "B" or next_mode == "P")
            and count == 1 then
          reduced_encode_mode[ri] = "u"
        elseif current_mode == "D" and next_mode == "D" and count == 1 then
          reduced_encode_mode[ri] = "u"
        elseif current_mode == "D" and next_mode == "P" and count <= 2 then
          fill_mode(reduced_encode_mode, ri, count, "u")
        end
      end
    end
    if reduced_encode_mode[ri] ~= "p" and reduced_encode_mode[ri] ~= "u"
        and reduced_encode_mode[ri] ~= "B" then
      current_mode = reduced_encode_mode[ri]
    end
  end

  -- Initial FNC1. The 5-bit punct shift is hardcoded: this fires at the start
  -- of the first segment, which is Upper. Later segments see bp > gs1_bp.
  if bp == gs1_bp and gs1 then
    bp = bin_append(0, 5, binary, bp)
    bp = bin_append(0, 5, binary, bp)
    bp = bin_append(0, 3, binary, bp)
  end

  if eci ~= 0 then
    bp = bin_append(0, initial_mode == "D" and 4 or 5, binary, bp)
    bp = bin_append(0, 5, binary, bp)
    if eci <= 9 then
      bp = bin_append(1, 3, binary, bp)
      bp = bin_append(2 + eci, 4, binary, bp)
    elseif eci <= 99 then
      bp = bin_append(2, 3, binary, bp)
      bp = bin_append(2 + eci // 10, 4, binary, bp)
      bp = bin_append(2 + eci % 10, 4, binary, bp)
    elseif eci <= 999 then
      bp = bin_append(3, 3, binary, bp)
      bp = bin_append(2 + eci // 100, 4, binary, bp)
      bp = bin_append(2 + (eci % 100) // 10, 4, binary, bp)
      bp = bin_append(2 + eci % 10, 4, binary, bp)
    elseif eci <= 9999 then
      bp = bin_append(4, 3, binary, bp)
      bp = bin_append(2 + eci // 1000, 4, binary, bp)
      bp = bin_append(2 + (eci % 1000) // 100, 4, binary, bp)
      bp = bin_append(2 + (eci % 100) // 10, 4, binary, bp)
      bp = bin_append(2 + eci % 10, 4, binary, bp)
    elseif eci <= 99999 then
      bp = bin_append(5, 3, binary, bp)
      bp = bin_append(2 + eci // 10000, 4, binary, bp)
      bp = bin_append(2 + (eci % 10000) // 1000, 4, binary, bp)
      bp = bin_append(2 + (eci % 1000) // 100, 4, binary, bp)
      bp = bin_append(2 + (eci % 100) // 10, 4, binary, bp)
      bp = bin_append(2 + eci % 10, 4, binary, bp)
    else
      bp = bin_append(6, 3, binary, bp)
      bp = bin_append(2 + eci // 100000, 4, binary, bp)
      bp = bin_append(2 + (eci % 100000) // 10000, 4, binary, bp)
      bp = bin_append(2 + (eci % 10000) // 1000, 4, binary, bp)
      bp = bin_append(2 + (eci % 1000) // 100, 4, binary, bp)
      bp = bin_append(2 + (eci % 100) // 10, 4, binary, bp)
      bp = bin_append(2 + eci % 10, 4, binary, bp)
    end
  end

  local function put_sym(mode, ch)
    if mode == "U" or mode == "u" then
      if ch == SP then return app(1, 5) end
      return app(tabs.AztecSymbolChar[ch], 5)
    elseif mode == "L" then
      if ch == SP then return app(1, 5) end
      return app(tabs.AztecSymbolChar[ch], 5)
    elseif mode == "M" then
      if ch == SP then return app(1, 5) end
      if ch == CR then return app(14, 5) end
      return app(tabs.AztecSymbolChar[ch], 5)
    elseif mode == "P" or mode == "p" then
      if gs1 and ch == GS then
        if not app(0, 5) then return false end
        return app(0, 3)
      elseif ch == CR then
        return app(1, 5)
      elseif ch == MARK_A then
        return app(2, 5)
      elseif ch == MARK_B then
        return app(3, 5)
      elseif ch == MARK_C then
        return app(4, 5)
      elseif ch == MARK_D then
        return app(5, 5)
      elseif ch == COMMA then
        return app(17, 5)
      elseif ch == DOT then
        return app(19, 5)
      end
      return app(tabs.AztecSymbolChar[ch], 5)
    elseif mode == "D" then
      if ch == SP then return app(1, 4) end
      if ch == COMMA then return app(12, 4) end
      if ch == DOT then return app(13, 4) end
      return app(tabs.AztecSymbolChar[ch], 4)
    end
    return true
  end

  -- Byte mode is a shift. From Punct or Digit the C latches to Upper first,
  -- because the shift returns there. `current_mode` is not left at "B".
  local function latch(from, to)
    if from == "U" then
      if to == "L" then return app(28, 5)
      elseif to == "M" then return app(29, 5)
      elseif to == "P" then
        if not app(29, 5) then return false end
        return app(30, 5)
      elseif to == "p" then return app(0, 5)
      elseif to == "D" then return app(30, 5)
      elseif to == "B" then return app(31, 5)
      end
    elseif from == "L" then
      if to == "U" then
        if not app(30, 5) then return false end
        return app(14, 4)
      elseif to == "u" then return app(28, 5)
      elseif to == "M" then return app(29, 5)
      elseif to == "P" then
        if not app(29, 5) then return false end
        return app(30, 5)
      elseif to == "p" then return app(0, 5)
      elseif to == "D" then return app(30, 5)
      elseif to == "B" then return app(31, 5)
      end
    elseif from == "M" then
      if to == "U" then return app(29, 5)
      elseif to == "L" then return app(28, 5)
      elseif to == "P" then return app(30, 5)
      elseif to == "p" then return app(0, 5)
      elseif to == "D" then
        if not app(29, 5) then return false end
        return app(30, 5)
      elseif to == "B" then return app(31, 5)
      end
    elseif from == "P" then
      if to == "U" then return app(31, 5)
      elseif to == "L" then
        if not app(31, 5) then return false end
        return app(28, 5)
      elseif to == "M" then
        if not app(31, 5) then return false end
        return app(29, 5)
      elseif to == "D" then
        if not app(31, 5) then return false end
        return app(30, 5)
      elseif to == "B" then
        if not app(31, 5) then return false end
        current_mode = "U"
        return app(31, 5)
      end
    elseif from == "D" then
      if to == "U" then return app(14, 4)
      elseif to == "u" then return app(15, 4)
      elseif to == "L" then
        if not app(14, 4) then return false end
        return app(28, 5)
      elseif to == "M" then
        if not app(14, 4) then return false end
        return app(29, 5)
      elseif to == "P" then
        if not app(14, 4) then return false end
        if not app(29, 5) then return false end
        return app(30, 5)
      elseif to == "p" then return app(0, 4)
      elseif to == "B" then
        if not app(14, 4) then return false end
        current_mode = "U"
        return app(31, 5)
      end
    end
    return true
  end

  local function encode_bytes(at)
    local count = 0
    while at + count < reduced_length and reduced_encode_mode[at + count] == "B" do
      count = count + 1
    end
    if count > 2047 + 2078 then return nil end
    local big_batch = 0
    if count > 2047 then
      big_batch = count > 2078 and 2078 or count
      if not app(big_batch - 31, 16) then return nil end
      for _ = 1, big_batch do
        if not app(reduced_source[at], 8) then return nil end
        at = at + 1
      end
      count = count - big_batch
    end
    if count ~= 0 then
      if big_batch ~= 0 then
        if not app(31, 5) then return nil end
      end
      if count > 31 then
        if not app(count - 31, 16) then return nil end
      else
        if not app(count, 5) then return nil end
      end
      for _ = 1, count do
        if not app(reduced_source[at], 8) then return nil end
        at = at + 1
      end
    end
    return at
  end

  current_mode = initial_mode
  i = 0
  while i < reduced_length do
    local mode = reduced_encode_mode[i]
    if mode ~= current_mode then
      if not latch(current_mode, mode) then return nil end
      if mode == "B" then
        -- The C `continue`s past the latch update, so a shift is not a latch.
        local ni = encode_bytes(i)
        if not ni then return nil end
        i = ni
      else
        if mode ~= "u" and mode ~= "p" then
          current_mode = mode
        end
        if not put_sym(mode, reduced_source[i]) then return nil end
        i = i + 1
      end
    else
      if not put_sym(mode, reduced_source[i]) then return nil end
      i = i + 1
    end
  end

  return bp
end

-- ISO/IEC 24778:2008 7.3.1.2. A codeword whose first B-1 bits are all 0 or all
-- 1 gains a dummy opposite bit in the last position, and the real bit starts
-- the next codeword. Returns the new length, or 0 when it will not fit.
local function az_bitrun_stuff(binary, data_length, codeword_size, data_maxsize, adjusted)
  local j, count = 0, 0
  for bi = 0, data_length - 1 do
    if (j + 1) % codeword_size == 0 then
      if count == 0 or count == codeword_size - 1 then
        if j > data_maxsize then return 0 end
        adjusted[j] = count == 0 and "1" or "0"
        j = j + 1
        count = binary[bi] == "1" and 1 or 0
      else
        count = 0
      end
    elseif binary[bi] == "1" then
      count = count + 1
    end
    if j > data_maxsize then return 0 end
    adjusted[j] = binary[bi]
    j = j + 1
  end
  return j
end

-- Pad with 1s out to a codeword boundary. A codeword that is then all 1s
-- has its last bit flipped to 0, including when the pad itself caused it.
local function az_add_padding(padbits, codeword_size, adjusted, adjusted_length)
  for _ = 1, padbits do
    adjusted[adjusted_length] = "1"
    adjusted_length = adjusted_length + 1
  end
  local count = 0
  for pi = adjusted_length - codeword_size, adjusted_length - 1 do
    if adjusted[pi] == "1" then count = count + 1 end
  end
  if count == codeword_size then
    adjusted[adjusted_length - 1] = "0"
  end
  return adjusted_length
end

-- Table 3. Layers are 1-based, matching the C's `layers` after it is chosen.
local function az_codeword_size(layers)
  if layers <= 2 then return 6
  elseif layers <= 8 then return 8
  elseif layers <= 22 then return 10
  else return 12
  end
end

-- Skip the reference-grid columns and rows. Positions past 10 grow by one
-- every 15 modules.
local function az_avoid(output)
  if output > 10 then
    output = output + (output - 11) // 15 + 1
  end
  return output
end

local function az_populate_map(AztecMap, layers)
  local offset = tabs.AztecOffset[layers - 1]
  local endoffset = 151 - offset
  for layer = 0, layers - 1 do
    local start = (112 * layer) + (16 * layer * layer) + 2
    local length = 28 + (layer * 4) + ((layer + 1) * 4)
    local n = start
    local x = 64 - (layer * 2)
    local y = 63 - (layer * 2)
    local av0 = az_avoid(y) * 151
    local av1 = az_avoid(y - 1) * 151
    local endn = start + length
    while n < endn do
      local avxi = az_avoid(x)
      x = x + 1
      AztecMap[av0 + avxi] = n
      n = n + 1
      AztecMap[av1 + avxi] = n
      n = n + 1
    end
    x = 78 + (layer * 2)
    y = 64 - (layer * 2)
    av0 = az_avoid(x)
    av1 = az_avoid(x + 1)
    endn = endn + length
    while n < endn do
      local avyi = az_avoid(y) * 151
      y = y + 1
      AztecMap[avyi + av0] = n
      n = n + 1
      AztecMap[avyi + av1] = n
      n = n + 1
    end
    x = 77 + (layer * 2)
    y = 78 + (layer * 2)
    av0 = az_avoid(y) * 151
    av1 = az_avoid(y + 1) * 151
    endn = endn + length
    while n < endn do
      local avxi = az_avoid(x)
      x = x - 1
      AztecMap[av0 + avxi] = n
      n = n + 1
      AztecMap[av1 + avxi] = n
      n = n + 1
    end
    x = 63 - (layer * 2)
    y = 77 + (layer * 2)
    av0 = az_avoid(x)
    av1 = az_avoid(x - 1)
    endn = endn + length
    while n < endn do
      local avyi = az_avoid(y) * 151
      y = y - 1
      AztecMap[avyi + av0] = n
      n = n + 1
      AztecMap[avyi + av1] = n
      n = n + 1
    end
  end

  for y = 0, 14 do
    local row = tabs.AztecMapCore[y]
    local dest = (y + 68) * 151 + 68
    for x = 0, 14 do
      AztecMap[dest + x] = row[x]
    end
  end

  local gy = offset <= 11 and 11 or tabs.AztecMapGridYOffsets[(offset - 11) // 16]
  while gy < endoffset do
    for x = offset, endoffset - 1 do
      AztecMap[(x * 151) + gy] = x & 1
      AztecMap[(gy * 151) + x] = x & 1
    end
    gy = gy + 16
  end
end

local function rs_data(codeword_size, data_blocks, ecc_blocks, data_part, ecc_part)
  if codeword_size == 6 then
    local rs = reedsol.init_gf(0x43)
    reedsol.init_code(rs, ecc_blocks, 1)
    reedsol.encode_uint(rs, data_blocks, data_part, ecc_part)
  elseif codeword_size == 8 then
    local rs = reedsol.init_gf(0x12d)
    reedsol.init_code(rs, ecc_blocks, 1)
    reedsol.encode_uint(rs, data_blocks, data_part, ecc_part)
  elseif codeword_size == 10 then
    local rs = reedsol.uint_init_gf(0x409, 1023)
    reedsol.uint_init_code(rs, ecc_blocks, 1)
    reedsol.uint_encode(rs, data_blocks, data_part, ecc_part)
  else
    local rs = reedsol.uint_init_gf(0x1069, 4095)
    reedsol.uint_init_code(rs, ecc_blocks, 1)
    reedsol.uint_encode(rs, data_blocks, data_part, ecc_part)
  end
end

local function plot_symbol(symbol, compact, layers, bit_pattern)
  local dim
  if compact then
    local offset = tabs.AztecCompactOffset[layers - 1]
    local end_offset = 27 - offset
    for y = offset, end_offset - 1 do
      local y_map = y * 27
      for x = offset, end_offset - 1 do
        local map = tabs.AztecCompactMap[y_map + x]
        if map == 1 or (map >= 2 and bit_pattern[map - 2] == "1") then
          common.set_module(symbol, y - offset, x - offset)
        end
      end
      symbol.row_height[y - offset] = 1
    end
    dim = 27 - (2 * offset)
  else
    local offset = tabs.AztecOffset[layers - 1]
    local end_offset = 151 - offset
    local AztecMap = {}
    for i = 0, AZTEC_MAP_SIZE - 1 do AztecMap[i] = 0 end
    az_populate_map(AztecMap, layers)
    for y = offset, end_offset - 1 do
      local y_map = y * 151
      for x = offset, end_offset - 1 do
        local map = AztecMap[y_map + x]
        if map == 1 or (map >= 2 and bit_pattern[map - 2] == "1") then
          common.set_module(symbol, y - offset, x - offset)
        end
      end
      symbol.row_height[y - offset] = 1
    end
    dim = 151 - (2 * offset)
  end
  symbol.height = dim
  symbol.rows = dim
  symbol.width = dim
end

function M.aztec(symbol, source)
  local gs1 = (symbol.input_mode & 0x07) == common.GS1_MODE
  local reader_init = (symbol.output_options & common.READER_INIT) ~= 0
  if gs1 and reader_init then
    return nil, "Error 501: Cannot use Reader Initialisation in GS1 mode"
  end
  -- structapp.count is 0. Nothing is prepended and gs1_bp stays 0.
  if gs1 then
    source = source:gsub("%[", "\29")
  end

  local binary = {}
  local data_length = text_process(source and (function()
    local t = {}
    for n = 1, #source do t[n - 1] = source:byte(n) end
    return t
  end)() or {}, source and #source or 0, 0, binary, gs1, 0, symbol.eci or 0, "U")
  if not data_length then
    return nil, "Error 502: Input too long, requires too many codewords (maximum 1661)"
  end

  local warn_body
  local opt1 = symbol.option_1
  if opt1 == nil then opt1 = -1 end
  if opt1 < -1 or opt1 > 4 then
    local body = format("503: Error correction level '%d' out of range (1 to 4)", opt1)
    if symbol.warn_level == 2 then
      return nil, "Error " .. body
    end
    warn_body = body .. ", ignoring"
    opt1 = -1
  end

  local compact_loop_start = reader_init and 1 or 4
  local adjusted = {}
  local compact, layers, codeword_size, data_maxsize, adjusted_length
  local opt2 = symbol.option_2
  if opt2 == nil then opt2 = 0 end

  if opt2 == 0 then
    local ecc_level = opt1
    if ecc_level <= 0 then ecc_level = 2 end
    local adjustment_size = 0
    repeat
      compact = false
      layers = 0
      data_maxsize = 0
      for sz = compact_loop_start, 1, -1 do
        if data_length + adjustment_size <= tabs.AztecCompactDataSizes[ecc_level - 1][sz - 1] then
          layers = sz
          compact = true
          data_maxsize = tabs.AztecCompactDataSizes[ecc_level - 1][sz - 1]
        end
      end
      if not compact then
        for sz = 32, 1, -1 do
          if data_length + adjustment_size <= tabs.AztecDataSizes[ecc_level - 1][sz - 1] then
            layers = sz
            compact = false
            data_maxsize = tabs.AztecDataSizes[ecc_level - 1][sz - 1]
          end
        end
      end
      if layers == 0 then
        local maxcw = tabs.AztecDataSizes[ecc_level - 1][31] // 12
        if adjustment_size == 0 then
          return nil, format(
            "Error 707: Input too long for ECC level %d, requires too many codewords (maximum %d)",
            ecc_level, maxcw)
        end
        return nil, format(
          "Error 504: Input too long for ECC level %d, requires %d codewords (maximum %d)",
          ecc_level, (data_length + adjustment_size + 11) // 12, maxcw)
      end

      codeword_size = az_codeword_size(layers)
      local stuff_limit = adjustment_size ~= 0 and data_maxsize or AZTEC_BIN_CAPACITY
      adjusted_length = az_bitrun_stuff(binary, data_length, codeword_size, stuff_limit, adjusted)
      if adjusted_length == 0 then
        return nil, format(
          "Error 705: Input too long for ECC level %d, requires too many codewords (maximum %d)",
          ecc_level, stuff_limit // codeword_size)
      end
      adjustment_size = adjusted_length - data_length

      local remainder = adjusted_length % codeword_size
      local padbits = codeword_size - remainder
      if padbits == codeword_size then padbits = 0 end
      adjusted_length = az_add_padding(padbits, codeword_size, adjusted, adjusted_length)
    until adjusted_length <= data_maxsize

    symbol.option_2 = compact and layers or (layers + 4)
  else
    if opt2 < 0 or opt2 > 36 then
      return nil, format("Error 510: Version '%d' out of range (1 to 36)", opt2)
    end
    if reader_init then
      if opt2 >= 2 and opt2 <= 4 then
        opt2 = 5
        symbol.option_2 = 5
      elseif opt2 > 26 then
        return nil, format(
          "Error 709: Version '%d' out of range for Reader Initialisation symbols (maximum 26)",
          opt2)
      end
    end
    if opt2 <= 4 then
      compact = true
      layers = opt2
    else
      compact = false
      layers = opt2 - 4
    end
    codeword_size = az_codeword_size(layers)
    if compact then
      data_maxsize = codeword_size * (tabs.AztecCompactSizes[layers - 1] - 3)
    else
      data_maxsize = codeword_size * (tabs.AztecSizes[layers - 1] - 3)
    end
    adjusted_length = az_bitrun_stuff(binary, data_length, codeword_size, data_maxsize, adjusted)
    if adjusted_length == 0 then
      return nil, format(
        "Error 704: Input too long for Version %d, requires too many codewords (maximum %d)",
        symbol.option_2, data_maxsize // codeword_size)
    end
    local remainder = adjusted_length % codeword_size
    local padbits = codeword_size - remainder
    if padbits == codeword_size then padbits = 0 end
    if adjusted_length + padbits > data_maxsize then
      return nil, format(
        "Error 505: Input too long for Version %d, requires %d codewords (maximum %d)",
        symbol.option_2, (adjusted_length + padbits) // codeword_size,
        data_maxsize // codeword_size)
    end
    adjusted_length = az_add_padding(padbits, codeword_size, adjusted, adjusted_length)
  end

  if reader_init and layers > 22 then
    return nil, format(
      "Error 506: Input too long for Reader Initialisation, requires %d layers (maximum 22)",
      layers)
  end

  local data_blocks = adjusted_length // codeword_size
  local ecc_blocks
  if compact then
    ecc_blocks = tabs.AztecCompactSizes[layers - 1] - data_blocks
    if layers == 4 then ecc_blocks = ecc_blocks + 12 end
  else
    ecc_blocks = tabs.AztecSizes[layers - 1] - data_blocks
  end

  if ecc_blocks == 3 then
    warn_body = "706: Number of ECC codewords 3 at minimum"
    symbol.option_1 = -1
  else
    -- (float)(ecc_blocks - 3) / (data_blocks + ecc_blocks), then the C's stripf.
    local ecc_ratio = stripf((ecc_blocks - 3) / (data_blocks + ecc_blocks))
    if ecc_ratio < stripf(0.05) then
      warn_body = format(
        "708: Number of ECC codewords %d less than 5%% + 3 of data codewords %d",
        ecc_blocks, data_blocks)
      symbol.option_1 = 0
    else
      local t165, t295, t43 = stripf(0.165), stripf(0.295), stripf(0.43)
      if ecc_ratio < t165 then
        symbol.option_1 = 1
      elseif ecc_ratio < t295 then
        symbol.option_1 = 2
      elseif ecc_ratio < t43 then
        symbol.option_1 = 3
      else
        symbol.option_1 = 4
      end
    end
    symbol.option_1 = symbol.option_1 | (math.floor(stripf(ecc_ratio * 100.0)) << 8)
  end

  local data_part, ecc_part = {}, {}
  for di = 0, data_blocks - 1 do data_part[di] = 0 end
  for ei = 0, ecc_blocks - 1 do ecc_part[ei] = 0 end
  for di = 0, data_blocks - 1 do
    local val = 0
    local base = di * codeword_size
    for p = 0, codeword_size - 1 do
      if adjusted[base + p] == "1" then
        val = val | (1 << (codeword_size - (p + 1)))
      end
    end
    data_part[di] = val
  end
  rs_data(codeword_size, data_blocks, ecc_blocks, data_part, ecc_part)
  for ei = 0, ecc_blocks - 1 do
    adjusted_length = bin_append(ecc_part[ei], codeword_size, adjusted, adjusted_length)
  end

  -- Data on the outside, Reed-Solomon on the inside. Index 0 is the last bit.
  local bit_pattern = {}
  for bi = 0, AZTEC_MAP_POSN_MAX do bit_pattern[bi] = "0" end
  local total_bits = (data_blocks + ecc_blocks) * codeword_size
  for bi = 0, total_bits - 1 do
    bit_pattern[bi] = adjusted[total_bits - bi - 1]
  end

  local descriptor = {}
  local layers_m1 = layers - 1
  local dbm1 = data_blocks - 1
  if compact then
    descriptor[0] = (layers_m1 & 0x02) ~= 0 and "1" or "0"
    descriptor[1] = (layers_m1 & 0x01) ~= 0 and "1" or "0"
    descriptor[2] = (reader_init or (dbm1 & 0x20) ~= 0) and "1" or "0"
    for di = 3, 7 do
      descriptor[di] = (dbm1 & (0x10 >> (di - 3))) ~= 0 and "1" or "0"
    end
  else
    for di = 0, 4 do
      descriptor[di] = (layers_m1 & (0x10 >> di)) ~= 0 and "1" or "0"
    end
    descriptor[5] = (reader_init or (dbm1 & 0x400) ~= 0) and "1" or "0"
    for di = 6, 15 do
      descriptor[di] = (dbm1 & (0x200 >> (di - 6))) ~= 0 and "1" or "0"
    end
  end

  local desc_data, desc_ecc = {}, {}
  for di = 0, 3 do
    local o = di * 4
    desc_data[di] = ((descriptor[o] == "1") and 1 or 0) << 3
      | ((descriptor[o + 1] == "1") and 1 or 0) << 2
      | ((descriptor[o + 2] == "1") and 1 or 0) << 1
      | ((descriptor[o + 3] == "1") and 1 or 0)
  end
  local drs = reedsol.init_gf(0x13)
  if compact then
    reedsol.init_code(drs, 5, 1)
    reedsol.encode_c(drs, 2, desc_data, desc_ecc)
    for di = 0, 4 do
      local v = desc_ecc[di]
      local o = di * 4 + 8
      descriptor[o] = (v & 0x08) ~= 0 and "1" or "0"
      descriptor[o + 1] = (v & 0x04) ~= 0 and "1" or "0"
      descriptor[o + 2] = (v & 0x02) ~= 0 and "1" or "0"
      descriptor[o + 3] = (v & 0x01) ~= 0 and "1" or "0"
    end
  else
    reedsol.init_code(drs, 6, 1)
    reedsol.encode_c(drs, 4, desc_data, desc_ecc)
    for di = 0, 5 do
      local v = desc_ecc[di]
      local o = di * 4 + 16
      descriptor[o] = (v & 0x08) ~= 0 and "1" or "0"
      descriptor[o + 1] = (v & 0x04) ~= 0 and "1" or "0"
      descriptor[o + 2] = (v & 0x02) ~= 0 and "1" or "0"
      descriptor[o + 3] = (v & 0x01) ~= 0 and "1" or "0"
    end
  end

  local desc_at = compact and (2000 - 2) or (20000 - 2)
  for di = 0, 39 do
    bit_pattern[desc_at + di] = descriptor[di] == "1" and "1" or "0"
  end

  plot_symbol(symbol, compact, layers, bit_pattern)

  if warn_body then
    if symbol.warn_level == 2 then
      return nil, "Error " .. warn_body
    end
    symbol.warning = "Warning " .. warn_body
  end
  return true
end

return M
