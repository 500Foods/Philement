-- zint/maxicode.lua -- ports backend/maxicode.c `zint_maxicode`.
--
-- One segment. Structured Append is not encoded: the count is treated as 0,
-- so the two Structured Append codewords are never written. No human-readable
-- text; MaxiCode has none.
--
-- The text encoder is the BWIPP shortest-path search. It keeps 16 rows of
-- history because a codeword looks back at most 9 characters, and 16 is the
-- next power of two the C masks with. Rows before the start of the message
-- stay zero, which is "already in Code Set A, cost 0".
--
-- Modes 4, 5 and 6 write the message at index 11. The first 9 of those
-- codewords are copied into the primary (indexes 1..9) and the copy left
-- behind is overwritten by the primary ECC. Modes 2 and 3 build the primary
-- from the postcode instead, and the message starts at index 20.
--
-- Reed-Solomon is GF(0x43), first root a^1. `reedsol.encode_c` already
-- reverses the remainder, which is what `zint_rs_encode` returns. Copy
-- `ecc[j]` straight across. Reversing again scans wrong.

local common = require "zint.common"
local reedsol = require "zint.reedsol"
local tabs = require "zint.maxicode_tabs"

local M = {}

local code_set = tabs.code_set
local symbol_char = tabs.symbol_char
local grid = tabs.grid

local MX_A, MX_B, MX_E, MX_C, MX_D = 0, 1, 2, 3, 4
local MX_STATES = 5

local MX_OP_DGTS = 0
local MX_OP_SETA = 0x01
local MX_OP_SETB = 0x02
local MX_OP_SETE = 0x04
local MX_OP_SETC = 0x08
local MX_OP_SETD = 0x10
local MX_OP_SHA  = 0x20 | MX_OP_SETA
local MX_OP_2SHA = 0x40 | MX_OP_SETA
local MX_OP_3SHA = 0x80 | MX_OP_SETA
local MX_OP_SHB  = 0x20 | MX_OP_SETB
local MX_OP_SHE  = 0x20 | MX_OP_SETE
local MX_OP_SHC  = 0x20 | MX_OP_SETC
local MX_OP_SHD  = 0x20 | MX_OP_SETD

local MX_OP_DGTS_IDX = 0
local MX_OP_SHA_IDX  = 6

-- Row is the set being entered, column is the set being left.
-- A 2-long latch is two copies of the same codeword, so the backwards
-- write below does not reorder it.
local LATCH_SEQ = {
  [MX_A] = { [0] = { [0] = 0 }, { [0] = 63 }, { [0] = 58 }, { [0] = 58 }, { [0] = 58 } },
  [MX_B] = { [0] = { [0] = 63 }, { [0] = 0 }, { [0] = 63 }, { [0] = 63 }, { [0] = 63 } },
  [MX_E] = { [0] = { [0] = 62, 62 }, { [0] = 62, 62 }, { [0] = 0 }, { [0] = 62, 62 }, { [0] = 62, 62 } },
  [MX_C] = { [0] = { [0] = 60, 60 }, { [0] = 60, 60 }, { [0] = 60, 60 }, { [0] = 0 }, { [0] = 60, 60 } },
  [MX_D] = { [0] = { [0] = 61, 61 }, { [0] = 61, 61 }, { [0] = 61, 61 }, { [0] = 61, 61 }, { [0] = 0 } },
}
local LATCH_LEN = {
  [MX_A] = { [0] = 0, 1, 1, 1, 1 },
  [MX_B] = { [0] = 1, 0, 1, 1, 1 },
  [MX_E] = { [0] = 2, 2, 0, 2, 2 },
  [MX_C] = { [0] = 2, 2, 2, 0, 2 },
  [MX_D] = { [0] = 2, 2, 2, 2, 0 },
}

