-- zint/svg.lua -- ports backend/svg.c
--
-- The last stage: rectangles and strings in, SVG text out. By the time control
-- reaches here every number has already been decided, so nothing in this file
-- does arithmetic on a coordinate -- it only formats. That division is worth
-- preserving when extending the port: if a byte is wrong and it is not a
-- number, the bug is here; if it is a number, the bug is upstream.
--
-- The literal whitespace matters. zint indents <desc> and <g> by one space and
-- their children by two, and puts the content of a <text> element on its own
-- line indented by three. All of that is compared byte for byte, so the string
-- literals below are exact and should not be tidied.

local common = require "zint.common"
local output = require "zint.output"

local M = {}

local putsf = output.putsf
local format = string.format
local ceil = math.ceil

-- backend/svg.c `svg_make_html_friendly()` -- the five XML predefined entities.
-- zint escapes all five in the human-readable line, including the quote forms
-- that are harmless in element content. Unreachable from Code 39, whose
-- character set contains none of them; Code 128 and GS1-128 data can hit it.
local HTML_ENTITY = {
  [">"] = "&gt;", ["<"] = "&lt;", ["&"] = "&amp;",
  ['"'] = "&quot;", ["'"] = "&apos;",
}

local function make_html_friendly(text)
  return (text:gsub("[><&\"']", HTML_ENTITY))
end

-- backend/svg.c `svg_pick_colour()` -- the eight Ultracode rectangle colours.
-- Only ever reached with the default -1, which every caller filters out first,
-- so this is dead code for us. Kept so the branches below read the same as the
-- C rather than looking like they are missing a case.
local RGBS = {
  "00ffff", "0000ff", "ff00ff", "ff0000",
  "ffff00", "00ff00", "000000", "ffffff",
}

local function pick_colour(colour)
  local idx = (colour >= 1 and colour <= 8) and colour or 7 -- 7 is black
  return RGBS[idx]
end

-- backend/svg.c `svg_put_fattrib()` -- a float attribute, closing quote and all.
local function put_fattrib(prefix, dp, val)
  return putsf(prefix, dp, val) .. '"'
end

-- backend/svg.c `svg_put_opacity_close()`. Two jobs in one function, as in the
-- C: emit an opacity attribute if the colour is not fully opaque, then close
-- the tag -- self-closing when `close`, open otherwise.
--
-- The alpha test is `~= 0xff`, not `< 0xff`, so a fully opaque colour given as
-- six hex digits and one given as eight both skip the attribute. Three decimal
-- places, then out_putsf strips what it can, so an alpha of 0x80 comes out as
-- opacity="0.502".
local function put_opacity_close(alpha, val, close)
  local out = ""
  if alpha ~= 0xff then
    out = out .. put_fattrib(' opacity="', 3, val)
  end
  if close then out = out .. "/" end
  return out .. ">\n"
end

