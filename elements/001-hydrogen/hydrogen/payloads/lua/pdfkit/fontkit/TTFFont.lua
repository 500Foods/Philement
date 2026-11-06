-- fontkit/TTFFont.lua -- port of fontkit/src/TTFFont.js
--
-- A whole font file. Everything under tables/ and glyph/ hangs off this: it owns
-- the directory, decodes a table the first time somebody names it, hands out
-- glyphs and caches them, and is the object pdfkit's font layer talks to.
--
-- Nothing is read up front except the directory. That is deliberate upstream and
-- worth keeping -- embedding a font in a PDF touches head, hhea, hmtx, maxp,
-- name, post, OS/2, cmap, loca and glyf, and a font may well carry thirty tables
-- this port has no decoder for at all. Those stay as an offset and a length,
-- which is all the subset writer needs to copy them.
--
-- Three things about the shape of this file:
--
-- Upstream defines a lazily-parsing property on the instance for every table tag
-- in the directory, so `font.head` is a field access that decodes on first read.
-- That is reproduced with an __index metamethod, so the call sites read the same
-- (`font.hmtx`, `font["OS/2"]`) and no table is decoded until one does.
--
-- The metric getters -- postscriptName, ascent, unitsPerEm and the rest -- are
-- getters upstream and methods here, so they are `font:ascent()` at the call
-- site. Lua has no property syntax, and making them fields would mean either
-- computing all of them eagerly or overloading __index with two kinds of thing.
--
-- The @cache decorated getters (bbox, characterSet, _cmapProcessor,
-- _layoutEngine) are methods that memoise into a field named after themselves.

local base = require("pdfkit.fontkit.base")
local reader = require("pdfkit.fontkit.reader")
local encodings = require("pdfkit.fontkit.encodings")
local utils = require("pdfkit.fontkit.utils")
local Directory = require("pdfkit.fontkit.tables.directory")
local tables = require("pdfkit.fontkit.tables")
local CmapProcessor = require("pdfkit.fontkit.CmapProcessor")
local BBox = require("pdfkit.fontkit.glyph.BBox")
local TTFGlyph = require("pdfkit.fontkit.glyph.TTFGlyph")

local Reader = reader.Reader

local TTFFont = {}

--------------------------------------------------------------------------------
-- Construction
--------------------------------------------------------------------------------

-- Field lookup on a font instance.
--
-- Table tags are checked before methods, which is the order upstream ends up
-- with: its per-tag properties are defined on the instance and so shadow
-- anything on the prototype. No tag in the registry shares a name with a method
-- here, so nothing is actually shadowed either way -- but a future table called
-- `layout` or `version` should resolve to the table, as it would there.
local font_mt = {
    __index = function(self, key)
        local lazy = rawget(self, "_lazyTables")
        local entry = lazy and lazy[key]
        if entry then
            return TTFFont._getTable(self, entry)
        end

        return TTFFont[key]
    end,
}

--- Does this look like an SFNT file?
---
--- The four magic values: 'true' for an old Macintosh TrueType file, 'OTTO' for
--- PostScript outlines, and 00 01 00 00 for everything else. 'ttcf' is a
--- collection and is handled a level up, not here.
function TTFFont.probe(buffer)
    local format = buffer:sub(1, 4)
    return format == "true"
        or format == "OTTO"
        or format == "\0\1\0\0"
end

--- Wrap a reader positioned at the start of a font.
---
--- Not necessarily at the start of the file: in a TrueType collection each font
--- has its own directory somewhere inside the shared data, which is why the
--- position is remembered rather than assumed to be zero.
function TTFFont.new(stream, variationCoords, mt)
    local self = setmetatable({}, mt or font_mt)

    self.type = "TTF"
    self.defaultLanguage = nil
    self.stream = stream
    self.variationCoords = variationCoords

    self._directoryPos = stream.pos
    self._tables = {}
    self._glyphs = {}
    self._lazyTables = {}

    self:_decodeDirectory()

    -- Every tag the port has a decoder for becomes a lazily-decoded field. A
    -- zero-length table is skipped: some fonts keep a stub directory entry for a
    -- table they no longer carry, and handing it to a decoder reads whatever
    -- happens to follow it in the file.
    --
    -- `glyf` is registered for writing only and has no decoder, so it drops out
    -- here and `font.glyf` stays nil -- as it is upstream, where the table is
    -- declared but never listed among the ones a font exposes.
    for tag, entry in pairs(self.directory.tables) do
        if tables[tag] and tables[tag].decode and entry.length > 0 then
            self._lazyTables[tag] = entry
        end
    end

    return self
