-- zint/pdf417.lua -- PDF417 and Compact PDF417.
--
-- backend/pdf417.c `zint_pdf417()` / `pdf_enc()`. One segment. MicroPDF417
-- is not encoded. Structured Append is not encoded (`structapp` is absent,
-- so the macro block is skipped). ECC is the mod-929 coefficient loop, not
-- reedsol.lua.
--
-- symbol.symbology 55 is standard PDF417. 56 is compact (one-module stop).
-- symbol.option_1 is the security level (-1 picks it). option_2 is columns
-- (0 automatic, 1..30). option_3 is rows (0 automatic, else 3..90).
-- Codeword arrays are 0-based.

local common = require "zint.common"
local tabs = require "zint.pdf417_tabs"

local M = {}

local coefrs = tabs.coefrs
local bitpattern = tabs.bitpattern

local PDF_ALP, PDF_LOW, PDF_MIX, PDF_PNC = 1, 2, 3, 4
local PDF_TEX, PDF_BYT, PDF_NUM = 4, 5, 6
local PDF_NUM_MODES = 6

local T_ALPHA, T_LOWER, T_MIXED, T_PUNCT = 1, 2, 4, 8
local T_ALWMX = T_ALPHA | T_LOWER | T_MIXED
local T_MXPNC = T_MIXED | T_PUNCT

local PDF_MAX_LEN = 2710
local BARCODE_PDF417COMP = 56

-- TEX sub-mode assignments. Values from backend/pdf417.c `pdf_asciix`.
local pdf_asciix = {
  [0] = 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 8, 0, 0, 12, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  7, 8, 8, 4, 12, 4, 4, 8, 8, 8, 12, 4, 12, 12, 12, 12,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 12, 8, 8, 4, 8, 8,
  8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 8, 4, 8,
  8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 8, 8, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}

-- TEX sub-mode values. backend/pdf417.c `pdf_asciiy`, indices 0..126.
local pdf_asciiy = {
  [0] = 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 15, 0, 0, 11, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  26, 10, 20, 15, 18, 21, 10, 28, 23, 24, 22, 20, 13, 16, 17, 19,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 0, 1, 23, 2, 25,
  3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 4, 5, 6, 24, 7,
  8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 21, 27, 9,
}

local function real_mode(m)
  if m <= PDF_TEX then return PDF_TEX end
  return m
end

local function table_to_mode(t)
  local add = 0
  if (t & 0x07) ~= 0 then add = 1 end
  return (t >> 1) + add
end

local function isdigit(b)
  return b >= 48 and b <= 57
end

local function cnt_digits(source, length, position)
  local i = position
  while i < length and isdigit(source[i]) do
    i = i + 1
  end
  return i - position
end

local function flag(cond)
  if cond then return 1 end
  return 0
end

local function err_id(id, msg)
  return nil, "Error " .. id .. ": " .. msg
end

-- C `round()` of a non-negative double: half away from zero.
local function c_round(x)
  return math.floor(x + 0.5)
end

local function pdf_quelmode(codeascii)
  if isdigit(codeascii) then return PDF_NUM end
  if pdf_asciix[codeascii] ~= 0 then return PDF_TEX end
  return PDF_BYT
end

local function pdf_textprocess_switch(curtable, newtable, chainet, wnet)
  if curtable == T_ALPHA then
    if newtable == T_LOWER then
      chainet[wnet] = 27
      wnet = wnet + 1
    elseif newtable == T_MIXED then
      chainet[wnet] = 28
      wnet = wnet + 1
    elseif newtable == T_PUNCT then
      chainet[wnet] = 28
      chainet[wnet + 1] = 25
      wnet = wnet + 2
    end
  elseif curtable == T_LOWER then
    if newtable == T_ALPHA then
      chainet[wnet] = 28
      chainet[wnet + 1] = 28
      wnet = wnet + 2
    elseif newtable == T_MIXED then
      chainet[wnet] = 28
      wnet = wnet + 1
    elseif newtable == T_PUNCT then
      chainet[wnet] = 28
      chainet[wnet + 1] = 25
      wnet = wnet + 2
    end
  elseif curtable == T_MIXED then
    if newtable == T_ALPHA then
      chainet[wnet] = 28
      wnet = wnet + 1
    elseif newtable == T_LOWER then
      chainet[wnet] = 27
      wnet = wnet + 1
    elseif newtable == T_PUNCT then
      chainet[wnet] = 25
      wnet = wnet + 1
    end
  elseif curtable == T_PUNCT then
    if newtable == T_ALPHA then
      chainet[wnet] = 29
      wnet = wnet + 1
    elseif newtable == T_LOWER then
      chainet[wnet] = 29
      chainet[wnet + 1] = 27
      wnet = wnet + 2
    elseif newtable == T_MIXED then
      chainet[wnet] = 29
      chainet[wnet + 1] = 28
      wnet = wnet + 2
    end
  end
  return wnet
