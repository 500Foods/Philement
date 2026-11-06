-- zint/reedsol.lua -- ports backend/reedsol.c
--
-- Cliff Hones' Reed-Solomon encoder, the systematic-encoder half only. zint
-- never decodes, so there is no syndrome or Berlekamp-Massey code to port: the
-- whole job is to divide the data polynomial by a generator polynomial over
-- GF(2^8) and keep the remainder.
--
-- `encode` does not reverse the remainder. QR and Data Matrix undo that
-- themselves. `encode_c` and `encode_uint` reverse, matching
-- `zint_rs_encode` and `zint_rs_encode_uint`. `uint_init_gf` builds the
-- log tables at runtime for Aztec's 10-bit and 12-bit fields, and
-- `uint_encode` reverses to match `zint_rs_uint_encode`.
--
-- Two things about the arithmetic are worth stating up front, because they are
-- the reason this file is short and fast rather than long and full of modulo:
--
--   * Multiplication in GF(2^8) is done as alog[log[a] + log[b]]. That is only
--     valid if the sum can exceed 254 without wrapping, so the antilog table is
--     stored at twice its period -- 510 entries, the second 255 a copy of the
--     first. See zint/reedsol_logs.lua, where the generator asserts exactly
--     that. No `% 255` appears anywhere below, and that is deliberate.
--   * log(0) is undefined, and the C stores a 0 in that slot and then leans on
--     a separate `zero` flag to avoid ever using it. The port keeps the same
--     shape rather than substituting nil, because a nil would turn a logic
--     error into an arithmetic error one frame further away from the cause.

local logs = require "zint.reedsol_logs"

local M = {}

-- backend/reedsol.c `rs_init_gf()`. The C indexes a small dispatch table by
-- `prime_poly >> 5` and stores raw pointers; here the field is looked up by the
-- polynomial itself, since there is no cycle to save. QR uses 0x11d. Data
-- Matrix uses 0x12d. Aztec's mode message uses 0x13. MaxiCode and Aztec's
-- 6-bit codewords use 0x43. The 10-bit and 12-bit fields are built by
-- `uint_init_gf`, not stored here.
local FIELDS = {
  [0x11d] = { logt = logs.logt_0x11d, alog = logs.alog_0x11d },
  [0x12d] = { logt = logs.logt_0x12d, alog = logs.alog_0x12d },
  [0x13]  = { logt = logs.logt_0x13,  alog = logs.alog_0x13 },
  [0x43]  = { logt = logs.logt_0x43,  alog = logs.alog_0x43 },
}

-- Returns the `rs_t` equivalent: a plain table carrying the field tables, and
-- later the generator polynomial that init_code() builds into it.
function M.init_gf(prime_poly)
  local field = FIELDS[prime_poly]
  if not field then
    error(("reedsol: no log tables for prime polynomial 0x%x"):format(prime_poly))
  end
  return { logt = field.logt, alog = field.alog }
end

-- backend/reedsol.c `rs_init_code()`. Builds the generator polynomial
--
--   g(x) = (x + a^index)(x + a^(index+1)) ... (x + a^(index+nsym-1))
--
-- one factor at a time, in place. `index` is 0 for QR (a^0 = 1 is the first
-- root); Data Matrix uses 1.
--
-- The inner loop is the schoolbook multiply-and-add: multiplying the running
-- polynomial by (x + a^index) means shifting every coefficient up one degree
-- (the `^= rspoly[k - 1]` term) and scaling the unshifted copy by a^index (the
-- alog/logt term). Walking k downwards lets both happen in one array.
function M.init_code(rs, nsym, index)
  local logt, alog = rs.logt, rs.alog
  local rspoly, log_rspoly = {}, {}

  rs.nsym = nsym

  rspoly[0] = 1
  for i = 1, nsym do
    rspoly[i] = 1
    for k = i - 1, 1, -1 do
      if rspoly[k] ~= 0 then
        rspoly[k] = alog[logt[rspoly[k]] + index]   -- multiply coeff by a^index
      end
      rspoly[k] = rspoly[k] ~ rspoly[k - 1]         -- add coeff of x^(k-1) * x
    end
    rspoly[0] = alog[logt[rspoly[0]] + index]
    index = index + 1
  end

  -- Cache the logs of the generator's coefficients so the encode loop is one
  -- table lookup rather than two, and record whether any coefficient is zero.
  -- A zero coefficient is the one case where log_rspoly[] holds the fake log(0)
  -- and must not be used; `zero` is how the encoder knows to check.
  rs.zero = false
  for i = 0, nsym do
    log_rspoly[i] = logt[rspoly[i]]   -- C comment: "For simplicity allow log of 0"
    if rspoly[i] == 0 then rs.zero = true end
  end

  rs.rspoly, rs.log_rspoly = rspoly, log_rspoly