end

function TTFFont:setDefaultLanguage(lang)
    self.defaultLanguage = lang
end

--------------------------------------------------------------------------------
-- Tables
--------------------------------------------------------------------------------

--- Decode a table if it has not been decoded, and return it.
---
--- A decoder that raises leaves the table undefined and the font usable. That is
--- upstream's behaviour and it is load-bearing: fonts in the wild have malformed
--- post and OS/2 tables, and refusing to open them would lose the ninety per
--- cent of the font that is fine. Note that nothing is cached on failure, so a
--- broken table is retried -- and re-raises -- on every access, exactly as there.
function TTFFont:_getTable(entry)
    local cached = self._tables[entry.tag]
    if cached ~= nil then
        return cached
    end

    local ok, res = pcall(self._decodeTable, self, entry)

    if not ok then
        if base.logErrors then
            io.stderr:write("Error decoding table " .. entry.tag .. "\n")
            io.stderr:write(tostring(res) .. "\n")
        end
        return nil
    end

    self._tables[entry.tag] = res
    return res
end

--- A reader positioned at the first byte of `tag`, or nil if the font has no
--- such table.
---
--- Upstream moves the font's one shared stream and hands that back, then puts it
--- where it found it afterwards. A fresh cursor over the same data does the same
--- job without the save-and-restore, and without two callers holding the same
--- cursor being a bug waiting to happen.
function TTFFont:_getTableStream(tag)
    local entry = self.directory.tables[tag]
    if entry then
        return Reader.new(self.stream.data, entry.offset)
    end

    return nil
end

function TTFFont:_decodeDirectory()
    self.directory = Directory.decode(Reader.new(self.stream.data, self._directoryPos))
    return self.directory
end

--- Run a table's decoder over its bytes.
---
--- The font is passed in because several decoders need something from another
--- table -- loca needs head.indexToLocFormat and maxp.numGlyphs, post version 4
--- needs maxp.numGlyphs -- and reach it through `font`. That is a re-entrant
--- decode, and it works because the cursor is per-call.
function TTFFont:_decodeTable(entry)
    local stream = self:_getTableStream(entry.tag)
    return tables[entry.tag].decode(stream, self, entry.length)
end

--------------------------------------------------------------------------------
-- Names
--------------------------------------------------------------------------------

--- The first argument that is neither nil nor the empty string.
---
--- Reproduces a chain of `||` in JS, where an empty string is falsy and so falls
--- through to the next candidate. A font with a present but empty postscriptName
--- for one language should use another language's, not the empty one.
local function first_truthy(...)
    for i = 1, select("#", ...) do
        local v = select(i, ...)
        if v ~= nil and v ~= false and v ~= "" then
            return v
        end
    end

    return nil
end

--- A string out of the `name` table. `lang` is a BCP-47 code.
---
--- The fallback chain is upstream's, in order: the requested language, the
--- font's default, the library default, English, and finally whichever language
--- happened to be recorded first -- with the comment "seriously, ANY language
--- would be fine", which is fair for a font whose only name records are Korean.
--- That last step needs an insertion order that a Lua table does not have, so
--- the name decoder records one alongside.
function TTFFont:getName(key, lang)
    lang = lang or self.defaultLanguage or base.defaultLanguage

    local name = self.name
    local record = name and name.records[key]

    if record then
        local firstLang
        local order = name.langOrder and name.langOrder[record]
        if order then
            firstLang = order[1]
        else
            firstLang = next(record)
        end

        return first_truthy(
            record[lang],
            record[self.defaultLanguage],
            record[base.defaultLanguage],
            record["en"],
            firstLang ~= nil and record[firstLang] or nil
        )
    end

    return nil
end

--- The unique PostScript name, e.g. "Helvetica-Bold". This is the one pdfkit
--- puts in /BaseFont, prefixed with a subset tag.
function TTFFont:postscriptName()
    return self:getName("postscriptName")
end

--- The full name, e.g. "Helvetica Bold".
function TTFFont:fullName()
    return self:getName("fullName")
end

--- The family name, e.g. "Helvetica".
function TTFFont:familyName()
    return self:getName("fontFamily")
end

--- The sub-family, e.g. "Bold".
function TTFFont:subfamilyName()
    return self:getName("fontSubfamily")
end

function TTFFont:copyright()
    return self:getName("copyright")
end

--- The version, as the string the designer wrote -- not the head table's number.
function TTFFont:version()
    return self:getName("version")
end

--------------------------------------------------------------------------------
-- Metrics
--------------------------------------------------------------------------------

--- Height of the tallest ascender, in font units.
function TTFFont:ascent()
    return self.hhea.ascent
