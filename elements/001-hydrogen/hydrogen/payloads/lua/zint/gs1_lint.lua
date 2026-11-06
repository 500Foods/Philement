-- src/zint/gs1_lint.lua -- GS1 Application Identifier checks.
--
-- Behavioral port of backend/gs1.c (the primitive checkers) and the generated
-- backend/gs1_lint.h from zint 2.16.0. The AI syntax is GS1 General
-- Specifications Release 25, January 2025, as that header encodes it.
--
-- libzint - the open source barcode library
-- Copyright (C) 2021-2025 Robin Stuart <rstuart114@gmail.com>
-- SPDX-License-Identifier: BSD-3-Clause
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
--
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
-- 3. Neither the name of the project nor the names of its contributors
--    may be used to endorse or promote products derived from this software
--    without specific prior written permission.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
-- POSSIBILITY OF SUCH DAMAGE.

local M = {}

local C82 = {
  0, 1, 82, 82, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 82,
  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 82, 82, 82, 82, 55, 82,
  56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
}

local C32 = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ"
local WEIGHTS = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
  59, 61, 67, 71, 73, 79, 83,
}

-- ISO 3166-1 and ISO 4217 bitsets, copied from backend/iso3166.h and iso4217.h.
local ISO3166_N = {
  0x10, 0x15, 0x11, 0x91, 0x11, 0x11, 0x1D, 0x11, 0x51, 0x15, 0x50, 0x14,
  0x11, 0x11, 0x11, 0x11, 0x10, 0x11, 0x11, 0x51, 0x44, 0xC4, 0x14, 0x91,
  0x11, 0x18, 0x51, 0x44, 0x84, 0xC7, 0x44, 0x45, 0x54, 0x54, 0x18, 0x00,
  0x11, 0x11, 0x11, 0x11, 0x11, 0x51, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
  0x11, 0x41, 0x11, 0x45, 0x46, 0x54, 0x44, 0x45, 0x44, 0x44, 0x44, 0x44,
  0x11, 0x10, 0x1D, 0x11, 0x11, 0x11, 0xE9, 0x10, 0x10, 0x44, 0x44, 0x44,
  0xB4, 0x87, 0x40, 0x11, 0x11, 0x11, 0x45, 0x44, 0x4C, 0x50, 0xD8, 0x44,
  0x44, 0x44, 0x45, 0xC0, 0x47, 0x10, 0x10, 0x13, 0x10, 0x11, 0x11, 0x15,
  0x11, 0x11, 0x11, 0x59, 0x91, 0x00, 0x04, 0x84, 0x07, 0x01, 0x44, 0x54,
  0x00, 0x10, 0x84, 0x40,
}
local ISO3166_A = {
  0x78, 0x59, 0xDF, 0xEE, 0xEF, 0xBD, 0xDD, 0xDE, 0x27, 0x3F, 0x84, 0x15,
  0x80, 0xD4, 0x00, 0x0E, 0x00, 0x5C, 0x09, 0xB0, 0x9F, 0xFB, 0x15, 0x00,
  0x8D, 0x06, 0x18, 0x78, 0x0F, 0x40, 0x40, 0x03, 0x00, 0x1D, 0x2B, 0xF4,
  0x41, 0x81, 0x4F, 0xFD, 0xFC, 0xFF, 0xD7, 0x25, 0x4B, 0x08, 0x00, 0x01,
  0x40, 0x3C, 0x8F, 0x53, 0x01, 0x00, 0x00, 0x40, 0x00, 0x51, 0xF1, 0xFD,
  0xE7, 0x3A, 0xBB, 0x9F, 0x9A, 0x41, 0x10, 0x04, 0x57, 0x85, 0x40, 0x00,
  0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x04, 0x40, 0x00,
  0x01,
}
local ISO4217_N = {
  0x00, 0x11, 0x00, 0x00, 0x11, 0x10, 0x1D, 0x10, 0x11, 0x01, 0x10, 0x04,
  0x01, 0x11, 0x10, 0x10, 0x10, 0x01, 0x01, 0x11, 0x00, 0x44, 0x00, 0x10,
  0x01, 0x08, 0x41, 0x40, 0x40, 0x41, 0x04, 0x00, 0x40, 0x40, 0x00, 0x00,
  0x10, 0x00, 0x00, 0x00, 0x11, 0x11, 0x10, 0x11, 0x11, 0x11, 0x01, 0x01,
  0x10, 0x51, 0x11, 0x45, 0x46, 0x44, 0x04, 0x40, 0x40, 0x44, 0x00, 0x00,
  0x11, 0x00, 0x05, 0x01, 0x11, 0x10, 0x30, 0x00, 0x10, 0x44, 0x40, 0x00,
  0x04, 0x44, 0x40, 0x11, 0x01, 0x00, 0x00, 0x04, 0x48, 0x40, 0x00, 0x00,
  0x00, 0x04, 0x04, 0x40, 0x45, 0x00, 0x00, 0x01, 0x00, 0x10, 0x11, 0x11,
  0x00, 0x11, 0x11, 0x00, 0x81, 0x00, 0x04, 0x04, 0x04, 0x01, 0x00, 0x14,
  0x00, 0x00, 0x44, 0x00, 0x20, 0x00, 0x00, 0xF0, 0x67, 0xB5, 0xFD, 0xFB,
  0xBF, 0xBF, 0x3F, 0x47, 0xA4,
}

