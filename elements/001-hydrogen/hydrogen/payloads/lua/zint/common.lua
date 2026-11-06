-- zint/common.lua -- ports backend/common.c (and the bits of backend/library.c
-- that set a symbol up).
--
-- Most of this module exists for one reason: single precision. zint carries
-- every coordinate as a C float, and a coordinate that has drifted by a single
-- ulp formats to a different string, so the SVG stops being byte-identical.
-- Lua numbers are doubles, so anything the C computes in float has to be
-- narrowed back after each step. See `stripf` below.
--
-- The other half of the module is the module bitmap: the 2-D grid of set and
-- unset modules that every encoder writes into and that vector.lua reads back
-- out. Indices are 0-based throughout, matching the C, because every loop bound
-- in the encoders is copied from the C and translating them would be a
-- reliable source of off-by-ones.

local M = {}

local pack, unpack = string.pack, string.unpack
local floor, ceil = math.floor, math.ceil

-- ------------------------------------------------------------ single precision

-- backend/common.c `stripf()`. There it is a volatile round-trip whose job is
-- to stop the compiler holding excess x87 precision; here it does the actual
-- narrowing to float32.
--
-- `string.pack("<f", x)` writes x as a 4-byte IEEE single, rounding to nearest;
-- unpacking reads it back as a double. The pair is exactly "round this double
-- to float32", which is all we need.
--
-- Why "compute in double, then narrow" reproduces native float arithmetic
-- exactly -- it is not obvious, because rounding twice can in general land
-- somewhere a single rounding would not:
--
--   * On x86-64 a C float operation is an SSE single-precision instruction. It
--     rounds to float32 immediately, so there is no hidden extra precision
--     carried between statements the way there was with x87.
--   * The exact mathematical result of + - * / on two float32 values needs at
--     most 2*24+1 = 49 mantissa bits before you can decide how it rounds. A
--     double carries 53. So the intermediate double is always faithful enough
--     that narrowing it lands where the hardware would have.
--
-- That guarantee is per operation. It does *not* survive chaining several
-- operations before narrowing, which is why every caller narrows after each
-- step and brackets the narrowing the way C brackets the expression.
function M.stripf(x)
  return (unpack("<f", pack("<f", x)))
end

local f32 = M.stripf

-- C `roundf()`: nearest, ties away from zero. Deliberately not the usual
-- `math.floor(x + 0.5)` shorthand, which rounds -0.5 to 0 where C gives -1.
-- Only the CMYK colour conversion reaches this with a value that could be
-- negative, but the asymmetry is free to get right.
function M.roundf(x)
  if x >= 0 then return floor(x + 0.5) end
  return ceil(x - 0.5)
end

-- ----------------------------------------------------------------- characters

-- backend/common.h `ctoi()` -- hex digit to value, -1 if it is not one.
-- Byte comparisons rather than `tonumber(c, 16)` so the result does not depend
-- on the locale or on Lua's numeric parsing accepting things C would not.
function M.ctoi(c)
  local b = c:byte()
  if not b then return -1 end
  if b >= 0x30 and b <= 0x39 then return b - 0x30 end -- '0'-'9'
  if b >= 0x41 and b <= 0x46 then return b - 0x37 end -- 'A'-'F'
  if b >= 0x61 and b <= 0x66 then return b - 0x57 end -- 'a'-'f'
  return -1
end

-- ----------------------------------------------------------------- UTF-8