end

local function pdf_text_num_length(liste, indexliste, start)
  local len = 0
  for i = start, indexliste - 1 do
    if liste[1][i] == PDF_BYT then break end
    len = len + liste[0][i]
    if len >= 5 then break end
  end
  return len
end

-- No-output twin of `pdf_textprocess()`. Returns the sub-mode length and the
-- table left active.
local function pdf_text_submode_length(chaine, start, length, curtable)
  local listet = {}
  for i = 0, length - 1 do
    listet[i] = pdf_asciix[chaine[start + i]]
  end
  local chainet = {}
  local wnet = 0
  for j = 0, length - 1 do
    if (listet[j] & curtable) ~= 0 then
      wnet = wnet + 1
    else
      local newtable
      local temporary = false
      if j == length - 1 or (listet[j] & listet[j + 1]) == 0 then
        if (listet[j] & T_ALPHA) ~= 0 and curtable == T_LOWER then
          wnet = wnet + 2
          temporary = true
        elseif (listet[j] & T_PUNCT) ~= 0 then
          wnet = wnet + 2
          temporary = true
        else
          newtable = listet[j]
        end
      else
        newtable = listet[j] & listet[j + 1]
      end
      if not temporary then
        if newtable == T_ALWMX then
          newtable = T_ALPHA
        elseif newtable == T_MXPNC then
          newtable = T_MIXED
        end
        wnet = pdf_textprocess_switch(curtable, newtable, chainet, wnet)
        curtable = newtable
        wnet = wnet + 1
      end
    end
  end
  return wnet, curtable
end

local function pdf_num_stay(chaine, indexliste, liste, i)
  if liste[0][i] >= 13 or (indexliste == 1 and liste[0][i] > 5) then
    return true
  end
  if liste[0][i] < 11 then
    return false
  end

  local curtable = T_ALPHA
  local not_tex = i == 0 or liste[1][i - 1] == PDF_BYT
  local last_len = 0
  if not not_tex then
    last_len, curtable = pdf_text_submode_length(chaine, liste[2][i - 1], liste[0][i - 1], curtable)
  end
  local last_ml = curtable == T_MIXED

  curtable = T_ALPHA
  not_tex = i == indexliste - 1 or liste[1][i + 1] == PDF_BYT
  local next_len = 0
  if not not_tex then
    next_len = pdf_text_submode_length(chaine, liste[2][i + 1], liste[0][i + 1], curtable)
  end
  local num_cws = ((last_len + 1) >> 1) + 1 + 4 + flag(liste[0][i] > 11) + 1
      + ((next_len + 1) >> 1)

  curtable = T_MIXED
  next_len = 0
  if not not_tex then
    next_len = pdf_text_submode_length(chaine, liste[2][i + 1], liste[0][i + 1], curtable)
  end
  local tex_cws = (last_len + flag(not last_ml) + liste[0][i] + next_len + 1) >> 1
  return num_cws <= tex_cws
end

-- In-place, matching the C: later reads of liste[i - 1] see consolidated rows.
local function pdf_appendix_d_encode(chaine, liste, indexliste)
  local i, last, stayintext = 0, 0, 0
  while i < indexliste do
    if liste[1][i] == PDF_NUM and pdf_num_stay(chaine, indexliste, liste, i) then
      liste[0][last] = liste[0][i]
      liste[1][last] = PDF_NUM
      liste[2][last] = liste[2][i]
      stayintext = 0
      last = last + 1
      i = i + 1
    elseif (liste[1][i] == PDF_TEX or liste[1][i] == PDF_NUM)
        and (stayintext ~= 0 or i == indexliste - 1 or liste[0][i] >= 5
          or pdf_text_num_length(liste, indexliste, i) >= 5) then
      liste[0][last] = liste[0][i]
      liste[1][last] = PDF_TEX
      liste[2][last] = liste[2][i]
      stayintext = 0
      local nexti = i + 1
      while nexti < indexliste do
        if liste[1][nexti] == PDF_NUM and pdf_num_stay(chaine, indexliste, liste, nexti) then
          break
        elseif liste[1][nexti] == PDF_BYT then
          break
        end
        liste[0][last] = liste[0][last] + liste[0][nexti]
        nexti = nexti + 1
      end
      last = last + 1
      i = nexti
    else
      liste[0][last] = liste[0][i]
      liste[1][last] = PDF_BYT
      liste[2][last] = liste[2][i]
      stayintext = 0
      local nexti = i + 1
      while nexti < indexliste do
        if liste[1][nexti] ~= PDF_BYT then
          if liste[0][last] == 1 and last > 0 and liste[1][last - 1] == PDF_TEX then
            stayintext = 1
            break
          end
          if liste[0][nexti] >= 5 or pdf_text_num_length(liste, indexliste, nexti) >= 5 then
            break
          end
        end
        liste[0][last] = liste[0][last] + liste[0][nexti]
        nexti = nexti + 1
      end
      last = last + 1
      i = nexti
    end
  end
  return last
