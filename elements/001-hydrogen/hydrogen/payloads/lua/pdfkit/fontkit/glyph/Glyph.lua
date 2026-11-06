-- fontkit/glyph/Glyph.lua -- port of fontkit/src/glyph/Glyph.js
--
-- One glyph in a font: an id, the code points that reach it, and the metrics
-- and outline that go with it. Never constructed directly -- TTFFont:getGlyph
-- hands them out and caches them, so two lookups of the same id give the same
-- object and the same cached measurements.
--
-- The outline lives in a subclass, since a TrueType font stores it as quadratic
-- contours in `glyf` and a PostScript one as a charstring program in `CFF `.
-- Everything else -- advance widths, bearings, names -- is format independent
-- and lives here.
--
-- The properties upstream exposes as lazily-cached getters (`cbox`, `bbox`,
-- `path`, `advanceWidth`, `advanceHeight`, `name`) are methods here, each
-- memoising into a `_`-prefixed field on first call. Same laziness, same
-- caching, different syntax at the call site.

local unicode = require("pdfkit.fontkit.unicode")
local Path = require("pdfkit.fontkit.glyph.Path")
local StandardNames = require("pdfkit.fontkit.glyph.StandardNames")

-- Zero-based, so the border is one short of the count.
local STANDARD_NAME_COUNT = #StandardNames + 1

local Glyph = {}
Glyph.__index = Glyph

--- Set up a glyph on an already-created table.
---
--- Split out from `new` so a subclass can build its own instance with its own
--- metatable and still run the base constructor over it, which is what
--- `extends Glyph` gets for free in JS.
---
--- `codePoints` is one-based and may be empty (a glyph reached only through
--- substitution) or hold several (a ligature).
function Glyph.init(self, id, codePoints, font)
    self.id = id
    self.codePoints = codePoints
    self._font = font

    -- TODO upstream: get this info from GDEF if available.
    --
    -- A glyph counts as a mark only if *every* code point behind it is one, so
    -- a ligature of a base and a mark is not itself a mark.
    local isMark = #codePoints > 0
    for i = 1, #codePoints do
        if not unicode.is_mark(codePoints[i]) then
            isMark = false
            break
        end
    end

    self.isMark = isMark
    self.isLigature = #codePoints > 1

    return self
end

function Glyph.new(id, codePoints, font)
    return Glyph.init(setmetatable({}, Glyph), id, codePoints, font)
end

--------------------------------------------------------------------------------
-- Outline
--------------------------------------------------------------------------------

--- The outline. Overridden by every subclass; the base returns an empty path.
function Glyph:_getPath()
    return Path.new()
end

function Glyph:_getCBox()
    return self:path():cbox()
end

function Glyph:_getBBox()
    return self:path():bbox()
end

--- The glyph outline as a Path, decoded once and kept.
function Glyph:path()
    if not self._path then
        self._path = self:_getPath()
    end
    return self._path
end

--- The control box: fast, and an overestimate where a curve bulges inside its
--- control points.
function Glyph:cbox()
    if not self._cbox then
        self._cbox = self:_getCBox()
    end
    return self._cbox
end

--- The exact bounding box.
function Glyph:bbox()
    if not self._bbox then
        self._bbox = self:_getBBox()
    end
    return self._bbox
end

--- The outline scaled from font units to `size` points.
function Glyph:getScaledPath(size)
    local scale = 1 / self._font:unitsPerEm() * size
    return self:path():scale(scale)
end

--------------------------------------------------------------------------------
-- Metrics
--------------------------------------------------------------------------------

--- One glyph's row out of hmtx or vmtx.
---
--- Those tables are run-length compressed at the tail: fonts whose last glyphs
--- all share an advance store the advance once and then only the bearings. A
--- glyph past the end of the metrics array takes the last advance and its own
--- bearing from the second array.
function Glyph:_getTableMetrics(table_)
    if self.id < table_.metrics.length then
        return table_.metrics:get(self.id)
    end

    local metric = table_.metrics:get(table_.metrics.length - 1)
    return {
        advance = metric and metric.advance or 0,
        bearing = table_.bearings:get(self.id - table_.metrics.length) or 0,
    }
