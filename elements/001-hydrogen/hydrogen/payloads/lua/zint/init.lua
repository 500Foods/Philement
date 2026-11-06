-- src/zint/init.lua -- the pure-Lua zint port.
--
-- Library entry point. Callers use `local zint = require "zint"`. There is no
-- forced global. `zint.barcode(opts)` returns an SVG document, or a PDF form
-- table when opts.filetype is "PDF".
--
-- opts fields are named after the zint long option they come from and take the
-- same values. A flag is on when its key is truthy. A valued option may be
-- given as a string or a number. Strings are parsed the way the 2.16.0 CLI
-- parses them, which is why the command-line wrapper passes the text through
-- unchanged. Float parsing lives in options.lua.
--
--   barcode       number or name; defaults to 20 (CODE128) when absent
--   data          symbol data content
--   output        path to write the SVG to
--   fg, bg        colour as RGB, RGBA, or "C,M,Y,K"
--   filetype      "SVG" (default) or "PDF" (form XObject content stream)
--   gs1           treat input as GS1 data
--   gs1parens     "()" rather than "[]" as AI delimiters
--   nobackground  omit the background rect
--
--   scale         output scale multiplier, >= 0.01
--   scalexdimdp   "xdim[,resolution]", e.g. "0.33mm,600dpi" -- overrides scale
--   height        symbol height in modules, 0.5 to 2000
--   heightperrow  read height as per-row rather than overall
--   compliantheight  use the standard's height rather than zint's default 50
--   border        border width in modules, 0 to 1000
--   bind          boundary bars above and below
--   bindtop       boundary bar above only
--   box           box around the symbol
--   whitesp       horizontal whitespace in modules, 0 to 1000
--   vwhitesp      vertical whitespace in modules, 0 to 1000
--   quietzones    add the standards' quiet zones
--   noquietzones  remove them
--   notext        no human readable text
--   small         smaller human readable text
--   bold          bolder human readable text
--   textgap       gap between symbol and text, -5 to 10
--   embedfont     inline the font as a base64 @font-face
--   guarddescent  UPC/EAN guard bar descent, 0 to 50
--   dotty         draw circles rather than squares
--   dotsize       dot diameter in modules, >= 0.01
--   rotate        0, 90, 180 or 270
--   vers          Interleaved 2 of 5 check digit: 1 shows it, 2 hides it.
--                 DataBar Expanded Stacked reads it as columns per row.
--                 Data Matrix reads it as the symbol size, 1 to 48.
--   cols          DataBar Expanded Stacked columns per row, 1 to 11.
--                 Data Matrix also reads it as the symbol size, and it is
--                 applied after vers, so it wins when both are set.
--   rows          DataBar Expanded Stacked maximum rows, 2 to 11.
--                 An in-range value is ignored for every other symbol.
--   reverse       white on black
--   init          reader initialisation (programming) symbol
--   esc           process escape sequences in the data
--   extraesc      process Code 128 "\^A"/"\^B"/"\^C" code-set escapes
--
-- Runs with no shell, no native modules and no access to zint: the point of
-- the port is that it works inside a browser or an embedded Lua interpreter
-- where the binary is not there to fall back on.
--
-- The modules under zint/ mirror the C files they are ported from, so a
-- function here can be read next to its original:
--
--   zint/common.lua   backend/common.c   float32 narrowing, module bitmap
--   zint/output.lua   backend/output.c   number formatting, colours, offsets
--   zint/vector.lua   backend/vector.c   bitmap -> rectangles and strings
--   zint/svg.lua      backend/svg.c      rectangles and strings -> SVG text
--   zint/code.lua     backend/code.c     Code 39
--   zint/code128.lua  backend/code128.c  Code 128, GS1-128, NVE-18,
--                                      Code 128 suppress C
--   zint/flat.lua     backend/postal.c   Flattermarken only
--   zint/pdf417.lua   backend/pdf417.c   PDF417 and Compact PDF417
--   zint/maxicode.lua backend/maxicode.c MaxiCode
--   zint/aztec.lua    backend/aztec.c    Aztec
--   zint/imail.lua    backend/imail.c    USPS Intelligent Mail
--   zint/large.lua    backend/large.c    128-bit arithmetic for Intelligent Mail
--   zint/upcean.lua   backend/upcean.c   UPC-A, UPC-E, EAN-8, EAN-13, ISBN,
--                                      EAN-2, EAN-5
--   zint/c25inter.lua backend/2of5inter.c  Interleaved 2 of 5, ITF-14
--   zint/dbar.lua     backend/rss.c      GS1 DataBar Expanded, Expanded Stacked,
--                                      Omnidirectional, Stacked Omnidirectional
--   zint/dmatrix.lua  backend/dmatrix.c  Data Matrix ECC 200
--   zint/gs1.lua      backend/gs1.c      GS1 AI parsing and input reduction
--   zint/eci.lua      backend/eci.c      UTF-8 to ISO 8859-1
--   zint/options.lua  (this repo)        option parsing, from the manual and the CLI
--   zint/pdf.lua      (this repo)        the same vector list, as PDF operators
--   zint/fonts.lua     backend/fonts/*_woff2.h  the --embedfont WOFF2 blobs
--   zint/fonts_ttf.lua backend/fonts/*_ttf.h    TrueType bytes for the PDF form

-- SVG decimals are part of the byte-identical claim. The C locale's decimal
-- point is a period. Callers that need another numeric locale can restore it
-- after barcode() returns.
os.setlocale("C", "numeric")

local common  = require "zint.common"
local eci     = require "zint.eci"
local options = require "zint.options"
local vector  = require "zint.vector"
local svg     = require "zint.svg"
local code    = require "zint.code"
local code128 = require "zint.code128"
local flat    = require "zint.flat"
local pdf417  = require "zint.pdf417"
local maxicode = require "zint.maxicode"
local aztec   = require "zint.aztec"
local imail   = require "zint.imail"
local upcean  = require "zint.upcean"
local c25inter = require "zint.c25inter"
local dbar    = require "zint.dbar"
local dmatrix = require "zint.dmatrix"
local qr      = require "zint.qr"
local gs1     = require "zint.gs1"

-- ------------------------------------------------------------- symbologies
--
-- frontend/main.c `get_barcode_name()` resolves --barcode when it is not a
-- number: lower case, drop a leading "barcode", drop everything that is not
-- alphanumeric, then look the remainder up.

local BARCODE_C25INTER  = 3
local BARCODE_CODE39    = 8
local BARCODE_EAN8      = 10
local BARCODE_EAN2      = 11
local BARCODE_EAN5      = 12
local BARCODE_EAN13     = 15
local BARCODE_GS1_128   = 16
local BARCODE_CODE128   = 20
local BARCODE_FLAT      = 28
local BARCODE_CODE128AB = 60
local BARCODE_UPCA      = 34
local BARCODE_UPCA_CHK  = 35
local BARCODE_UPCE      = 37
local BARCODE_UPCE_CHK  = 38
local BARCODE_PDF417    = 55
local BARCODE_PDF417COMP = 56
local BARCODE_MAXICODE  = 57
local BARCODE_QRCODE    = 58
local BARCODE_USPS_IMAIL = 85
local BARCODE_AZTEC     = 92
local BARCODE_ISBNX     = 69
local BARCODE_DATAMATRIX = 71
local BARCODE_DBAR_OMN  = 29
local BARCODE_DBAR_EXP  = 31
local BARCODE_NVE18     = 75
local BARCODE_DBAR_OMNSTK = 80
local BARCODE_DBAR_EXPSTK = 81
local BARCODE_ITF14     = 89

local NAMES = {
  -- Interleaved 2 of 5. The spellings are the ones frontend/main.c accepts
  -- after it has stripped spaces and punctuation.
  c25inter = BARCODE_C25INTER,
  c25interleaved = BARCODE_C25INTER,
  ["2of5inter"] = BARCODE_C25INTER,
  ["2of5interleaved"] = BARCODE_C25INTER,
  code2of5inter = BARCODE_C25INTER,
  code2of5interleaved = BARCODE_C25INTER,
  interleaved2of5 = BARCODE_C25INTER,
  interleavedcode2of5 = BARCODE_C25INTER,
  itf14    = BARCODE_ITF14,
  nve18    = BARCODE_NVE18,
  -- GS1 DataBar. frontend/main.c, composite synonyms left out.
  -- Omnidirectional takes digits. It is not force-gs1, and --gs1 is refused.
  dbaromn = BARCODE_DBAR_OMN,
  dbaromni = BARCODE_DBAR_OMN,
  databaromn = BARCODE_DBAR_OMN,
  databaromni = BARCODE_DBAR_OMN,
  rss14 = BARCODE_DBAR_OMN,
  dbaromnstk = BARCODE_DBAR_OMNSTK,
  databaromnstk = BARCODE_DBAR_OMNSTK,
  dbarstackedomn = BARCODE_DBAR_OMNSTK,
  dbarstackedomni = BARCODE_DBAR_OMNSTK,
  databarstackedomn = BARCODE_DBAR_OMNSTK,
  databarstackedomni = BARCODE_DBAR_OMNSTK,
  rss14stackomni = BARCODE_DBAR_OMNSTK,
  databarexp = BARCODE_DBAR_EXP,
  databarexpanded = BARCODE_DBAR_EXP,
  dbarexp = BARCODE_DBAR_EXP,
  dbarexpanded = BARCODE_DBAR_EXP,
  rssexp = BARCODE_DBAR_EXP,
  databarexpandedstacked = BARCODE_DBAR_EXPSTK,
  databarexpandedstk = BARCODE_DBAR_EXPSTK,
  databarexpstk = BARCODE_DBAR_EXPSTK,
  dbarexpandedstacked = BARCODE_DBAR_EXPSTK,
  dbarexpandedstk = BARCODE_DBAR_EXPSTK,
  dbarexpstk = BARCODE_DBAR_EXPSTK,
  rssexpstack = BARCODE_DBAR_EXPSTK,
  code39   = BARCODE_CODE39,
  ean8     = BARCODE_EAN8,
  ean2     = BARCODE_EAN2,
  ean2addon = BARCODE_EAN2,
  ean5     = BARCODE_EAN5,
  ean5addon = BARCODE_EAN5,
  ean13    = BARCODE_EAN13,
  isbn     = BARCODE_ISBNX,
  isbnx    = BARCODE_ISBNX,
  gs1128   = BARCODE_GS1_128,
  ean128   = BARCODE_GS1_128, -- synonym, as in main.c
  code128  = BARCODE_CODE128,
  code128ab = BARCODE_CODE128AB,
  code128b = BARCODE_CODE128AB, -- synonym, as in main.c
  flat     = BARCODE_FLAT,
  upca     = BARCODE_UPCA,
  upcachk  = BARCODE_UPCA_CHK,
  upce     = BARCODE_UPCE,
  upcechk  = BARCODE_UPCE_CHK,
  qrcode   = BARCODE_QRCODE,
  datamatrix = BARCODE_DATAMATRIX,
  pdf417   = BARCODE_PDF417,
  pdf417comp = BARCODE_PDF417COMP,
  pdf417trunc = BARCODE_PDF417COMP, -- synonym, as in main.c
  maxicode = BARCODE_MAXICODE,
  uspsimail = BARCODE_USPS_IMAIL,
  aztec    = BARCODE_AZTEC,
  azteccode = BARCODE_AZTEC, -- synonym, as in main.c
}

local function symbology_number(barcode)
  if barcode == nil then return BARCODE_CODE128 end -- --barcode omitted
  if type(barcode) == "number" then return barcode end

  local n = barcode:gsub("[A-Z]", function(c) return string.char(c:byte() | 0x20) end)
  if n:sub(1, 7) == "barcode" then n = n:sub(8) end
  n = n:gsub("[^%w]", "")
  return NAMES[n]
end

-- backend/library.c `barcode_symbology()`, restricted to what is implemented.
local ENCODERS = {
  [BARCODE_C25INTER] = c25inter.c25inter,
  [BARCODE_ITF14]    = c25inter.itf14,
  [BARCODE_CODE39]  = code.code39,
  [BARCODE_CODE128] = code128.code128,
  -- Same encoder. Symbology 60 never selects code set C.
  [BARCODE_CODE128AB] = code128.code128,
  -- Flattermarken. Digits only, no human-readable line.
  [BARCODE_FLAT] = flat.flat,
  -- NVE-18 is an SSCC. It builds a GS1-128 "(00)" string itself, so the
  -- driver must not reduce the caller's digits first.
  [BARCODE_NVE18] = code128.nve18,
  [BARCODE_GS1_128] = function(symbol, source, opts)
    -- GS1-128 is one of the symbologies `check_force_gs1()` lists, which means
    -- GS1 mode is on whether or not --gs1 was given: passing the flag changes
    -- nothing here. It also means `ZBarcode_Encode_Segs` does *not* reduce the
    -- input on the encoder's behalf, as it does for the symbologies that only
    -- enter GS1 mode when asked -- gs1_128_cc() calls gs1_verify() itself, so
    -- the reduction happens once, here.
    local reduced, err = gs1.verify(symbol, source)
    if not reduced then return nil, err end
    return code128.gs1_128(symbol, source, reduced)
  end,
  -- Both UPC-A variants run the same encoder. 34 appends the check digit and 35
  -- expects it supplied; upcean.lua tells them apart by symbol.symbology, which
  -- is also how the C does it -- the two share a case label in library.c.
  [BARCODE_UPCA]     = upcean.upca,
  [BARCODE_UPCA_CHK] = upcean.upca,
  -- EAN-13 is one id for both shapes. Twelve digits grow a check digit;
  -- thirteen must already include the right one. There is no EAN13_CHK.
  [BARCODE_EAN13]    = upcean.ean13,
  -- Standalone magazine add-ons. They draw the add-on with no body.
  [BARCODE_EAN2]     = upcean.ean2,
  [BARCODE_EAN5]     = upcean.ean5,
  -- ISBN is an EAN-13 whose input is an ISBN-10, an SBN, or an ISBN-13.
  [BARCODE_ISBNX]    = upcean.isbn,
  -- EAN-8, same shape as EAN-13: seven digits grow a check, eight must match.
  [BARCODE_EAN8]     = upcean.ean8,
  -- 37 computes the check from six digits (or seven, with a number system).
  -- 38 requires it. An eight-digit 37 still checks the eighth character.
  [BARCODE_UPCE]     = upcean.upce,
  [BARCODE_UPCE_CHK] = upcean.upce,
  -- QR is the other side of the GS1-128 coin: it is gs1-compliant but NOT
  -- force-gs1, so `ZBarcode_Encode_Segs` reduces the input before the encoder
  -- runs and qr.c never calls gs1_verify() itself. See the reduction step in
  -- barcode() below.
  [BARCODE_QRCODE]   = qr.qrcode,
  -- Expanded calls gs1_verify itself. Stacked has no human-readable text.
  [BARCODE_DBAR_EXP] = dbar.expanded,
  [BARCODE_DBAR_EXPSTK] = dbar.expanded,
  -- Omnidirectional checks its own digit string. It is not gs1-compliant, so
  -- the driver refuses --gs1 and does not reduce the input first.
  [BARCODE_DBAR_OMN] = dbar.omn,
  [BARCODE_DBAR_OMNSTK] = dbar.omn,
  -- Data Matrix is gs1-compliant and not force-gs1, so the driver reduces
  -- the element string first. The encoder prefixes FNC1 itself.
  [BARCODE_DATAMATRIX] = dmatrix.datamatrix,
  -- PDF417 and Compact PDF417 share one encoder. Symbology 56 drops the
  -- right row indicator and the full stop pattern.
  [BARCODE_PDF417] = pdf417.pdf417,
  [BARCODE_PDF417COMP] = pdf417.pdf417,
  -- MaxiCode reads symbol.primary. Modes 2 and 3 are structured; 4, 5, and
  -- 6 are the message alone.
  [BARCODE_MAXICODE] = maxicode.maxicode,
  -- Aztec is gs1-compliant and not force-gs1, so the driver reduces an
  -- element string before the encoder runs.
  [BARCODE_AZTEC] = aztec.aztec,
  -- Intelligent Mail is a tracking code plus an optional ZIP. It is not
  -- GS1 and it does not take an ECI.
  [BARCODE_USPS_IMAIL] = imail.imail,
}

-- backend/library.c `check_force_gs1()`. These are in GS1 mode whether or not
-- --gs1 was given, and they reduce their own input. Everything else that is
-- gs1-compliant has the input reduced for it.
local FORCE_GS1 = {
  [BARCODE_GS1_128] = true,
  [BARCODE_NVE18] = true,
  [BARCODE_DBAR_EXP] = true,
  [BARCODE_DBAR_EXPSTK] = true,
}

-- backend/common.c `is_dotty()`. Dotty mode is a matrix-symbology feature.
-- QR, Data Matrix, and Aztec draw as dots. MaxiCode does not: its plot is
-- hexagons. --dotty on anything else is an error rather than a no-op.
local IS_DOTTY = {
  [BARCODE_QRCODE] = true,
  [BARCODE_DATAMATRIX] = true,
  [BARCODE_AZTEC] = true,
}

-- backend/library.c `gs1_compliant()` unioned with `check_force_gs1()`, cut
-- down to the ones here. --gs1 on anything not in this table is an error, not a
-- no-op, so Code 39, Code 128, PDF417, MaxiCode, and Intelligent Mail refuse it.
local GS1_COMPLIANT = {
  [BARCODE_GS1_128] = true,
  [BARCODE_NVE18] = true,
  [BARCODE_DBAR_EXP] = true,
  [BARCODE_DBAR_EXPSTK] = true,
  [BARCODE_QRCODE]  = true,
  [BARCODE_DATAMATRIX] = true,
  [BARCODE_AZTEC] = true,
}

-- backend/library.c `supports_eci()`. It words the empty-input message, and it
-- is the list that decides whether a failed character-set conversion is
-- retried under another ECI. QR handles its own charset. The others do
-- not: Latin-1 is folded first, and an ECI is set only when that fold fails.
-- Intelligent Mail is not on this list.
local SUPPORTS_ECI = {
  [BARCODE_QRCODE] = true,
  [BARCODE_DATAMATRIX] = true,
  [BARCODE_PDF417] = true,
  [BARCODE_PDF417COMP] = true,
  [BARCODE_MAXICODE] = true,
  [BARCODE_AZTEC] = true,
}

-- backend/library.c `extended_or_reduced_charset()`. The symbologies listed
-- there are the ones that understand character sets themselves; everything else
-- falls through to `reduced_charset()`, which folds the input to ISO 8859-1
-- before the encoder sees it. QR is the only one of the five on the elite list,
-- which is why it -- alone -- receives raw UTF-8.
local HANDLES_OWN_CHARSET = {
  [BARCODE_QRCODE] = true,
}

-- backend/zint.h.
local ZINT_MAX_DATA_LEN = 17400

-- backend/library.c, "Check other symbol fields". A second range check on top
-- of the front end's, and a stricter one: main.c lets --border go to 1000 and
-- this refuses anything over 100, so `--border 500` warns nowhere and simply
-- produces no file. Returns the message, or nil when everything is in range.
--
-- The float bounds are f32'd for the reason options.lua spells out -- 0.01 as a
-- double is not 0.01f, and `--scale 0.01` sits exactly on that boundary.
local FIELD_CHECKS = {
  { "scale",             common.stripf(0.01), common.stripf(200.0),
    "Error 227: Scale out of range (0.01 to 200)" },
  { "dot_size",          common.stripf(0.01), common.stripf(20.0),
    "Error 221: Dot size out of range (0.01 to 20)" },
  { "height",            common.stripf(0.0),  common.stripf(2000.0),
    "Error 765: Height out of range (0 to 2000)" },
  { "guard_descent",     common.stripf(0.0),  common.stripf(50.0),
    "Error 769: Guard bar descent out of range (0 to 50)" },
  { "text_gap",          common.stripf(-5.0), common.stripf(10.0),
    "Error 219: Text gap out of range (-5 to 10)" },
  { "whitespace_width",  0, 100,
    "Error 766: Whitespace width out of range (0 to 100)" },
  { "whitespace_height", 0, 100,
    "Error 767: Whitespace height out of range (0 to 100)" },
  { "border_width",      0, 100,
    "Error 768: Border width out of range (0 to 100)" },
}

local function check_symbol_fields(symbol)
  for _, c in ipairs(FIELD_CHECKS) do
    local v = symbol[c[1]]
    if v < c[2] or v > c[3] then return c[4] end
  end
  return nil
end

-- backend/library.c `strip_bom()`. RFC 3629 says a leading byte order mark is
-- not part of the text, so it goes -- unless it is the *whole* text, which the
-- strict `> 3` guards: three bytes of BOM and nothing else would otherwise
-- reduce to empty input.
local function strip_bom(source)
  if #source > 3 and source:sub(1, 3) == "\239\187\191" then
    return source:sub(4)
  end
  return source
end

-- ------------------------------------------------------------------- driver

local zint = {}

-- backend/library.c `ZBarcode_Encode_and_Print()`, collapsed to the one output
-- format this port covers. Returns the SVG text as well as writing it, which
-- makes the port testable without touching the filesystem.
function zint.barcode(opts)
  local symbology = symbology_number(opts.barcode)
  local encode = symbology and ENCODERS[symbology]
  if not encode then
    return nil, "Error 206: Symbology out of range"
  end

  local symbol = common.new_symbol()
  symbol.symbology = symbology

  -- Everything the command line would have done to the symbol before libzint
  -- ever sees it. Note 0 is truthy in Lua, so a rotation of 0 -- the common
  -- case -- passes this test; only a nil return means failure.
  local rotate_angle, aerr, warned = options.apply(symbol, opts)
  if rotate_angle == nil then return nil, aerr end

  -- --------------------------------------- backend/library.c ZBarcode_Encode_Segs
  --
  -- What follows is that function's preamble in its original order. The order
  -- is load-bearing in one respect only -- every branch below either produces
  -- an error and no file, or leaves the data changed for the next step -- but
  -- it is kept anyway so the two can be read side by side.

  local raw = opts.data or ""

  -- Segment length checks. The suffix on the empty-input message is not
  -- decoration: the C only mentions "segment 0" for symbologies that can have
  -- more than one segment in the first place.
  if #raw == 0 then
    return nil, "Error 778: No input data" ..
      (SUPPORTS_ECI[symbology] and " (segment 0 empty)" or "")
  end

  -- De-escaping. The C runs `escape_char_process` twice: once here, discarding
  -- the result and keeping only the length, and again further down to actually
  -- produce the bytes. One pass is enough as long as the error surfaces at this
  -- point rather than later, which is what the two-pass structure is really
  -- buying -- so the result is computed here and held until the C would have
  -- used it.
  --
  -- Doing it up front is also why `--esc --gs1` reduces the *de-escaped* data,
  -- and why the "\^A" sequences --extraesc looks for have to survive --esc
  -- untouched.
  local source = raw
  if (symbol.input_mode & common.ESCAPE_MODE) ~= 0 then
    local escaped, eerr = common.escape_char_process(symbol, raw)
    if not escaped then return nil, eerr end
    source = escaped
    if #source > ZINT_MAX_DATA_LEN then
      return nil, "Error 797: Input data too long"
    end
  elseif #raw > ZINT_MAX_DATA_LEN then
    return nil, "Error 777: Input data too long"
  end

  local ferr = check_symbol_fields(symbol)
  if ferr then return nil, ferr end

  if (symbol.input_mode & 0x07) == common.GS1_MODE and not GS1_COMPLIANT[symbology] then
    return nil, "Error 220: Selected symbology does not support GS1 mode"
  end

  -- Note this runs on the data as typed, before de-escaping: `é` is four
  -- ASCII characters and passes whatever it later expands to.
  if (symbol.input_mode & 0x07) == common.UNICODE_MODE
      and not common.is_valid_utf8(raw) then
    return nil, "Error 245: Invalid UTF-8 in input data"
  end

  if (symbol.input_mode & 0x07) == common.UNICODE_MODE then
    source = strip_bom(source)
  end

  -- GS1 reduction, for the symbologies that do not do it themselves.
  --
  -- `gs1_verify()` parses the AI syntax and rewrites the input into the form
  -- the encoders want: brackets stripped, FNC1 separators inserted as "[", and
  -- the whole thing validated. library.c runs it here for anything gs1-compliant
  -- that is not force-gs1, and *skips* it for the force-gs1 set because those
  -- encoders call it themselves. GS1-128 is in the skipped set; QR is not.
  --
  -- Running it twice would be worse than not running it at all -- the second
  -- pass would see "[" where the first left it and reject the input -- so the
  -- FORCE_GS1 guard is load-bearing, not defensive.
  if (symbol.input_mode & 0x07) == common.GS1_MODE and not FORCE_GS1[symbology] then
    local reduced, gerr = gs1.verify(symbol, source)
    if not reduced then return nil, gerr end
    source = reduced
  end

  -- `extended_or_reduced_charset()`. The default arm folds UTF-8 down to
  -- ISO 8859-1, so `é` reaches the encoder as one byte rather than the two it
  -- was typed as. Getting this wrong is not a rounding error: it encodes a
  -- different number of characters and produces a different symbol.
  -- Data Matrix is not on the elite charset list, so Latin-1 is tried
  -- first and the encoder sees the folded bytes. When that fold fails and
  -- the symbology supports ECI, pick the lowest single-byte ECI (or 26,
  -- raw UTF-8) and convert before the encoder runs. QR never reaches this
  -- arm: it handles its own charset and retries on 800/242/240 below.
  local pending_eci_warning
  if (symbol.input_mode & 0x07) == common.UNICODE_MODE
      and not HANDLES_OWN_CHARSET[symbology] then
    local converted = eci.utf8_to_latin1(source)
    if converted then
      source = converted
    elseif SUPPORTS_ECI[symbology] and (symbol.eci or 0) == 0 then
      local best = eci.get_best_eci(source)
      local reported = 0
      if best ~= 3 then
        symbol.eci = best
        reported = best
      end
      if eci.is_eci_convertible(best) then
        converted = eci.utf8_to_eci(best, source)
        if not converted then
          return nil, "Error 204: Invalid character in input data (ISO/IEC 8859-1 only)"
        end
        source = converted
      end
      pending_eci_warning = string.format(
        "Warning 222: Encoded data includes ECI %d", reported)
    else
      return nil, "Error 204: Invalid character in input data (ISO/IEC 8859-1 only)"
    end
  end

  local ok, err = encode(symbol, source, opts)

  -- backend/library.c, the retry after `extended_or_reduced_charset()`. QR is
  -- the only symbology here that both supports ECI and folds characters itself.
  -- When that fold fails, the segment ECI is still 0, and the input is Unicode,
  -- pick the lowest single-byte ECI that fits (or 26, raw UTF-8) and encode
  -- again. ECI 3 is the default and is not written back; the warning then
  -- names 0, which is what `first_eci_set` stays at in that case. Latin-1 has
  -- already failed by the time this runs, so the number that comes back is
  -- higher. The warning replaces anything the second encode had to say.
  if not ok and SUPPORTS_ECI[symbology]
      and (symbol.input_mode & 0x07) == common.UNICODE_MODE
      and (symbol.eci or 0) == 0
      and type(err) == "string"
      and (err:sub(1, 10) == "Error 800:" or err:sub(1, 10) == "Error 242:"
           or err:sub(1, 10) == "Error 240:") then
    local best = eci.get_best_eci(source)
    local reported = 0
    if best ~= 3 then
      symbol.eci = best
      reported = best
    end
    ok, err = encode(symbol, source, opts)
    if ok then
      symbol.warning = string.format("Warning 222: Encoded data includes ECI %d", reported)
    end
  end
  -- The C overwrites any other warning with 222 once the ECI retry sticks.
  if ok and pending_eci_warning then
    symbol.warning = pending_eci_warning
  end
  if not ok then return nil, err end

  -- backend/library.c, tail of `ZBarcode_Encode_Segs()`. A general fallback for
  -- any encoder that left the height alone -- Code 128 is one, because
  -- ISO/IEC 15417 leaves height to the application. Code 39 sets its own, so
  -- this does not fire for it.
  if symbol.height < 0.5 then
    common.set_height(symbol, 0.0, 50.0, 0.0)
  end

  -- frontend/main.c prints symbol->errtxt as soon as ZBarcode_Encode returns,
  -- before ZBarcode_Print runs -- so a height warning appears ahead of anything
  -- the output stage has to say. Only one message survives, because the C has
  -- one buffer for it.
  --
  -- Reachable today only through --compliantheight; without it every encoder
  -- passes a minimum row height of zero and there is nothing to breach.
  if symbol.warning and io and io.stderr then
    io.stderr:write(symbol.warning, "\n")
  end

  -- backend/library.c `check_output_args()`, on the output path rather than the
  -- encode path -- which is why it runs here, after a symbol has already been
  -- built. Its rotation branch is unreachable from the command line, because
  -- options.apply has already thrown out any angle that is not one of the four;
  -- dotty mode is the branch that fires.
  if (symbol.output_options & common.BARCODE_DOTTY_MODE) ~= 0
      and not IS_DOTTY[symbology] then
    return nil, "Error 224: Selected symbology cannot be rendered as dots"
  end

  local ft = "SVG"
  if opts.filetype and opts.filetype ~= "" then
    ft = tostring(opts.filetype):upper()
  end
  if ft ~= "SVG" and ft ~= "PDF" then
    return nil, "Error 142: File type '" .. tostring(opts.filetype) .. "' not supported"
  end

  vector.plot(symbol, rotate_angle)
  local out
  if ft == "PDF" then
    out = require("zint.pdf").form(symbol)
  else
    out = svg.plot(symbol)
  end

  if opts.output then
    local payload = ft == "PDF" and out.content or out
    local f, oerr = io.open(opts.output, "wb")
    if not f then return nil, oerr end
    f:write(payload)
    f:close()
  end

  return out, symbol.warning, warned
end

return zint