local OP_TAB = {
  [MX_OP_DGTS_IDX] = { op = MX_OP_DGTS, intake = 9 },
  { op = MX_OP_SETA, intake = 1 },
  { op = MX_OP_SETB, intake = 1 },
  { op = MX_OP_SETE, intake = 1 },
  { op = MX_OP_SETC, intake = 1 },
  { op = MX_OP_SETD, intake = 1 },
  { op = MX_OP_SHA,  intake = 1 },
  { op = MX_OP_2SHA, intake = 2 },
  { op = MX_OP_3SHA, intake = 3 },
  { op = MX_OP_SHB,  intake = 1 },
  { op = MX_OP_SHE,  intake = 1 },
  { op = MX_OP_SHC,  intake = 1 },
  { op = MX_OP_SHD,  intake = 1 },
}

-- Sentinel -1 ends each row. DGTS is chosen separately, not listed here.
local CODE_SET_OPS = {
  [MX_A] = { [0] = 1, 9, 10, 11, 12, -1 },
  [MX_B] = { [0] = 2, 6, 7, 8, 10, 11, 12, -1 },
  [MX_E] = { [0] = 3, 11, 12, -1 },
  [MX_C] = { [0] = 4, 10, 12, -1 },
  [MX_D] = { [0] = 5, 10, 11, -1 },
}

-- Code Set B values of the characters that also live in another set.
-- z_posn(" ,./:", ch) + 47.
local SETB_MULTI = {
  [32] = 47, [44] = 48, [46] = 49, [47] = 50, [58] = 51,
}

local function err(id, msg)
  return nil, "Error " .. id .. ": " .. msg
end

local function errf(id, fmt, ...)
  return nil, "Error " .. id .. ": " .. string.format(fmt, ...)
end

local function isdigit(c)
  return c >= 48 and c <= 57
end

local function to_int(bytes, i, n)
  local val = 0
  for k = 0, n - 1 do
    local c = bytes[i + k]
    if not isdigit(c) then return -1 end
    val = val * 10 + (c - 48)
  end
  return val
end

local function can(op, ch, num_a)
  if op == MX_OP_2SHA or op == MX_OP_3SHA then
    return num_a >= (op == MX_OP_3SHA and 3 or 2)
  end
  return (code_set[ch] & op) ~= 0
end

local function symbol_ch(op, ch)
  if code_set[ch] == (op & 0x1F) or (op & MX_OP_SETA) ~= 0 then
    return symbol_char[ch]
  end
  if (op & MX_OP_SETB) ~= 0 then
    local v = SETB_MULTI[ch]
    if v then return v end
  end
  if (op & MX_OP_SETE) ~= 0 and ch >= 28 and ch <= 30 then
    return ch + 4
  end
  if ch == 32 then return 59 end
  return ch
end

-- Encode one op, prepending. The caller walks the path from the end.
local function mx_enc(codewords, ci, source, i, op)
  if op == MX_OP_DGTS then
    local value = (source[i] - 48) * 100000000
                + (source[i + 1] - 48) * 10000000
                + (source[i + 2] - 48) * 1000000
                + (source[i + 3] - 48) * 100000
                + (source[i + 4] - 48) * 10000
                + (source[i + 5] - 48) * 1000
                + (source[i + 6] - 48) * 100
                + (source[i + 7] - 48) * 10
                + (source[i + 8] - 48)
    ci = ci - 1; codewords[ci] = value & 0x3F
    ci = ci - 1; codewords[ci] = (value & 0xFC0) >> 6
    ci = ci - 1; codewords[ci] = (value & 0x3F000) >> 12
    ci = ci - 1; codewords[ci] = (value & 0xFC0000) >> 18
    ci = ci - 1; codewords[ci] = (value & 0x3F000000) >> 24
    ci = ci - 1; codewords[ci] = 31
  elseif op == MX_OP_2SHA then
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i + 1])
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i])
    ci = ci - 1; codewords[ci] = 56
  elseif op == MX_OP_3SHA then
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i + 2])
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i + 1])
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i])
    ci = ci - 1; codewords[ci] = 57
  else
    ci = ci - 1; codewords[ci] = symbol_ch(op, source[i])
    if (op & 0x20) ~= 0 then
      local sh = 59
      if op == MX_OP_SHC then sh = sh + 1
      elseif op == MX_OP_SHD then sh = sh + 2
      elseif op == MX_OP_SHE then sh = sh + 3
      end
      ci = ci - 1
      codewords[ci] = sh
    end
  end
  return ci