-- backend/svg.c `svg_plot()`.
--
-- Returns the document as a string instead of writing it to a FILE*. The
-- caller does the io.open, which keeps every filesystem touch in one place and
-- makes the port testable without one.
function M.plot(symbol)
  local vector = symbol.vector
  local out = {}
  local function w(s) out[#out + 1] = s end

  -- Colours are resolved once up front. The alpha decides two separate things:
  -- whether an opacity attribute appears, and -- for the background only --
  -- whether the rectangle is emitted at all.
  local fgred, fggreen, fgblue, fg_alpha = output.colour_get_rgb(symbol.fgcolour)
  local fg_alpha_opacity = 0.0
  if fg_alpha ~= 0xff then fg_alpha_opacity = common.stripf(fg_alpha / 255.0) end
  -- Uppercase: zint writes colours with %02X here even though it accepts them
  -- in either case on input.
  local fgcolour_string = format("%02X%02X%02X", fgred, fggreen, fgblue)

  local bgred, bggreen, bgblue, bg_alpha = output.colour_get_rgb(symbol.bgcolour)
  local bg_alpha_opacity = 0.0
  if bg_alpha ~= 0xff then bg_alpha_opacity = common.stripf(bg_alpha / 255.0) end
  local bgcolour_string = format("%02X%02X%02X", bgred, bggreen, bgblue)

  -- The only place the image dimensions are rounded. Everything inside keeps
  -- its fractional part; the canvas is whole numbers.
  local width = ceil(vector.width)
  local height = ceil(vector.height)

  w('<?xml version="1.0" standalone="no"?>\n')
  w('<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"'
    .. ' "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">\n')
  w(format('<svg width="%d" height="%d" version="1.1" xmlns="http://www.w3.org/2000/svg">\n',
           width, height))
  w(" <desc>Zint Generated Symbol</desc>\n")

  -- UPC/EAN get a monospaced face so the digits under the guard bars line up.
  -- Needed here as well as at the text elements, because --embedfont inlines
  -- whichever of the two faces this symbol will actually ask for.
  local upcean = common.is_upcean(symbol.symbology)

  -- --embedfont. The blob goes in verbatim -- it is already base64 in the C's
  -- header files, so nothing is encoded here. Note the second half of the test:
  -- with no text to draw there is no font to embed, so --notext --embedfont
  -- emits no <style> at all.
  if (symbol.output_options & common.EMBED_VECTOR_FONT) ~= 0 and #vector.strings > 0 then
    -- Required lazily. The two blobs are 20KB of source between them and this
    -- branch is rarely taken; paying the parse on every run would be a tax on
    -- the common case for no benefit.
    local fonts = require "zint.fonts"
    w(format(' <style>@font-face {font-family:"%s"; src:url(data:font/woff2;base64,%s);}</style>\n',
             upcean and "OCRB" or "Arimo", upcean and fonts.upcean or fonts.normal))
  end

  -- The group carries the foreground colour, which is how the <path> below
  -- gets away with having no fill of its own.
  w(format(' <g id="barcode" fill="#%s">\n', fgcolour_string))

  -- A fully transparent background is omitted rather than drawn -- this is
  -- what --nobackground does, by setting bgcolour to "ffffff00".
  if bg_alpha ~= 0 then
    w(format('  <rect x="0" y="0" width="%d" height="%d" fill="#%s"',
             width, height, bgcolour_string))
    w(put_opacity_close(bg_alpha, bg_alpha_opacity, true))
  end

  if #vector.rectangles > 0 then
    -- current_colour starts at 0, which is not a valid colour index -- it is a
    -- "nothing emitted yet" sentinel, which is why the flush test below checks
    -- for it explicitly. Every run of same-coloured rectangles becomes one
    -- <path>; with the default -1 throughout, that is one path for the whole
    -- symbol.
    local current_colour = 0
    w('  <path d="')
    for _, rect in ipairs(vector.rectangles) do
      if current_colour ~= 0 and rect.colour ~= current_colour then
        w('"')
        if current_colour ~= -1 then
          w(format(' fill="#%s"', pick_colour(current_colour)))
        end
        w(put_opacity_close(fg_alpha, fg_alpha_opacity, true))
        w('  <path d="')
      end
      current_colour = rect.colour
      -- Each rectangle is four relative commands and a close: move to the top
      -- left, right by width, down by height, back left by width, Z. No spaces
      -- between rectangles -- the command letters are their own delimiters.
      w(putsf("M", 2, rect.x))
      w(putsf(" ", 2, rect.y))
      w(putsf("h", 2, rect.width))
      w(putsf("v", 2, rect.height))
      w(putsf("h-", 2, rect.width))
      w("Z")
    end
    w('"')
    if current_colour ~= -1 then
      w(format(' fill="#%s"', pick_colour(current_colour)))
    end
    w(put_opacity_close(fg_alpha, fg_alpha_opacity, true))
  end

  -- Hexagons sit between the rectangles and the circles, which is where
  -- svg.c emits them. MaxiCode is the only symbol that has any. One path,
  -- filled by the group, with a Z between each hexagon and no extra space.
  -- Rotation 0 and 180 are point-up; 90 and 270 are flat-top.
  if #vector.hexagons > 0 then
    local previous_diameter = 0.0
    local radius, half_radius, half_sqrt3_radius = 0.0, 0.0, 0.0
    local half_sqrt3 = common.stripf(0.43301270189221932338)
    w('  <path d="')
    for _, hex in ipairs(vector.hexagons) do
      if previous_diameter ~= hex.diameter then
        previous_diameter = hex.diameter
        radius = common.stripf(0.5 * previous_diameter)
        half_radius = common.stripf(0.25 * previous_diameter)
        half_sqrt3_radius = common.stripf(half_sqrt3 * previous_diameter)
      end
      if hex.rotation == 0 or hex.rotation == 180 then
        w(putsf("M", 2, hex.x))
        w(putsf(" ", 2, common.stripf(hex.y + radius)))
        w(putsf("L", 2, common.stripf(hex.x + half_sqrt3_radius)))
        w(putsf(" ", 2, common.stripf(hex.y + half_radius)))
        w(putsf("L", 2, common.stripf(hex.x + half_sqrt3_radius)))
        w(putsf(" ", 2, common.stripf(hex.y - half_radius)))
        w(putsf("L", 2, hex.x))
        w(putsf(" ", 2, common.stripf(hex.y - radius)))
        w(putsf("L", 2, common.stripf(hex.x - half_sqrt3_radius)))
        w(putsf(" ", 2, common.stripf(hex.y - half_radius)))
        w(putsf("L", 2, common.stripf(hex.x - half_sqrt3_radius)))
        w(putsf(" ", 2, common.stripf(hex.y + half_radius)))
      else
        w(putsf("M", 2, common.stripf(hex.x - radius)))
        w(putsf(" ", 2, hex.y))
        w(putsf("L", 2, common.stripf(hex.x - half_radius)))
        w(putsf(" ", 2, common.stripf(hex.y + half_sqrt3_radius)))
        w(putsf("L", 2, common.stripf(hex.x + half_radius)))
        w(putsf(" ", 2, common.stripf(hex.y + half_sqrt3_radius)))
        w(putsf("L", 2, common.stripf(hex.x + radius)))
        w(putsf(" ", 2, hex.y))
        w(putsf("L", 2, common.stripf(hex.x + half_radius)))
        w(putsf(" ", 2, common.stripf(hex.y - half_sqrt3_radius)))
        w(putsf("L", 2, common.stripf(hex.x - half_radius)))
        w(putsf(" ", 2, common.stripf(hex.y - half_sqrt3_radius)))
      end
      w("Z")
    end
    w('"')
    w(put_opacity_close(fg_alpha, fg_alpha_opacity, true))
  end

  -- Circles. Dotty mode is one disc per set module. MaxiCode's bullseye is
  -- three rings, which is the only thing that sets a non-zero `width`.
  --
  -- `previous_diameter` is a one-entry cache: dotty mode gives every circle the
  -- same diameter, so the halving happens once no matter how many dots there
  -- are. Carried over as-is because it also decides the *value* -- 0.5f * d
  -- narrowed once and reused is not always 0.5 * d recomputed in double.
  local previous_diameter, radius = 0.0, 0.0
  for _, circle in ipairs(vector.circles) do
    if previous_diameter ~= circle.diameter then
      previous_diameter = circle.diameter
      radius = common.stripf(0.5 * previous_diameter)
    end
    w("  <circle")
    w(put_fattrib(' cx="', 2, circle.x))
    w(put_fattrib(' cy="', 2, circle.y))
    -- Three decimals for a ring, two for a disc. A ring is a fraction of a
    -- module across and needs the extra place; a disc does not.
    w(put_fattrib(' r="', circle.width ~= 0 and 3 or 2, radius))

    -- circle.colour is the C's legacy background-coloured circle, which nothing
    -- sets any more; its branch is omitted rather than ported blind.
    if circle.width ~= 0 then
      w(format(' stroke="#%s"', fgcolour_string))
      w(put_fattrib(' stroke-width="', 3, circle.width))
      w(' fill="none"')
    end
    w(put_opacity_close(fg_alpha, fg_alpha_opacity, true))
  end

  -- --bold thickens the human-readable line, but never UPC/EAN's -- OCRB has no
  -- bold cut, so asking for one would just let the renderer synthesise a smeared
  -- approximation and lose the digit alignment the face exists for.
  local bold = (symbol.output_options & common.BOLD_TEXT) ~= 0 and not upcean

  for _, str in ipairs(vector.strings) do
    local halign = str.halign == 2 and "end" or (str.halign == 1 and "start" or "middle")
    w("  <text")
    w(put_fattrib(' x="', 2, str.x))
    w(put_fattrib(' y="', 2, str.y))
    w(format(' text-anchor="%s"', halign))
    if upcean then
      w(' font-family="OCRB, monospace"')
    else
      w(' font-family="Arimo, Arial, sans-serif"')
    end
    w(put_fattrib(' font-size="', 1, str.fsize))
    if bold then w(' font-weight="bold"') end
    -- Text is the one thing vector_rotate cannot place by moving coordinates --
    -- the glyphs themselves have to turn -- so it leaves a rotation behind for
    -- here. The centre of rotation is the anchor the element already has, so
    -- the same x and y are printed a second time inside rotate().
    if str.rotation ~= 0 then
      w(format(' transform="rotate(%d', str.rotation))
      w(putsf(",", 2, str.x))
      w(putsf(",", 2, str.y))
      w(')"')
    end
    w(put_opacity_close(fg_alpha, fg_alpha_opacity, false))
    w(format("   %s\n", make_html_friendly(str.text)))
    w("  </text>\n")
  end

  w(" </g>\n</svg>\n")

  return table.concat(out)
end

return M
