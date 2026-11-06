-- fontkit/layout/LayoutEngine.lua
--   -- port of fontkit/src/layout/LayoutEngine.js
--
-- Text in, glyphs and positions out. This is what font:layout() is.
--
-- The work is in two halves. *Substitution* rewrites the glyph list -- f and i
-- become the fi ligature, a Devanagari vowel sign moves in front of the
-- consonant it is written after. *Positioning* then decides where each glyph
-- goes and how far the pen moves afterwards: kerning, mark attachment, cursive
-- joining.
--
-- Neither half is done here. This file picks an engine to do them, applies the
-- fallbacks for when there is no engine or the engine declined, and owns the
-- order the whole thing happens in. Three engines exist upstream:
--
--   AATLayoutEngine   Apple's `morx`, where shaping is a state machine compiled
--                     into the font. Not ported -- no font pdfkit is likely to
--                     be handed carries it without also carrying GSUB/GPOS.
--   OTLayoutEngine    OpenType GSUB and GPOS. Ported separately; until the
--                     tables are registered the `font.GSUB` test below is nil
--                     and this stays unused.
--   (none)            What a plain TrueType font gets. Substitution does not
--                     happen; positioning is the kern table and the Unicode
--                     mark fallback, both of which are here.
--
-- A font with no layout tables at all still lays out. It gets one glyph per
-- character with its own advance width, kerned if there is a kern table, and
-- that is what the great majority of Latin text needs.

local KernProcessor = require("pdfkit.fontkit.layout.KernProcessor")
local UnicodeLayoutEngine = require("pdfkit.fontkit.layout.UnicodeLayoutEngine")
local GlyphRun = require("pdfkit.fontkit.layout.GlyphRun")
local GlyphPosition = require("pdfkit.fontkit.layout.GlyphPosition")
local Script = require("pdfkit.fontkit.layout.Script")
local reader = require("pdfkit.fontkit.reader")

local LayoutEngine = {}
LayoutEngine.__index = LayoutEngine

function LayoutEngine.new(font)
    local self = setmetatable({
        font = font,
        unicodeLayoutEngine = nil,
        kernProcessor = nil,
        engine = nil,
    }, LayoutEngine)

    -- AAT is tried before OpenType upstream, on the grounds that a `morx` font
    -- carries shaping rules for scripts fontkit has no shaper for. Not ported.
    --
    -- The test is on the directory rather than on `font.morx`, and that is the
    -- whole point of it: there is no morx decoder, so `font.morx` is nil for
    -- every font and the branch would never be taken. A Macintosh font -- the
    -- corpus has one, PalatinoBold, which carries both morx and kern -- would
    -- then quietly fall through to the kern table and lay out differently from
    -- fontkit rather than saying so.
    if font.directory.tables.morx then
        error("AAT layout (morx) is not supported")
    elseif font.GSUB or font.GPOS then
        local OTLayoutEngine = require("pdfkit.fontkit.opentype.OTLayoutEngine")
        self.engine = OTLayoutEngine.new(font)
    end

    return self
end