end

local function pdf_textprocess_end(chainemc, mclength, is_last_seg, chainet, wnet, curtable)
  local tex_padded = wnet & 1
  if tex_padded ~= 0 then
    if is_last_seg then
      chainet[wnet] = 29
      wnet = wnet + 1
    else
      chainet[wnet] = 28 + flag(curtable == T_PUNCT)
      wnet = wnet + 1
      if curtable == T_ALPHA or curtable == T_LOWER then
        curtable = T_MIXED
      else
        curtable = T_ALPHA
      end
    end
  end
  local i = 0
  while i < wnet do
    chainemc[mclength] = (30 * chainet[i]) + chainet[i + 1]
    mclength = mclength + 1
    i = i + 2
  end
  return mclength, curtable, tex_padded
end

local function pdf_textprocess(chainemc, mclength, chaine, start, length, lastmode, is_last_seg, curtable)
  local real_last = real_mode(lastmode)
  if real_last ~= PDF_TEX then
    curtable = T_ALPHA
    chainemc[mclength] = 900
    mclength = mclength + 1
  end
  local listet0, listet1 = {}, {}
  for index = 0, length - 1 do
    local codeascii = chaine[start + index]
    listet0[index] = pdf_asciix[codeascii]
    listet1[index] = pdf_asciiy[codeascii]
  end
  local chainet = {}
  local wnet = 0
  for j = 0, length - 1 do
    if (listet0[j] & curtable) ~= 0 then
      chainet[wnet] = listet1[j]
      wnet = wnet + 1
    else
      local newtable
      local temporary = false
      if j == length - 1 or (listet0[j] & listet0[j + 1]) == 0 then
        if (listet0[j] & T_ALPHA) ~= 0 and curtable == T_LOWER then
          chainet[wnet] = 27
          chainet[wnet + 1] = listet1[j]
          wnet = wnet + 2
          temporary = true
        elseif (listet0[j] & T_PUNCT) ~= 0 then
          chainet[wnet] = 29
          chainet[wnet + 1] = listet1[j]
          wnet = wnet + 2
          temporary = true
        else
          newtable = listet0[j]
        end
      else
        newtable = listet0[j] & listet0[j + 1]
      end
      if not temporary then
        if newtable == T_ALWMX then
          newtable = T_ALPHA
        elseif newtable == T_MXPNC then
          newtable = T_MIXED
        end
        wnet = pdf_textprocess_switch(curtable, newtable, chainet, wnet)
        curtable = newtable
        chainet[wnet] = listet1[j]
        wnet = wnet + 1
      end
    end
  end
  return pdf_textprocess_end(chainemc, mclength, is_last_seg, chainet, wnet, curtable)
end

-- Returns mclength, curtable, tex_padded, and the index of the next segment.
local function pdf_textprocess_minimal(chainemc, mclength, chaine, liste, indexliste,
    lastmode, is_last_seg, curtable, i)
  local real_last = real_mode(lastmode)
  if real_last ~= PDF_TEX then
    curtable = T_ALPHA
    chainemc[mclength] = 900
    mclength = mclength + 1
  end
  local chainet = {}
  local wnet = 0
  local tex_padded
  local newtables = { [0] = 0, T_ALPHA, T_LOWER, T_MIXED, T_PUNCT }
  local ii = i
  while ii < indexliste and real_mode(liste[1][ii]) == PDF_TEX do
    local newtable = newtables[liste[1][ii]]
    local from = liste[2][ii]
    for j = 0, liste[0][ii] - 1 do
      local c = chaine[from + j]
      local t_table = pdf_asciix[c]
      if t_table == 0 then
        if (wnet & 1) ~= 0 then
          chainet[wnet] = 29
          wnet = wnet + 1
          if curtable == T_PUNCT then curtable = T_ALPHA end
        end
        local k = 0
        while k < wnet do
          chainemc[mclength] = (30 * chainet[k]) + chainet[k + 1]
          mclength = mclength + 1
          k = k + 2
        end
        chainemc[mclength] = 913
        chainemc[mclength + 1] = c
        mclength = mclength + 2
        wnet = 0
      else
        if newtable ~= curtable then
          wnet = pdf_textprocess_switch(curtable, newtable, chainet, wnet)
          curtable = newtable
        end
        if curtable == T_LOWER and t_table == T_ALPHA then
          chainet[wnet] = 27
          wnet = wnet + 1
        elseif curtable ~= T_PUNCT and (t_table & T_PUNCT) ~= 0
            and (curtable ~= T_MIXED or (t_table & T_MIXED) == 0) then
          chainet[wnet] = 29
          wnet = wnet + 1
        end
        chainet[wnet] = pdf_asciiy[c]
        wnet = wnet + 1
      end
    end
    ii = ii + 1
  end
  mclength, curtable, tex_padded = pdf_textprocess_end(
    chainemc, mclength, is_last_seg, chainet, wnet, curtable)
  return mclength, curtable, tex_padded, ii