-- backend/common.c `decode_utf8()` -- Bjoern Hoehrmann's UTF-8 DFA, which zint
-- uses verbatim (https://bjoern.hoehrmann.de/utf-8/decoder/dfa/) and which is
-- therefore reproduced verbatim here rather than replaced with a hand-rolled
-- validator. The DFA is what decides which byte sequences zint calls valid, and
-- the interesting cases are precisely the ones an obvious implementation gets
-- wrong: overlong encodings, UTF-16 surrogates, and anything past U+10FFFF.
--
-- Three states matter to callers: 0 means a codepoint has just been completed,
-- 12 means the input is rejected and will stay rejected, anything else means
-- mid-sequence.
--
-- The class table is the first 256 bytes of the C's `utf8d`, written as the
-- byte ranges it actually encodes rather than as 256 literals.
local UTF8_CLASS = {}
do
  local RANGES = {
    { 0x00, 0x7f,  0 }, -- ASCII: a complete codepoint on its own
    { 0x80, 0x8f,  1 }, -- continuation bytes, split three ways because which
    { 0x90, 0x9f,  9 }, -- ones are legal depends on the lead byte that
    { 0xa0, 0xbf,  7 }, -- preceded them
    { 0xc0, 0xc1,  8 }, -- two-byte leads that could only be overlong
    { 0xc2, 0xdf,  2 },
    { 0xe0, 0xe0, 10 }, -- three-byte lead needing an a0..bf continuation
    { 0xe1, 0xec,  3 },
    { 0xed, 0xed,  4 }, -- three-byte lead that must not reach a surrogate
    { 0xee, 0xef,  3 },
    { 0xf0, 0xf0, 11 }, -- four-byte lead needing a 90..bf continuation
    { 0xf1, 0xf3,  6 },
    { 0xf4, 0xf4,  5 }, -- four-byte lead capped at U+10FFFF
    { 0xf5, 0xff,  8 }, -- past the end of Unicode
  }
  for _, r in ipairs(RANGES) do
    for b = r[1], r[2] do UTF8_CLASS[b] = r[3] end
  end
end

-- The second part of `utf8d`: `state + class` indexes straight into it, so it
-- is one flat 0-based table rather than a table of rows. States are numbered in
-- steps of 12 for exactly that reason.
local UTF8_NEXT = {
  [0] = 0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, -- state  0: at a boundary
       12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, -- state 12: rejected, absorbing
       12,  0, 12, 12, 12, 12, 12,  0, 12,  0, 12, 12, -- state 24: one byte to go
       12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, -- state 36: two bytes to go
       12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, -- state 48: after e0
       12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, -- state 60: after ed
       12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, -- state 72: after f0
       12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, -- state 84: after f1..f3
       12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, -- state 96: after f4
}

-- Feeds one byte to the DFA. C passes state and codepoint by pointer; here they
-- go in and come back out, in that order.
function M.decode_utf8(state, codep, byte)
  local class = UTF8_CLASS[byte]
  if state ~= 0 then
    codep = (byte & 0x3f) | (codep << 6)
  else
    -- The class doubles as a mask width: a lead byte's payload is whatever
    -- survives shifting 0xff right by its class.
    codep = (0xff >> class) & byte
  end
  return UTF8_NEXT[state + class], codep
end

-- backend/common.c `is_valid_utf8()`. Note the two ways to fail: a byte that is
-- rejected outright, and running out of input mid-sequence -- which is why the
-- final state has to be checked as well as each transition.
function M.is_valid_utf8(source)
  local state, codep = 0, 0
  for i = 1, #source do
    state, codep = M.decode_utf8(state, codep, source:byte(i))
    if state == 12 then return false end
  end
  return state == 0
end

-- ----------------------------------------------------------- symbology tests

-- backend/common.c `is_ean()`. The EAN side of the family, as opposed to UPC-A
-- and UPC-E. Quiet zones and the human-readable flag key off this, because an
-- EAN-13's text is 13 characters with no add-on and a UPC-A's is 12, so a
-- shared "longer than 12" test would steal the right quiet zone.
--
-- Encoded: EAN-8 (10), EAN-2 (11), EAN-5 (12), EAN-13 (15), ISBN (69).
-- The legacy EANX ids are not.
function M.is_ean(symbology)
  return symbology == 10 or symbology == 11 or symbology == 12
      or symbology == 15 or symbology == 69
end

-- backend/common.c `is_upcean()`. The C lists the EAN, UPC and ISBN variants
-- plus their composite forms. Encoded here: EAN-8 (10), EAN-2 (11), EAN-5
-- (12), EAN-13 (15), UPC-A (34 and 35), UPC-E (37 and 38), and ISBN (69).
--
-- Four files branch on this predicate and they must agree: output.lua sizes
-- the quiet zones for the outside digits, vector.lua splits the text and hangs
-- the guard bars below the rest, svg.lua switches the SVG face to OCR-B, and
-- pdf.lua returns that face's TrueType bytes. Any one of them disagreeing
-- produces a symbol that is subtly, consistently wrong.
function M.is_upcean(symbology)
  return M.is_ean(symbology) or symbology == 34 or symbology == 35
      or symbology == 37 or symbology == 38
end

-- ------------------------------------------------------- flags and constants

-- backend/zint.h `output_options` bits. These are a bit field on the symbol
-- rather than separate booleans because that is how the C carries them, and
-- several places test two or three at once -- see the bind/box block in
-- vector.lua, which asks for BOX|BIND|BIND_TOP in a single test.
M.BARCODE_BIND_TOP        = 0x0001 -- boundary bar above the symbol only
M.BARCODE_BIND            = 0x0002 -- boundary bars above and below
M.BARCODE_BOX             = 0x0004 -- box around the symbol
M.BARCODE_STDOUT          = 0x0008
M.READER_INIT             = 0x0010 -- reader initialisation (programming) symbol
M.SMALL_TEXT              = 0x0020 -- font height 5 rather than 7
M.BOLD_TEXT               = 0x0040
M.CMYK_COLOUR             = 0x0080 -- EPS/TIF only, so inert for us
M.BARCODE_DOTTY_MODE      = 0x0100 -- circles rather than squares
M.GS1_GS_SEPARATOR        = 0x0200
M.OUT_BUFFER_INTERMEDIATE = 0x0400
M.BARCODE_QUIET_ZONES     = 0x0800 -- add the standards' quiet zones
M.BARCODE_NO_QUIET_ZONES  = 0x1000 -- and this takes them away again
M.COMPLIANT_HEIGHT        = 0x2000 -- use the standard's height, not zint's 50
M.EANUPC_GUARD_WHITESPACE = 0x4000 -- "<" / ">" quiet zone indicators
M.EMBED_VECTOR_FONT       = 0x8000 -- inline the WOFF2 as a @font-face

-- backend/zint.h `input_mode`. The low three bits are an enum (DATA / UNICODE /
-- GS1) and everything above is a flag, which is why --binary and --gs1 mask
-- with ~0x07 while the rest OR themselves in.
M.DATA_MODE         = 0
M.UNICODE_MODE      = 1
M.GS1_MODE          = 2
M.ESCAPE_MODE       = 0x0008
M.GS1PARENS_MODE    = 0x0010
M.GS1NOCHECK_MODE   = 0x0020
M.HEIGHTPERROW_MODE = 0x0040
M.FAST_MODE         = 0x0080
M.EXTRA_ESCAPE_MODE = 0x0100

-- ------------------------------------------------------------------ escapes

-- backend/library.c `esc_base()` -- the numeric escapes. Returns the byte
-- value, or nil and a message.
--
-- `\d` and `\o` take exactly three digits, `\x` exactly two, and all three
-- reject a digit outside their base rather than stopping early: "\d1A2" is an
-- error, not a 1 followed by "A2".
local function esc_base(s, in_posn, base)
  local min_len = base == "x" and 2 or 3
  if in_posn + min_len > #s then
    return nil, "232: Incomplete '\\" .. base .. "' escape sequence in input data"
  end

  local c1 = M.ctoi(s:sub(in_posn + 1, in_posn + 1))
  local c2 = M.ctoi(s:sub(in_posn + 2, in_posn + 2))
  local c3 = M.ctoi(s:sub(in_posn + 3, in_posn + 3))

  if base == "d" then
    if c1 >= 0 and c1 <= 9 and c2 >= 0 and c2 <= 9 and c3 >= 0 and c3 <= 9 then
      return c1 * 100 + c2 * 10 + c3
    end
  elseif base == "o" then
    if c1 >= 0 and c1 <= 7 and c2 >= 0 and c2 <= 7 and c3 >= 0 and c3 <= 7 then
      return (c1 << 6) | (c2 << 3) | c3
    end
  else
    if c1 >= 0 and c2 >= 0 then return (c1 << 4) | c2 end
  end

  local named = base == "d" and "decimal" or base == "o" and "octal" or "hexadecimal"
  return nil, "233: Invalid character for '\\" .. base
    .. "' escape sequence in input data (" .. named .. " only)"
end

-- The single-character escapes, C's switch collapsed into a table. Mostly the
-- familiar C ones; `\E`, `\G` and `\R` are zint's own, for the transmission
-- control characters a barcode reader may be asked to emit.
local SIMPLE_ESCAPE = {
  ["0"] = 0x00, -- Null
  ["E"] = 0x04, -- End of Transmission
  ["a"] = 0x07, -- Bell
  ["b"] = 0x08, -- Backspace
  ["t"] = 0x09, -- Horizontal tab
  ["n"] = 0x0a, -- Line feed
  ["v"] = 0x0b, -- Vertical tab
  ["f"] = 0x0c, -- Form feed
  ["r"] = 0x0d, -- Carriage return
  ["e"] = 0x1b, -- Escape
  ["G"] = 0x1d, -- Group Separator
  ["R"] = 0x1e, -- Record Separator
  ["\\"] = 0x5c,
}

-- backend/library.c `escape_char_process()` -- what --esc turns on. Returns the
-- de-escaped string, or nil and a message.
--
-- The C runs this twice, once to size a buffer and once to fill it; only the
-- filling pass is here, since a Lua table needs no sizing. It also runs it per
-- segment, and this port has no multi-segment input.
--
-- `\^` is the odd one out: it is *not* expanded. Under --extraesc it and the
-- character after it are copied through verbatim so that code128.lua can find
-- them later, which is why `\^A` survives a pass that turns `\n` into a byte.
function M.escape_char_process(symbol, source)
  local out = {}
  local in_posn = 1 -- 1-based, unlike the C
  local length = #source
  local extra_escape_mode = (symbol.input_mode & M.EXTRA_ESCAPE_MODE) ~= 0
                            and symbol.symbology == 20 -- BARCODE_CODE128

  while in_posn <= length do
    local c = source:sub(in_posn, in_posn)
    if c ~= "\\" then
      out[#out + 1] = c
      in_posn = in_posn + 1
    else
      if in_posn + 1 > length then
        return nil, "236: Incomplete escape character in input data"
      end
      local ch = source:sub(in_posn + 1, in_posn + 1)
      local simple = SIMPLE_ESCAPE[ch]

      if simple then
        out[#out + 1] = string.char(simple)
        in_posn = in_posn + 2

      elseif ch == "^" then
        if not extra_escape_mode then
          return nil, "798: Escape '\\^' only valid for Code 128 in extra escape mode"
        end
        out[#out + 1] = "\\^"
        in_posn = in_posn + 2
        -- The character after the marker rides along, and the C explicitly
        -- allows a `\^` with nothing after it at the very end of the input.
        if in_posn <= length then
          out[#out + 1] = source:sub(in_posn, in_posn)
          in_posn = in_posn + 1
        end

      elseif ch == "d" or ch == "o" or ch == "x" then
        local val, err = esc_base(source, in_posn + 1, ch)
        if not val then return nil, err end
        out[#out + 1] = string.char(val)
        in_posn = in_posn + (ch == "x" and 4 or 5)

      elseif ch == "u" or ch == "U" then
        -- Four hex digits for \u, six for \U, then encoded as UTF-8. Note the
        -- digits are required to be present even when the value would fit in
        -- fewer: \u41 is an error, A is "A".
        local ndigits = ch == "u" and 4 or 6
        if in_posn + 1 + ndigits > length then
          return nil, "209: Incomplete '\\" .. ch .. "' escape sequence in input data"
        end
        local unicode = 0
        for i = 1, ndigits do
          local val = M.ctoi(source:sub(in_posn + 1 + i, in_posn + 1 + i))
          if val == -1 then
            return nil, "211: Invalid character for '\\" .. ch
              .. "' escape sequence in input data (hexadecimal only)"
          end
          unicode = (unicode << 4) | val
        end
        -- Reversed BOM, the UTF-16 surrogate range and anything above the
        -- Unicode maximum. All three are unencodable rather than merely odd.
        if unicode == 0xfffe or (unicode >= 0xd800 and unicode < 0xe000)
            or unicode > 0x10ffff then
          return nil, "246: Invalid value for '\\" .. ch .. "' escape sequence in input data"
        end
        -- utf8.char would do this, but it accepts values this rejects and its
        -- range differs by Lua version; the explicit form matches the C.
        if unicode < 0x80 then
          out[#out + 1] = string.char(unicode)
        elseif unicode < 0x800 then
          out[#out + 1] = string.char(0xC0 | (unicode >> 6), 0x80 | (unicode & 0x3F))
        elseif unicode < 0x10000 then
          out[#out + 1] = string.char(0xE0 | (unicode >> 12),
                                      0x80 | ((unicode >> 6) & 0x3F),
                                      0x80 | (unicode & 0x3F))
        else
          out[#out + 1] = string.char(0xF0 | (unicode >> 18),
                                      0x80 | ((unicode >> 12) & 0x3F),
                                      0x80 | ((unicode >> 6) & 0x3F),
                                      0x80 | (unicode & 0x3F))
        end
        in_posn = in_posn + 2 + ndigits

      else
        return nil, "234: Unrecognised escape character '\\" .. ch .. "' in input data"
      end
    end
  end

  return table.concat(out)
end

-- --------------------------------------------------------------- the symbol

-- backend/library.c `set_symbol_defaults()`, plus the two fields
-- frontend/main.c overrides before parsing options (`input_mode`, which the CLI
-- raises from DATA_MODE to UNICODE_MODE, and the output file name, which we do
-- not carry).
--
-- These values are not cosmetic. Several of them decide whether an attribute
-- appears in the output at all -- see `show_hrt` (no text line when 0) and the
-- colour strings (an alpha of 00 suppresses the background rect entirely). Any
-- default that looks arbitrary was copied from the C rather than chosen.
function M.new_symbol()
  return {
    symbology = 20,       -- BARCODE_CODE128, which is also what an omitted -b gives
    scale = 1.0,          -- vector.lua doubles this; 1.0 means the usual 2x output
    fgcolour = "000000",
    bgcolour = "ffffff",
    show_hrt = 1,         -- human readable text on
    input_mode = M.UNICODE_MODE, -- main.c:1440 -- the *CLI* default, not the library's
    dot_size = 4.0 / 5.0, -- --dotsize; only read in dotty mode
    text_gap = 1.0,       -- gap between symbol and text, in modules
    guard_descent = 5.0,  -- UPC/EAN guard bar overhang
    dpmm = 0.0,           -- set by --scalexdimdp; raster metadata, inert for SVG
    option_1 = -1,        -- per-symbology; -1 means "not specified"
    option_2 = 0,
    option_3 = 0,
    height = 0.0,         -- 0 means "encoder picks", via set_height below
    rows = 0,
    width = 0,
    whitespace_width = 0,  -- --whitesp
    whitespace_height = 0, -- --vwhitesp
    border_width = 0,      -- --border; inert unless --bind/--bindtop/--box
    output_options = 0,    -- the bit field above
    row_height = {},      -- 0-based
    encoded = {},         -- 0-based rows of 0-based module flags
    text = "",            -- human readable line, set by the encoder
    primary = "",         -- MaxiCode structured primary, 127 characters
    -- The C's `errtxt` in its warning role. An encoder that finishes but has
    -- something to say leaves it here and the driver prints it once, after
    -- encoding, which is where main.c prints it too. Errors do not come through
    -- this field -- they are a nil return.
    warning = nil,
  }
end

-- Modules are indexed exactly as in C: row and column both from zero. Rows are
-- created lazily and hold only the set modules, so `encoded` is sparse -- which
-- is why `module_is_set` has to cope with a missing row rather than assuming
-- the grid was pre-filled.
function M.set_module(symbol, y, x)
  local row = symbol.encoded[y]
  if not row then
    row = {}
    symbol.encoded[y] = row
  end
  row[x] = true
end

-- Returns 1 or 0, not true/false. vector.lua compares two of these for equality
-- when it measures a run of like modules, and 1/0 keeps that comparison the
-- same shape as the C.
function M.module_is_set(symbol, y, x)
  local row = symbol.encoded[y]
  return (row and row[x]) and 1 or 0
end

-- backend/common.c `expand()` -- turn a run-length string of alternating bar
-- and space widths into set modules. The string is what every linear encoder
-- builds: "1211212111..." where each digit is the width in modules of the next
-- element. It always starts on a bar, and `latch` flips on every element.
--
-- Note this appends a row and widens the symbol; an encoder that calls it twice
-- gets a two-row symbol, which is how the stacked symbologies work. None of
-- ours do, but the bookkeeping is the C's and is left intact.
function M.expand(symbol, data)
  local row = symbol.rows
  symbol.rows = symbol.rows + 1

  local writer = 0
  local latch = true -- true = bar

  for reader = 1, #data do
    local num = M.ctoi(data:sub(reader, reader))
    for _ = 1, num do
      if latch then M.set_module(symbol, row, writer) end
      writer = writer + 1
    end
    latch = not latch
  end

  if writer > symbol.width then symbol.width = writer end
end

-- backend/common.c `set_height()`. Decides the height of every row that the
-- encoder left at zero, splitting whatever height is going between them.
--
-- The three arguments are the symbology's opinion: the minimum a standard
-- allows, what to use when nobody asked for anything, and a ceiling. Which of
-- the three an encoder passes depends on COMPLIANT_HEIGHT: without it every
-- encoder passes a min and max of zero and only the default matters, which is
-- why plain output is 50 modules tall whatever the symbology says.
--
-- Breaching min or max is a warning, not an error: the symbol is still drawn at
-- the height it computed and the file is still written, so the SVG bytes are
-- the same either way. The message is still produced, because "same bytes" is
-- not the same claim as "same behaviour" -- see the note on `symbol.warning`.
--
-- The C signals it by return code and leaves the text in `symbol->errtxt` for
-- main.c to print. Here it goes straight onto the symbol: the port already
-- distinguishes an error from a warning by whether a function returns nil, so
-- the second channel the C needs for severity is redundant. Only the last
-- warning survives, as in the C -- errtxt is one buffer.
function M.set_height(symbol, min_row_height, default_height, max_height)
  local fixed_height = 0.0
  local zero_count = 0
  local rows = symbol.rows > 0 and symbol.rows or 1

  for i = 0, rows - 1 do
    local rh = symbol.row_height[i] or 0.0
    if rh ~= 0 then
      fixed_height = f32(fixed_height + rh)
    else
      zero_count = zero_count + 1
    end
  end

  if zero_count > 0 then
    local row_height
    if symbol.height ~= 0 then
      -- --height was given. --heightperrow reinterprets it as the height of
      -- each row rather than of the symbol; for the single-row symbologies
      -- here the two readings coincide, and it is the stacked ones (and QR,
      -- where it is rejected) that tell them apart.
      if (symbol.input_mode & M.HEIGHTPERROW_MODE) ~= 0 then
        row_height = f32(symbol.height)
      else
        row_height = f32((symbol.height - fixed_height) / zero_count)
      end
    elseif default_height ~= 0 then
      row_height = f32(default_height / zero_count)
    else
      row_height = f32(min_row_height)
    end
    if row_height < 0.5 then row_height = 0.5 end -- absolute minimum
    -- Note the test is on the *row* height against the *row* minimum, before
    -- the rows are added back up. A five-row symbol 100 tall is compliant if
    -- each row clears the bar, not if the total does.
    --
    -- min_row_height is zero unless --compliantheight was given, and zero is
    -- the "no opinion" value rather than a floor of nothing -- hence the guard.
    if min_row_height ~= 0 and f32(row_height) < f32(min_row_height) then
      symbol.warning = "Warning 247: Height not compliant with standards (too small)"
    end
    -- Recomputed rather than kept, so symbol.height is exactly the sum of the
    -- rows -- which is what vector.lua goes on to use.
    symbol.height = f32(f32(row_height * zero_count) + fixed_height)
  else
    symbol.height = f32(fixed_height) -- every row fixed; any given height is ignored
  end

  -- The ceiling, unlike the floor, is tested against the whole symbol. No
  -- symbology in this port passes a non-zero max -- the stacked ones are where
  -- it earns its keep -- so this cannot fire yet.
  if max_height ~= 0 and f32(symbol.height) > f32(max_height) then
    symbol.warning = string.format(
      "Warning 248: Height not compliant with standards (maximum %.4g)", max_height)
  end
end

return M
