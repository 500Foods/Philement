-- fontkit/opentype/GlyphInfo.lua -- port of fontkit/src/opentype/GlyphInfo.js
--
-- What the shaper works on. A Glyph is a thing in the font; a GlyphInfo is one
-- position in one run of text, and it is mutable in ways a Glyph is not -- the
-- whole point of shaping is that this object's `id` changes as substitutions
-- fire.
--
-- Alongside the id it carries the bookkeeping the lookups need:
--
--   codePoints         the characters this glyph came from. Grows when glyphs
--                      are ligated, so a following mark can still tell what it
--                      is attached to.
--   features           which features are switched on for this glyph. Global
--                      features are on everywhere; `frac` is on only for the
--                      digits either side of a fraction slash.
--   isBase/isLigature/isMark   what kind of glyph this is, which drives the
--                      lookup flags that let a rule step over marks.
--   ligatureID         glyphs that were ligated together share one, so mark
--   ligatureComponent  attachment can find which component of the ligature a
--                      mark belongs to.
--
-- The `id` setter is where the class flags are computed, so they can never drift
-- out of step with the id. Lua has no property setters, so it is a method here
-- and every assignment goes through it -- see the note on `setId`.

local unicode = require("pdfkit.fontkit.unicode")
local OTProcessor = require("pdfkit.fontkit.opentype.OTProcessor")

local GlyphInfo = {}
GlyphInfo.__index = GlyphInfo

--- @param font        the font, for its GDEF table
--- @param id          the glyph id
--- @param codePoints  the characters this glyph came from
--- @param features    an array of feature tags, or a map of tag to boolean
function GlyphInfo.new(font, id, codePoints, features)
    local self = setmetatable({}, GlyphInfo)

    self._font = font
    self.codePoints = codePoints or {}

    -- Before the features, because the setter reads `codePoints` and writes
    -- `substituted`, both of which the lines below depend on.
    self:setId(id)

    self.features = {}
    if type(features) == "table" then
        if #features > 0 then
            for _, feature in ipairs(features) do
                self.features[feature] = true
            end
        else
            for tag, on in pairs(features) do
                self.features[tag] = on
            end
        end
    end

    self.ligatureID = nil
    self.ligatureComponent = nil
    self.isLigated = false
    self.cursiveAttachment = nil
    self.markAttachment = nil
    self.shaperInfo = nil

    -- Set true by `setId` a moment ago. The initial id is not a substitution,
    -- so it is cleared again -- upstream does exactly this, in this order.
    self.substituted = false

    self.isMultiplied = false

    return self
end

--- Change the glyph id, and with it everything that depends on the id.
---
--- In fontkit this is a property setter, so `glyph.id = x` runs it. Lua has no
--- such thing short of a `__newindex` metamethod on every GlyphInfo, which would
--- put a metatable lookup in front of every field read in the shaper for the
--- sake of one field. So it is an ordinary method, and the rule is that nothing
--- ever assigns `.id` directly -- the substitution lookups in GSUBProcessor all
--- call this.
function GlyphInfo:setId(id)
    self.id = id
    self.substituted = true

    local GDEF = self._font.GDEF
    if GDEF and GDEF.glyphClassDef then
        -- The font says what kind of glyph this is.
        local classID = OTProcessor.class_id(id, GDEF.glyphClassDef)
        self.isBase = classID == 1
        self.isLigature = classID == 2
        self.isMark = classID == 3
        self.markAttachmentType = GDEF.markAttachClassDef
            and OTProcessor.class_id(id, GDEF.markAttachClassDef)
            or 0
    else
        -- No GDEF: guess from the characters. Good enough for Latin, where a
        -- mark is a mark and a letter is a letter, and unreliable for scripts
        -- where a glyph's role depends on its context.
        self.isMark = #self.codePoints > 0
        for _, cp in ipairs(self.codePoints) do
            if not unicode.is_mark(cp) then
                self.isMark = false
                break
            end
        end
        self.isBase = not self.isMark
        self.isLigature = #self.codePoints > 1
        self.markAttachmentType = 0
    end
end

--- A fresh GlyphInfo with the same id, characters and features.
---
--- Used by multiple substitution, which turns one glyph into several and needs
--- somewhere to put the extras. Note what is *not* copied: ligature ids, mark
--- attachments and `isLigated` all start clean, because the copies are new
--- glyphs rather than the same glyph in another place.
function GlyphInfo:copy()
    return GlyphInfo.new(self._font, self.id, self.codePoints, self.features)
end

return GlyphInfo