end

local function pdf_byteprocess(chainemc, mclength, chaine, start, length, lastmode)
  local real_last = real_mode(lastmode)
  if length == 1 then
    chainemc[mclength] = real_last == PDF_TEX and 913 or 901
    chainemc[mclength + 1] = chaine[start]
    return mclength + 2
  end
  if length % 6 == 0 then
    chainemc[mclength] = 924
  else
    chainemc[mclength] = 901
  end
  mclength = mclength + 1
  local len = 0
  while len < length do
    local chunkLen = length - len
    if chunkLen >= 6 then
      chunkLen = 6
      len = len + chunkLen
      local total = 0
      repeat
        chunkLen = chunkLen - 1
        local mantisa = chaine[start]
        start = start + 1
        total = total | (mantisa << (chunkLen * 8))
      until chunkLen == 0
      chunkLen = 5
      repeat
        chunkLen = chunkLen - 1
        chainemc[mclength + chunkLen] = total % 900
        total = total // 900
      until chunkLen == 0
      mclength = mclength + 5
    else
      len = len + chunkLen
      while chunkLen > 0 do
        chainemc[mclength] = chaine[start]
        mclength = mclength + 1
        start = start + 1
        chunkLen = chunkLen - 1
      end
    end
  end
  return mclength
end

local function pdf_numbprocess(chainemc, mclength, chaine, start, length)
  chainemc[mclength] = 902
  mclength = mclength + 1
  local j = 0
  while j < length do
    local longueur = length - j
    if longueur > 44 then longueur = 44 end
    local len = longueur + 1
    local chainemod = {}
    chainemod[0] = 1
    for loop = 1, len - 1 do
      chainemod[loop] = chaine[start + loop + j - 1] - 48
    end
    local dummy = {}
    local dumlength = 0
    repeat
      local p = 0
      local nombre = 0
      for loop = 0, len - 1 do
        nombre = nombre * 10 + chainemod[loop]
        if nombre < 900 then
          if p ~= 0 then
            chainemod[p] = 0
            p = p + 1
          end
        else
          chainemod[p] = nombre // 900
          nombre = nombre - chainemod[p] * 900
          p = p + 1
        end
      end
      dummy[dumlength] = nombre
      dumlength = dumlength + 1
      len = p
    until p == 0
    for loop = dumlength - 1, 0, -1 do
      chainemc[mclength] = dummy[loop]
      mclength = mclength + 1
    end
    j = j + longueur
  end
  return mclength
end

local function pdf_table_length(source, length, position, t_table)
  local i = position
  while i < length and (pdf_asciix[source[i]] & t_table) ~= 0 do
    i = i + 1
  end
  return i - position
end

-- `previous` is the prior edge, or nil. `prev_index` is its slot (0 = none).
local function pdf_new_edge(mode, from, len, t_table, lastmode, prev_index, previous)
  local real = real_mode(mode)
  local previousMode, real_previousMode, units, size
  if previous then
    previousMode = previous.mode
    real_previousMode = real_mode(previousMode)
    if real ~= real_previousMode then
      size = previous.size + previous.unit_size + 1
      units = 0
    else
      size = previous.size
      units = previous.units
    end
  else
    previousMode = lastmode
    real_previousMode = real_mode(previousMode)
    prev_index = 0
    if real ~= real_previousMode or real_previousMode ~= PDF_TEX then
      size = 1
    else
      size = 0
    end
    units = 0
  end

  local unit_size = 0
  if mode == PDF_ALP then
    if t_table ~= 0 then
      if previousMode ~= mode and real_previousMode == PDF_TEX then
        units = units + 1 + flag(previousMode == PDF_LOW)
      end
      units = units + (1 + flag((t_table & T_ALPHA) == 0)) * len
    else
      if (units & 1) ~= 0 then units = units + 1 end
      if previousMode ~= mode and real_previousMode == PDF_TEX then
        units = units + 1 + flag(previousMode == PDF_LOW)
      end
      units = units + 4
    end
    unit_size = (units + 1) >> 1
  elseif mode == PDF_LOW then
    if t_table ~= 0 then
      if previousMode ~= mode then
        units = units + 1 + flag(previousMode == PDF_PNC)
      end
      units = units + (1 + flag((t_table & T_LOWER) == 0)) * len
    else
      if (units & 1) ~= 0 then units = units + 1 end
      if previousMode ~= mode then
        units = units + 1 + flag(previousMode == PDF_PNC)
      end
      units = units + 4
    end
    unit_size = (units + 1) >> 1
  elseif mode == PDF_MIX then
    if t_table ~= 0 then
      if previousMode ~= mode then
        units = units + 1 + flag(previousMode == PDF_PNC)
      end
      units = units + (1 + flag((t_table & T_MIXED) == 0)) * len
    else
      if (units & 1) ~= 0 then units = units + 1 end
      if previousMode ~= mode then
        units = units + 1 + flag(previousMode == PDF_PNC)
      end
      units = units + 4
    end
    unit_size = (units + 1) >> 1
  elseif mode == PDF_PNC then
    if t_table ~= 0 then
      if previousMode ~= mode then
        units = units + 1 + flag(previousMode ~= PDF_MIX)
      end
      units = units + len
    else
      if (units & 1) ~= 0 then
        units = units + 3
      elseif previousMode ~= mode then
        units = units + 1 + flag(previousMode ~= PDF_MIX)
      end
      units = units + 4
    end
    unit_size = (units + 1) >> 1
  elseif mode == PDF_BYT then
    units = units + len
    local dv = units // 6
    unit_size = dv * 5 + (units - dv * 6)
  elseif mode == PDF_NUM then
    units = units + len
    local dv = units // 44
    local md = units - dv * 44
    unit_size = dv * 15
    if md ~= 0 then unit_size = unit_size + md // 3 + 1 end
  end

  return {
    mode = mode,
    from = from,
    len = len,
    units = units,
    unit_size = unit_size,
    size = size,
    previous = prev_index,
  }, size + unit_size
