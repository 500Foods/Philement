-- zint/vector.lua -- ports backend/vector.c
--
-- Turns the module bitmap into the rectangles and strings svg.lua writes out.
-- This is where the geometry lives: everything about how big the image is,
-- where the bars sit in it, and where the human-readable line goes.
--
-- Every arithmetic step below mirrors a C float expression, so each one is
-- narrowed with f32; the grouping of the narrowing follows C's evaluation
-- order, because that is what decides the last bit of the result. Expressions
-- that look gratuitously nested -- f32(f32(a + b) + f32(c + d)) -- are that
-- shape on purpose. See PORTING-NOTES.md entry 2.
--
-- Coordinates here are in modules, not output units. vector_scale at the end
-- multiplies everything by 2 (for the default --scale of 1), which is why the
-- font size below is 7 and the SVG says 14.

local common = require "zint.common"
local output = require "zint.output"

local M = {}

local f32 = common.stripf
local module_is_set = common.module_is_set

-- ------------------------------------------------------------------- pieces

-- backend/vector.c `vector_add_rect()`.
local function add_rect(vector, x, y, width, height)
  -- Colour -1 is the default in the C and means "inherit the group fill",
  -- which is what keeps a `fill` attribute off the path element. It is not a
  -- placeholder: svg.c branches on `!= -1` to decide whether to emit one, so
  -- initialising this to 0 would put a redundant fill="#00ffff" on every
  -- symbol. Only Ultracode, which sets real colour indices, ever uses another
  -- value.
  local rect = { x = x, y = y, width = width, height = height, colour = -1 }
  vector.rectangles[#vector.rectangles + 1] = rect
  return rect
end

-- backend/vector.c `vector_add_string()`. `halign` is 0 centred, 1 left, 2
-- right; only 0 is reachable until UPC-A arrives, which uses all three.
-- `rotation` starts at 0 and is set by rotate_vector below, which is also what
-- puts a transform= on the element.
local function add_string(vector, text, x, y, fsize, width, halign)
  vector.strings[#vector.strings + 1] = {
    text = text, x = x, y = y, fsize = fsize,
    width = width, halign = halign, rotation = 0,
  }
end

-- backend/vector.c `vector_add_circle()`. Dotty mode is the only caller here;
-- in the C, Maxicode's bullseye uses it too, which is why there is a `width`
-- (a stroke width -- a circle with one is drawn as an unfilled ring) and a
-- `colour` (legacy, always 0 now).
local function add_circle(vector, x, y, diameter, width, colour)
  vector.circles[#vector.circles + 1] = {
    x = x, y = y, diameter = diameter, width = width, colour = colour,
  }
end

-- backend/vector.c `vector_add_hexagon()`. MaxiCode only. Rotation starts at
-- 0 (point up) and is set by rotate_vector.
local function add_hexagon(vector, x, y, diameter)
  vector.hexagons[#vector.hexagons + 1] = {
    x = x, y = y, diameter = diameter, rotation = 0,
  }
end

-- --------------------------------------------------------------- scale/merge

-- backend/vector.c `vector_reduce_rectangles()` -- merge vertically adjacent
-- rectangles that share an x, a width and a colour, so a column of stacked
-- modules becomes one tall rectangle instead of many short ones.
--
-- For a linear symbology this does nothing at all: there is one row, so no two
-- rectangles are ever vertically adjacent. QR is the first case where it does
-- real work, and it does a lot of it -- a 100x100 symbol starts with thousands
-- of one-module rectangles.
--
-- Kept as the C's O(n^2) double scan rather than something cleverer, because
-- the merge order decides the order rectangles end up in the path data, and
-- the path data is compared byte for byte. A hash-bucketed version would be
-- faster and would emit the same rectangles in a different sequence.
local function reduce_rectangles(vector)
  local rects = vector.rectangles
  local i = 1

  while i <= #rects do
    local rect = rects[i]
    local j = i + 1
    while j <= #rects do
      local target = rects[j]
      -- f32 on the sum because the C compares two floats, and rect.y +
      -- rect.height is computed in float there before the comparison.
      if rect.x == target.x and rect.width == target.width
          and f32(rect.y + rect.height) == target.y and rect.colour == target.colour then
        rect.height = f32(rect.height + target.height)
        table.remove(rects, j)
      else
        j = j + 1
      end
    end
    i = i + 1
  end
end

-- backend/vector.c `vector_scale()`. Vector scale is twice `symbol->scale`,
-- which is why the font sizes in plot_vector are half what comes out: the
-- default scale of 1.0 doubles everything, so a font_height of 7 emits
-- font-size="14".
--
-- The floor of 0.2 is the C's "minimum vector scale 0.1" -- 0.1 before the
-- doubling. --scale 0.01 is accepted by the CLI and lands under it.
local function scale_vector(symbol, vector)
  local scale = f32(symbol.scale * 2.0)
  if scale < 0.2 then scale = 0.2 end

  vector.width = f32(vector.width * scale)
  vector.height = f32(vector.height * scale)

  for _, rect in ipairs(vector.rectangles) do
    rect.x = f32(rect.x * scale)
    rect.y = f32(rect.y * scale)
    rect.height = f32(rect.height * scale)
    rect.width = f32(rect.width * scale)
  end

  for _, hex in ipairs(vector.hexagons) do
    hex.x = f32(hex.x * scale)
    hex.y = f32(hex.y * scale)
    hex.diameter = f32(hex.diameter * scale)
  end

  for _, circle in ipairs(vector.circles) do
    circle.x = f32(circle.x * scale)
    circle.y = f32(circle.y * scale)
    circle.diameter = f32(circle.diameter * scale)
    circle.width = f32(circle.width * scale)
  end

  for _, str in ipairs(vector.strings) do
    str.x = f32(str.x * scale)
    str.y = f32(str.y * scale)
    str.width = f32(str.width * scale)
    str.fsize = f32(str.fsize * scale)
  end
end

-- backend/vector.c `vector_rotate()`, run after scaling. Note this is a real
-- coordinate transform, not an SVG `transform` on the group: every rectangle
-- and circle is moved and, at 90 and 270, has its width and height swapped.
-- The only thing that does get an SVG transform is a text element, and that is
-- because glyphs cannot be rotated by moving their anchor -- see `rotation`,
-- which svg.lua turns into `transform="rotate(...)"`.
--
-- The `temp` dance in the 90 and 270 cases is the C's, and the order matters:
-- x is read into temp before y overwrites it. Lua could do these as multiple
-- assignment, but keeping the C's sequence makes the two readable side by side.
--
-- The image's own width and height swap last, once every child has been placed
-- against the pre-swap values.
local function rotate_vector(vector, rotate_angle)
  if rotate_angle == 0 then return end

  for _, rect in ipairs(vector.rectangles) do
    if rotate_angle == 90 then
      local temp = rect.x
      rect.x = f32(vector.height - f32(rect.y + rect.height))
      rect.y = temp
      temp = rect.width
      rect.width = rect.height
      rect.height = temp
    elseif rotate_angle == 180 then
      rect.x = f32(vector.width - f32(rect.x + rect.width))
      rect.y = f32(vector.height - f32(rect.y + rect.height))
    elseif rotate_angle == 270 then
      local temp = rect.x
      rect.x = rect.y
      rect.y = f32(vector.width - f32(temp + rect.width))
      temp = rect.width
      rect.width = rect.height
      rect.height = temp
    end
  end

  for _, hex in ipairs(vector.hexagons) do
    if rotate_angle == 90 then
      local temp = hex.x
      hex.x = f32(vector.height - hex.y)
      hex.y = temp
      hex.rotation = 90
    elseif rotate_angle == 180 then
      hex.x = f32(vector.width - hex.x)
      hex.y = f32(vector.height - hex.y)
      hex.rotation = 180
    elseif rotate_angle == 270 then
      local temp = hex.x
      hex.x = hex.y
      hex.y = f32(vector.width - temp)
      hex.rotation = 270
    end
  end

  for _, circle in ipairs(vector.circles) do
    if rotate_angle == 90 then
      local temp = circle.x
      circle.x = f32(vector.height - circle.y)
      circle.y = temp
    elseif rotate_angle == 180 then
      circle.x = f32(vector.width - circle.x)
      circle.y = f32(vector.height - circle.y)
    elseif rotate_angle == 270 then
      local temp = circle.x
      circle.x = circle.y
      circle.y = f32(vector.width - temp)
    end
  end

  for _, str in ipairs(vector.strings) do
    if rotate_angle == 90 then
      local temp = str.x
      str.x = f32(vector.height - str.y)
      str.y = temp
      str.rotation = 90
    elseif rotate_angle == 180 then
      str.x = f32(vector.width - str.x)
      str.y = f32(vector.height - str.y)
      str.rotation = 180
    elseif rotate_angle == 270 then
      local temp = str.x
      str.x = str.y
      str.y = f32(vector.width - temp)
      str.rotation = 270
    end
  end

  if rotate_angle == 90 or rotate_angle == 270 then
    vector.width, vector.height = vector.height, vector.width
  end
end

-- ------------------------------------------------------------- plot_vector

-- backend/vector.c `plot_vector()`, restricted to the branches this port can
-- reach. No Ultracode (coloured rectangles), no composite symbols. MaxiCode
-- is the hexagon branch: a bullseye of three rings and one hexagon per
-- set module. Aztec and PDF417 use the generic rectangle path.
--
-- The remaining shape is: work out the bounding box, walk the bitmap turning
-- runs of set modules into rectangles (or dots), place the text, draw any
-- border, then merge, scale and rotate.
function M.plot(symbol, rotate_angle)
  rotate_angle = rotate_angle or 0
  local vector = {
    width = 0.0, height = 0.0,
    rectangles = {}, hexagons = {}, circles = {}, strings = {},
  }
  symbol.vector = vector

  local opts = symbol.output_options

  -- Fixes up symbol.height as a side effect; must run before height is read.
  local large_bar_height = output.large_bar_height(symbol)

  -- UPC/EAN carries its add-on inside the same symbol, so "how wide is the
  -- barcode" and "how wide is symbol.width" stop being the same question:
  -- main_width is the part before the add-on, and everything past it is laid
  -- out differently. For every other symbology the two are equal.
  --
  -- `upceanflag` is zero for those, and the digit count of the family member
  -- otherwise -- 12 for UPC-A, 13 for EAN-13. Read it as a three-way switch:
  -- 0 for "not UPC/EAN", 2 or 5 for a standalone add-on, >= 6 for a real symbol.
  local main_width = symbol.width
  local upceanflag, addon, addon_len, addon_gap = 0, "", 0, 0
  if common.is_upcean(symbol.symbology) then
    upceanflag, main_width, addon, addon_len, addon_gap = output.process_upcean(symbol)
  end

  -- `show_hrt` is what --notext clears; an encoder that produced no text line
  -- (QR) hides it the other way.
  local hide_text = (symbol.show_hrt == 0) or (#symbol.text == 0)

  local xoffset, yoffset, roffset, boffset, qz_right =
    output.set_whitespace_offsets(symbol, hide_text)

  -- comp_xoffset is zero without a composite, so this is just xoffset -- except
  -- that the C declares it `int` and assigns a float to it, so the value is
  -- truncated. Every quiet zone and whitespace this port can produce is a whole
  -- number of modules, which makes the truncation a no-op today; it is written
  -- out because "happens to be integral" is a property of the current option
  -- set rather than a guarantee. Offsets are never negative, so floor is the
  -- C's truncate-toward-zero.
  local xoffset_comp = math.floor(xoffset)

  -- Dotty mode. The two fudges are zint compensating for antialiasing again:
  -- a dot wider than 1X spills past its module, and the image has to grow to
  -- hold the overspill. Under 1X nothing spills, so the offset is simply the
  -- module's midpoint -- (1 - dot_size) / 2 + dot_size / 2 == 1/2 whatever the
  -- size is.
  local dot_overspill = 0.0
  local dot_offset = 0.0
  if (opts & common.BARCODE_DOTTY_MODE) ~= 0 then
    if symbol.dot_size < 1.0 then
      dot_overspill = 0.0
      dot_offset = 0.5
    else
      dot_overspill = f32(f32(symbol.dot_size - 1.0) + 0.1)
      dot_offset = f32(f32(symbol.dot_size / 2.0) + 0.05)
    end
  end

  vector.width = f32(f32(symbol.width + dot_overspill) + f32(xoffset + roffset))

  -- Font metrics. These come from vector.c's own table for the Arimo face zint
  -- ships metrics for, and they are "scaled by 2" in the C's words -- i.e.
  -- these are half the numbers that reach the SVG, because vector_scale
  -- doubles them.
  --
  -- The fudge factor is the antialiasing allowance again, this time under the
  -- text: 0.02 per unit of font height. The descent is how far the face drops
  -- below the baseline, and differs per size because it is a measured value
  -- for that face at that size, not a ratio.
  local antialias_fudge_factor = f32(0.02)
  local descent = f32(1.32779717)        -- Arimo descent at font height 7
  local descent_small = f32(0.948426545) -- ... and at font height 5

  -- UPC/EAN uses a different face (OCRB, not Arimo) at a bigger size, and
  -- positions it from the top of the digits rather than from the baseline --
  -- hence `digit_ascender`, an estimate of how far a digit's top sits below the
  -- font box, taken as a flat 22% of the size. The C's comment calls the naming
  -- out: this is height above the digits, not the typographic ascent.
  local digit_ascender_factor = f32(0.22)

  -- --guardwhitespace prints "<" and ">" in the quiet zones so a printer can
  -- see they were not trimmed. The fudges nudge the two indicators outwards.
  -- The CLI does not expose --guardwhitespace. The bit still has to be honoured
  -- when a caller sets it, because EAN-8 and EAN-13 draw the indicators and the
  -- oracle does too.
  local upcean_guard_whitespace = (opts & common.EANUPC_GUARD_WHITESPACE) ~= 0
  local gws_left_fudge = f32(0.5)  -- EAN-8 "<", nudged toward the left edge
  local gws_right_fudge = f32(0.5)

  local small_text = (opts & common.SMALL_TEXT) ~= 0
  local font_height, antialias_fudge, text_gap, guard_descent
  local digit_ascender, upcae_outside_font_height = 0.0, 0

  if upceanflag ~= 0 then
    -- --bold is ignored for UPC/EAN, so there is no branch on it here; svg.lua
    -- is where that is enforced.
    font_height = small_text and 7 or 10
    digit_ascender = f32(font_height * digit_ascender_factor)
    antialias_fudge = f32(font_height * antialias_fudge_factor)
    -- The outside digits -- the number system digit and the check digit, which
    -- sit outside the guard bars -- are drawn smaller. GS1 General
    -- Specifications 5.2.5: "the size of the first and last digits should be
    -- reduced to a maximum width equivalent to four modules".
    upcae_outside_font_height = small_text and 6 or 7
    -- Subtracting the ascender here is what makes the UPC/EAN text sit tight
    -- against the bars: --textgap is measured to the top of the *digits*, not
    -- to the top of the font box. With the default gap of 1 this is negative.
    text_gap = f32(symbol.text_gap - digit_ascender)
    -- Guard bars hang below the rest. Standalone add-ons have none.
    guard_descent = upceanflag >= 6 and symbol.guard_descent or 0.0
  else
    font_height = small_text and 5 or 7
    antialias_fudge = f32(font_height * antialias_fudge_factor)
    text_gap = symbol.text_gap
    guard_descent = 0.0
  end

  local textoffset
  if hide_text then
    textoffset = guard_descent
  else
    textoffset = f32(font_height + f32(text_gap + antialias_fudge))
    -- --notext aside, the image still has to be tall enough for the guard bars.
    -- With the defaults the text is the taller of the two, so this rarely
    -- fires; --textgap -5 or --guarddescent 20 makes it the deciding clause.
    if upceanflag ~= 0 and textoffset < guard_descent then
      textoffset = guard_descent
    end
  end

  vector.height = f32(f32(f32(symbol.height + textoffset) + dot_overspill) + f32(yoffset + boffset))

  -- Where the last row's rectangles start in the list. Only the guard-bar pass
  -- below reads it, and only for UPC/EAN, which has one row -- so it is always
  -- zero here. Kept because the C keeps it, and because it is the thing that
  -- would need to be right if a stacked UPC/EAN composite were ever added.
  local last_row_start = 0
  -- Baseline for the add-on's digits, computed during the bitmap scan because
  -- that is where the add-on's own row geometry is worked out.
  local addon_text_yposn = 0.0

  if symbol.symbology == 57 then
    -- backend/vector.c, the MaxiCode arm. It replaces the width and height
    -- just computed: the grid is fixed, and the bullseye is not a module.
    -- `1.1547` is 2/√3 and `0.866` is √3/2, both already the C's float
    -- literals rather than a freshly computed root.
    local two_div_sqrt3 = f32(1.1547)
    local sqrt3_div_two = f32(0.866)
    local hex_diameter = f32(1.0)
    local hex_radius = f32(hex_diameter / 2.0)
    local hex_ydiameter = f32(two_div_sqrt3 * hex_diameter)
    local hex_yradius = f32(hex_ydiameter / 2.0)
    local yposn_offset = f32(sqrt3_div_two * hex_diameter)

    vector.width = f32(f32(30 * hex_diameter) + f32(xoffset + roffset))
    vector.height = f32(f32(f32(32 * yposn_offset) + hex_ydiameter)
      + f32(yoffset + boffset))

    local bull_x = f32(f32(f32(14.5) * hex_diameter) + xoffset)
    local bull_y = f32(vector.height / 2.0)
    local bull_d_incr = f32(f32(f32(hex_diameter * 9.0) - hex_ydiameter) / 5.0)
    local bull_width = f32(bull_d_incr / 2.0)
    local function bull_d(n)
      return f32(f32(hex_ydiameter + f32(bull_d_incr * n)) - bull_width)
    end
    add_circle(vector, bull_x, bull_y, bull_d(5), bull_width, 0)
    add_circle(vector, bull_x, bull_y, bull_d(3), bull_width, 0)
    add_circle(vector, bull_x, bull_y, bull_d(1), bull_width, 0)

    for r = 0, symbol.rows - 1 do
      local odd_row = r & 1
      local hex_yposn = f32(f32(f32(r * yposn_offset) + hex_yradius) + yoffset)
      local xposn_offset = f32((odd_row ~= 0 and hex_diameter or hex_radius) + xoffset)
      for i = 0, symbol.width - odd_row - 1 do
        if module_is_set(symbol, r, i) == 1 then
          add_hexagon(vector, f32(f32(i * hex_diameter) + xposn_offset),
                      hex_yposn, hex_diameter)
        end
      end
    end

  elseif (opts & common.BARCODE_DOTTY_MODE) ~= 0 then
    -- One circle per set module, no run merging -- dots do not join up the way
    -- rectangles do. Note the C puts this branch *before* the UPC/EAN one, so a
    -- dotty UPC-A would come out here rather than with guard bars -- except
    -- that library.c refuses --dotty for every linear symbology first, so the
    -- combination never arrives.
    for r = 0, symbol.rows - 1 do
      for i = 0, symbol.width - 1 do
        if module_is_set(symbol, r, i) == 1 then
          add_circle(vector, f32(f32(i + dot_offset) + xoffset),
                     f32(f32(r + dot_offset) + yoffset), symbol.dot_size, 0, 0)
        end
      end
    end

  elseif upceanflag >= 6 then
    -- UPC/EAN with a body rather than a standalone add-on. Same run merging as
    -- the generic branch below, with one addition: once the scan passes
    -- main_width, everything left is the add-on, and the add-on's bars are
    -- shorter and start lower -- its digits are printed *above* it rather than
    -- below, so the bars have to make room.
    --
    -- The latch is set on the first block past main_width and never cleared,
    -- which is why the geometry is computed once rather than per block.
    local yposn = yoffset
    local addon_latch = false
    local addon_row_yposn, addon_row_height = 0.0, 0.0
    local rect_count = 0

    for r = 0, symbol.rows - 1 do
      local rh = symbol.row_height[r] or 0.0
      local row_height = rh ~= 0 and rh or large_bar_height
      last_row_start = rect_count

      local i = 0
      while i < symbol.width do
        local fill = module_is_set(symbol, r, i)
        local block_width = 1
        while (i + block_width < symbol.width)
            and module_is_set(symbol, r, i + block_width) == fill do
          block_width = block_width + 1
        end

        if r == symbol.rows - 1 and i > main_width and not addon_latch then
          addon_text_yposn = f32(f32(yposn + font_height) - digit_ascender)
          if addon_text_yposn < 0.0 then addon_text_yposn = 0.0 end
          addon_row_yposn =
            f32(f32(f32(yposn + font_height) + text_gap) + antialias_fudge)
          addon_row_height = f32(row_height - f32(addon_row_yposn - yposn))
          -- ISO/IEC 15420 Figure 5 descends the add-on to the guard line for
          -- every member of the family, including UPC-A and UPC-E. zint 2.16
          -- does that. Earlier releases stopped UPC-A and UPC-E short of it.
          addon_row_height = f32(addon_row_height + guard_descent)
          if addon_row_height < 0.5 then addon_row_height = 0.5 end
          addon_latch = true
        end

        if fill == 1 then
          if addon_latch then
            add_rect(vector, f32(i + xoffset), addon_row_yposn,
                     block_width, addon_row_height)
          else
            add_rect(vector, f32(i + xoffset), yposn, block_width, row_height)
          end
          rect_count = rect_count + 1
        end

        i = i + block_width
      end

      yposn = f32(yposn + row_height)
    end

  else
    -- Rectangles. The generic branch: Code 39, Code 128, GS1-128, QR.
    --
    -- Runs of equal modules are measured first and then emitted as a single
    -- rectangle, so a wide bar is one rect rather than several. Unset runs are
    -- measured too, and skipped: the measuring is what advances i past them.
    --
    -- A standalone EAN-2 or EAN-5 prints its digits above the bars, so the
    -- bars start below that line. A symbol with a body took the branch above.
    local yposn = yoffset
    if upceanflag ~= 0 and not hide_text then
      yposn = f32(f32(f32(yposn + font_height) + text_gap) + antialias_fudge)
    end
    for r = 0, symbol.rows - 1 do
      local rh = symbol.row_height[r] or 0.0
      local row_height = rh ~= 0 and rh or large_bar_height

      local i = 0
      while i < symbol.width do
        local fill = module_is_set(symbol, r, i)
        local block_width = 1
        while (i + block_width < symbol.width)
            and module_is_set(symbol, r, i + block_width) == fill do
          block_width = block_width + 1
        end
        if fill == 1 then
          add_rect(vector, f32(i + xoffset), yposn, block_width, row_height)
        end
        i = i + block_width
      end

      yposn = f32(yposn + row_height)
    end
  end

  -- Guard bar extension. The guard bars -- and the two digits that sit outside
  -- them -- run further down than the rest so the human-readable line has
  -- somewhere to go without colliding with the code.
  --
  -- The C identifies them by *position in the rectangle list*, not by module
  -- index, and each family member gets its own hard-coded set. For UPC-A the
  -- set is {0,1, 2,3, 14,15, 26,27, 28,29}, which is derived rather than
  -- arbitrary: the pattern is 3 + 6*4 + 5 + 6*4 + 3 = 59 elements alternating
  -- bar and space from a bar, so the bars land at even element indices. That
  -- puts the left guard's two bars at 0-1, the number system digit's at 2-3,
  -- the centre guard's at 14-15, the check digit's at 26-27 and the right
  -- guard's at 28-29.
  --
  -- Note this runs before the text and border rectangles are added, so the list
  -- holds nothing but bars at this point -- which is what makes indexing by
  -- position safe.
  if guard_descent ~= 0 and upceanflag >= 6 then
    -- Rectangle indices into the last row of bars, before text and borders.
    -- UPC-A extends the number-system digit and the check digit as well as
    -- the three guards. EAN-13's first digit is not a bar pattern, so only
    -- the guards descend: left 0-1, centre 14-15, right 28-29.
    local guard_rects
    if upceanflag == 13 then
      guard_rects = {
        [0] = true, [1] = true, [14] = true, [15] = true,
        [28] = true, [29] = true,
      }
    elseif upceanflag == 8 then
      -- EAN-8: left guard, centre guard, right guard. Eight digits, so the
      -- centre lands earlier than on a 95-module symbol.
      guard_rects = {
        [0] = true, [1] = true, [10] = true, [11] = true,
        [20] = true, [21] = true,
      }
    elseif upceanflag == 6 then
      -- UPC-E: left guard and the three-bar stop. There is no centre guard
      -- and no separate check-digit bars.
      guard_rects = {
        [0] = true, [1] = true, [14] = true, [15] = true, [16] = true,
      }
    else
      guard_rects = {
        [0] = true, [1] = true, [2] = true, [3] = true, [14] = true,
        [15] = true, [26] = true, [27] = true, [28] = true, [29] = true,
      }
    end
    for idx, rect in ipairs(vector.rectangles) do
      if guard_rects[(idx - 1) - last_row_start] then
        rect.height = f32(rect.height + guard_descent)
      end
    end
  end

  -- Human readable text.
  if not hide_text then
    -- A bottom border sits between the symbol and the text, so the text has to
    -- drop by its width. --bindtop draws no bottom bar, hence the exclusion --
    -- and note the test is BOX|BIND only, so --bindtop on its own never moves
    -- the text even though it does move `yoffset`. Both branches do this, and
    -- both do it after the baseline is worked out rather than as part of it.
    local function drop_for_border(y)
      if symbol.border_width > 0
          and (opts & (common.BARCODE_BOX | common.BARCODE_BIND)) ~= 0
          and (opts & common.BARCODE_BIND_TOP) == 0 then
        return f32(y + symbol.border_width)
      end
      return y
    end

    if upceanflag >= 6 then
      -- UPC/EAN. Not one centred string. The digits have to line up with the
      -- bars they belong to, and an outside digit sits in the quiet zone.
      -- Each family member has its own slots, below. The numbers are raw
      -- module counts, unscaled -- vector_scale multiplies them at the end.
      --
      -- Note this baseline subtracts the antialias fudge where the generic
      -- branch below subtracts the font's descent. The two are not the same
      -- quantity and neither is a substitute for the other: OCRB is positioned
      -- from the top of the digits, so what would be a descent correction has
      -- already been folded into `text_gap` via `digit_ascender`.
      local text_yposn = f32(f32(f32(f32(yoffset + symbol.height)
                                     + font_height) + text_gap) - antialias_fudge)
      text_yposn = drop_for_border(text_yposn)

      local t = symbol.text

      if upceanflag == 13 then
        -- EAN-13. The first digit is full size and right-aligned in the left
        -- quiet zone. The nudge is 0.1, not UPC-A's 0.35, and the face is the
        -- normal OCR-B size rather than the smaller outside face. Six digits
        -- sit under each half: centres at 24.5 and 70.5, each group 6 * 8.5
        -- modules wide. The add-on centres match UPC-A (105 / 119) because
        -- the body is the same 95 modules. A guard-whitespace ">" is drawn
        -- even with no add-on; UPC-A leaves that slot to the check digit.
        local group_width = f32(f32(6.0) * f32(8.5))
        add_string(vector, t:sub(1, 1),
                   f32(-f32(f32(5.0) - f32(0.1)) + xoffset_comp), text_yposn,
                   font_height, f32(8.5), 2)
        add_string(vector, t:sub(2, 7),
                   f32(f32(f32(24.0) + f32(0.5)) + xoffset_comp), text_yposn,
                   font_height, group_width, 0)
        add_string(vector, t:sub(8, 13),
                   f32(f32(f32(71.0) - f32(0.5)) + xoffset_comp), text_yposn,
                   font_height, group_width, 0)
        if addon_len ~= 0 then
          add_string(vector, addon,
                     f32(f32((addon_len == 2 and 105.0 or 119.0) + xoffset_comp)
                         + addon_gap),
                     addon_text_yposn, font_height, f32(addon_len * 8.5), 0)
          if upcean_guard_whitespace then
            add_string(vector, ">",
                       f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                       addon_text_yposn, font_height, f32(8.5), 2)
          end
        elseif upcean_guard_whitespace then
          add_string(vector, ">",
                     f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                     text_yposn, font_height, f32(8.5), 2)
        end
      elseif upceanflag == 8 then
        -- EAN-8. Four digits under each half, centres 17.5 and 49.5, each
        -- group 4 * 8.5 modules. No outside digit. A guard-whitespace "<"
        -- sits in the left quiet zone and a ">" in the right, or above the
        -- add-on when there is one. Add-on centres are 77 and 91.
        local group_width = f32(f32(4.0) * f32(8.5))
        if upcean_guard_whitespace then
          add_string(vector, "<",
                     f32(f32(-f32(7.0) - gws_left_fudge) + xoffset_comp),
                     text_yposn, font_height, f32(8.5), 1)
        end
        add_string(vector, t:sub(1, 4),
                   f32(f32(f32(17.0) + f32(0.5)) + xoffset_comp), text_yposn,
                   font_height, group_width, 0)
        add_string(vector, t:sub(5, 8),
                   f32(f32(f32(50.0) - f32(0.5)) + xoffset_comp), text_yposn,
                   font_height, group_width, 0)
        if addon_len ~= 0 then
          add_string(vector, addon,
                     f32(f32((addon_len == 2 and 77.0 or 91.0) + xoffset_comp)
                         + addon_gap),
                     addon_text_yposn, font_height, f32(addon_len * 8.5), 0)
          if upcean_guard_whitespace then
            add_string(vector, ">",
                       f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                       addon_text_yposn, font_height, f32(8.5), 2)
          end
        elseif upcean_guard_whitespace then
          add_string(vector, ">",
                     f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                     text_yposn, font_height, f32(8.5), 2)
        end
      elseif upceanflag == 6 then
        -- UPC-E. Number system in the left quiet zone, six digits centred at
        -- 24.5 across 6 * 8.5 modules, check digit just past the 51-module
        -- stop. The check sits at 2X rather than the spec's 3X so it does not
        -- meet a descended add-on. Add-on centres are 61 and 75. No ">"
        -- without an add-on: the check digit already occupies that side.
        local outside_width = f32(6.2)
        local group_width = f32(f32(6.0) * f32(8.5))
        add_string(vector, t:sub(1, 1),
                   f32(-f32(f32(5.0) - f32(0.35)) + xoffset_comp), text_yposn,
                   upcae_outside_font_height, outside_width, 2)
        add_string(vector, t:sub(2, 7),
                   f32(f32(f32(24.0) + f32(0.5)) + xoffset_comp), text_yposn,
                   font_height, group_width, 0)
        add_string(vector, t:sub(8, 8),
                   f32(f32(f32(f32(51.0) - f32(0.35)) + f32(2.0)) + xoffset_comp),
                   text_yposn, upcae_outside_font_height, outside_width, 1)
        if addon_len ~= 0 then
          add_string(vector, addon,
                     f32(f32((addon_len == 2 and 61.0 or 75.0) + xoffset_comp)
                         + addon_gap),
                     addon_text_yposn, font_height, f32(addon_len * 8.5), 0)
          if upcean_guard_whitespace then
            add_string(vector, ">",
                       f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                       addon_text_yposn, font_height, f32(8.5), 2)
          end
        end
      else
      local outside_width = f32(6.2)     -- four modules, near enough
      local group_width = f32(5.0 * 8.5) -- five digits at 8.5 modules each

      -- Number system digit, in the left quiet zone, hanging off the left edge
      -- and right-aligned so it grows away from the guard bar rather than into
      -- it. The 0.35 is a hand-tuned nudge, not a derived value.
      add_string(vector, t:sub(1, 1),
                 f32(-f32(f32(5.0) - f32(0.35)) + xoffset_comp), text_yposn,
                 upcae_outside_font_height, outside_width, 2)
      -- The two inner groups of five, centred under their halves.
      add_string(vector, t:sub(2, 6), f32(f32(28.0) + xoffset_comp), text_yposn,
                 font_height, group_width, 0)
      add_string(vector, t:sub(7, 11), f32(f32(67.0) + xoffset_comp), text_yposn,
                 font_height, group_width, 0)
      -- Check digit, in the right quiet zone, left-aligned for the same reason.
      -- GS1's human-readable figure says 5X. zint 2.16 places it at 4X so the
      -- digit does not meet a descended add-on. (95 - 0.35) + 4.
      add_string(vector, t:sub(12, 12),
                 f32(f32(f32(f32(95.0) - f32(0.35)) + f32(4.0)) + xoffset_comp),
                 text_yposn, upcae_outside_font_height, outside_width, 1)

      -- The add-on's digits go above its bars, at the baseline the bitmap scan
      -- worked out. 105 and 119 are the centres of a 2- and a 5-digit add-on
      -- measured from the symbol's own origin, before the gap is added.
      if addon_len ~= 0 then
        add_string(vector, addon,
                   f32(f32((addon_len == 2 and 105.0 or 119.0) + xoffset_comp)
                       + addon_gap),
                   addon_text_yposn, font_height, f32(addon_len * 8.5), 0)
        if upcean_guard_whitespace then
          add_string(vector, ">",
                     f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                     addon_text_yposn, font_height, f32(8.5), 2)
        end
      end
      -- The C has no `else if (upcean_guard_whitespace)` here for UPC-A or
      -- UPC-E, unlike EAN-8 and EAN-13: with no add-on there is no ">" on a
      -- UPC symbol, because the check digit is already occupying that space.
      end

    elseif upceanflag ~= 0 then
      -- Standalone EAN-2 / EAN-5. The digits sit above the bars, centred on
      -- the symbol. A bottom border is already excluded: the test includes
      -- --bindtop, which the body symbols do not.
      local text_xposn = f32(main_width / 2.0 + xoffset)
      local text_yposn = f32(f32(yoffset + font_height) - digit_ascender)
      if symbol.border_width > 0
          and (opts & (common.BARCODE_BOX | common.BARCODE_BIND | common.BARCODE_BIND_TOP)) ~= 0 then
        text_yposn = f32(text_yposn - symbol.border_width)
      end
      if text_yposn < 0.0 then text_yposn = 0.0 end
      add_string(vector, symbol.text, text_xposn, text_yposn, font_height,
                 f32(#symbol.text * 8.5), 0)
      if upcean_guard_whitespace then
        add_string(vector, ">",
                   f32(f32(f32(symbol.width + gws_right_fudge) + qz_right) + xoffset),
                   text_yposn, font_height, f32(8.5), 2)
      end

    else
      -- Everything else: one centred string under the symbol. Worked out as the
      -- bottom edge of the text box, then lifted by the font's descent -- SVG's
      -- y on a <text> is the baseline, not the bottom.
      local text_xposn = f32(f32(main_width / 2.0) + xoffset_comp)
      local text_yposn = f32(f32(f32(yoffset + symbol.height) + font_height) + text_gap)
      text_yposn = f32(text_yposn - (small_text and descent_small or descent))
      text_yposn = drop_for_border(text_yposn)
      add_string(vector, symbol.text, text_xposn, text_yposn, font_height, symbol.width, 0)
    end
  end

  -- Bind/box. --border sets the width; one of --bind, --bindtop or --box has to
  -- be given as well or nothing is drawn, which is why the CLI's --border alone
  -- looks like it does nothing.
  --
  -- `horz_outside` is the matrix-symbology layout (QR here): the bars run the
  -- full height of the image rather than hugging the symbol, because a matrix
  -- code has no text line under it to stay clear of.
  if symbol.border_width > 0
      and (opts & (common.BARCODE_BOX | common.BARCODE_BIND | common.BARCODE_BIND_TOP)) ~= 0 then
    local horz_outside = symbol.symbology == 58 or symbol.symbology == 71
    local ybind_top = f32(yoffset - symbol.border_width)
    -- Equivalent to yoffset + symbol.height + dot_overspill, but computed from
    -- the far edge the way the C does so the rounding matches.
    local ybind_bot = f32(f32(vector.height - textoffset) - boffset)
    if horz_outside then
      ybind_top = 0.0
      ybind_bot = f32(vector.height - symbol.border_width)
    elseif upceanflag == 2 or upceanflag == 5 then
      -- The add-on's text is above the bars, so the box follows the bars
      -- down by the same textoffset the bitmap scan added.
      ybind_top = f32(ybind_top + textoffset)
      ybind_bot = f32(ybind_bot + textoffset)
    end

    add_rect(vector, 0.0, ybind_top, vector.width, symbol.border_width)
    if (opts & common.BARCODE_BIND_TOP) == 0 then
      add_rect(vector, 0.0, ybind_bot, vector.width, symbol.border_width)
    end

    if (opts & common.BARCODE_BOX) ~= 0 then
      local xbox_right = f32(vector.width - symbol.border_width)
      local box_top = yoffset
      -- Equivalent to symbol.height, again computed the long way round.
      local box_height = f32(f32(f32(f32(vector.height - textoffset) - dot_overspill) - yoffset) - boffset)
      if horz_outside then
        box_top = symbol.border_width
        box_height = f32(vector.height - symbol.border_width * 2)
      elseif upceanflag == 2 or upceanflag == 5 then
        box_top = f32(box_top + textoffset)
      end
      add_rect(vector, 0.0, box_top, symbol.border_width, box_height)       -- left
      add_rect(vector, xbox_right, box_top, symbol.border_width, box_height) -- right
    end
  end

  reduce_rectangles(vector)
  scale_vector(symbol, vector)
  rotate_vector(vector, rotate_angle)
end

return M