end

--- Depth of the lowest descender. Negative in a sane font.
function TTFFont:descent()
    return self.hhea.descent
end

--- Extra space to leave between lines, beyond ascent minus descent.
function TTFFont:lineGap()
    return self.hhea.lineGap
end

--- Where to draw an underline, relative to the baseline.
function TTFFont:underlinePosition()
    return self.post.underlinePosition
end

function TTFFont:underlineThickness()
    return self.post.underlineThickness
end

--- Degrees counter-clockwise from vertical, for the caret in an italic font.
function TTFFont:italicAngle()
    return self.post.italicAngle
end

--- Height of a capital letter. From OS/2 if there is one; otherwise the ascent,
--- which overstates it, but is what a font without OS/2 leaves available.
---
--- The two-step test is deliberate. Upstream is `os2 ? os2.capHeight : ascent`,
--- a ternary that asks only whether the table exists -- so a version 0 or 1
--- OS/2, which has no capHeight field at all, yields undefined rather than the
--- ascent. Writing this as `os2 and os2.capHeight or self:ascent()` would fall
--- through to the ascent instead and quietly disagree on every such font.
function TTFFont:capHeight()
    local os2 = self["OS/2"]
    if os2 then return os2.capHeight end
    return self:ascent()
end

--- Height of a lower-case x. Zero when unknown, as upstream -- and nil, not
--- zero, when the OS/2 table is too old to carry the field. See capHeight.
function TTFFont:xHeight()
    local os2 = self["OS/2"]
    if os2 then return os2.xHeight end
    return 0
end

function TTFFont:numGlyphs()
    return self.maxp.numGlyphs
end

--- The size of the font's coordinate grid: 1000 for PostScript outlines, and
--- usually 1024 or 2048 for TrueType. Every metric above is in these units, and
--- pdfkit divides by it to get the thousandths of an em a PDF wants.
function TTFFont:unitsPerEm()
    return self.head.unitsPerEm
end

--- The box that contains every glyph in the font.
---
--- Read from head rather than measured -- it is what the font declares, which is
--- what /FontBBox should carry even where a glyph pokes outside it.
function TTFFont:bbox()
    if not self._bbox then
        local head = self.head
        self._bbox = BBox.new(head.xMin, head.yMin, head.xMax, head.yMax)
    end

    return self._bbox
end

--------------------------------------------------------------------------------
-- Character mapping
--------------------------------------------------------------------------------

function TTFFont:_cmapProcessor()
    if not self._cmapProcessorCache then
        self._cmapProcessorCache = CmapProcessor.new(self.cmap)
    end

    return self._cmapProcessorCache
end

--- Every unicode code point the font has a glyph for.
function TTFFont:characterSet()
    if not self._characterSet then
        self._characterSet = self:_cmapProcessor():get_character_set()
    end

    return self._characterSet
end

function TTFFont:hasGlyphForCodePoint(codePoint)
    return self:_cmapProcessor():lookup(codePoint) ~= 0
end

--- The glyph a single code point maps to, with no substitutions applied -- there
--- is no surrounding text to apply them from.
function TTFFont:glyphForCodePoint(codePoint)
    return self:getGlyph(self:_cmapProcessor():lookup(codePoint), { codePoint })
end

