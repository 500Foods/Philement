-- font/embedded.lua -- port of pdfkit/lib/font/embedded.js
--
-- A font that travels inside the PDF. Everything a reader needs is carried:
-- the outlines, the widths, and a table saying which characters each glyph
-- stands for so the text can still be selected and searched.
--
-- The PDF this produces is a Type0 (composite) font, and the shape is fixed:
--
--     Type0 font           the object the page's /Font resource names
--       DescendantFonts -> CIDFontType2       the widths and the descriptor
--                            FontDescriptor   the flags, the box, the metrics
--                              FontFile2   -> the subsetted TrueType file
--       ToUnicode       -> a CMap stream      glyph id back to characters
--
-- Text is written as glyph ids, not characters -- /Encoding is Identity-H, so
-- every two bytes in a string is a glyph number. That is what makes ligatures
-- and shaping expressible at all, and it is also why the ToUnicode map has to
-- exist: without it, selecting the text in a reader yields the glyph numbers.
--
-- The other half of this file is layout. A run of text is measured by handing it
-- to fontkit, which returns a glyph per character (after substitutions) and a
-- position for each. Those positions are in font units and get scaled to the
-- thousandths of an em a PDF works in, once, here.

local Font = require("pdfkit.font")
local object = require("pdfkit.object")

local EmbeddedFont = Font.subclass()

--- A number as exactly four lowercase hex digits.
---
--- Upstream pads to at least four and then takes the *last* four, so a value
--- above 0xffff silently loses its top bits rather than widening the field.
--- Nothing that reaches here should exceed a UTF-16 code unit -- the surrogate
--- splitting below is what guarantees it -- but the truncation is reproduced
--- because the range check it replaces would be a behaviour change.
local function toHex(num)
    return string.format("%04x", num & 0xFFFF)
end

--- JavaScript's `a || b`, for the numbers below.
---
--- Lua's `or` only falls through on nil and false; JavaScript's also falls
--- through on zero and NaN. Both extra cases are reachable here -- a font can
--- declare a cap height of zero, and an OS/2 table older than version 2 has no
--- cap height field at all -- and in both the fallback is what upstream writes.
local function js_or(value, fallback)
    if value == nil or value == 0 or value ~= value then
        return fallback
    end
    return value
end

--- Arithmetic on a metric the font may not have.
---
--- JavaScript multiplies `undefined` into NaN and carries on; Lua raises on nil.
--- Turning it into NaN here means a font missing the field fails where PDFKit
--- fails -- in the number writer, which refuses to emit NaN -- rather than at
--- the multiplication, and that anything merely measuring the text sees the same
--- NaN it would there.
local NaN = 0.0 / 0.0

local function defined(value)
    if value == nil then
        return NaN
    end
    return value
end

--------------------------------------------------------------------------------
-- Construction
--------------------------------------------------------------------------------

function EmbeddedFont.new(document, font, id)
    local self = setmetatable({}, EmbeddedFont)

    self.document = document
    self.font = font
    self.id = id
    self.subset = self.font:createSubset()

    -- Two parallel arrays indexed by *subset* glyph id: what each glyph is
    -- worth, and which characters it stands for. Both start with the entry for
    -- glyph 0, which every subset contains and which is .notdef.
    --
    -- Lua indexes from one and the PDF indexes from zero, so subset glyph `gid`
    -- lives at `[gid + 1]` throughout this file. Subset ids are handed out in
    -- order from zero, so both arrays stay dense and the shift is the only
    -- difference.
    self.unicode = { { 0 } }

    -- Note that this first width is in FONT units while every width added later
    -- is scaled to thousandths of an em. That is upstream, unchanged: the
    -- constructor takes the advance raw and encode() multiplies by the scale.
    -- On a 2048-unit font it makes /W claim .notdef is roughly twice as wide as
    -- it is -- harmless, since nothing is ever set in .notdef, and matching it
    -- is the difference between our /W array and PDFKit's.
    self.widths = { self.font:getGlyph(0):advanceWidth() }

    self.name = self.font:postscriptName()

    -- Font units to thousandths of an em. Every metric below crosses this line.
    self.scale = 1000 / self.font:unitsPerEm()
    self.ascender = self.font:ascent() * self.scale
    self.descender = self.font:descent() * self.scale
    self.xHeight = defined(self.font:xHeight()) * self.scale
    self.capHeight = defined(self.font:capHeight()) * self.scale
    self.lineGap = self.font:lineGap() * self.scale
    self.bbox = self.font:bbox()

    if document.options.fontLayoutCache ~= false then
        self.layoutCache = {}
    end

    return self
end

--------------------------------------------------------------------------------
-- Layout
--------------------------------------------------------------------------------

