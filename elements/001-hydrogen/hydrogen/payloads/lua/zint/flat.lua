-- src/zint/flat.lua -- Flattermarken, from backend/postal.c `zint_flat`.
--
-- A digit mark used in print finishing, not a postal symbol. Each digit is
-- a short run of element widths. Digit 1 is two elements; the others are
-- four. There is no human-readable line and no quiet zone.

local common = require "zint.common"

local M = {}

-- backend/postal.c `FlatTable`. Digit 1 is the two-character entry "18".
local FLAT = {
  "0504", "18", "0117", "0216", "0315",
  "0414", "0513", "0612", "0711", "0810",
}

function M.flat(symbol, source)
  if #source > 128 then
    return nil, string.format("Error 494: Input length %d too long (maximum 128)", #source)
  end
  local bad = source:find("[^0-9]")
  if bad then
    return nil, string.format(
      "Error 495: Invalid character at position %d in input (digits only)", bad)
  end
  local dest = {}
  for i = 1, #source do
    dest[#dest + 1] = FLAT[source:byte(i) - 47]
  end
  common.expand(symbol, table.concat(dest))
  return true
end

return M