--- One glyph per character in `string`, again with no shaping. Use layout() for
--- anything a reader will see; this is the cmap and nothing else.
---
--- The loop runs one step past the end of the string, because a character is
--- only emitted once the next one has been seen: a variation selector modifies
--- the character before it, and so has to be in hand before that character can
--- be turned into a glyph. `state` starts at -1 so the first step, which has
--- nothing behind it, emits nothing.
function TTFFont:glyphsForString(str)
    local glyphs = {}
    local cps = utils.code_points(str)
    local len = #cps
    local last = -1
    local state = -1

    for idx = 1, len + 1 do
        local code = 0
        local nextState = 0

        if idx <= len then
            code = cps[idx]

            -- 1 if this is a variation selector, 0 otherwise. The two ranges are
            -- the sixteen selectors in the BMP and the two hundred and forty in
            -- plane 14.
            if (0xFE00 <= code and code <= 0xFE0F)
                or (0xE0100 <= code and code <= 0xE01EF) then
                nextState = 1
            end
        end

        if state == 0 and nextState == 1 then
            -- A variation selector after an ordinary character: the pair maps
            -- through the format 14 subtable together.
            glyphs[#glyphs + 1] =
                self:getGlyph(self:_cmapProcessor():lookup(last, code), { last, code })
        elseif state == 0 and nextState == 0 then
            glyphs[#glyphs + 1] = self:glyphForCodePoint(last)
        end

        last = code
        state = nextState
    end

    return glyphs
end

--------------------------------------------------------------------------------
-- Layout
--------------------------------------------------------------------------------

-- Required at the point of use rather than at the top of the file. LayoutEngine
-- reaches back into the font, so requiring it here would be a cycle; and a font
-- that is only being measured or subsetted never loads the shaping code at all.
local function layout_engine(self)
    if not self._layoutEngineCache then
        local LayoutEngine = require("pdfkit.fontkit.layout.LayoutEngine")
        self._layoutEngineCache = LayoutEngine.new(self)
    end

    return self._layoutEngineCache
end

TTFFont._layoutEngine = layout_engine

--- Shape a string: the glyphs to draw and where to put each one.
---
--- This is the entry point pdfkit's embedded font calls, and the one that turns
--- a sequence of characters into something typographically correct rather than
--- merely character-by-character.
function TTFFont:layout(str, userFeatures, script, language, direction)
    return layout_engine(self):layout(str, userFeatures, script, language, direction)
end

--- Which strings would produce glyph `gid`. The reverse of layout, used to build
--- the ToUnicode map that makes a PDF's text selectable.
function TTFFont:stringsForGlyph(gid)
    return layout_engine(self):stringsForGlyph(gid)
end

--- The OpenType feature tags the font supports.
function TTFFont:availableFeatures(script, language)
    return layout_engine(self):getAvailableFeatures(script, language)
end

TTFFont.getAvailableFeatures = TTFFont.availableFeatures

--------------------------------------------------------------------------------
-- Glyphs
--------------------------------------------------------------------------------

--- The glyph for an id, without any colour or bitmap wrapper.
---
--- `characters` is remembered on the glyph rather than used: the caller knows
--- what it looked the glyph up for, and the ToUnicode map later needs to know.
--- Cached, so two lookups of the same id give the same object -- which is what
--- makes the memoised metrics on it worth having.
function TTFFont:_getBaseGlyph(glyph, characters)
    if not self._glyphs[glyph] then
        if self.directory.tables.glyf then
            self._glyphs[glyph] = TTFGlyph.new(glyph, characters or {}, self)
        elseif self.directory.tables["CFF "] then
            local CFFGlyph = require("pdfkit.fontkit.glyph.CFFGlyph")
            self._glyphs[glyph] = CFFGlyph.new(glyph, characters or {}, self)
        elseif self.directory.tables.CFF2 then
            -- CFF2 is the variable-font outline format. Not ported.
            return nil
        end
    end

    return self._glyphs[glyph]
end

--- The glyph for an id.
---
--- Upstream wraps the base glyph in SBIXGlyph for a font with Apple bitmap
--- strikes, or COLRGlyph for a layered colour font. Neither is ported, and both
--- only add a way to get at the colour artwork -- the outline, the advance and
--- the id underneath are the same, and those are what a PDF carries.
function TTFFont:getGlyph(glyph, characters)
    return self:_getBaseGlyph(glyph, characters)
end

--------------------------------------------------------------------------------
-- Subsetting
--------------------------------------------------------------------------------

--- A Subset to add glyphs to, and then encode as a new font file.
function TTFFont:createSubset()
    if self.directory.tables["CFF "] then
        local CFFSubset = require("pdfkit.fontkit.subset.CFFSubset")
        return CFFSubset.new(self)
    end

    local TTFSubset = require("pdfkit.fontkit.subset.TTFSubset")
    return TTFSubset.new(self)
end

--------------------------------------------------------------------------------
-- Variations
--------------------------------------------------------------------------------

-- Variable fonts are not ported: variationAxes, namedVariations, getVariation
-- and the GlyphVariationProcessor are all absent. The glyph code still tests
-- `font._variationProcessor` in the places upstream does, and finds nil, so the
-- variation branches are skipped rather than removed. Adding the processor later
-- is then a matter of defining that one field.

--- "Give me the font called X."
---
--- Every format implements this, and it means something different in each: to a
--- collection it picks one of the fonts in the file, and to a single font it
--- picks a named instance of a variable font. fontkit.create calls it whenever a
--- caller supplied a name, without knowing which kind it opened -- so a plain
--- .ttf opened with a family name lands here.
---
--- Upstream forwards to getVariation, which raises exactly this message on a
--- font with no axes. Since a font that *does* have axes is not ported either,
--- the message is unconditional.
function TTFFont:getFont()
    error("Variations require a font with the fvar, gvar and glyf, or CFF2 tables.")
end

return TTFFont