-- Package type codes. The C binary-searches two sorted tables adapted from
-- the GS1 Syntax Dictionary (SPDX-License-Identifier: Apache-2.0).
local PACK2 = {
  ['1A']=true, ['1B']=true, ['1D']=true, ['1F']=true, ['1G']=true, ['1W']=true, ['2C']=true, ['3A']=true,
  ['3H']=true, ['43']=true, ['44']=true, ['4A']=true, ['4B']=true, ['4C']=true, ['4D']=true, ['4F']=true,
  ['4G']=true, ['4H']=true, ['5H']=true, ['5L']=true, ['5M']=true, ['6H']=true, ['6P']=true, ['7A']=true,
  ['7B']=true, ['8A']=true, ['8B']=true, ['8C']=true, ['AA']=true, ['AB']=true, ['AC']=true, ['AD']=true,
  ['AF']=true, ['AG']=true, ['AH']=true, ['AI']=true, ['AJ']=true, ['AL']=true, ['AM']=true, ['AP']=true,
  ['AT']=true, ['AV']=true, ['B4']=true, ['BB']=true, ['BC']=true, ['BD']=true, ['BE']=true, ['BF']=true,
  ['BG']=true, ['BH']=true, ['BI']=true, ['BJ']=true, ['BK']=true, ['BL']=true, ['BM']=true, ['BN']=true,
  ['BO']=true, ['BP']=true, ['BQ']=true, ['BR']=true, ['BS']=true, ['BT']=true, ['BU']=true, ['BV']=true,
  ['BW']=true, ['BX']=true, ['BY']=true, ['BZ']=true, ['CA']=true, ['CB']=true, ['CC']=true, ['CD']=true,
  ['CE']=true, ['CF']=true, ['CG']=true, ['CH']=true, ['CI']=true, ['CJ']=true, ['CK']=true, ['CL']=true,
  ['CM']=true, ['CN']=true, ['CO']=true, ['CP']=true, ['CQ']=true, ['CR']=true, ['CS']=true, ['CT']=true,
  ['CU']=true, ['CV']=true, ['CW']=true, ['CX']=true, ['CY']=true, ['CZ']=true, ['DA']=true, ['DB']=true,
  ['DC']=true, ['DG']=true, ['DH']=true, ['DI']=true, ['DJ']=true, ['DK']=true, ['DL']=true, ['DM']=true,
  ['DN']=true, ['DP']=true, ['DR']=true, ['DS']=true, ['DT']=true, ['DU']=true, ['DV']=true, ['DW']=true,
  ['DX']=true, ['DY']=true, ['E1']=true, ['E2']=true, ['E3']=true, ['EC']=true, ['ED']=true, ['EE']=true,
  ['EF']=true, ['EG']=true, ['EH']=true, ['EI']=true, ['EN']=true, ['FB']=true, ['FC']=true, ['FD']=true,
  ['FE']=true, ['FI']=true, ['FL']=true, ['FO']=true, ['FP']=true, ['FR']=true, ['FT']=true, ['FW']=true,
  ['FX']=true, ['GB']=true, ['GI']=true, ['GL']=true, ['GR']=true, ['GU']=true, ['GY']=true, ['GZ']=true,
  ['HA']=true, ['HB']=true, ['HC']=true, ['HG']=true, ['HN']=true, ['HR']=true, ['IA']=true, ['IB']=true,
  ['IC']=true, ['ID']=true, ['IE']=true, ['IF']=true, ['IG']=true, ['IH']=true, ['IK']=true, ['IL']=true,
  ['IN']=true, ['IZ']=true, ['JB']=true, ['JC']=true, ['JG']=true, ['JR']=true, ['JT']=true, ['JY']=true,
  ['KG']=true, ['KI']=true, ['LE']=true, ['LG']=true, ['LT']=true, ['LU']=true, ['LV']=true, ['LZ']=true,
  ['MA']=true, ['MB']=true, ['MC']=true, ['ME']=true, ['MR']=true, ['MS']=true, ['MT']=true, ['MW']=true,
  ['MX']=true, ['NA']=true, ['NE']=true, ['NF']=true, ['NG']=true, ['NS']=true, ['NT']=true, ['NU']=true,
  ['NV']=true, ['OA']=true, ['OB']=true, ['OC']=true, ['OD']=true, ['OE']=true, ['OF']=true, ['OK']=true,
  ['OT']=true, ['OU']=true, ['P2']=true, ['PA']=true, ['PB']=true, ['PC']=true, ['PD']=true, ['PE']=true,
  ['PF']=true, ['PG']=true, ['PH']=true, ['PI']=true, ['PJ']=true, ['PK']=true, ['PL']=true, ['PN']=true,
  ['PO']=true, ['PP']=true, ['PR']=true, ['PT']=true, ['PU']=true, ['PV']=true, ['PX']=true, ['PY']=true,
  ['PZ']=true, ['QA']=true, ['QB']=true, ['QC']=true, ['QD']=true, ['QF']=true, ['QG']=true, ['QH']=true,
  ['QJ']=true, ['QK']=true, ['QL']=true, ['QM']=true, ['QN']=true, ['QP']=true, ['QQ']=true, ['QR']=true,
  ['QS']=true, ['RD']=true, ['RG']=true, ['RJ']=true, ['RK']=true, ['RL']=true, ['RO']=true, ['RT']=true,
  ['RZ']=true, ['S1']=true, ['SA']=true, ['SB']=true, ['SC']=true, ['SD']=true, ['SE']=true, ['SH']=true,
  ['SI']=true, ['SK']=true, ['SL']=true, ['SM']=true, ['SO']=true, ['SP']=true, ['SS']=true, ['ST']=true,
  ['SU']=true, ['SV']=true, ['SW']=true, ['SX']=true, ['SY']=true, ['SZ']=true, ['T1']=true, ['TB']=true,
  ['TC']=true, ['TD']=true, ['TE']=true, ['TG']=true, ['TI']=true, ['TK']=true, ['TL']=true, ['TN']=true,
  ['TO']=true, ['TR']=true, ['TS']=true, ['TT']=true, ['TU']=true, ['TV']=true, ['TW']=true, ['TY']=true,
  ['TZ']=true, ['UC']=true, ['UN']=true, ['VA']=true, ['VG']=true, ['VI']=true, ['VK']=true, ['VL']=true,
  ['VN']=true, ['VO']=true, ['VP']=true, ['VQ']=true, ['VR']=true, ['VS']=true, ['VY']=true, ['WA']=true,
  ['WB']=true, ['WC']=true, ['WD']=true, ['WF']=true, ['WG']=true, ['WH']=true, ['WJ']=true, ['WK']=true,
  ['WL']=true, ['WM']=true, ['WN']=true, ['WP']=true, ['WQ']=true, ['WR']=true, ['WS']=true, ['WT']=true,
  ['WU']=true, ['WV']=true, ['WW']=true, ['WX']=true, ['WY']=true, ['WZ']=true, ['X3']=true, ['XA']=true,
  ['XB']=true, ['XC']=true, ['XD']=true, ['XF']=true, ['XG']=true, ['XH']=true, ['XJ']=true, ['XK']=true,
  ['YA']=true, ['YB']=true, ['YC']=true, ['YD']=true, ['YF']=true, ['YG']=true, ['YH']=true, ['YJ']=true,
  ['YK']=true, ['YL']=true, ['YM']=true, ['YN']=true, ['YP']=true, ['YQ']=true, ['YR']=true, ['YS']=true,
  ['YT']=true, ['YV']=true, ['YW']=true, ['YX']=true, ['YY']=true, ['YZ']=true, ['ZA']=true, ['ZB']=true,
  ['ZC']=true, ['ZD']=true, ['ZF']=true, ['ZG']=true, ['ZH']=true, ['ZJ']=true, ['ZK']=true, ['ZL']=true,
  ['ZM']=true, ['ZN']=true, ['ZP']=true, ['ZQ']=true, ['ZR']=true, ['ZS']=true, ['ZT']=true, ['ZU']=true,
  ['ZV']=true, ['ZW']=true, ['ZX']=true, ['ZY']=true, ['ZZ']=true,
}
local PACK3 = {
  ['200']=true, ['201']=true, ['202']=true, ['203']=true, ['204']=true, ['205']=true, ['206']=true, ['210']=true,
  ['211']=true, ['212']=true, ['APE']=true, ['BGE']=true, ['BME']=true, ['BRI']=true, ['CBL']=true, ['CCE']=true,
  ['DPE']=true, ['FOB']=true, ['FPE']=true, ['LAB']=true, ['MPE']=true, ['OPE']=true, ['PAE']=true, ['PLP']=true,
  ['POP']=true, ['PPE']=true, ['PUE']=true, ['RB1']=true, ['RB2']=true, ['RB3']=true, ['RCB']=true, ['SEC']=true,
  ['STL']=true, ['TEV']=true, ['THE']=true, ['TRE']=true, ['TTE']=true, ['TWE']=true, ['UUE']=true, ['WRP']=true,
  ['X11']=true, ['X12']=true, ['X15']=true, ['X16']=true, ['X17']=true, ['X18']=true, ['X19']=true, ['X20']=true,
}

local function b(data, i)
  return data:byte(i + 1) or 0
end

local function isdigit(c)
  return c >= 48 and c <= 57
end

local function isupper(c)
  return c >= 65 and c <= 90
end

local function adj(data_len, offset)
  if data_len < offset then return 0 end
  return data_len - offset
end

local function to_int(data, offset, length)
  local val, non = 0, false
  for i = 0, length - 1 do
    local c = b(data, offset + i)
    val = val * 10 + (c - 48)
    if not isdigit(c) then non = true end
  end
  if non then return -1 end
  return val
end

local function fail(err, posn, msg)
  err.no = 3
  err.posn = posn
  err.msg = msg
  return false
end

local function failf(err, posn, fmt, ...)
  return fail(err, posn, string.format(fmt, ...))
end

local function bit_set(codes, cc)
  local bucket = cc >> 3
  if bucket < 0 or bucket >= #codes then return false end
  return (codes[bucket + 1] & (1 << (cc & 7))) ~= 0
end

local function iso3166_numeric(cc)
  return bit_set(ISO3166_N, cc)
end

local function iso3166_alpha2(s)
  local c0, c1 = s:byte(1), s:byte(2)
  if not c0 or not c1 then return false end
  if c0 < 65 or c0 > 90 or c1 < 65 or c1 > 90 then return false end
  return bit_set(ISO3166_A, (c0 - 65) * 26 + (c1 - 65))
end

local function iso4217_numeric(cc)
  return bit_set(ISO4217_N, cc)
end

local function c82_at(c)
  if c < 33 or c > 122 then return 82 end
  return C82[c - 32]
end

local DAYS = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
local HEX = "0123456789ABCDEFabcdef"

local function gs1_numeric(data, data_len, offset, min, max, err)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 then
    local de = offset + (n > max and max or n)
    for d = offset, de - 1 do
      local c = b(data, d)
      if not isdigit(c) then
        return failf(err, d + 1, "Non-numeric character '%c'", c)
      end
    end
  end
  return true
end

local function gs1_cset82(data, data_len, offset, min, max, err)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 then
    local de = offset + (n > max and max or n)
    for d = offset, de - 1 do
      local c = b(data, d)
      if c < 33 or c > 122 or c82_at(c) == 82 then
        return failf(err, d + 1, "Invalid CSET 82 character '%c'", c)
      end
    end
  end
  return true
end

local function gs1_cset39(data, data_len, offset, min, max, err)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 then
    local de = offset + (n > max and max or n)
    for d = offset, de - 1 do
      local c = b(data, d)
      if (c < 48 and c ~= 35 and c ~= 45 and c ~= 47) or (c > 57 and c < 65) or c > 90 then
        return failf(err, d + 1, "Invalid CSET 39 character '%c'", c)
      end
    end
  end
  return true
end

local function gs1_cset64(data, data_len, offset, min, max, err)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 then
    local de = offset + (n > max and max or n)
    local d = offset
    while d < de do
      local c = b(data, d)
      if (c < 48 and c ~= 45) or (c > 57 and c < 65) or (c > 90 and c < 97 and c ~= 95) or c > 122 then
        if c == 61 and (d + 1 == de or (d + 2 == de and b(data, d + 1) == 61)) and n % 3 == 0 then
          break
        end
        return failf(err, d + 1, "Invalid CSET 64 character '%c'", c)
      end
      d = d + 1
    end
  end
  return true
end