end

local function eci_length(eci)
  if eci == 0 then return 0 end
  local n = 2
  if eci > 31 then n = n + 1 end
  if eci > 1023 then n = n + 1 end
  if eci > 32767 then n = n + 1 end
  return n
end

local function enc_eci(eci, codewords, ci)
  ci = ci - 1
  codewords[ci] = eci & 0x3F
  if eci > 31 then
    if eci > 1023 then
      ci = ci - 1
      codewords[ci] = (eci & 0xFC0) >> 6
      if eci > 32767 then
        ci = ci - 1
        codewords[ci] = (eci & 0x3F000) >> 12
        ci = ci - 1
        codewords[ci] = 0x38 | ((eci & 0xC0000) >> 18)
      else
        ci = ci - 1
        codewords[ci] = 0x30 | ((eci & 0x7000) >> 12)
      end
    else
      ci = ci - 1
      codewords[ci] = 0x20 | ((eci & 0x3C0) >> 6)
    end
  end
  ci = ci - 1
  codewords[ci] = 27
  return ci
end

local function get_best_length(state, i, ch, digits, num_a, best_lengths, best_origins, path_op, prior)
  local latch = LATCH_LEN[state]
  local min_len = 999999
  if digits >= 9 then
    local m = (i - 9) & 0x0F
    local org = best_origins[m][state]
    local len = best_lengths[m][org] + latch[org] + 6
    if len < min_len then
      path_op[state] = MX_OP_DGTS_IDX
      prior[state] = org
      min_len = len
    end
  else
    local ops = CODE_SET_OPS[state]
    local j = 0
    while ops[j] ~= -1 do
      local op_idx = ops[j]
      local op = OP_TAB[op_idx]
      if can(op.op, ch, num_a) then
        local m = (i - op.intake) & 0x0F
        local org = best_origins[m][state]
        local extra = op_idx >= MX_OP_SHA_IDX and 1 or 0
        local len = best_lengths[m][org] + latch[org] + op.intake + extra
        if len < min_len then
          path_op[state] = op_idx
          prior[state] = org
          min_len = len
        end
      end
      j = j + 1
    end
  end
  return min_len
end

local function get_best_origin(state, best_length)
  local latch = LATCH_LEN[state]
  local orglen = best_length[0] + latch[0]
  local best_org = 0
  for org = 1, MX_STATES - 1 do
    local len = best_length[org] + latch[org]
    if len < orglen then
      best_org = org
      orglen = len
    end
  end
  return best_org
end

local function has_scm_prefix(source, length)
  if length < 9 then return false end
  if source[0] ~= 0x5B or source[1] ~= 0x29 or source[2] ~= 0x3E
      or source[3] ~= 0x1E or source[4] ~= 0x30 or source[5] ~= 0x31
      or source[6] ~= 0x1D then
    return false
  end
  return isdigit(source[7]) and isdigit(source[8])
end

local TOO_LONG = "Error 553: Input too long, requires too many codewords (maximum 144)"