end

-- C passes the candidate's units as `existing_units` and the stored edge's
-- units as `new_units`. The return is used as a boolean; 0 is false.
local function pdf_new_units_better(mode, existing_units, new_units)
  if real_mode(mode) == PDF_TEX then
    if (new_units & 1) ~= (existing_units & 1) then
      return existing_units & 1
    end
  elseif mode == PDF_BYT then
    local existing_md = existing_units % 6
    if new_units % 6 ~= existing_md then
      return existing_md
    end
  end
  return flag(new_units < existing_units)
end

local function pdf_add_edge(edges, mode, from, len, t_table, lastmode, prev_index, previous)
  local edge, new_size = pdf_new_edge(mode, from, len, t_table, lastmode, prev_index, previous)
  local v_ij = (from + len) * PDF_NUM_MODES + mode - 1
  local existing = edges[v_ij]
  if not existing or existing.mode == 0 then
    edges[v_ij] = edge
    return
  end
  local v_size = existing.size + existing.unit_size
  if v_size > new_size
      or (v_size == new_size and pdf_new_units_better(mode, edge.units, existing.units) ~= 0) then
    edges[v_ij] = edge
  end
end

local function pdf_add_edges(source, length, lastmode, edges, from, prev_index, previous)
  local c = source[from]
  local t_table = pdf_asciix[c]
  if (t_table & T_ALPHA) ~= 0 then
    local len = pdf_table_length(source, length, from, T_ALPHA)
    pdf_add_edge(edges, PDF_ALP, from, len, T_ALPHA, lastmode, prev_index, previous)
  end
  if t_table == 0 or (t_table & T_PUNCT) ~= 0 then
    pdf_add_edge(edges, PDF_ALP, from, 1, t_table & (~T_ALPHA), lastmode, prev_index, previous)
  end
  if (t_table & T_LOWER) ~= 0 then
    local len = pdf_table_length(source, length, from, T_LOWER)
    pdf_add_edge(edges, PDF_LOW, from, len, T_LOWER, lastmode, prev_index, previous)
  end
  if t_table == 0 or (t_table & (T_PUNCT | T_ALPHA)) ~= 0 then
    pdf_add_edge(edges, PDF_LOW, from, 1, t_table & (~T_LOWER), lastmode, prev_index, previous)
  end
  if (t_table & T_MIXED) ~= 0 then
    local len = pdf_table_length(source, length, from, T_MIXED)
    pdf_add_edge(edges, PDF_MIX, from, len, T_MIXED, lastmode, prev_index, previous)
    if len > 1 and isdigit(source[from + 1]) then
      pdf_add_edge(edges, PDF_MIX, from, 1, T_MIXED, lastmode, prev_index, previous)
    end
  end
  if t_table == 0 or (t_table & T_PUNCT) ~= 0 then
    pdf_add_edge(edges, PDF_MIX, from, 1, t_table & (~T_MIXED), lastmode, prev_index, previous)
  end
  if (t_table & T_PUNCT) ~= 0 then
    local len = pdf_table_length(source, length, from, T_PUNCT)
    pdf_add_edge(edges, PDF_PNC, from, len, T_PUNCT, lastmode, prev_index, previous)
  end
  if t_table == 0 then
    pdf_add_edge(edges, PDF_PNC, from, 1, t_table, lastmode, prev_index, previous)
  end
  if isdigit(c) then
    local len = cnt_digits(source, length, from)
    pdf_add_edge(edges, PDF_NUM, from, len, 0, lastmode, prev_index, previous)
  end
  pdf_add_edge(edges, PDF_BYT, from, 1, 0, lastmode, prev_index, previous)