end

--- Advance widths and side bearings, computed once and cached.
---
--- `cbox` is passed in by the TrueType subclass, which has already read the
--- glyph header and would otherwise pay for it twice.
function Glyph:_getMetrics(cbox)
    if self._metrics then
        return self._metrics
    end

    local hmetrics = self:_getTableMetrics(self._font.hmtx)
    local advanceWidth = hmetrics.advance
    local leftBearing = hmetrics.bearing

    local advanceHeight, topBearing

    -- For vertical metrics, use vmtx if available, or fall back to global data
    -- from OS/2 or hhea. The fallback makes every glyph the same height, which
    -- is wrong for vertical text but right for the mark positioning that reads
    -- it in horizontal text.
    if self._font.vmtx then
        local vmetrics = self:_getTableMetrics(self._font.vmtx)
        advanceHeight = vmetrics.advance
        topBearing = vmetrics.bearing
    else
        if cbox == nil then
            cbox = self:cbox()
        end

        local os2 = self._font["OS/2"]
        if os2 and os2.version > 0 then
            advanceHeight = math.abs(os2.typoAscender - os2.typoDescender)
            topBearing = os2.typoAscender - cbox.maxY
        else
            local hhea = self._font.hhea
            advanceHeight = math.abs(hhea.ascent - hhea.descent)
            topBearing = hhea.ascent - cbox.maxY
        end
    end

    -- Variable fonts adjust the advance per instance. Not reached until the
    -- variation processor is ported; the branch is here so the shape of the
    -- function matches upstream.
    if self._font._variationProcessor and self._font.HVAR then
        advanceWidth = advanceWidth
            + self._font._variationProcessor:getAdvanceAdjustment(self.id, self._font.HVAR)
    end

    self._metrics = {
        advanceWidth = advanceWidth,
        advanceHeight = advanceHeight,
        leftBearing = leftBearing,
        topBearing = topBearing,
    }

    return self._metrics
end

--- How far the pen moves after drawing this glyph, in font units.
function Glyph:advanceWidth()
    if not self._advanceWidth then
        self._advanceWidth = self:_getMetrics().advanceWidth
    end
    return self._advanceWidth
end

function Glyph:advanceHeight()
    if not self._advanceHeight then
        self._advanceHeight = self:_getMetrics().advanceHeight
    end
    return self._advanceHeight
end

--- Where the caret may be placed inside a ligature. Only AAT fonts carry the
--- information; the base returns nothing, as upstream does.
function Glyph:ligatureCaretPositions()
    return nil
end

--------------------------------------------------------------------------------
-- Name
--------------------------------------------------------------------------------

--- The PostScript name of the glyph, from whichever form of `post` is present.
---
--- Returns nil when the font has no post table, or has version 3, which
--- declares that the names are not stored -- so a nil here is a fact about the
--- font, not a failure.
function Glyph:_getName()
    local post = self._font.post
    if not post then
        return nil
    end

    local version = post.version

    if version == 1 then
        -- The glyph order *is* the standard order.
        return StandardNames[self.id]
    elseif version == 2 then
        -- One index per glyph, into the standard names or the font's own list.
        -- Both arrays here are one-based while the ids indexing them are not.
        local id = post.glyphNameIndex[self.id + 1]
        if id < STANDARD_NAME_COUNT then
            return StandardNames[id]
        end

        return post.names[id - STANDARD_NAME_COUNT + 1]
    elseif version == 2.5 then
        return StandardNames[self.id + post.offsets[self.id + 1]]
    elseif version == 4 then
        -- Version 4 stores a character code, not a name; upstream turns it
        -- into the one-character string of that code with fromCharCode, which
        -- takes the low sixteen bits of the uint32 and nothing else. Masked
        -- here for the same reason.
        return utf8.char(post.map[self.id + 1] & 0xFFFF)
    end

    return nil
end

--- The glyph's name, looked up once.
function Glyph:name()
    if self._name == nil then
        self._name = self:_getName() or false
    end
    return self._name or nil
end

return Glyph