local function text_process(codewords, mode, raw, eci, scm_vv)
  local source = {}
  local length = #raw
  for i = 1, length do source[i - 1] = raw:byte(i) end

  -- "[)>" RS "01" GS vv. C writes the two controls as octal; Lua \ddd is decimal.
  local have_eci_scm = false
  if scm_vv ~= -1 then
    local pstr = "[)>" .. "\30" .. "01" .. "\29" .. string.format("%02d", scm_vv)
    local prefixed = {}
    for i = 1, 9 do prefixed[i - 1] = pstr:byte(i) end
    for i = 0, length - 1 do prefixed[i + 9] = source[i] end
    source = prefixed
    length = length + 9
    have_eci_scm = eci ~= 0
  elseif has_scm_prefix(source, length) then
    have_eci_scm = eci ~= 0
  end

  local cp = mode > 3 and 11 or 20
  local max_len = (mode == 5 and 77 or 93) + 11

  local best_lengths, best_origins = {}, {}
  for m = 0, 15 do
    best_lengths[m] = {}
    best_origins[m] = {}
    for s = 0, MX_STATES - 1 do
      best_lengths[m][s] = 0
      best_origins[m][s] = 0
    end
  end
  local path_ops, prior_sets = {}, {}

  local digits, num_a = 0, 0
  local ci = cp
  local elen = eci_length(eci)
  if elen ~= 0 then
    ci = ci + elen
    if ci > max_len then return TOO_LONG end
    digits = 0
  end

  -- A standalone zero row, used only when there is nothing to encode.
  -- The library never calls the encoder with an empty segment; this keeps
  -- a direct call from indexing a row that was never written.
  local best_length = {}
  for s = 0, MX_STATES - 1 do best_length[s] = 0 end

  local no_eci_scm_check = not have_eci_scm
  for i = 0, length - 1 do
    local ch = source[i]
    digits = (isdigit(ch) and (no_eci_scm_check or i >= 9)) and (digits + 1) or 0
    num_a = (code_set[ch] & MX_OP_SETA) ~= 0 and (num_a + 1) or 0

    local mcur = i & 0x0F
    best_length = best_lengths[mcur]
    local best_origin = best_origins[mcur]
    local path_op, prior = {}, {}
    for s = 0, MX_STATES - 1 do
      path_op[s] = 0
      prior[s] = 0
    end
    path_ops[i] = path_op
    prior_sets[i] = prior

    for state = 0, MX_STATES - 1 do
      best_length[state] = get_best_length(state, i, ch, digits, num_a,
        best_lengths, best_origins, path_op, prior)
    end
    for state = 0, MX_STATES - 1 do
      best_origin[state] = get_best_origin(state, best_length)
    end
  end

  local min_len, min_state = 999999, 0
  for state = 0, MX_STATES - 1 do
    local len = best_length[state]
    if len < min_len then
      min_state = state
      min_len = len
    end
  end
  if ci + min_len > max_len then return TOO_LONG end

  ci = ci + min_len
  local ci_top = ci
  local state = min_state
  local i = length
  local eci_scm_check = have_eci_scm
  while i > 0 do
    local ch_i = i - 1
    local pcs = prior_sets[ch_i][state]
    local op = OP_TAB[path_ops[ch_i][state]]
    if eci_scm_check and i == 9 then
      ci = enc_eci(eci, codewords, ci)
      eci = 0
    end
    i = i - op.intake
    ci = mx_enc(codewords, ci, source, i, op.op)
    if state ~= pcs then
      local llen = LATCH_LEN[state][pcs]
      local seq = LATCH_SEQ[state][pcs]
      for j = 0, llen - 1 do
        ci = ci - 1
        codewords[ci] = seq[j]
      end
      state = pcs
    end
  end
  if eci ~= 0 then
    ci = enc_eci(eci, codewords, ci)
  end

  cp = ci_top
  if cp < max_len and (min_state == MX_C or min_state == MX_D) then
    codewords[cp] = 58
    cp = cp + 1
  end
  if cp < max_len then
    local pad = min_state == MX_E and 28 or 33
    for j = cp, max_len - 1 do codewords[j] = pad end
  end
  if cp > max_len then return TOO_LONG end

  if mode > 3 then
    for j = 0, 8 do codewords[1 + j] = codewords[11 + j] end
  end
  return nil