end

local function pdf_define_modes(liste, source, length, lastmode)
  local edges = {}
  pdf_add_edges(source, length, lastmode, edges, 0, 0, nil)
  for i = 1, length - 1 do
    local v_i = i * PDF_NUM_MODES
    for j = 0, PDF_NUM_MODES - 1 do
      local e = edges[v_i + j]
      if e and e.mode ~= 0 then
        pdf_add_edges(source, length, lastmode, edges, i, v_i + j, e)
      end
    end
  end

  local v_i = length * PDF_NUM_MODES
  local minimalJ, minimalSize = -1, 2147483647
  for j = 0, PDF_NUM_MODES - 1 do
    local edge = edges[v_i + j]
    if edge and edge.mode ~= 0 then
      local edge_size = edge.size + edge.unit_size
      if edge_size < minimalSize then
        minimalSize = edge_size
        minimalJ = j
      end
    end
  end
  if minimalJ < 0 then
    return nil
  end

  local edge = edges[v_i + minimalJ]
  local mode_len = 0
  local mode_start = length
  while edge do
    local current_mode = edge.mode
    local current_from = edge.from
    mode_len = mode_len + edge.len
    if edge.previous ~= 0 then
      edge = edges[edge.previous]
    else
      edge = nil
    end
    if not edge or edge.mode ~= current_mode then
      mode_start = mode_start - 1
      liste[0][mode_start] = mode_len
      liste[1][mode_start] = current_mode
      liste[2][mode_start] = current_from
      mode_len = 0
    end
  end
  local indexliste = length - mode_start
  if mode_start ~= 0 then
    -- memmove: the source sits above dest and the ranges overlap when there
    -- are more modes than mode_start.
    local n0, n1, n2 = {}, {}, {}
    for k = 0, indexliste - 1 do
      n0[k] = liste[0][mode_start + k]
      n1[k] = liste[1][mode_start + k]
      n2[k] = liste[2][mode_start + k]
    end
    for k = 0, indexliste - 1 do
      liste[0][k] = n0[k]
      liste[1][k] = n1[k]
      liste[2][k] = n2[k]
    end
  end
  return indexliste
end