--- Shape a run of text and scale the result into PDF units.
---
--- The positions come back from fontkit in font units; each of the four numbers
--- on a position is scaled in place. `advanceWidth` is then written over the top
--- rather than scaled, because it is not one of the four -- it is the glyph's
--- own unadjusted width, which the text mixin subtracts from the positioned
--- advance to recover the kern.
---
--- In place, and the run is cached: a caller that means to adjust a position has
--- to copy it first, and mixins/text.lua does.
function EmbeddedFont:layoutRun(text, features)
    local run = self.font:layout(text, features)

    for i = 1, #run.positions do
        local position = run.positions[i]

        -- Upstream walks the position's own keys, which are exactly these four.
        position.xAdvance = position.xAdvance * self.scale
        position.yAdvance = position.yAdvance * self.scale
        position.xOffset = position.xOffset * self.scale
        position.yOffset = position.yOffset * self.scale

        position.advanceWidth = run.glyphs[i]:advanceWidth() * self.scale
    end

    return run
end

function EmbeddedFont:layoutCached(text)
    if not self.layoutCache then
        return self:layoutRun(text)
    end

    local cached = self.layoutCache[text]
    if cached then
        return cached
    end

    local run = self:layoutRun(text)
    self.layoutCache[text] = run
    return run
end