--- Lay out a string, or a list of glyphs that has already been mapped.
---
--- @param str       the text, or an array of Glyph objects
--- @param features  OpenType feature tags to apply -- may be omitted entirely,
---                  in which case the remaining arguments shuffle along
--- @param script    ISO 15924 tag; detected from the text when nil
--- @param language  language tag; nil for the script's default
--- @param direction "ltr" or "rtl"; nil for the script's own direction
function LayoutEngine:layout(str, features, script, language, direction)
    -- `features` is optional in the middle of the argument list, which is only
    -- resolvable by type: a string there is a script tag, and everything after
    -- it has been passed one position early.
    if type(features) == "string" then
        direction = language
        language = script
        script = features
        features = {}
    end

    local glyphs
    if type(str) == "string" then
        -- The script is what decides which shaper runs and which direction the
        -- text goes in, so it is worth guessing when not told.
        if script == nil then
            script = Script.forString(str)
        end

        glyphs = self.font:glyphsForString(str)
    else
        if script == nil then
            local codePoints = {}
            for _, glyph in ipairs(str) do
                for _, cp in ipairs(glyph.codePoints) do
                    codePoints[#codePoints + 1] = cp
                end
            end

            script = Script.forCodePoints(codePoints)
        end

        glyphs = str
    end

    local glyphRun = GlyphRun.new(glyphs, features, script, language, direction)

    -- Empty text still gets a run, with an empty position list rather than none:
    -- callers index the two lists in step and would otherwise have to test.
    if #glyphs == 0 then
        glyphRun.positions = {}
        return glyphRun
    end

    if self.engine and self.engine.setup then
        self.engine:setup(glyphRun)
    end

    self:substitute(glyphRun)
    self:position(glyphRun)

    self:hideDefaultIgnorables(glyphRun.glyphs, glyphRun.positions)

    if self.engine and self.engine.cleanup then
        self.engine:cleanup()
    end

    return glyphRun
end

--- Rewrite the glyph list. Nothing happens without an advanced engine.
function LayoutEngine:substitute(glyphRun)
    if self.engine and self.engine.substitute then
        self.engine:substitute(glyphRun)
    end
end

--- Fill in the run's positions.
---
--- Every glyph starts at its own advance width with no offset -- the position it
--- would have if nothing else applied. What follows only ever adjusts these in
--- place, so the run is well-formed at every point.
function LayoutEngine:position(glyphRun)
    glyphRun.positions = {}
    for i, glyph in ipairs(glyphRun.glyphs) do
        glyphRun.positions[i] = GlyphPosition.new(glyph:advanceWidth())
    end

    -- The advanced engine reports back which features it actually applied, which
    -- is how the two fallbacks below know whether they are needed. A nil means
    -- it did no positioning at all.
    local positioned = nil
    if self.engine and self.engine.position then
        positioned = self.engine:position(glyphRun)
    end

    -- No GPOS mark positioning: stack the marks by their Unicode properties.
    if not positioned and (not self.engine or self.engine.fallbackPosition) then
        if not self.unicodeLayoutEngine then
            self.unicodeLayoutEngine = UnicodeLayoutEngine.new(self.font)
        end

        self.unicodeLayoutEngine:positionGlyphs(glyphRun.glyphs, glyphRun.positions)
    end

    -- No GPOS kerning: use the old kern table, unless the caller asked for
    -- kerning off. `~= false` and not `== nil`, so that a feature map which
    -- names kern explicitly is honoured either way.
    if (not positioned or not positioned.kern)
        and glyphRun.features.kern ~= false
        and self.font.kern then
        if not self.kernProcessor then
            self.kernProcessor = KernProcessor.new(self.font)
        end

        self.kernProcessor:process(glyphRun.glyphs, glyphRun.positions)
        glyphRun.features.kern = true
    end
end

--- Replace the invisible formatting characters with a zero-width space.
---
--- Joiners, bidi controls, variation selectors and the like have done their work
--- by now -- they steered the shaper. Left in the run they would draw as .notdef
--- boxes, so each becomes a space with both advances zeroed.
function LayoutEngine:hideDefaultIgnorables(glyphs, positions)
    local space = self.font:glyphForCodePoint(0x20)
    for i = 1, #glyphs do
        -- A glyph built without characters has no code points. Upstream reads
        -- past the end of the array and tests `undefined`, which coerces to zero
        -- and answers no; here the absence is checked for.
        local ch = glyphs[i].codePoints[1]
        if ch and self:isDefaultIgnorable(ch) then
            glyphs[i] = space
            positions[i].xAdvance = 0
            positions[i].yAdvance = 0
        end
    end
end

--- Is this one of the characters that should never be drawn?
---
--- Unicode's Default_Ignorable_Code_Point property from DerivedCoreProperties,
--- minus U+115F, U+1160, U+3164 and U+FFA0 -- the Hangul and halfwidth fillers,
--- which are ignorable by the letter of the standard but are relied on to
--- occupy space. Harfbuzz and Uniscribe both make the same exception.
---
--- Written as a dispatch on the high bytes rather than a range list because it
--- is called once per glyph and answers no almost every time.
function LayoutEngine:isDefaultIgnorable(ch)
    local plane = ch >> 16
    if plane == 0 then
        local hi = ch >> 8
        if hi == 0x00 then
            return ch == 0x00AD                                 -- soft hyphen
        elseif hi == 0x03 then
            return ch == 0x034F                                 -- combining grapheme joiner
        elseif hi == 0x06 then
            return ch == 0x061C                                 -- Arabic letter mark
        elseif hi == 0x17 then
            return 0x17B4 <= ch and ch <= 0x17B5                -- Khmer inherent vowels
        elseif hi == 0x18 then
            return 0x180B <= ch and ch <= 0x180E                -- Mongolian selectors
        elseif hi == 0x20 then
            return (0x200B <= ch and ch <= 0x200F)              -- ZWSP..RLM
                or (0x202A <= ch and ch <= 0x202E)              -- bidi embedding
                or (0x2060 <= ch and ch <= 0x206F)              -- word joiner, deprecated formats
        elseif hi == 0xFE then
            return (0xFE00 <= ch and ch <= 0xFE0F)              -- variation selectors
                or ch == 0xFEFF                                 -- byte order mark
        elseif hi == 0xFF then
            return 0xFFF0 <= ch and ch <= 0xFFF8                -- unassigned specials
        else
            return false
        end
    else
        if plane == 0x01 then
            return (0x1BCA0 <= ch and ch <= 0x1BCA3)            -- shorthand format controls
                or (0x1D173 <= ch and ch <= 0x1D17A)            -- musical beams and phrases
        elseif plane == 0x0E then
            return 0xE0000 <= ch and ch <= 0xE0FFF              -- tags and variation selectors
        else
            return false
        end
    end
end

--- The feature tags available for a script and language.
---
--- The kern table is reported as a `kern` feature even though it is not an
--- OpenType feature at all, since to a caller asking "can this font kern?" the
--- distinction is not one they can act on.
function LayoutEngine:getAvailableFeatures(script, language)
    local features = {}

    if self.engine then
        for _, tag in ipairs(self.engine:getAvailableFeatures(script, language)) do
            features[#features + 1] = tag
        end
    end

    if self.font.kern then
        local found = false
        for _, tag in ipairs(features) do
            if tag == "kern" then
                found = true
                break
            end
        end

        if not found then
            features[#features + 1] = "kern"
        end
    end

    return features
end

--- Every string that would produce glyph `gid`.
---
--- The reverse of layout, and necessarily incomplete -- it inverts the cmap, and
--- with an advanced engine the ligature substitutions too, but not everything a
--- shaper can do. Used to build a PDF's ToUnicode map, where a glyph with no
--- known string is simply left out and becomes unselectable text.
function LayoutEngine:stringsForGlyph(gid)
    -- A set that remembers insertion order, which is what a JavaScript Set is
    -- and what the caller gets back.
    local result = {}
    local seen = {}

    local function add(s)
        if not seen[s] then
            seen[s] = true
            result[#result + 1] = s
        end
    end

    for _, codePoint in ipairs(self.font:_cmapProcessor():code_points_for_glyph(gid)) do
        add(reader.utf8_encode(codePoint))
    end

    if self.engine and self.engine.stringsForGlyph then
        for _, s in ipairs(self.engine:stringsForGlyph(gid)) do
            add(s)
        end
    end

    return result
end

return LayoutEngine