local function emit_eci(chainemc, mclength, eci)
  if eci <= 899 then
    chainemc[mclength] = 927
    chainemc[mclength + 1] = eci
    return mclength + 2
  elseif eci <= 810899 then
    chainemc[mclength] = 926
    chainemc[mclength + 1] = (eci // 900) - 1
    chainemc[mclength + 2] = eci % 900
    return mclength + 3
  end
  chainemc[mclength] = 925
  chainemc[mclength + 1] = eci - 810900
  return mclength + 2
end

-- One segment. Returns mclength, or nil and an error string.
local function pdf_initial(symbol, chaine, length, eci, chainemc)
  local liste = { [0] = {}, [1] = {}, [2] = {} }
  local fast = ((symbol.input_mode or 0) & common.FAST_MODE) ~= 0
  local indexliste
  -- Default mode is Text Compaction Alpha. curtable starts upper alpha.
  local lastmode = PDF_ALP
  local curtable = T_ALPHA
  local tex_padded = 0

  if fast then
    local mode = pdf_quelmode(chaine[0])
    local indexchaine = 0
    indexliste = 0
    repeat
      liste[1][indexliste] = mode
      liste[2][indexliste] = indexchaine
      liste[0][indexliste] = 0
      while indexchaine < length and liste[1][indexliste] == mode do
        liste[0][indexliste] = liste[0][indexliste] + 1
        indexchaine = indexchaine + 1
        if indexchaine < length then
          mode = pdf_quelmode(chaine[indexchaine])
        end
      end
      indexliste = indexliste + 1
    until indexchaine >= length
    indexliste = pdf_appendix_d_encode(chaine, liste, indexliste)
  else
    indexliste = pdf_define_modes(liste, chaine, length, lastmode)
    if not indexliste then
      return err_id(749, "Insufficient memory for mode buffers")
    end
  end

  local mclength = 0
  mclength = mclength + 1 -- length descriptor
  if ((symbol.output_options or 0) & common.READER_INIT) ~= 0 then
    chainemc[mclength] = 921
    mclength = mclength + 1
  end
  if eci ~= 0 then
    if eci > 811799 then
      return err_id(472, string.format("ECI code '%d' out of range (0 to 811799)", symbol.eci))
    end
    mclength = emit_eci(chainemc, mclength, eci)
  end

  local indexchaine = 0
  local i = 0
  while i < indexliste do
    local seg_len = liste[0][i]
    local real = real_mode(liste[1][i])
    if real == PDF_TEX then
      if fast then
        mclength, curtable, tex_padded = pdf_textprocess(
          chainemc, mclength, chaine, indexchaine, seg_len, lastmode, true, curtable)
        indexchaine = indexchaine + seg_len
        lastmode = PDF_ALP
        i = i + 1
      else
        local next_i
        mclength, curtable, tex_padded, next_i = pdf_textprocess_minimal(
          chainemc, mclength, chaine, liste, indexliste, lastmode, true, curtable, i)
        if next_i < indexliste then
          indexchaine = liste[2][next_i]
        else
          indexchaine = length
        end
        lastmode = table_to_mode(curtable)
        i = next_i
      end
    elseif real == PDF_BYT then
      mclength = pdf_byteprocess(chainemc, mclength, chaine, indexchaine, seg_len, lastmode)
      if real_mode(lastmode) ~= PDF_TEX or seg_len ~= 1 then
        lastmode = PDF_BYT
      elseif curtable == T_PUNCT and tex_padded ~= 0 then
        curtable = T_ALPHA
      end
      indexchaine = indexchaine + seg_len
      i = i + 1
    else
      mclength = pdf_numbprocess(chainemc, mclength, chaine, indexchaine, seg_len)
      lastmode = PDF_NUM
      indexchaine = indexchaine + seg_len
      i = i + 1
    end
  end
  return mclength
end

local function append_bits(symbol, row, x, val, nbits)
  for bit = nbits - 1, 0, -1 do
    if (val & (1 << bit)) ~= 0 then
      common.set_module(symbol, row, x)
    end
    x = x + 1
  end
  return x
end

-- `pdf_enc`. Returns nil, or nil and an error string. Sets symbol.warning
-- only for warnings raised inside this function (746 / 748). A warning
-- already on the symbol is left for `z_set_height` to replace, matching the
-- C: `pdf_enc`'s own error_number starts clear, so a 460/461 warning does
-- not suppress the height warning.
local function pdf_enc(symbol, chaine, length)
  if length > PDF_MAX_LEN then
    return err_id(463, string.format("Input length %d too long (maximum 2710)", length))
  end
  if length == 0 then
    return err_id(228, "No input data (segment 0 empty)")
  end

  local chainemc = {}
  local mclength, err = pdf_initial(symbol, chaine, length, symbol.eci or 0, chainemc)
  if not mclength then return nil, err end

  local ecc = symbol.option_1
  if ecc < 0 then
    local data_cws = mclength - 1 -- structapp_cp is 0
    if data_cws <= 40 then
      ecc = 2
    elseif data_cws <= 160 then
      ecc = 3
    elseif data_cws <= 320 then
      ecc = 4
    elseif data_cws <= 863 then
      ecc = 5
    else
      ecc = 6
    end
  end
  local ecc_cws = 2 << ecc
  local longueur = mclength + ecc_cws
  if longueur > 928 then
    return err_id(464, "Input too long, requires too many codewords (maximum 928)")
  end

  local cols = symbol.option_2
  local rows = symbol.option_3
  local enc_warn = false
  if rows ~= 0 then
    if cols < 1 then
      cols = (longueur + rows - 1) // rows
      if cols <= 1 then
        cols = 1
      else
        while cols > 30 and rows < 90 do
          rows = rows + 1
          cols = (longueur + rows - 1) // rows
        end
        while cols >= 1 and rows < 90 and rows * cols > 928 do
          rows = rows + 1
          cols = (longueur + rows - 1) // rows
        end
        if rows * cols > 928 then
          return err_id(465, "Input too long, requires too many codewords (maximum 928)")
        end
      end
    else
      while rows <= 90 and rows * cols < longueur do
        rows = rows + 1
      end
      if rows > 90 or rows * cols > 928 then
        return err_id(745, string.format("Input too long for number of columns '%d'", cols))
      end
    end
    if rows ~= symbol.option_3 then
      symbol.warning = string.format(
        "Warning 746: Number of rows increased from %d to %d", symbol.option_3, rows)
      enc_warn = true
    end
  else
    if cols < 1 then
      cols = c_round(math.sqrt((longueur - 1) / 3.0))
    end
    rows = (longueur + cols - 1) // cols
    if rows <= 3 then
      rows = 3
    else
      while rows > 90 and cols < 30 do
        cols = cols + 1
        rows = (longueur + cols - 1) // cols
      end
      while rows >= 3 and cols < 30 and rows * cols > 928 do
        cols = cols + 1
        rows = (longueur + cols - 1) // cols
      end
      if rows * cols > 928 then
        return err_id(747, "Input too long, requires too many codewords (maximum 928)")
      end
      if symbol.option_2 ~= 0 and cols ~= symbol.option_2 then
        symbol.warning = string.format(
          "Warning 748: Number of columns increased from %d to %d", symbol.option_2, cols)
        enc_warn = true
      end
    end
  end

  symbol.option_1 = ecc
  symbol.option_2 = cols
  symbol.option_3 = rows

  local padding = rows * cols - longueur
  for _ = 1, padding do
    chainemc[mclength] = 900
    mclength = mclength + 1
  end
  chainemc[0] = mclength

  local offset
  if ecc == 1 then offset = 2
  elseif ecc == 2 then offset = 6
  elseif ecc == 3 then offset = 14
  elseif ecc == 4 then offset = 30
  elseif ecc == 5 then offset = 62
  elseif ecc == 6 then offset = 126
  elseif ecc == 7 then offset = 254
  elseif ecc == 8 then offset = 510
  else offset = 0
  end

  local mccorrection = {}
  for j = 0, ecc_cws - 1 do mccorrection[j] = 0 end
  for i = 0, mclength - 1 do
    local total = (chainemc[i] + mccorrection[ecc_cws - 1]) % 929
    for j = ecc_cws - 1, 1, -1 do
      mccorrection[j] = (mccorrection[j - 1] + 929 - (total * coefrs[offset + j]) % 929) % 929
    end
    mccorrection[0] = (929 - (total * coefrs[offset]) % 929) % 929
  end
  for i = ecc_cws - 1, 0, -1 do
    local v = mccorrection[i]
    if v ~= 0 then
      chainemc[mclength] = 929 - v
    else
      chainemc[mclength] = 0
    end
    mclength = mclength + 1
  end

  local c1 = (rows - 1) // 3
  local c2 = ecc * 3 + (rows - 1) % 3
  local c3 = cols - 1
  local width = 0
  local compact = symbol.symbology == BARCODE_PDF417COMP
  for i = 0, rows - 1 do
    local k = (i // 3) * 30
    local dummy = {}
    for j = 0, cols - 1 do
      dummy[j + 1] = chainemc[i * cols + j]
    end
    local cluster
    local imod = i % 3
    if imod == 0 then
      dummy[0] = k + c1
      dummy[cols + 1] = k + c3
      cluster = 0
    elseif imod == 1 then
      dummy[0] = k + c2
      dummy[cols + 1] = k + c1
      cluster = 929
    else
      dummy[0] = k + c3
      dummy[cols + 1] = k + c2
      cluster = 1858
    end
    local x = append_bits(symbol, i, 0, 0x1FEA8, 17)
    for j = 0, cols do
      x = append_bits(symbol, i, x, bitpattern[cluster + dummy[j]], 16)
      x = x + 1
    end
    if not compact then
      x = append_bits(symbol, i, x, bitpattern[cluster + dummy[cols + 1]], 16)
      x = x + 1
      x = append_bits(symbol, i, x, 0x3FA29, 18)
    else
      common.set_module(symbol, i, x)
      x = x + 1
    end
    width = x
  end
  symbol.width = width
  symbol.rows = rows

  -- no_errtxt when this function already wrote a warning.
  local saved = symbol.warning
  common.set_height(symbol, 3.0, 0.0, 0.0)
  if enc_warn then symbol.warning = saved end
  return nil
end

function M.pdf417(symbol, source)
  local option_1 = symbol.option_1
  if option_1 == nil then option_1 = -1 end
  local option_2 = symbol.option_2 or 0
  local option_3 = symbol.option_3 or 0
  symbol.option_1 = option_1
  symbol.option_2 = option_2
  symbol.option_3 = option_3

  if option_1 < -1 or option_1 > 8 then
    symbol.warning = string.format(
      "Warning 460: Error correction level '%d' out of range (0 to 8), ignoring", option_1)
    symbol.option_1 = -1
  end
  if option_2 < 0 or option_2 > 30 then
    symbol.warning = string.format(
      "Warning 461: Number of columns '%d' out of range (1 to 30), ignoring", option_2)
    symbol.option_2 = 0
  end
  if option_3 ~= 0 and (option_3 < 3 or option_3 > 90) then
    symbol.warning = nil
    return err_id(466, string.format("Number of rows '%d' out of range (3 to 90)", option_3))
  end
  if symbol.option_2 ~= 0 and option_3 ~= 0 and symbol.option_2 * option_3 > 928 then
    symbol.warning = nil
    return err_id(475, string.format(
      "Columns x rows value '%d' out of range (1 to 928)", symbol.option_2 * option_3))
  end

  local chaine = {}
  for i = 1, #source do
    chaine[i - 1] = source:byte(i)
  end
  local _, err = pdf_enc(symbol, chaine, #source)
  if err then
    symbol.warning = nil
    return nil, err
  end
  return true
end

return M
