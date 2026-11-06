-- zint/code.lua -- ports the Code 39 part of backend/code.c
--
-- code.c in the real thing also holds Code 11, Code 93, PZN, LOGMARS, HIBC-39,
-- Excode 39 and Code 32. All of them are out of scope, and several of them are
-- Code 39 with a twist -- LOGMARS widens the wide bars, HIBC adds a check
-- character -- so the branches that mention them are dropped here rather than
-- ported and left unreachable.
--
-- Code 39 is the simplest symbology in the set: a fixed table lookup per
-- character with no state, no code-set switching and no checksum by default.
-- That is why it was first -- it exercises the whole output path while adding
-- almost nothing of its own to debug.

local common = require "zint.common"

local M = {}

local f32 = common.stripf

-- backend/common.c `to_upper()` -- ASCII only. Deliberately not Lua's
-- string.upper, which goes through the C library's toupper and so depends on
-- the locale. `[a-z]` in a Lua pattern is a byte range and does not.
local function to_upper(s)
  return (s:gsub("[a-z]", function(c) return string.char(c:byte() & 0x5F) end))
end

-- backend/code.c `SILVER` -- the Code 39 character set, in the order the
-- encoding table uses. Index into this and you have the index into C39Table.
-- The last four (a b c d) are Code 93 shift characters; Code 39 only ever
-- looks up the first 43, which is what the `limit` argument below enforces.
local SILVER = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%abcd"

-- backend/code.c `C39Table` -- ten element widths per character. Nine of them
-- are the character itself (five bars and four spaces, alternating, of which
-- exactly three are wide) and the tenth is the narrow inter-character gap.
-- Every row therefore sums to 6*1 + 3*2 + 1 = 13 modules.
--
-- Transcribed mechanically from the C rather than by hand, and checked on the
-- way across: 43 rows, 10 digits each, every row summing to 13.
local C39Table = {
  "1112212111", "2112111121", "1122111121", "2122111111",
  "1112211121", "2112211111", "1122211111", "1112112121",
  "2112112111", "1122112111", "2111121121", "1121121121",
  "2121121111", "1111221121", "2111221111", "1121221111",
  "1111122121", "2111122111", "1121122111", "1111222111",
  "2111111221", "1121111221", "2121111211", "1111211221",
  "2111211211", "1121211211", "1111112221", "2111112211",
  "1121112211", "1111212211", "2211111121", "1221111121",
  "2221111111", "1211211121", "2211211111", "1221211111",
  "1211112121", "2211112111", "1221112111", "1212121111",
  "1212111211", "1211121211", "1112121211",
}

-- backend/common.c `posn()`, precomputed. 0-based, matching the C, because the
-- result indexes C39Table and is also summed for the check digit.
local POSN = {}
for i = 1, #SILVER do POSN[SILVER:sub(i, i)] = i - 1 end

-- backend/common.c `is_sane_lookup()`, one character at a time. The limit is
-- what keeps the Code 93 shift characters out: they are in SILVER but are not
-- valid Code 39 input.
local function posn_sane(c, limit)
  local p = POSN[c]
  if p and p < limit then return p end
  return nil
end

-- backend/code.c `code39()`.
--
-- The task's inputs are always valid, so the length and character checks below
-- can never fire. They are kept because they are cheap and because a port that
-- silently produces a wrong symbol for bad input is worse than one that says
-- so -- but nothing downstream depends on the exact error text.
function M.code39(symbol, source)
  source = to_upper(source)

  if #source > 86 then
    -- 13 (start) + 86*13 + 12 (stop) is the widest the C's buffer allows.
    return nil, "323: Input too long (86 character maximum)"
  end

  local posns = {}
  for i = 1, #source do
    local p = posn_sane(source:sub(i, i), 43) -- up to "%"; excludes a b c d
    if not p then
      return nil, "324: Invalid character in data "
        .. "(alphanumerics, space and \"-.$/+%\" only)"
    end
    posns[i] = p
  end

  -- Start and stop are both the '*' pattern. The start carries a trailing gap
  -- like every data character; the stop does not, because nothing follows it
  -- -- hence 10 digits against 9.
  local dest = { "1211212111" }
  for i = 1, #source do
    dest[#dest + 1] = C39Table[posns[i] + 1]
  end
  -- option_2 selects a mod-43 check digit, visible or hidden. The command line
  -- would set it with --vers, which is outside the task's option set, so the
  -- whole branch is omitted along with the running `counter` it needs.
  dest[#dest + 1] = "121121211"

  common.expand(symbol, table.concat(dest))

  -- --compliantheight. ISO/IEC 16388:2007 4.4 (e) asks for 15% of the symbol's
  -- width excluding quiet zones, and since X is left to the application zint
  -- expresses the width in modules: (C + 2) * 9 + (C + 1) == 10 * C + 19, where
  -- C is the character count.
  --
  -- The default stays 50 unless the minimum exceeds it, so this only changes
  -- the output from 32 characters up -- (10 * 32 + 19) * 0.15 == 50.85. Below
  -- that the flag is real but invisible, which makes for a confusing test case.
  --
  -- option_2 == 1 is a visible mod-43 check digit, which adds a character to
  -- the symbol; the encoder's own check-digit branch is not ported (--vers is
  -- not accepted), so this reads as `length` in practice.
  if (symbol.output_options & common.COMPLIANT_HEIGHT) ~= 0 then
    local chars = symbol.option_2 == 1 and #source + 1 or #source
    -- 0.15f is not 0.15: narrowing the literal is as necessary as narrowing the
    -- result, and forgetting it is the classic way to be one ulp out.
    local min_height = f32(f32(10.0 * chars + 19.0) * f32(0.15))
    common.set_height(symbol, min_height, min_height > 50.0 and min_height or 50.0, 0.0)
  else
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  -- Code 39's human-readable line shows the start and stop characters, which
  -- is unusual -- most symbologies show the data alone. Note this uses the
  -- uppercased source, so lowercase input reads back uppercase.
  symbol.text = "*" .. source .. "*"

  return true
end

return M
