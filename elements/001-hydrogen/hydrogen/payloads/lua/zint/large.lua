-- zint/large.lua -- the 128-bit helpers USPS Intelligent Mail uses.
--
-- backend/large.c. Only the operations `zint_usps_imail` calls are here:
-- load a decimal string, add, multiply, divide, clear a bit, and split the
-- result into bytes. A value is `{lo, hi}`, two Lua integers holding the
-- unsigned 64-bit halves. Arithmetic wraps the way `uint64_t` does. Comparisons
-- are unsigned, because a Lua `<` is signed and these values cross 2^63.

local M = {}

local MASK32 = 0xFFFFFFFF
local BASE32 = 0x100000000 -- 2^32

-- Unsigned less-than on two Lua integers.
local function ult(a, b)
  local as, bs = a < 0, b < 0
  if as ~= bs then return not as end
  return a < b
end

-- `qhat >= base or tmp > sum`, unsigned. The C `||` skips the sum when the
-- first test is true; the sum is still a wrapping uint64 add.
local function uge_or(qhat, base, tmp, sum)
  if not ult(qhat, base) then return true end
  return ult(sum, tmp)
end

-- Unsigned 64/64. Returns quotient, remainder. Both are Lua integers.
local function udivmod(n, d)
  if d == 0 then error("large: divide by zero") end
  if n >= 0 and d > 0 then return n // d, n % d end
  local q, r, extra = 0, 0, 0
  for i = 63, 0, -1 do
    extra = (r < 0) and 1 or 0
    r = (r << 1) | ((n >> i) & 1)
    if extra == 1 or not ult(r, d) then
      local low = r - d
      if extra == 1 and ult(r, d) then
        extra = 0
      else
        extra = extra == 1 and 1 or 0
      end
      r = low
      q = q | (1 << i)
    end
  end
  return q, r
end

-- backend/large.c `zint_large_load_str_u64()`. Stops at the first non-digit.
-- The running total is a uint64, so it wraps rather than growing into `hi`.
function M.load_str_u64(s, length)
  local val = 0
  length = length or #s
  for i = 1, length do
    local c = s:byte(i)
    if c < 48 or c > 57 then break end
    val = val * 10 + (c - 48)
  end
  return { lo = val, hi = 0 }
end

function M.copy(t)
  return { lo = t.lo, hi = t.hi }
end

-- backend/large.c `zint_large_add_u64()`.
function M.add_u64(t, s)
  local sum = t.lo + s
  if ult(sum, s) then t.hi = t.hi + 1 end
  t.lo = sum
end

-- backend/large.c `zint_large_mul_u64()`. Schoolbook product of the two
-- 32-bit halves. Every step wraps at 64 bits.
function M.mul_u64(t, s)
  local thi = t.hi
  local tlo0 = t.lo & MASK32
  local tlo1 = t.lo >> 32
  local s0 = s & MASK32
  local s1 = s >> 32
  local tmp = s0 * tlo0
  local p00 = tmp & MASK32
  tmp = (s1 * tlo0) + (tmp >> 32)
  local k10 = tmp >> 32
  tmp = (s0 * tlo1) + (tmp & MASK32)
  t.lo = (tmp << 32) + p00
  t.hi = (s1 * tlo1) + k10 + (tmp >> 32) + thi * s
end

-- backend/large.c `zint_large_div_u64()`. Replaces `t` with the quotient and
-- returns the remainder. `v` is a uint64.
function M.div_u64(t, v)
  local qhi = 0
  if ult(v, BASE32) then
    local qh, _ = udivmod(t.hi, v)
    qhi = qh
    local tmp = ((t.hi - qhi * v) << 32) + (t.lo >> 32)
    local qhat1, _ = udivmod(tmp, v)
    tmp = ((tmp - qhat1 * v) << 32) + (t.lo & MASK32)
    local qhat0, rem = udivmod(tmp, v)
    t.lo = (qhat1 << 32) | qhat0
    t.hi = qhi
    return rem
  end

  if not ult(t.hi, v) then
    local q, r = udivmod(t.hi, v)
    qhi = q
    t.hi = r
  end

  local norm_shift = 0
  local x = v
  if x ~= 0 then
    local n, y = 64, x >> 32
    if y ~= 0 then n = n - 32; x = y end
    y = x >> 16; if y ~= 0 then n = n - 16; x = y end
    y = x >> 8;  if y ~= 0 then n = n - 8;  x = y end
    y = x >> 4;  if y ~= 0 then n = n - 4;  x = y end
    y = x >> 2;  if y ~= 0 then n = n - 2;  x = y end
    y = x >> 1;  if y ~= 0 then n = n - 1;  x = y end
    norm_shift = n - x
  end

  v = v << norm_shift
  local vn1 = v >> 32
  local vn0 = v & MASK32
  local tnhi, tnlo
  if norm_shift > 0 then
    tnhi = (t.hi << norm_shift) | (t.lo >> (64 - norm_shift))
    tnlo = t.lo << norm_shift
  else
    tnhi = t.hi
    tnlo = t.lo
  end
  local tnlo1 = tnlo >> 32
  local tnlo0 = tnlo & MASK32

  local qhat1, rhat = udivmod(tnhi, vn1)
  local tmp = qhat1 * vn0
  while uge_or(qhat1, BASE32, tmp, (rhat << 32) + tnlo1) do
    qhat1 = qhat1 - 1
    rhat = rhat + vn1
    if not ult(rhat, BASE32) then break end
    tmp = tmp - vn0
  end

  local rnhilo1 = (tnhi << 32) + tnlo1 - (qhat1 * v)
  local qhat0
  qhat0, rhat = udivmod(rnhilo1, vn1)
  tmp = qhat0 * vn0
  while uge_or(qhat0, BASE32, tmp, (rhat << 32) + tnlo0) do
    qhat0 = qhat0 - 1
    rhat = rhat + vn1
    if not ult(rhat, BASE32) then break end
    tmp = tmp - vn0
  end

  t.lo = (qhat1 << 32) | qhat0
  t.hi = qhi
  return ((rnhilo1 << 32) + tnlo0 - (qhat0 * v)) >> norm_shift
end

-- backend/large.c `zint_large_unset_bit()`.
function M.unset_bit(t, bit)
  if bit < 64 then
    t.lo = t.lo & ~(1 << bit)
  elseif bit < 128 then
    t.hi = t.hi & ~(1 << (bit - 64))
  end
end

-- backend/large.c `zint_large_uchar_array()`, little-endian chunks of `bits`
-- bits. The result is 0-based. IMail asks for 13 bytes of 8 bits.
function M.uchar_array(t, size, bits)
  if bits <= 0 then
    bits = 8
  elseif bits > 32 then
    bits = 32
  end
  local mask = ~((-1) << bits)
  local out = {}
  for i = 0, size - 1 do out[i] = 0 end
  local i, j = 0, 0
  while i < size and j < 64 do
    out[size - 1 - i] = (t.lo >> j) & mask
    i = i + 1
    j = j + bits
  end
  if i < size then
    if j ~= 64 then
      j = j - 64
      local hi_low = t.hi & ~((-1) << j)
      out[size - i] = (hi_low << (bits - j)) | ((t.lo >> (64 - (bits - j))) & mask)
    else
      j = 0
    end
    while i < size and j < 64 do
      out[size - 1 - i] = (t.hi >> j) & mask
      i = i + 1
      j = j + bits
    end
  end
  return out
end

return M