end

local function do_primary_ecc(codewords)
  local rs = reedsol.init_gf(0x43)
  reedsol.init_code(rs, 10, 1)
  local ecc = {}
  reedsol.encode_c(rs, 10, codewords, ecc)
  for j = 0, 9 do codewords[10 + j] = ecc[j] end
end

local function do_secondary_ecc(codewords, datalen, eclen)
  local rs = reedsol.init_gf(0x43)
  reedsol.init_code(rs, eclen, 1)
  local data, ecc = {}, {}
  for j = 0, datalen - 1, 2 do
    data[j >> 1] = codewords[j + 20]
  end
  reedsol.encode_c(rs, datalen >> 1, data, ecc)
  for j = 0, eclen - 1 do
    codewords[datalen + (j << 1) + 20] = ecc[j]
  end
  for j = 0, datalen - 1, 2 do
    data[j >> 1] = codewords[j + 1 + 20]
  end
  reedsol.encode_c(rs, datalen >> 1, data, ecc)
  for j = 0, eclen - 1 do
    codewords[datalen + (j << 1) + 1 + 20] = ecc[j]
  end
end

local function do_primary_2(codewords, postcode_num, postcode_len, country, service)
  codewords[0] = ((postcode_num & 0x03) << 4) | 2
  codewords[1] = (postcode_num & 0xFC) >> 2
  codewords[2] = (postcode_num & 0x3F00) >> 8
  codewords[3] = (postcode_num & 0xFC000) >> 14
  codewords[4] = (postcode_num & 0x3F00000) >> 20
  codewords[5] = ((postcode_num & 0x3C000000) >> 26) | ((postcode_len & 0x03) << 4)
  codewords[6] = ((postcode_len & 0x3C) >> 2) | ((country & 0x03) << 4)
  codewords[7] = (country & 0xFC) >> 2
  codewords[8] = ((country & 0x300) >> 8) | ((service & 0x0F) << 2)
  codewords[9] = (service & 0x3F0) >> 4
end

local function do_primary_3(codewords, postcode, country, service)
  for i = 0, 5 do postcode[i] = symbol_char[postcode[i]] end
  codewords[0] = ((postcode[5] & 0x03) << 4) | 3
  codewords[1] = ((postcode[4] & 0x03) << 4) | ((postcode[5] & 0x3C) >> 2)
  codewords[2] = ((postcode[3] & 0x03) << 4) | ((postcode[4] & 0x3C) >> 2)
  codewords[3] = ((postcode[2] & 0x03) << 4) | ((postcode[3] & 0x3C) >> 2)
  codewords[4] = ((postcode[1] & 0x03) << 4) | ((postcode[2] & 0x3C) >> 2)
  codewords[5] = ((postcode[0] & 0x03) << 4) | ((postcode[1] & 0x3C) >> 2)
  codewords[6] = ((postcode[0] & 0x3C) >> 2) | ((country & 0x03) << 4)
  codewords[7] = (country & 0xFC) >> 2
  codewords[8] = ((country & 0x300) >> 8) | ((service & 0x0F) << 2)
  codewords[9] = (service & 0x3F0) >> 4
end

local MARKERS = {
  { 0, 28 }, { 0, 29 },
  { 9, 10 }, { 9, 11 }, { 10, 11 },
  { 15, 7 },
  { 16, 8 },
  { 16, 20 }, { 17, 20 },
  { 22, 10 }, { 23, 10 },
  { 22, 17 }, { 23, 17 },
}