end

-- backend/reedsol.c `rs_encode()`. Synthetic division of the data by the
-- generator, LFSR style: `res` is the remainder register, `m` the feedback term.
--
-- NOTE ON THE OUTPUT ORDER. The C's own comment says the codes are placed "in
-- reverse order in res", and it means it -- res[0] is the last ECC codeword.
-- Every caller un-reverses it on the way out; qr_add_ecc reads
-- `ecc_block[ecc_block_length - j - 1]`. Getting this backwards produces a
-- symbol with a plausible shape and wrong ECC, which no amount of staring at
-- the module grid will reveal, so the reversal is left in the caller exactly
-- where the C puts it rather than being tidied away here.
--
-- The C splits this into two near-identical loops, one with the `rspoly[k]`
-- test in the inner loop and one without, purely to avoid a branch when the
-- generator has no zero coefficients. That is a micro-optimisation, not a
-- semantic difference: when `zero` is false, `rspoly[k]` is never 0 and the
-- test is always true. One loop here, same results.
--
-- `data` and `res` are 0-based, matching the C.
function M.encode(rs, datalen, data, res)
  local logt, alog = rs.logt, rs.alog
  local rspoly, log_rspoly = rs.rspoly, rs.log_rspoly
  local nsym = rs.nsym

  for i = 0, nsym - 1 do res[i] = 0 end

  for i = 0, datalen - 1 do
    local m = res[nsym - 1] ~ data[i]
    if m ~= 0 then
      local log_m = logt[m]
      for k = nsym - 1, 1, -1 do
        if rspoly[k] ~= 0 then
          res[k] = res[k - 1] ~ alog[log_m + log_rspoly[k]]
        else
          res[k] = res[k - 1]
        end
      end
      res[0] = alog[log_m + log_rspoly[0]]
    else
      -- Feedback is zero, so the register just shifts up: the C does this with
      -- a memmove and a store, which is the same thing.
      for k = nsym - 1, 1, -1 do res[k] = res[k - 1] end
      res[0] = 0
    end
  end
end

-- backend/reedsol.c reverses the remainder before `zint_rs_encode` and
-- `zint_rs_encode_uint` return. QR and Data Matrix still call `encode` and
-- undo it themselves. Callers that copy `ecc[j]` straight across use this.
local function reverse_res(res, nsym)
  local half = nsym // 2
  for i = 0, half - 1 do
    res[i], res[nsym - 1 - i] = res[nsym - 1 - i], res[i]
  end
end

function M.encode_c(rs, datalen, data, res)
  M.encode(rs, datalen, data, res)
  reverse_res(res, rs.nsym)
end

-- Same remainder as `encode_c`. The C takes `unsigned int` data so a codeword
-- can sit above 255, which this loop already allows: it never masks to a byte.
M.encode_uint = M.encode_c

-- backend/reedsol.c `zint_rs_uint_init_gf()`. `logmod` is 1023 for 10-bit
-- Aztec and 4095 for 12-bit. The antilog table is doubled so the encoder
-- never reduces an index modulo `logmod`.
function M.uint_init_gf(prime_poly, logmod)
  local b = logmod + 1
  local logt, alog = {}, {}
  logt[0] = 0
  local p = 1
  for v = 0, logmod - 1 do
    alog[v] = p
    alog[logmod + v] = p
    logt[p] = v
    p = p << 1
    if (p & b) ~= 0 then p = p ~ prime_poly end
  end
  return { logt = logt, alog = alog }
end

-- backend/reedsol.c `zint_rs_uint_init_code()`. Same generator as `init_code`.
-- Coefficients stay inside the field, so the C's unsigned-short store does
-- not truncate anything this port builds.
function M.uint_init_code(rs, nsym, index)
  M.init_code(rs, nsym, index)
end

-- backend/reedsol.c `zint_rs_uint_encode()`, including the reverse.
function M.uint_encode(rs, datalen, data, res)
  M.encode(rs, datalen, data, res)
  reverse_res(res, rs.nsym)
end

return M