local function gs1_csum(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local take = n > max and max or n
    local de = offset + take - 1
    local checksum = 0
    local factor = (min & 1) ~= 0 and 1 or 3
    for d = offset, de - 1 do
      checksum = checksum + (b(data, d) - 48) * factor
      factor = factor ~ 2
    end
    checksum = 10 - (checksum % 10)
    if checksum == 10 then checksum = 0 end
    if checksum ~= b(data, de) - 48 then
      return failf(err, de + 1, "Bad checksum '%c', expected '%c'", b(data, de), checksum + 48)
    end
  end
  return true
end

local function gs1_csumalpha(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 and n < 2 then
    err.no = 4
    return false
  end
  if not length_only and n ~= 0 then
    local take = n > max and max or n
    local de = offset + take - 2
    local checksum = 0
    for d = offset, de - 1 do
      local c = b(data, d)
      checksum = checksum + c82_at(c) * WEIGHTS[de - d]
    end
    checksum = checksum % 1021
    local c1 = C32:byte((checksum >> 5) + 1)
    local c2 = C32:byte((checksum & 0x1F) + 1)
    if b(data, de) ~= c1 or b(data, de + 1) ~= c2 then
      return failf(err, de + 1, "Bad checksum '%s', expected '%c%c'",
        data:sub(de + 1, de + 2), c1, c2)
    end
  end
  return true
end

local GS1_GCP_MIN_LENGTH = 4

local function gs1_gcppos1(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 and n < 2 then
    err.no = 4
    return false
  end
  if not length_only and n ~= 0 then
    if n < GS1_GCP_MIN_LENGTH then
      return failf(err, offset + 1, "GS1 Company Prefix length %d too short (minimum 4)", n)
    end
    for i = 0, GS1_GCP_MIN_LENGTH - 1 do
      local c = b(data, offset + i)
      if not isdigit(c) then
        return failf(err, offset + i + 1, "Non-numeric company prefix '%c'", c)
      end
    end
  end
  return true
end

local function gs1_gcppos2(data, data_len, offset, min, max, err, length_only)
  return gs1_gcppos1(data, data_len, offset + 1, min - 1, max - 1, err, length_only)
end

local function gs1_yyyymmd0(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 8) then return false end
  if not length_only and n ~= 0 then
    local month = to_int(data, offset + 4, 2)
    if month == 0 or month > 12 then
      return failf(err, offset + 5, "Invalid month '%s'", data:sub(offset + 5, offset + 6))
    end
    local day = to_int(data, offset + 6, 2)
    if day ~= 0 and day > DAYS[month + 1] then
      return failf(err, offset + 7, "Invalid day '%s'", data:sub(offset + 7, offset + 8))
    end
    if month == 2 and day == 29 then
      local year = to_int(data, offset, 4)
      if (year & 3) ~= 0 or (year % 100 == 0 and year % 400 ~= 0) then
        return failf(err, offset + 7, "Invalid day '%s'", data:sub(offset + 7, offset + 8))
      end
    end
  end
  return true
end

local function gs1_yyyymmdd(data, data_len, offset, min, max, err, length_only)
  if not gs1_yyyymmd0(data, data_len, offset, min, max, err, length_only) then
    return false
  end
  local n = adj(data_len, offset)
  if not length_only and n ~= 0 then
    if to_int(data, offset + 6, 2) == 0 then
      return failf(err, offset + 7, "Invalid day '%s'", data:sub(offset + 7, offset + 8))
    end
  end
  return true
end

local function gs1_yymmd0(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 6) then return false end
  if not length_only and n ~= 0 then
    local buf = "20" .. data:sub(offset + 1, offset + 6)
    if not gs1_yyyymmd0(buf, 8, 0, min, max, err, length_only) then
      err.posn = err.posn + offset - 2
      return false
    end
  end
  return true
end

local function gs1_yymmdd(data, data_len, offset, min, max, err, length_only)
  if not gs1_yymmd0(data, data_len, offset, min, max, err, length_only) then
    return false
  end
  local n = adj(data_len, offset)
  if not length_only and n ~= 0 then
    if to_int(data, offset + 4, 2) == 0 then
      return failf(err, offset + 5, "Invalid day '%s'", data:sub(offset + 5, offset + 6))
    end
  end
  return true
end

local function gs1_hhmi(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 4) then return false end
  if not length_only and n ~= 0 then
    local hour = to_int(data, offset, 2)
    if hour > 23 then
      return failf(err, offset + 1, "Invalid hour of day '%s'", data:sub(offset + 1, offset + 2))
    end
    local mins = to_int(data, offset + 2, 2)
    if mins > 59 then
      return failf(err, offset + 3, "Invalid minutes in the hour '%s'", data:sub(offset + 3, offset + 4))
    end
  end
  return true
end

local function hour_check(data, data_len, offset, min, err, length_only, kind)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 2) then return false end
  if not length_only and n ~= 0 then
    local val = to_int(data, offset, 2)
    local limit = kind == "hh" and 23 or 59
    if val > limit then
      local msg = kind == "hh" and "Invalid hour of day '%s'"
        or (kind == "mi" and "Invalid minutes in the hour '%s'"
        or "Invalid seconds in the minute '%s'")
      return failf(err, offset + 1, msg, data:sub(offset + 1, offset + 2))
    end
  end
  return true
end

local function gs1_hh(data, data_len, offset, min, max, err, length_only)
  return hour_check(data, data_len, offset, min, err, length_only, "hh")
end

local function gs1_mi(data, data_len, offset, min, max, err, length_only)
  return hour_check(data, data_len, offset, min, err, length_only, "mi")
end

local function gs1_ss(data, data_len, offset, min, max, err, length_only)
  return hour_check(data, data_len, offset, min, err, length_only, "ss")
end

local function gs1_iso3166(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 3) then
    if offset ~= 0 then err.no = 4 end
    return false
  end
  if not length_only and n ~= 0 then
    if not iso3166_numeric(to_int(data, offset, 3)) then
      return failf(err, offset + 1, "Unknown country code '%s'", data:sub(offset + 1, offset + 3))
    end
  end
  return true
end

local function gs1_iso3166999(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 3) then return false end
  if not length_only and n ~= 0 then
    local cc = to_int(data, offset, 3)
    if cc ~= 999 and not iso3166_numeric(cc) then
      return failf(err, offset + 1, "Unknown country code '%s'", data:sub(offset + 1, offset + 3))
    end
  end
  return true
end

local function gs1_iso3166alpha2(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 2) then return false end
  if not length_only and n ~= 0 then
    if not iso3166_alpha2(data:sub(offset + 1, offset + 2)) then
      return failf(err, offset + 1, "Unknown country code '%s'", data:sub(offset + 1, offset + 2))
    end
  end
  return true
end

local function gs1_iso4217(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 3) then return false end
  if not length_only and n ~= 0 then
    if not iso4217_numeric(to_int(data, offset, 3)) then
      return failf(err, offset + 1, "Unknown currency code '%s'", data:sub(offset + 1, offset + 3))
    end
  end
  return true
end

local function gs1_pcenc(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local d = offset
    while d < de do
      if b(data, d) == 37 then
        if de - d < 3 then
          return fail(err, d + 1, "Invalid % escape")
        end
        d = d + 1
        if not HEX:find(string.char(b(data, d)), 1, true) then
          return fail(err, d + 1, "Invalid character for percent encoding")
        end
        d = d + 1
        if not HEX:find(string.char(b(data, d)), 1, true) then
          return fail(err, d + 1, "Invalid character for percent encoding")
        end
      end
      d = d + 1
    end
  end
  return true
end

local function gs1_yesno(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local c = b(data, offset)
    if c ~= 48 and c ~= 49 then
      return fail(err, offset + 1, "Neither 0 nor 1 for yes or no")
    end
  end
  return true
end

local function gs1_importeridx(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local c = b(data, offset)
    if (c < 48 and c ~= 45) or (c > 57 and c < 65) or (c > 90 and c < 97 and c ~= 95) or c > 122 then
      return failf(err, offset + 1, "Invalid importer index '%c'", c)
    end
  end
  return true
end

local function gs1_nonzero(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local val = to_int(data, offset, n > max and max or n)
    if val == 0 then
      return fail(err, offset + 1, "Zero not permitted")
    end
  end
  return true
end

local function gs1_winding(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local c = b(data, offset)
    if c ~= 48 and c ~= 49 and c ~= 57 then
      return failf(err, offset + 1, "Invalid winding direction '%c'", c)
    end
  end
  return true
end

local function gs1_zero(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    if b(data, offset) ~= 48 then
      return fail(err, offset + 1, "Zero is required")
    end
  end
  return true
end

local function gs1_pieceoftotal(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 4) then return false end
  if not length_only and n ~= 0 then
    local pieces = to_int(data, offset, 2)
    if pieces == 0 then
      return fail(err, offset + 1, "Piece number cannot be zero")
    end
    local total = to_int(data, offset + 2, 2)
    if total == 0 then
      return fail(err, offset + 1, "Total number cannot be zero")
    end
    if pieces > total then
      return failf(err, offset + 1, "Piece number '%s' exceeds total '%s'",
        data:sub(offset + 1, offset + 2), data:sub(offset + 3, offset + 4))
    end
  end
  return true
end

local function gs1_iban(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 and n < 5 then
    err.no = 4
    return false
  end
  if not length_only and n ~= 0 then
    local d = offset
    local de = offset + (n > max and max or n)
    if not isupper(b(data, d)) or not isupper(b(data, d + 1)) then
      return failf(err, d + 1, "Non-alphabetic IBAN country code '%s'", data:sub(d + 1, d + 2))
    end
    if not iso3166_alpha2(data:sub(d + 1, d + 2)) then
      return failf(err, d + 1, "Invalid IBAN country code '%s'", data:sub(d + 1, d + 2))
    end
    d = d + 2
    if not isdigit(b(data, d)) or not isdigit(b(data, d + 1)) then
      return failf(err, d + 1, "Non-numeric IBAN checksum '%s'", data:sub(d + 1, d + 2))
    end
    local given = to_int(data, d, 2)
    d = d + 2
    local checksum = 0
    while d < de do
      local c = b(data, d)
      if c < 48 or (c > 57 and c < 65) or c > 90 then
        return failf(err, d + 1, "Invalid IBAN character '%c'", c)
      end
      if c >= 65 then
        checksum = checksum * 100 + c - 65 + 10
      else
        checksum = checksum * 10 + c - 48
      end
      checksum = checksum % 97
      d = d + 1
    end
    checksum = (((checksum * 100) % 97) + (b(data, offset) - 65 + 10)) * 100 + b(data, offset + 1) - 65 + 10
    checksum = checksum % 97
    checksum = (checksum * 100) % 97
    checksum = 98 - checksum
    if checksum ~= given then
      return failf(err, offset + 3, "Bad IBAN checksum '%s', expected '%02d'",
        data:sub(offset + 3, offset + 4), checksum)
    end
  end
  return true
end

local function gs1_nozeroprefix(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    if b(data, 0) == 48 and n ~= 1 then
      return fail(err, offset + 1, "Zero prefix is not permitted")
    end
  end
  return true
end

local function coupon_vli(data, data_len, d, name, vli_offset, vli_min, vli_max, vli_nine, err)
  if d + 1 > data_len then
    err.no = 3
    err.posn = d + 1
    err.msg = string.format("%s VLI missing", name)
    return nil
  end
  local vli = to_int(data, d, 1)
  if (vli < vli_min or vli > vli_max) and (vli ~= 9 or not vli_nine) then
    err.no = 3
    err.posn = d + 1
    if vli < 0 then
      err.msg = string.format("Non-numeric %s VLI '%c'", name, b(data, d))
    else
      err.msg = string.format("Invalid %s VLI '%c'", name, b(data, d))
    end
    return nil
  end
  d = d + 1
  if vli ~= 9 or not vli_nine then
    if d + vli + vli_offset > data_len then
      err.no = 3
      err.posn = d + 1
      err.msg = string.format("%s incomplete", name)
      return nil
    end
    local de = d + vli + vli_offset
    while d < de do
      if not isdigit(b(data, d)) then
        err.no = 3
        err.posn = d + 1
        err.msg = string.format("Non-numeric %s '%c'", name, b(data, d))
        return nil
      end
      d = d + 1
    end
  end
  return d
end

local function coupon_val(data, data_len, d, name, val_len, err)
  if d + val_len > data_len then
    err.no = 3
    err.posn = d + 1
    err.msg = string.format("%s incomplete", name)
    return nil
  end
  local val = to_int(data, d, val_len)
  if val < 0 then
    err.no = 3
    err.posn = d + 1
    err.msg = string.format("Non-numeric %s", name)
    return nil
  end
  return d + val_len, val
end

local function gs1_couponcode(data, data_len, offset, min, max, err, length_only)
  local min_req_len = 21
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 and n < min_req_len then
    err.no = 4
    return false
  end
  if not length_only and n ~= 0 then
    local d = offset
    local abs_len = n + offset
    local val
    d = coupon_vli(data, abs_len, d, "Primary GS1 Co. Prefix", 6, 0, 6, false, err)
    if d == nil then return false end
    d = coupon_val(data, abs_len, d, "Offer Code", 6, err)
    if d == nil then return false end
    d = coupon_vli(data, abs_len, d, "Save Value", 0, 1, 5, false, err)
    if d == nil then return false end
    d = coupon_vli(data, abs_len, d, "Primary Purch. Req.", 0, 1, 5, false, err)
    if d == nil then return false end
    d, val = coupon_val(data, abs_len, d, "Primary Purch. Req. Code", 1, err)
    if d == nil then return false end
    if val > 5 and val < 9 then
      return failf(err, d, "Invalid Primary Purch. Req. Code '%c'", b(data, d - 1))
    end
    d = coupon_val(data, abs_len, d, "Primary Purch. Family Code", 3, err)
    if d == nil then return false end
    while d < abs_len do
      local data_field = to_int(data, d, 1)
      d = d + 1
      if data_field == 1 then
        d, val = coupon_val(data, abs_len, d, "Add. Purch. Rules Code", 1, err)
        if d == nil then return false end
        if val > 3 then
          return failf(err, d, "Invalid Add. Purch. Rules Code '%c'", b(data, d - 1))
        end
        d = coupon_vli(data, abs_len, d, "2nd Purch. Req.", 0, 1, 5, false, err)
        if d == nil then return false end
        d, val = coupon_val(data, abs_len, d, "2nd Purch. Req. Code", 1, err)
        if d == nil then return false end
        if val > 4 and val < 9 then
          return failf(err, d, "Invalid 2nd Purch. Req. Code '%c'", b(data, d - 1))
        end
        d = coupon_val(data, abs_len, d, "2nd Purch. Family Code", 3, err)
        if d == nil then return false end
        d = coupon_vli(data, abs_len, d, "2nd Purch. GS1 Co. Prefix", 6, 0, 6, true, err)
        if d == nil then return false end
      elseif data_field == 2 then
        d = coupon_vli(data, abs_len, d, "3rd Purch. Req.", 0, 1, 5, false, err)
        if d == nil then return false end
        d, val = coupon_val(data, abs_len, d, "3rd Purch. Req. Code", 1, err)
        if d == nil then return false end
        if val > 4 and val < 9 then
          return failf(err, d, "Invalid 3rd Purch. Req. Code '%c'", b(data, d - 1))
        end
        d = coupon_val(data, abs_len, d, "3rd Purch. Family Code", 3, err)
        if d == nil then return false end
        d = coupon_vli(data, abs_len, d, "3rd Purch. GS1 Co. Prefix", 6, 0, 6, true, err)
        if d == nil then return false end
      elseif data_field == 3 then
        d = coupon_val(data, abs_len, d, "Expiration Date", 6, err)
        if d == nil then return false end
        if not gs1_yymmd0(data, abs_len, d - 6, 6, 6, err, false) then return false end
      elseif data_field == 4 then
        d = coupon_val(data, abs_len, d, "Start Date", 6, err)
        if d == nil then return false end
        if not gs1_yymmd0(data, abs_len, d - 6, 6, 6, err, false) then return false end
      elseif data_field == 5 then
        d = coupon_vli(data, abs_len, d, "Serial Number", 6, 0, 9, false, err)
        if d == nil then return false end
      elseif data_field == 6 then
        d = coupon_vli(data, abs_len, d, "Retailer ID", 6, 1, 7, false, err)
        if d == nil then return false end
      elseif data_field == 9 then
        d, val = coupon_val(data, abs_len, d, "Save Value Code", 1, err)
        if d == nil then return false end
        if (val > 2 and val < 5) or val > 6 then
          return failf(err, d, "Invalid Save Value Code '%c'", b(data, d - 1))
        end
        d, val = coupon_val(data, abs_len, d, "Save Value Applies To", 1, err)
        if d == nil then return false end
        if val > 2 then
          return failf(err, d, "Invalid Save Value Applies To '%c'", b(data, d - 1))
        end
        d = coupon_val(data, abs_len, d, "Store Coupon Flag", 1, err)
        if d == nil then return false end
        d, val = coupon_val(data, abs_len, d, "Don't Multiply Flag", 1, err)
        if d == nil then return false end
        if val > 1 then
          return failf(err, d, "Invalid Don't Multiply Flag '%c'", b(data, d - 1))
        end
      else
        err.no = 3
        err.posn = d
        if data_field < 0 then
          err.msg = string.format("Non-numeric Data Field '%c'", b(data, d - 1))
        else
          err.msg = string.format("Invalid Data Field '%c'", b(data, d - 1))
        end
        return false
      end
    end
  end
  return true
end

local function gs1_couponposoffer(data, data_len, offset, min, max, err, length_only)
  local min_len, max_len = 21, 36
  local n = adj(data_len, offset)
  if n < min then return false end
  if n ~= 0 and (n < min_len or n > max_len) then
    err.no = 4
    return false
  end
  if not length_only and n ~= 0 then
    local d = offset
    local val
    d, val = coupon_val(data, data_len, d, "Coupon Format", 1, err)
    if d == nil then return false end
    if val ~= 0 and val ~= 1 then
      return fail(err, d, "Coupon Format must be 0 or 1")
    end
    d = coupon_vli(data, data_len, d, "Coupon Funder ID", 6, 0, 6, false, err)
    if d == nil then return false end
    d = coupon_val(data, data_len, d, "Offer Code", 6, err)
    if d == nil then return false end
    d = coupon_vli(data, data_len, d, "Serial Number", 6, 0, 9, false, err)
    if d == nil then return false end
    if d ~= data_len then
      return fail(err, d + 1, "Reserved trailing characters")
    end
  end
  return true
end

local function gs1_latitude(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 10) then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local lat = 0
    for d = offset, de - 1 do
      lat = lat * 10 + (b(data, d) - 48)
    end
    if lat > 1800000000 then
      return fail(err, de, "Invalid latitude")
    end
  end
  return true
end

local function gs1_longitude(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 10) then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local lng = 0
    for d = offset, de - 1 do
      lng = lng * 10 + (b(data, d) - 48)
    end
    if lng > 3600000000 then
      return fail(err, de, "Invalid longitude")
    end
  end
  return true
end

local function gs1_mediatype(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min or (n ~= 0 and n < 2) then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local val = 0
    for d = offset, de - 1 do
      val = val * 10 + (b(data, d) - 48)
    end
    if val == 0 or (val > 10 and val < 80) then
      return fail(err, de + 1, "Invalid AIDC media type")
    end
  end
  return true
end

local function gs1_hyphen(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    for d = offset, de - 1 do
      if b(data, d) ~= 45 then
        return fail(err, d + 1, "Invalid temperature indicator (hyphen only)")
      end
    end
  end
  return true
end

local function gs1_iso5218(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local c = b(data, offset)
    if c ~= 48 and c ~= 49 and c ~= 50 and c ~= 57 then
      return fail(err, offset + 1, "Invalid biological sex code (0, 1, 2 or 9 only)")
    end
  end
  return true
end

local function gs1_posinseqslash(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local slash = nil
    for d = offset, de - 1 do
      local c = b(data, d)
      if not isdigit(c) then
        if c ~= 47 then
          return failf(err, d + 1, "Invalid character '%c' in sequence", c)
        end
        if slash then
          return fail(err, d + 1, "Single sequence separator ('/') only")
        end
        if d == offset or d + 1 == de then
          return fail(err, d + 1, "Sequence separator '/' cannot start or end")
        end
        slash = d
      end
    end
    if not slash then
      return fail(err, offset + 1, "No sequence separator ('/')")
    end
    local pos = to_int(data, offset, slash - offset)
    if pos == 0 then
      return fail(err, offset + 1, "Sequence position cannot be zero")
    end
    local tot = to_int(data, slash + 1, de - (slash + 1))
    if tot == 0 then
      return fail(err, slash + 2, "Sequence total cannot be zero")
    end
    if pos > tot then
      return fail(err, offset + 1, "Sequence position greater than total")
    end
  end
  return true
end

local function gs1_hasnondigit(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local de = offset + (n > max and max or n)
    local d = offset
    while d < de and isdigit(b(data, d)) do d = d + 1 end
    if d == de then
      return fail(err, offset + 1, "A non-digit character is required")
    end
  end
  return true
end

local function gs1_packagetype(data, data_len, offset, min, max, err, length_only)
  local n = adj(data_len, offset)
  if n < min then return false end
  if not length_only and n ~= 0 then
    local shown = data:sub(offset + 1, offset + n)
    local valid = false
    if n == 1 then
      valid = shown == "8" or shown == "9"
    elseif n == 2 then
      valid = PACK2[shown] == true
    elseif n == 3 then
      valid = PACK3[shown] == true
    end
    if not valid then
      return failf(err, offset + 1, "Invalid package type '%s'", shown)
    end
  end
  return true
end

local function gs1_n18_csum_gcppos2(data, data_len, err)
  return data_len == 18 and gs1_csum(data, data_len, 0, 18, 18, err, true) and gs1_gcppos2(data, data_len, 0, 18, 18, err, true) and gs1_numeric(data, data_len, 0, 18, 18, err) and gs1_csum(data, data_len, 0, 18, 18, err, false) and gs1_gcppos2(data, data_len, 0, 18, 18, err, false)
end

local function gs1_n14_csum_gcppos2(data, data_len, err)
  return data_len == 14 and gs1_csum(data, data_len, 0, 14, 14, err, true) and gs1_gcppos2(data, data_len, 0, 14, 14, err, true) and gs1_numeric(data, data_len, 0, 14, 14, err) and gs1_csum(data, data_len, 0, 14, 14, err, false) and gs1_gcppos2(data, data_len, 0, 14, 14, err, false)
end

local function gs1_x__20(data, data_len, err)
  return data_len >= 1 and data_len <= 20 and gs1_cset82(data, data_len, 0, 1, 20, err)
end

local function gs1_n6_yymmd0(data, data_len, err)
  return data_len == 6 and gs1_yymmd0(data, data_len, 0, 6, 6, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmd0(data, data_len, 0, 6, 6, err, false)
end

local function gs1_n2(data, data_len, err)
  return data_len == 2 and gs1_numeric(data, data_len, 0, 2, 2, err)
end

local function gs1_x__28(data, data_len, err)
  return data_len >= 1 and data_len <= 28 and gs1_cset82(data, data_len, 0, 1, 28, err)
end

local function gs1_x__30(data, data_len, err)
  return data_len >= 1 and data_len <= 30 and gs1_cset82(data, data_len, 0, 1, 30, err)
end

local function gs1_n__6(data, data_len, err)
  return data_len >= 1 and data_len <= 6 and gs1_numeric(data, data_len, 0, 1, 6, err)
end

local function gs1_n13_csum_gcppos1__x__17_(data, data_len, err)
  return data_len >= 13 and data_len <= 30 and gs1_csum(data, data_len, 0, 13, 13, err, true) and gs1_gcppos1(data, data_len, 0, 13, 13, err, true) and gs1_numeric(data, data_len, 0, 13, 13, err) and gs1_csum(data, data_len, 0, 13, 13, err, false) and gs1_gcppos1(data, data_len, 0, 13, 13, err, false) and gs1_cset82(data, data_len, 13, 0, 17, err)
end

local function gs1_n13_csum_gcppos1__n__12_(data, data_len, err)
  return data_len >= 13 and data_len <= 25 and gs1_csum(data, data_len, 0, 13, 13, err, true) and gs1_gcppos1(data, data_len, 0, 13, 13, err, true) and gs1_numeric(data, data_len, 0, 13, 13, err) and gs1_csum(data, data_len, 0, 13, 13, err, false) and gs1_gcppos1(data, data_len, 0, 13, 13, err, false) and gs1_numeric(data, data_len, 13, 0, 12, err)
end

local function gs1_n__8(data, data_len, err)
  return data_len >= 1 and data_len <= 8 and gs1_numeric(data, data_len, 0, 1, 8, err)
end

local function gs1_n6(data, data_len, err)
  return data_len == 6 and gs1_numeric(data, data_len, 0, 6, 6, err)
end

local function gs1_n__15(data, data_len, err)
  return data_len >= 1 and data_len <= 15 and gs1_numeric(data, data_len, 0, 1, 15, err)
end

local function gs1_n3_iso4217_n__15(data, data_len, err)
  return data_len >= 4 and data_len <= 18 and gs1_iso4217(data, data_len, 0, 3, 3, err, true) and gs1_numeric(data, data_len, 0, 3, 3, err) and gs1_iso4217(data, data_len, 0, 3, 3, err, false) and gs1_numeric(data, data_len, 3, 1, 15, err)
end

local function gs1_n4(data, data_len, err)
  return data_len == 4 and gs1_numeric(data, data_len, 0, 4, 4, err)
end

local function gs1_x__30_gcppos1(data, data_len, err)
  return data_len >= 1 and data_len <= 30 and gs1_gcppos1(data, data_len, 0, 1, 30, err, true) and gs1_cset82(data, data_len, 0, 1, 30, err) and gs1_gcppos1(data, data_len, 0, 1, 30, err, false)
end

local function gs1_n17_csum_gcppos1(data, data_len, err)
  return data_len == 17 and gs1_csum(data, data_len, 0, 17, 17, err, true) and gs1_gcppos1(data, data_len, 0, 17, 17, err, true) and gs1_numeric(data, data_len, 0, 17, 17, err) and gs1_csum(data, data_len, 0, 17, 17, err, false) and gs1_gcppos1(data, data_len, 0, 17, 17, err, false)
end

local function gs1_n13_csum_gcppos1(data, data_len, err)
  return data_len == 13 and gs1_csum(data, data_len, 0, 13, 13, err, true) and gs1_gcppos1(data, data_len, 0, 13, 13, err, true) and gs1_numeric(data, data_len, 0, 13, 13, err) and gs1_csum(data, data_len, 0, 13, 13, err, false) and gs1_gcppos1(data, data_len, 0, 13, 13, err, false)
end

local function gs1_n3_iso3166_x__9(data, data_len, err)
  return data_len >= 4 and data_len <= 12 and gs1_iso3166(data, data_len, 0, 3, 3, err, true) and gs1_numeric(data, data_len, 0, 3, 3, err) and gs1_iso3166(data, data_len, 0, 3, 3, err, false) and gs1_cset82(data, data_len, 3, 1, 9, err)
end

local function gs1_n3_iso3166(data, data_len, err)
  return data_len == 3 and gs1_iso3166(data, data_len, 0, 3, 3, err, true) and gs1_numeric(data, data_len, 0, 3, 3, err) and gs1_iso3166(data, data_len, 0, 3, 3, err, false)
end

local function gs1_n3_iso3166__n3__iso3166__n3__iso3166__n3__iso3166__n3__iso3166(data, data_len, err)
  return data_len >= 3 and data_len <= 15 and gs1_iso3166(data, data_len, 0, 3, 3, err, true) and gs1_iso3166(data, data_len, 3, 0, 3, err, true) and gs1_iso3166(data, data_len, 6, 0, 3, err, true) and gs1_iso3166(data, data_len, 9, 0, 3, err, true) and gs1_iso3166(data, data_len, 12, 0, 3, err, true) and gs1_numeric(data, data_len, 0, 3, 3, err) and gs1_iso3166(data, data_len, 0, 3, 3, err, false) and gs1_numeric(data, data_len, 3, 0, 3, err) and gs1_iso3166(data, data_len, 3, 0, 3, err, false) and gs1_numeric(data, data_len, 6, 0, 3, err) and gs1_iso3166(data, data_len, 6, 0, 3, err, false) and gs1_numeric(data, data_len, 9, 0, 3, err) and gs1_iso3166(data, data_len, 9, 0, 3, err, false) and gs1_numeric(data, data_len, 12, 0, 3, err) and gs1_iso3166(data, data_len, 12, 0, 3, err, false)
end

local function gs1_x__3(data, data_len, err)
  return data_len >= 1 and data_len <= 3 and gs1_cset82(data, data_len, 0, 1, 3, err)
end

local function gs1_x__35_pcenc(data, data_len, err)
  return data_len >= 1 and data_len <= 35 and gs1_pcenc(data, data_len, 0, 1, 35, err, true) and gs1_cset82(data, data_len, 0, 1, 35, err) and gs1_pcenc(data, data_len, 0, 1, 35, err, false)
end

local function gs1_x__70_pcenc(data, data_len, err)
  return data_len >= 1 and data_len <= 70 and gs1_pcenc(data, data_len, 0, 1, 70, err, true) and gs1_cset82(data, data_len, 0, 1, 70, err) and gs1_pcenc(data, data_len, 0, 1, 70, err, false)
end

local function gs1_x2_iso3166alpha2(data, data_len, err)
  return data_len == 2 and gs1_iso3166alpha2(data, data_len, 0, 2, 2, err, true) and gs1_cset82(data, data_len, 0, 2, 2, err) and gs1_iso3166alpha2(data, data_len, 0, 2, 2, err, false)
end

local function gs1_n10_latitude_n10_longitude(data, data_len, err)
  return data_len == 20 and gs1_latitude(data, data_len, 0, 10, 10, err, true) and gs1_longitude(data, data_len, 10, 10, 10, err, true) and gs1_numeric(data, data_len, 0, 10, 10, err) and gs1_latitude(data, data_len, 0, 10, 10, err, false) and gs1_numeric(data, data_len, 10, 10, 10, err) and gs1_longitude(data, data_len, 10, 10, 10, err, false)
end

local function gs1_n1_yesno(data, data_len, err)
  return data_len == 1 and gs1_yesno(data, data_len, 0, 1, 1, err, true) and gs1_numeric(data, data_len, 0, 1, 1, err) and gs1_yesno(data, data_len, 0, 1, 1, err, false)
end

local function gs1_n6_yymmd0_n4_hhmi(data, data_len, err)
  return data_len == 10 and gs1_yymmd0(data, data_len, 0, 6, 6, err, true) and gs1_hhmi(data, data_len, 6, 4, 4, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmd0(data, data_len, 0, 6, 6, err, false) and gs1_numeric(data, data_len, 6, 4, 4, err) and gs1_hhmi(data, data_len, 6, 4, 4, err, false)
end

local function gs1_n6_yymmdd(data, data_len, err)
  return data_len == 6 and gs1_yymmdd(data, data_len, 0, 6, 6, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmdd(data, data_len, 0, 6, 6, err, false)
end

local function gs1_n6__x1__hyphen(data, data_len, err)
  return data_len >= 6 and data_len <= 7 and gs1_hyphen(data, data_len, 6, 0, 1, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_cset82(data, data_len, 6, 0, 1, err) and gs1_hyphen(data, data_len, 6, 0, 1, err, false)
end

local function gs1_n13(data, data_len, err)
  return data_len == 13 and gs1_numeric(data, data_len, 0, 13, 13, err)
end

local function gs1_n6_yymmdd_n4_hhmi(data, data_len, err)
  return data_len == 10 and gs1_yymmdd(data, data_len, 0, 6, 6, err, true) and gs1_hhmi(data, data_len, 6, 4, 4, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmdd(data, data_len, 0, 6, 6, err, false) and gs1_numeric(data, data_len, 6, 4, 4, err) and gs1_hhmi(data, data_len, 6, 4, 4, err, false)
end

local function gs1_n__4(data, data_len, err)
  return data_len >= 1 and data_len <= 4 and gs1_numeric(data, data_len, 0, 1, 4, err)
end

local function gs1_x__12(data, data_len, err)
  return data_len >= 1 and data_len <= 12 and gs1_cset82(data, data_len, 0, 1, 12, err)
end

local function gs1_n6_yymmdd__n6__yymmdd(data, data_len, err)
  return data_len >= 6 and data_len <= 12 and gs1_yymmdd(data, data_len, 0, 6, 6, err, true) and gs1_yymmdd(data, data_len, 6, 0, 6, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmdd(data, data_len, 0, 6, 6, err, false) and gs1_numeric(data, data_len, 6, 0, 6, err) and gs1_yymmdd(data, data_len, 6, 0, 6, err, false)
end

local function gs1_x__10(data, data_len, err)
  return data_len >= 1 and data_len <= 10 and gs1_cset82(data, data_len, 0, 1, 10, err)
end

local function gs1_x__2(data, data_len, err)
  return data_len >= 1 and data_len <= 2 and gs1_cset82(data, data_len, 0, 1, 2, err)
end

local function gs1_n6_yymmdd__n4__hhmi(data, data_len, err)
  return data_len >= 6 and data_len <= 10 and gs1_yymmdd(data, data_len, 0, 6, 6, err, true) and gs1_hhmi(data, data_len, 6, 0, 4, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmdd(data, data_len, 0, 6, 6, err, false) and gs1_numeric(data, data_len, 6, 0, 4, err) and gs1_hhmi(data, data_len, 6, 0, 4, err, false)
end

local function gs1_n3_iso3166999_x__27(data, data_len, err)
  return data_len >= 4 and data_len <= 30 and gs1_iso3166999(data, data_len, 0, 3, 3, err, true) and gs1_numeric(data, data_len, 0, 3, 3, err) and gs1_iso3166999(data, data_len, 0, 3, 3, err, false) and gs1_cset82(data, data_len, 3, 1, 27, err)
end

local function gs1_n1_x1_x1_x1_importeridx(data, data_len, err)
  return data_len == 4 and gs1_importeridx(data, data_len, 3, 1, 1, err, true) and gs1_numeric(data, data_len, 0, 1, 1, err) and gs1_cset82(data, data_len, 1, 1, 1, err) and gs1_cset82(data, data_len, 2, 1, 1, err) and gs1_cset82(data, data_len, 3, 1, 1, err) and gs1_importeridx(data, data_len, 3, 1, 1, err, false)
end

local function gs1_x__4_packagetype(data, data_len, err)
  return data_len >= 1 and data_len <= 4 and gs1_packagetype(data, data_len, 0, 1, 4, err, true) and gs1_cset82(data, data_len, 0, 1, 4, err) and gs1_packagetype(data, data_len, 0, 1, 4, err, false)
end

local function gs1_x2_x__28(data, data_len, err)
  return data_len >= 3 and data_len <= 30 and gs1_cset82(data, data_len, 0, 2, 2, err) and gs1_cset82(data, data_len, 2, 1, 28, err)
end

local function gs1_n2_mediatype(data, data_len, err)
  return data_len == 2 and gs1_mediatype(data, data_len, 0, 2, 2, err, true) and gs1_numeric(data, data_len, 0, 2, 2, err) and gs1_mediatype(data, data_len, 0, 2, 2, err, false)
end

local function gs1_x__25(data, data_len, err)
  return data_len >= 1 and data_len <= 25 and gs1_cset82(data, data_len, 0, 1, 25, err)
end

local function gs1_n8_yyyymmdd(data, data_len, err)
  return data_len == 8 and gs1_yyyymmdd(data, data_len, 0, 8, 8, err, true) and gs1_numeric(data, data_len, 0, 8, 8, err) and gs1_yyyymmdd(data, data_len, 0, 8, 8, err, false)
end

local function gs1_n8_yyyymmdd_n4_hhmi(data, data_len, err)
  return data_len == 12 and gs1_yyyymmdd(data, data_len, 0, 8, 8, err, true) and gs1_hhmi(data, data_len, 8, 4, 4, err, true) and gs1_numeric(data, data_len, 0, 8, 8, err) and gs1_yyyymmdd(data, data_len, 0, 8, 8, err, false) and gs1_numeric(data, data_len, 8, 4, 4, err) and gs1_hhmi(data, data_len, 8, 4, 4, err, false)
end

local function gs1_n1_iso5218(data, data_len, err)
  return data_len == 1 and gs1_iso5218(data, data_len, 0, 1, 1, err, true) and gs1_numeric(data, data_len, 0, 1, 1, err) and gs1_iso5218(data, data_len, 0, 1, 1, err, false)
end

local function gs1_x__40_pcenc(data, data_len, err)
  return data_len >= 1 and data_len <= 40 and gs1_pcenc(data, data_len, 0, 1, 40, err, true) and gs1_cset82(data, data_len, 0, 1, 40, err) and gs1_pcenc(data, data_len, 0, 1, 40, err, false)
end

local function gs1_x__90_pcenc(data, data_len, err)
  return data_len >= 1 and data_len <= 90 and gs1_pcenc(data, data_len, 0, 1, 90, err, true) and gs1_cset82(data, data_len, 0, 1, 90, err) and gs1_pcenc(data, data_len, 0, 1, 90, err, false)
end

local function gs1_x3_posinseqslash(data, data_len, err)
  return data_len == 3 and gs1_posinseqslash(data, data_len, 0, 3, 3, err, true) and gs1_cset82(data, data_len, 0, 3, 3, err) and gs1_posinseqslash(data, data_len, 0, 3, 3, err, false)
end

local function gs1_n4_nonzero_n5_nonzero_n3_nonzero_n1_winding_n1(data, data_len, err)
  return data_len == 14 and gs1_nonzero(data, data_len, 0, 4, 4, err, true) and gs1_nonzero(data, data_len, 4, 5, 5, err, true) and gs1_nonzero(data, data_len, 9, 3, 3, err, true) and gs1_winding(data, data_len, 12, 1, 1, err, true) and gs1_numeric(data, data_len, 0, 4, 4, err) and gs1_nonzero(data, data_len, 0, 4, 4, err, false) and gs1_numeric(data, data_len, 4, 5, 5, err) and gs1_nonzero(data, data_len, 4, 5, 5, err, false) and gs1_numeric(data, data_len, 9, 3, 3, err) and gs1_nonzero(data, data_len, 9, 3, 3, err, false) and gs1_numeric(data, data_len, 12, 1, 1, err) and gs1_winding(data, data_len, 12, 1, 1, err, false) and gs1_numeric(data, data_len, 13, 1, 1, err)
end

local function gs1_n1_zero_n13_csum_gcppos1__x__16_(data, data_len, err)
  return data_len >= 14 and data_len <= 30 and gs1_zero(data, data_len, 0, 1, 1, err, true) and gs1_csum(data, data_len, 1, 13, 13, err, true) and gs1_gcppos1(data, data_len, 1, 13, 13, err, true) and gs1_numeric(data, data_len, 0, 1, 1, err) and gs1_zero(data, data_len, 0, 1, 1, err, false) and gs1_numeric(data, data_len, 1, 13, 13, err) and gs1_csum(data, data_len, 1, 13, 13, err, false) and gs1_gcppos1(data, data_len, 1, 13, 13, err, false) and gs1_cset82(data, data_len, 14, 0, 16, err)
end

local function gs1_n14_csum_gcppos2_n4_pieceoftotal(data, data_len, err)
  return data_len == 18 and gs1_csum(data, data_len, 0, 14, 14, err, true) and gs1_gcppos2(data, data_len, 0, 14, 14, err, true) and gs1_pieceoftotal(data, data_len, 14, 4, 4, err, true) and gs1_numeric(data, data_len, 0, 14, 14, err) and gs1_csum(data, data_len, 0, 14, 14, err, false) and gs1_gcppos2(data, data_len, 0, 14, 14, err, false) and gs1_numeric(data, data_len, 14, 4, 4, err) and gs1_pieceoftotal(data, data_len, 14, 4, 4, err, false)
end

local function gs1_x__34_iban(data, data_len, err)
  return data_len >= 1 and data_len <= 34 and gs1_iban(data, data_len, 0, 1, 34, err, true) and gs1_cset82(data, data_len, 0, 1, 34, err) and gs1_iban(data, data_len, 0, 1, 34, err, false)
end

local function gs1_n6_yymmdd_n2_hh__n2__mi__n2__ss(data, data_len, err)
  return data_len >= 8 and data_len <= 12 and gs1_yymmdd(data, data_len, 0, 6, 6, err, true) and gs1_hh(data, data_len, 6, 2, 2, err, true) and gs1_mi(data, data_len, 8, 0, 2, err, true) and gs1_ss(data, data_len, 10, 0, 2, err, true) and gs1_numeric(data, data_len, 0, 6, 6, err) and gs1_yymmdd(data, data_len, 0, 6, 6, err, false) and gs1_numeric(data, data_len, 6, 2, 2, err) and gs1_hh(data, data_len, 6, 2, 2, err, false) and gs1_numeric(data, data_len, 8, 0, 2, err) and gs1_mi(data, data_len, 8, 0, 2, err, false) and gs1_numeric(data, data_len, 10, 0, 2, err) and gs1_ss(data, data_len, 10, 0, 2, err, false)
end

local function gs1_x__50(data, data_len, err)
  return data_len >= 1 and data_len <= 50 and gs1_cset82(data, data_len, 0, 1, 50, err)
end

local function gs1_y__30_gcppos1(data, data_len, err)
  return data_len >= 1 and data_len <= 30 and gs1_gcppos1(data, data_len, 0, 1, 30, err, true) and gs1_cset39(data, data_len, 0, 1, 30, err) and gs1_gcppos1(data, data_len, 0, 1, 30, err, false)
end

local function gs1_n__12_nozeroprefix(data, data_len, err)
  return data_len >= 1 and data_len <= 12 and gs1_nozeroprefix(data, data_len, 0, 1, 12, err, true) and gs1_numeric(data, data_len, 0, 1, 12, err) and gs1_nozeroprefix(data, data_len, 0, 1, 12, err, false)
end

local function gs1_x__25_csumalpha_gcppos1(data, data_len, err)
  return data_len >= 1 and data_len <= 25 and gs1_csumalpha(data, data_len, 0, 1, 25, err, true) and gs1_gcppos1(data, data_len, 0, 1, 25, err, true) and gs1_cset82(data, data_len, 0, 1, 25, err) and gs1_csumalpha(data, data_len, 0, 1, 25, err, false) and gs1_gcppos1(data, data_len, 0, 1, 25, err, false)
end

local function gs1_x__25_csumalpha_gcppos1_hasnondigit(data, data_len, err)
  return data_len >= 1 and data_len <= 25 and gs1_csumalpha(data, data_len, 0, 1, 25, err, true) and gs1_gcppos1(data, data_len, 0, 1, 25, err, true) and gs1_hasnondigit(data, data_len, 0, 1, 25, err, true) and gs1_cset82(data, data_len, 0, 1, 25, err) and gs1_csumalpha(data, data_len, 0, 1, 25, err, false) and gs1_gcppos1(data, data_len, 0, 1, 25, err, false) and gs1_hasnondigit(data, data_len, 0, 1, 25, err, false)
end

local function gs1_n18_csum_gcppos1(data, data_len, err)
  return data_len == 18 and gs1_csum(data, data_len, 0, 18, 18, err, true) and gs1_gcppos1(data, data_len, 0, 18, 18, err, true) and gs1_numeric(data, data_len, 0, 18, 18, err) and gs1_csum(data, data_len, 0, 18, 18, err, false) and gs1_gcppos1(data, data_len, 0, 18, 18, err, false)
end

local function gs1_n__10(data, data_len, err)
  return data_len >= 1 and data_len <= 10 and gs1_numeric(data, data_len, 0, 1, 10, err)
end

local function gs1_z__90(data, data_len, err)
  return data_len >= 1 and data_len <= 90 and gs1_cset64(data, data_len, 0, 1, 90, err)
end

local function gs1_n15(data, data_len, err)
  return data_len == 15 and gs1_numeric(data, data_len, 0, 15, 15, err)
end

local function gs1_n32(data, data_len, err)
  return data_len == 32 and gs1_numeric(data, data_len, 0, 32, 32, err)
end

local function gs1_n18__n__2_(data, data_len, err)
  return data_len >= 18 and data_len <= 20 and gs1_numeric(data, data_len, 0, 18, 18, err) and gs1_numeric(data, data_len, 18, 0, 2, err)
end

local function gs1_x__70_couponcode(data, data_len, err)
  return data_len >= 1 and data_len <= 70 and gs1_couponcode(data, data_len, 0, 1, 70, err, true) and gs1_cset82(data, data_len, 0, 1, 70, err) and gs1_couponcode(data, data_len, 0, 1, 70, err, false)
end

local function gs1_x__70_couponposoffer(data, data_len, err)
  return data_len >= 1 and data_len <= 70 and gs1_couponposoffer(data, data_len, 0, 1, 70, err, true) and gs1_cset82(data, data_len, 0, 1, 70, err) and gs1_couponposoffer(data, data_len, 0, 1, 70, err, false)
end

local function gs1_x__70(data, data_len, err)
  return data_len >= 1 and data_len <= 70 and gs1_cset82(data, data_len, 0, 1, 70, err)
end

local function gs1_x__90(data, data_len, err)
  return data_len >= 1 and data_len <= 90 and gs1_cset82(data, data_len, 0, 1, 90, err)
end

function M.check(ai, data)
  local err = { no = 2, posn = 0, msg = "" }
  local data_len = #data
  local function run(fn)
    if fn(data, data_len, err) then return true end
    return false, err
  end
  if ai < 100 then
    if ai == 0 then return run(gs1_n18_csum_gcppos2) end
    if ai >= 1 and ai <= 3 then return run(gs1_n14_csum_gcppos2) end
    if ai == 10 or ai == 21 or ai == 22 then return run(gs1_x__20) end
    if (ai >= 11 and ai <= 13) or (ai >= 15 and ai <= 17) then return run(gs1_n6_yymmd0) end
    if ai == 20 then return run(gs1_n2) end
    if ai == 30 or ai == 37 then return run(gs1_n__8) end
    if ai == 90 then return run(gs1_x__30) end
    if ai >= 91 then return run(gs1_x__90) end
  elseif ai < 300 then
    if ai == 235 then return run(gs1_x__28) end
    if ai == 240 or ai == 241 or ai == 250 or ai == 251 then return run(gs1_x__30) end
    if ai == 242 then return run(gs1_n__6) end
    if ai == 243 or ai == 254 then return run(gs1_x__20) end
    if ai == 253 then return run(gs1_n13_csum_gcppos1__x__17_) end
    if ai == 255 then return run(gs1_n13_csum_gcppos1__n__12_) end
  elseif ai < 500 then
    if ai == 400 or ai == 403 then return run(gs1_x__30) end
    if ai == 401 then return run(gs1_x__30_gcppos1) end
    if ai == 402 then return run(gs1_n17_csum_gcppos1) end
    if ai >= 410 and ai <= 417 then return run(gs1_n13_csum_gcppos1) end
    if ai == 420 then return run(gs1_x__20) end
    if ai == 421 then return run(gs1_n3_iso3166_x__9) end
    if ai == 422 or ai == 424 or ai == 426 then return run(gs1_n3_iso3166) end
    if ai == 423 or ai == 425 then return run(gs1_n3_iso3166__n3__iso3166__n3__iso3166__n3__iso3166__n3__iso3166) end
    if ai == 427 then return run(gs1_x__3) end
  elseif ai < 800 then
    if ai >= 710 and ai <= 717 then return run(gs1_x__20) end
  elseif ai < 3200 then
    if (ai >= 3100 and ai <= 3105) or (ai >= 3110 and ai <= 3115) or (ai >= 3120 and ai <= 3125) or (ai >= 3130 and ai <= 3135) or (ai >= 3140 and ai <= 3145) or (ai >= 3150 and ai <= 3155) or (ai >= 3160 and ai <= 3165) then return run(gs1_n6) end
  elseif ai < 3300 then
    if ai <= 3205 or (ai >= 3210 and ai <= 3215) or (ai >= 3220 and ai <= 3225) or (ai >= 3230 and ai <= 3235) or (ai >= 3240 and ai <= 3245) or (ai >= 3250 and ai <= 3255) or (ai >= 3260 and ai <= 3265) or (ai >= 3270 and ai <= 3275) or (ai >= 3280 and ai <= 3285) or (ai >= 3290 and ai <= 3295) then return run(gs1_n6) end
  elseif ai < 3400 then
    if ai <= 3305 or (ai >= 3310 and ai <= 3315) or (ai >= 3320 and ai <= 3325) or (ai >= 3330 and ai <= 3335) or (ai >= 3340 and ai <= 3345) or (ai >= 3350 and ai <= 3355) or (ai >= 3360 and ai <= 3365) or (ai >= 3370 and ai <= 3375) then return run(gs1_n6) end
  elseif ai < 3500 then
    if ai <= 3405 or (ai >= 3410 and ai <= 3415) or (ai >= 3420 and ai <= 3425) or (ai >= 3430 and ai <= 3435) or (ai >= 3440 and ai <= 3445) or (ai >= 3450 and ai <= 3455) or (ai >= 3460 and ai <= 3465) or (ai >= 3470 and ai <= 3475) or (ai >= 3480 and ai <= 3485) or (ai >= 3490 and ai <= 3495) then return run(gs1_n6) end
  elseif ai < 3600 then
    if ai <= 3505 or (ai >= 3510 and ai <= 3515) or (ai >= 3520 and ai <= 3525) or (ai >= 3530 and ai <= 3535) or (ai >= 3540 and ai <= 3545) or (ai >= 3550 and ai <= 3555) or (ai >= 3560 and ai <= 3565) or (ai >= 3570 and ai <= 3575) then return run(gs1_n6) end
  elseif ai < 3700 then
    if ai <= 3605 or (ai >= 3610 and ai <= 3615) or (ai >= 3620 and ai <= 3625) or (ai >= 3630 and ai <= 3635) or (ai >= 3640 and ai <= 3645) or (ai >= 3650 and ai <= 3655) or (ai >= 3660 and ai <= 3665) or (ai >= 3670 and ai <= 3675) or (ai >= 3680 and ai <= 3685) or (ai >= 3690 and ai <= 3695) then return run(gs1_n6) end
  elseif ai < 4000 then
    if (ai >= 3900 and ai <= 3909) or (ai >= 3920 and ai <= 3929) then return run(gs1_n__15) end
    if (ai >= 3910 and ai <= 3919) or (ai >= 3930 and ai <= 3939) then return run(gs1_n3_iso4217_n__15) end
    if ai >= 3940 and ai <= 3943 then return run(gs1_n4) end
    if ai >= 3950 and ai <= 3955 then return run(gs1_n6) end
  elseif ai < 4400 then
    if ai == 4300 or ai == 4301 or ai == 4310 or ai == 4311 or ai == 4320 then return run(gs1_x__35_pcenc) end
    if (ai >= 4302 and ai <= 4306) or (ai >= 4312 and ai <= 4316) then return run(gs1_x__70_pcenc) end
    if ai == 4307 or ai == 4317 then return run(gs1_x2_iso3166alpha2) end
    if ai == 4308 or ai == 4319 then return run(gs1_x__30) end
    if ai == 4309 then return run(gs1_n10_latitude_n10_longitude) end
    if ai == 4318 then return run(gs1_x__20) end
    if ai >= 4321 and ai <= 4323 then return run(gs1_n1_yesno) end
    if ai == 4324 or ai == 4325 then return run(gs1_n6_yymmd0_n4_hhmi) end
    if ai == 4326 then return run(gs1_n6_yymmdd) end
    if ai >= 4330 and ai <= 4333 then return run(gs1_n6__x1__hyphen) end
  elseif ai < 7100 then
    if ai == 7001 then return run(gs1_n13) end
    if ai == 7002 then return run(gs1_x__30) end
    if ai == 7003 then return run(gs1_n6_yymmdd_n4_hhmi) end
    if ai == 7004 then return run(gs1_n__4) end
    if ai == 7005 then return run(gs1_x__12) end
    if ai == 7006 then return run(gs1_n6_yymmdd) end
    if ai == 7007 then return run(gs1_n6_yymmdd__n6__yymmdd) end
    if ai == 7008 then return run(gs1_x__3) end
    if ai == 7009 then return run(gs1_x__10) end
    if ai == 7010 then return run(gs1_x__2) end
    if ai == 7011 then return run(gs1_n6_yymmdd__n4__hhmi) end
    if ai >= 7020 and ai <= 7022 then return run(gs1_x__20) end
    if ai == 7023 then return run(gs1_x__30_gcppos1) end
    if ai >= 7030 and ai <= 7039 then return run(gs1_n3_iso3166999_x__27) end
    if ai == 7040 then return run(gs1_n1_x1_x1_x1_importeridx) end
    if ai == 7041 then return run(gs1_x__4_packagetype) end
  elseif ai < 7300 then
    if ai >= 7230 and ai <= 7239 then return run(gs1_x2_x__28) end
    if ai == 7240 then return run(gs1_x__20) end
    if ai == 7241 then return run(gs1_n2_mediatype) end
    if ai == 7242 then return run(gs1_x__25) end
    if ai == 7250 then return run(gs1_n8_yyyymmdd) end
    if ai == 7251 then return run(gs1_n8_yyyymmdd_n4_hhmi) end
    if ai == 7252 then return run(gs1_n1_iso5218) end
    if ai == 7253 or ai == 7254 or ai == 7259 then return run(gs1_x__40_pcenc) end
    if ai == 7255 then return run(gs1_x__10) end
    if ai == 7256 then return run(gs1_x__90_pcenc) end
    if ai == 7257 then return run(gs1_x__70_pcenc) end
    if ai == 7258 then return run(gs1_x3_posinseqslash) end
  elseif ai < 8100 then
    if ai == 8001 then return run(gs1_n4_nonzero_n5_nonzero_n3_nonzero_n1_winding_n1) end
    if ai == 8002 or ai == 8012 then return run(gs1_x__20) end
    if ai == 8003 then return run(gs1_n1_zero_n13_csum_gcppos1__x__16_) end
    if ai == 8004 then return run(gs1_x__30_gcppos1) end
    if ai == 8005 then return run(gs1_n6) end
    if ai == 8006 or ai == 8026 then return run(gs1_n14_csum_gcppos2_n4_pieceoftotal) end
    if ai == 8007 then return run(gs1_x__34_iban) end
    if ai == 8008 then return run(gs1_n6_yymmdd_n2_hh__n2__mi__n2__ss) end
    if ai == 8009 then return run(gs1_x__50) end
    if ai == 8010 then return run(gs1_y__30_gcppos1) end
    if ai == 8011 then return run(gs1_n__12_nozeroprefix) end
    if ai == 8013 then return run(gs1_x__25_csumalpha_gcppos1) end
    if ai == 8014 then return run(gs1_x__25_csumalpha_gcppos1_hasnondigit) end
    if ai == 8017 or ai == 8018 then return run(gs1_n18_csum_gcppos1) end
    if ai == 8019 then return run(gs1_n__10) end
    if ai == 8020 then return run(gs1_x__25) end
    if ai == 8030 then return run(gs1_z__90) end
    if ai == 8040 or ai == 8041 then return run(gs1_n15) end
    if ai == 8042 then return run(gs1_n32) end
    if ai == 8043 then return run(gs1_n18__n__2_) end
  elseif ai < 8200 then
    if ai == 8110 then return run(gs1_x__70_couponcode) end
    if ai == 8111 then return run(gs1_n4) end
    if ai == 8112 then return run(gs1_x__70_couponposoffer) end
  elseif ai < 8300 then
    if ai == 8200 then return run(gs1_x__70) end
  end
  err.no = 1
  return false, err
end

return M