function M.maxicode(symbol, source)
  local mode = symbol.option_1
  if mode == nil then mode = -1 end
  local primary = symbol.primary or ""
  local eci = symbol.eci or 0
  local lp = 0

  local codewords = {}
  for i = 0, 143 do codewords[i] = 0 end

  if mode <= 0 then
    lp = #primary
    if lp == 0 then
      if mode == 0 then
        return err(554, "Primary Message empty")
      end
      mode = 4
    else
      mode = 2
      for i = 0, lp - 7 do
        local c = primary:byte(i + 1)
        if not isdigit(c) and c ~= 32 then
          mode = 3
          break
        end
      end
    end
  end

  if mode < 2 or mode > 6 then
    return errf(550, "Mode '%d' out of range (2 to 6)", mode)
  end

  local scm_vv = -1
  if mode <= 3 then
    if lp == 0 then lp = #primary end
    if lp < 7 or lp > 15 then
      if lp == 0 then
        return err(548, "Primary Message empty")
      end
      return errf(551, "Primary Message length %d wrong (7 to 15 characters required)", lp)
    end
    local postcode_len = lp - 6
    local pbytes = {}
    for i = 1, lp do pbytes[i - 1] = primary:byte(i) end
    local country = to_int(pbytes, postcode_len, 3)
    local service = to_int(pbytes, postcode_len + 3, 3)
    if country == -1 or service == -1 then
      return err(552, "Non-numeric country code or service class in Primary Message")
    end

    local postcode = {}
    for i = 0, postcode_len - 1 do postcode[i] = pbytes[i] end

    if mode == 2 then
      for i = 0, postcode_len - 1 do
        if postcode[i] == 32 then
          postcode_len = i
          break
        end
        if not isdigit(postcode[i]) then
          return err(555, "Non-numeric postcode in Primary Message")
        end
      end
      if country == 840 and postcode_len == 5 then
        postcode[5], postcode[6], postcode[7], postcode[8] = 48, 48, 48, 48
        postcode_len = 9
      end
      do_primary_2(codewords, to_int(postcode, 0, postcode_len), postcode_len, country, service)
    else
      local nup = postcode_len < 6 and postcode_len or 6
      for i = 0, nup - 1 do
        local c = postcode[i]
        if c >= 97 and c <= 122 then postcode[i] = c & 0x5F end
      end
      for i = postcode_len, 5 do postcode[i] = 32 end
      for i = 0, 5 do
        local c = postcode[i]
        if c < 32 or (code_set[c] & MX_OP_SETA) == 0 then
          return err(556, "Invalid character in postcode in Primary Message")
        end
      end
      do_primary_3(codewords, postcode, country, service)
    end

    -- option_2 is vv + 1. 0 means "no SCM prefix".
    local opt2 = symbol.option_2 or 0
    if opt2 ~= 0 then
      if opt2 < 0 or opt2 > 100 then
        return errf(557, "SCM prefix version '%d' out of range (1 to 100)", opt2)
      end
      if eci == 25 or (eci >= 33 and eci <= 35) then
        return errf(547, "SCM prefix can not be used with ECI %d (ECI must be ASCII compatible)", eci)
      end
      scm_vv = opt2 - 1
    end
  else
    codewords[0] = mode
  end

  symbol.option_1 = mode

  local terr = text_process(codewords, mode, source, eci, scm_vv)
  if terr then return nil, terr end

  do_primary_ecc(codewords)
  if mode == 5 then
    do_secondary_ecc(codewords, 68, 28)
  else
    do_secondary_ecc(codewords, 84, 20)
  end

  for y = 0, 32 do
    for x = 0, 29 do
      local mod_seq = grid[y * 30 + x] + 5
      local block = mod_seq // 6
      if block ~= 0 then
        local bit = 5 - (mod_seq % 6)
        if ((codewords[block - 1] >> bit) & 1) ~= 0 then
          common.set_module(symbol, y, x)
        end
      end
    end
  end
  for _, m in ipairs(MARKERS) do
    common.set_module(symbol, m[1], m[2])
  end

  symbol.width = 30
  symbol.rows = 33
  -- no_errtxt: a short --height must not become a warning here.
  local saved = symbol.warning
  common.set_height(symbol, 5.0, 0.0, 0.0)
  symbol.warning = saved
  return true
end

return M