--- Lay out a string, one word at a time.
---
--- Splitting on spaces and tabs is a cache strategy, not a typographic one: a
--- paragraph repeats its words far more often than it repeats whole lines, so
--- shaping per word means the second "the" costs a table lookup. The delimiter
--- stays attached to the word before it, so the runs concatenate back to the
--- original string with nothing lost.
---
--- What it costs is shaping across the boundary: a ligature or kern pair
--- spanning a space will not be found, because the two sides are shaped
--- separately. For a space that is very nearly always right.
---
--- @param onlyWidth  skip collecting glyphs and positions and return the width
---                   alone -- what widthOfString needs, and it is the common
---                   case during line breaking
function EmbeddedFont:layout(text, features, onlyWidth)
    -- User features disable the cache: the cache is keyed by text alone, so a
    -- run shaped with different features must not be stored under it.
    if features then
        return self:layoutRun(text, features)
    end

    local glyphs = not onlyWidth and {} or nil
    local positions = not onlyWidth and {} or nil
    local advanceWidth = 0

    -- Byte positions, where upstream uses UTF-16 code units. The split points
    -- are the same either way: the two delimiters are ASCII, and no byte of a
    -- multi-byte UTF-8 sequence can be mistaken for one.
    local last = 0
    local index = 0
    local len = #text

    while index <= len do
        local needle = text:sub(index + 1, index + 1)

        if (index == len and last < index) or needle == " " or needle == "\t" then
            index = index + 1
            local run = self:layoutCached(text:sub(last + 1, index))

            if not onlyWidth then
                for _, glyph in ipairs(run.glyphs) do
                    glyphs[#glyphs + 1] = glyph
                end
                for _, position in ipairs(run.positions) do
                    positions[#positions + 1] = position
                end
            end

            advanceWidth = advanceWidth + run:advanceWidth()
            last = index
        else
            index = index + 1
        end
    end

    return { glyphs = glyphs, positions = positions, advanceWidth = advanceWidth }
end

--------------------------------------------------------------------------------
-- Encoding
--------------------------------------------------------------------------------

--- Turn text into what goes between the angle brackets of a PDF string.
---
--- Each glyph contributes four hex digits -- its id in the *subset*, not in the
--- source font, because Identity-H means the numbers in the string are indices
--- into the embedded file. Asking the subset for an id is also what pulls the
--- glyph into it, so this is where a document's font stops being the whole font.
---
--- Returns the hex pieces and the positions, as two values.
function EmbeddedFont:encode(text, features)
    local run = self:layout(text, features)
    local glyphs, positions = run.glyphs, run.positions

    local res = {}
    for i = 1, #glyphs do
        local glyph = glyphs[i]
        local gid = self.subset:includeGlyph(glyph.id)
        res[#res + 1] = toHex(gid)

        -- First sighting of this subset glyph records what it is worth and what
        -- it means. Later sightings are the same glyph, so nothing changes.
        if self.widths[gid + 1] == nil then
            self.widths[gid + 1] = glyph:advanceWidth() * self.scale
        end
        if self.unicode[gid + 1] == nil then
            self.unicode[gid + 1] = glyph.codePoints
        end
    end

    return res, positions
end

function EmbeddedFont:widthOfString(str, size, features)
    local width = self:layout(str, features, true).advanceWidth
    return width * (size / 1000)
end

--------------------------------------------------------------------------------
-- Embedding
--------------------------------------------------------------------------------

--- Write the font into the document.
---
--- Called once, at the end, from Font:finalize -- by which point every glyph the
--- document uses is in the subset and every width is known.
---
--- The order objects are created in here decides their object numbers, and the
--- order they are finished in decides where they land in the file. Both are
--- upstream's and neither is arbitrary as far as byte-identical output goes.
function EmbeddedFont:embed()
    -- A subset of a PostScript-outline font carries a CFF table rather than
    -- glyf, and is declared as a CIDFontType0 with a FontFile3. CFFSubset sets
    -- `cff`; a TrueType subset does not.
    local isCFF = self.subset.cff ~= nil

    local fontFile = self.document:ref()

    if isCFF then
        fontFile.data.Subtype = "CIDFontType0C"
    end

    -- Raw: the font file is bytes, and a trailing newline would corrupt it.
    fontFile:finish(self.subset:encode(), true)

    --------------------------------------------------------------------------
    -- The descriptor flags
    --------------------------------------------------------------------------
    --
    -- Six bits of the PDF's own idea of what a font is like. A reader uses them
    -- to pick a substitute when the embedded file will not load, which for an
    -- embedded subset is close to never -- so these are written correctly and
    -- almost never read.

    local os2 = self.font["OS/2"]
    -- The high byte of sFamilyClass is IBM's family classification. A signed
    -- shift upstream, so floor division rather than Lua's logical `>>`.
    local familyClass = ((os2 and os2.sFamilyClass) or 0) // 256

    local flags = 0
    if self.font.post.isFixedPitch ~= 0 then
        flags = flags | (1 << 0)   -- FixedPitch
    end
    if 1 <= familyClass and familyClass <= 7 then
        flags = flags | (1 << 1)   -- Serif
    end
    flags = flags | (1 << 2)       -- Symbolic: assume non-latin characters
    if familyClass == 10 then
        flags = flags | (1 << 3)   -- Script
    end
    if self.font.head.macStyle.italic then
        flags = flags | (1 << 6)   -- Italic
    end

    --------------------------------------------------------------------------
    -- The name
    --------------------------------------------------------------------------
    --
    -- A subsetted font must be named with a six-uppercase-letter tag and a plus
    -- sign, so that two documents embedding different subsets of one font are
    -- not taken for the same font by a reader that caches by name.
    --
    -- The tag is derived from the font's id within the document -- "F1", "F2".
    -- Characters 2 through 7 of that id are used, of which a two-character id
    -- has only one, and the rest come out of the `|| 73` fallback: 73 + 17 is
    -- 'Z'. So the first font in a document is BZZZZZ, the second CZZZZZ, and so
    -- on. Not the random tag the specification imagines, but it does distinguish
    -- subsets within a document, which is what a reader needs.
    local tag = {}
    for i = 2, 7 do
        local code = self.id:byte(i)
        if not code or code == 0 then
            code = 73
        end
        tag[#tag + 1] = string.char(code + 17)
    end
    tag = table.concat(tag)

    local postscriptName = self.font:postscriptName()
    local name = tag .. "+" .. (postscriptName and postscriptName:gsub(" ", "_") or "undefined")

    --------------------------------------------------------------------------
    -- The descriptor
    --------------------------------------------------------------------------

    local bbox = self.font:bbox()
    local descriptor = self.document:ref(object.dict({
        { "Type",     "FontDescriptor" },
        { "FontName", name },
        { "Flags",    flags },
        { "FontBBox", object.array({
            bbox.minX * self.scale,
            bbox.minY * self.scale,
            bbox.maxX * self.scale,
            bbox.maxY * self.scale,
        }) },
        { "ItalicAngle", self.font:italicAngle() },
        { "Ascent",      self.ascender },
        { "Descent",     self.descender },
        -- `||` upstream, so a font declaring a cap height of zero -- or an OS/2
        -- too old to have the field -- falls back to the ascent.
        { "CapHeight", js_or(self.font:capHeight(), self.font:ascent()) * self.scale },
        { "XHeight",   js_or(self.font:xHeight(), 0) * self.scale },
        -- The stem width. Upstream's comment is "not sure how to calculate
        -- this", and zero is what it writes.
        { "StemV", 0 },
    }))

    if isCFF then
        descriptor.data.FontFile3 = fontFile
    else
        descriptor.data.FontFile2 = fontFile
    end

    -- PDF/A-1 and PDF/UA want a CIDSet: a bitmap saying which CIDs the font
    -- actually defines, one bit per CID with the low CIDs in the high bits.
    -- Built from `widths`, so it covers the glyphs the *text* used -- not the
    -- extra components the subset pulled in for composites, which have widths
    -- in the embedded file but no entry here.
    if self.document.subset and self.document.subset == 1 then
        local maxCID = #self.widths - 1
        local bytes = {}
        for i = 1, math.ceil((maxCID + 1) / 8) do
            bytes[i] = 0
        end

        for cid = 0, maxCID do
            if self.widths[cid + 1] ~= nil then
                local i = (cid // 8) + 1
                bytes[i] = bytes[i] | (0x80 >> (cid % 8))
            end
        end

        for i = 1, #bytes do
            bytes[i] = string.char(bytes[i])
        end

        local CIDSetRef = self.document:ref()
        CIDSetRef:write(table.concat(bytes), true)
        CIDSetRef:finish()

        descriptor.data.CIDSet = CIDSetRef
    end

    descriptor:finish()

    --------------------------------------------------------------------------
    -- The descendant font
    --------------------------------------------------------------------------
    --
    -- Where the widths live. /W is the compact form the PDF spec allows:
    -- `[ first [w w w ...] ]` means the widths run consecutively from CID
    -- `first`, and since our CIDs are dense from zero the whole array is one
    -- such run.

    local descendantFontData = object.dict({
        { "Type",    "Font" },
        { "Subtype", "CIDFontType0" },
        { "BaseFont", name },
        { "CIDSystemInfo", object.dict({
            -- Strings, not names: (Adobe) rather than /Adobe.
            { "Registry",   object.str("Adobe") },
            { "Ordering",   object.str("Identity") },
            { "Supplement", 0 },
        }) },
        { "FontDescriptor", descriptor },
        { "W", object.array({ 0, object.array(self.widths) }) },
    })

    if not isCFF then
        -- Overwrites the Subtype set above without moving it -- the key keeps
        -- its position, and CIDToGIDMap is appended after /W.
        descendantFontData.Subtype = "CIDFontType2"
        -- The CID *is* the glyph id, which is what makes Identity-H work.
        descendantFontData.CIDToGIDMap = "Identity"
    end

    local descendantFont = self.document:ref(descendantFontData)
    descendantFont:finish()

    --------------------------------------------------------------------------
    -- The font dictionary
    --------------------------------------------------------------------------

    self.dictionary.data = object.dict({
        { "Type",     "Font" },
        { "Subtype",  "Type0" },
        { "BaseFont", name },
        -- Two bytes per glyph, big-endian, no character semantics at all.
        { "Encoding", "Identity-H" },
        { "DescendantFonts", object.array({ descendantFont }) },
        -- Called here, so the CMap object is created and written before the
        -- font dictionary that points at it.
        { "ToUnicode", self:toUnicodeCmap() },
    })

    return self.dictionary:finish()
end

--- Map the glyph ids in the PDF back to the characters they stand for.
---
--- Needed because Identity-H strings carry glyph numbers, which mean nothing
--- outside this font. Without this object, copying text out of the PDF gives
--- gibberish, and a search for a word in the document finds nothing.
---
--- One entry per subset glyph, and an entry may hold several characters: the
--- "ffi" ligature is one glyph and three code points, so selecting it has to
--- yield three characters.
function EmbeddedFont:toUnicodeCmap()
    local cmap = self.document:ref()

    local entries = {}
    for _, codePoints in ipairs(self.unicode) do
        local encoded = {}

        -- The CMap is defined over UTF-16, so anything outside the BMP goes in
        -- as a surrogate pair.
        for _, value in ipairs(codePoints) do
            if value > 0xFFFF then
                value = value - 0x10000
                encoded[#encoded + 1] = toHex(((value >> 10) & 0x3FF) | 0xD800)
                value = 0xDC00 | (value & 0x3FF)
            end

            encoded[#encoded + 1] = toHex(value)
        end

        entries[#entries + 1] = "<" .. table.concat(encoded, " ") .. ">"
    end

    -- bfrange takes at most 256 entries at a time, so the list is emitted in
    -- chunks, each declaring the glyph id range it covers.
    local chunkSize = 256
    local chunks = math.ceil(#entries / chunkSize)
    local ranges = {}

    for i = 0, chunks - 1 do
        local start = i * chunkSize
        local finish = math.min((i + 1) * chunkSize, #entries)

        ranges[#ranges + 1] = "<" .. toHex(start) .. "> <" .. toHex(finish - 1) .. "> ["
            .. table.concat(entries, " ", start + 1, finish) .. "]"
    end

    cmap:finish(table.concat({
        "/CIDInit /ProcSet findresource begin",
        "12 dict begin",
        "begincmap",
        "/CIDSystemInfo <<",
        "  /Registry (Adobe)",
        "  /Ordering (UCS)",
        "  /Supplement 0",
        ">> def",
        "/CMapName /Adobe-Identity-UCS def",
        "/CMapType 2 def",
        "1 begincodespacerange",
        "<0000><ffff>",
        "endcodespacerange",
        #ranges .. " beginbfrange",
        table.concat(ranges, "\n"),
        "endbfrange",
        "endcmap",
        "CMapName currentdict /CMap defineresource pop",
        "end",
        "end",
    }, "\n"))

    return cmap
end

return EmbeddedFont
