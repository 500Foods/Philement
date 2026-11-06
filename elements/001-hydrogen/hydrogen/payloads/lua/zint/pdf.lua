-- src/zint/pdf.lua -- PDF Form XObject content stream from symbol.vector.
--
-- Sibling of svg.lua. It reads the vector list. It does not parse SVG.
-- The origin is the bottom left and y grows up. SVG y grows down, so each
-- anchor is flipped across the vector height. The report writer supplies the
-- object number, the stream dictionary, and the Do invocation.
--
-- Rectangles are the bars. Text is one Tj per string. The content stream
-- names Helvetica and does not embed a face, so a caller that ignores the
-- font field behaves as before. When the symbol has text, the return also
-- carries the raw TrueType bytes for that face: OCR-B for UPC and EAN,
-- Arimo otherwise. The writer embeds them. This file does not assign object
-- numbers, build a FontFile2, compress, or subset.
-- Circles (dotty mode) are four cubic beziers. A circle with a width is a
-- stroked ring, which is how MaxiCode draws its bullseye. Hexagons are
-- filled polygons, one per MaxiCode module.

local common = require "zint.common"

local M = {}
local f32 = common.stripf

local function num(v)
  local s = ("%.8f"):format(v)
  s = s:gsub("0+$", ""):gsub("%.$", "")
  if s == "-0" then s = "0" end
  return s
end

local function pdf_string(text)
  return "(" .. text:gsub("\\", "\\\\"):gsub("%(", "\\("):gsub("%)", "\\)") .. ")"
end

local function flip_y(height, y, h)
  return f32(f32(height - y) - (h or 0))
end

-- One circle as a closed cubic. kappa is the standard 4*(sqrt(2)-1)/3.
local KAPPA = f32(0.5522847498)

local function circle_ops(out, cx, cy, r, stroke)
  local k = f32(KAPPA * r)
  out[#out + 1] = num(cx - r) .. " " .. num(cy) .. " m\n"
  out[#out + 1] = num(cx - r) .. " " .. num(cy + k) .. " "
    .. num(cx - k) .. " " .. num(cy + r) .. " "
    .. num(cx) .. " " .. num(cy + r) .. " c\n"
  out[#out + 1] = num(cx + k) .. " " .. num(cy + r) .. " "
    .. num(cx + r) .. " " .. num(cy + k) .. " "
    .. num(cx + r) .. " " .. num(cy) .. " c\n"
  out[#out + 1] = num(cx + r) .. " " .. num(cy - k) .. " "
    .. num(cx + k) .. " " .. num(cy - r) .. " "
    .. num(cx) .. " " .. num(cy - r) .. " c\n"
  out[#out + 1] = num(cx - k) .. " " .. num(cy - r) .. " "
    .. num(cx - r) .. " " .. num(cy - k) .. " "
    .. num(cx - r) .. " " .. num(cy) .. " c\n"
  out[#out + 1] = stroke and "h\nS\n" or "h\nf\n"
end

-- Point-up when rotation is 0 or 180, flat-top otherwise. Y is still
-- SVG-down here; the caller flips it.
local function hex_verts(hex)
  local d = hex.diameter
  local radius = f32(0.5 * d)
  local half_radius = f32(0.25 * d)
  local half_sqrt3 = f32(f32(0.43301270189221932338) * d)
  local x, y = hex.x, hex.y
  if hex.rotation == 0 or hex.rotation == 180 then
    return {
      { x, f32(y + radius) },
      { f32(x + half_sqrt3), f32(y + half_radius) },
      { f32(x + half_sqrt3), f32(y - half_radius) },
      { x, f32(y - radius) },
      { f32(x - half_sqrt3), f32(y - half_radius) },
      { f32(x - half_sqrt3), f32(y + half_radius) },
    }
  end
  return {
    { f32(x - radius), y },
    { f32(x - half_radius), f32(y + half_sqrt3) },
    { f32(x + half_radius), f32(y + half_sqrt3) },
    { f32(x + radius), y },
    { f32(x + half_radius), f32(y - half_sqrt3) },
    { f32(x - half_radius), f32(y - half_sqrt3) },
  }
end

local function text_matrix(rotation, x, y)
  if rotation == 90 then
    return "0 1 -1 0 " .. num(x) .. " " .. num(y) .. " Tm\n"
  elseif rotation == 180 then
    return "-1 0 0 -1 " .. num(x) .. " " .. num(y) .. " Tm\n"
  elseif rotation == 270 then
    return "0 -1 1 0 " .. num(x) .. " " .. num(y) .. " Tm\n"
  end
  return "1 0 0 1 " .. num(x) .. " " .. num(y) .. " Tm\n"
end

function M.form(symbol)
  local vector = symbol.vector
  local height = vector.height
  local out = {}

  out[#out + 1] = "0 0 0 rg\n"
  for _, rect in ipairs(vector.rectangles) do
    local y = flip_y(height, rect.y, rect.height)
    out[#out + 1] = num(rect.x) .. " " .. num(y) .. " "
      .. num(rect.width) .. " " .. num(rect.height) .. " re\nf\n"
  end

  for _, hex in ipairs(vector.hexagons or {}) do
    local verts = hex_verts(hex)
    for i, p in ipairs(verts) do
      local y = flip_y(height, p[2], 0)
      out[#out + 1] = num(p[1]) .. " " .. num(y) .. (i == 1 and " m\n" or " l\n")
    end
    out[#out + 1] = "h\nf\n"
  end

  for _, circle in ipairs(vector.circles) do
    local r = f32(0.5 * circle.diameter)
    local cy = flip_y(height, circle.y, 0)
    if (circle.width or 0) ~= 0 then
      out[#out + 1] = num(circle.width) .. " w\n"
      circle_ops(out, circle.x, cy, r, true)
    else
      circle_ops(out, circle.x, cy, r, false)
    end
  end

  for _, str in ipairs(vector.strings) do
    local y = flip_y(height, str.y, 0)
    out[#out + 1] = "BT\n"
    out[#out + 1] = "/F1 " .. num(str.fsize) .. " Tf\n"
    out[#out + 1] = text_matrix(str.rotation or 0, str.x, y)
    out[#out + 1] = pdf_string(str.text) .. " Tj\n"
    out[#out + 1] = "ET\n"
  end

  local form = {
    kind = "form-xobject",
    bbox = { 0, 0, vector.width, vector.height },
    content = table.concat(out),
    resources = {
      Font = { F1 = { BaseFont = "Helvetica", Subtype = "Type1" } },
    },
  }

  -- Same lazy require as svg.lua. The faces are tens of kilobytes and a
  -- symbol with no text has nothing to set them on.
  if #vector.strings > 0 then
    local fonts = require "zint.fonts_ttf"
    local upcean = common.is_upcean(symbol.symbology)
    form.font = {
      family = upcean and "OCRB" or "Arimo",
      format = "TTF",
      bytes = upcean and fonts.upcean or fonts.normal,
    }
  end

  return form
end

return M
