-- fontkit/opentype/GSUBProcessor.lua
--   -- port of fontkit/src/opentype/GSUBProcessor.js
--
-- Applying a substitution. Everything about *finding* the rule is in
-- OTProcessor; this is the part that changes the glyph run.
--
-- Four of the eight types do real work here -- swap one glyph for another, one
-- for several, one of several alternates, or several for one. Types 5 and 6 are
-- contextual and hand straight back to the shared machinery, and type 7 is an
-- indirection. Type 8 is unreachable: the table decoder raises before a subtable
-- of that type is ever built, for the reason set out in tables/GSUB.lua.
--
-- Ligature substitution is much the longest case, and almost none of its length
-- is about the substitution. Replacing `f` `i` with `fi` is three lines. The rest
-- is ligature bookkeeping: marks that were skipped over during the match have to
-- be told which component of the new ligature they belong to, so that GPOS can
-- later put an accent over the `i` of an `fi` rather than over the whole thing.
-- The comments in that section are Harfbuzz's, kept because the cases they
-- describe are ones no amount of reading the code would suggest.

local OTProcessor = require("pdfkit.fontkit.opentype.OTProcessor")
local GlyphInfo = require("pdfkit.fontkit.opentype.GlyphInfo")

local GSUBProcessor = setmetatable({}, { __index = OTProcessor })
GSUBProcessor.__index = GSUBProcessor

function GSUBProcessor.new(font, table_)
    local self = setmetatable({}, GSUBProcessor)
    OTProcessor.init(self, font, table_)
    return self
end

--- JS `x || 1` for a ligature component, where 0 and absent both mean "first".
local function component_or_1(n)
    if n == nil or n == 0 then
        return 1
    end
    return n
end

function GSUBProcessor:applyLookup(lookupType, table_)
    if lookupType == 1 then
        -- Single Substitution
        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        local glyph = self.glyphIterator:cur()
        if table_.version == 1 then
            -- Every covered glyph moves by the same amount. The mask matters:
            -- the delta is signed and the sum wraps, which is how a font shifts
            -- a block of glyphs near the top of the id space.
            glyph:setId((glyph.id + table_.deltaGlyphID) & 0xffff)
        elseif table_.version == 2 then
            glyph:setId(table_.substitute:at(index))
        end

        return true

    elseif lookupType == 2 then
        -- Multiple Substitution
        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        local sequence = table_.sequences:at(index)

        if #sequence == 0 then
            -- If the sequence length is zero, delete the glyph.
            -- The OpenType spec disallows this, but seems like Harfbuzz and
            -- Uniscribe allow it.
            table.remove(self.glyphs, self.glyphIterator.index)
            return true
        end

        local curGlyph = self.glyphIterator:cur()
        curGlyph:setId(sequence[1])
        curGlyph.ligatureComponent = 0

        local features = curGlyph.features

        -- The rest of the sequence becomes new glyphs, inserted after this one.
        -- They inherit the features and the shaper's state, so a later lookup
        -- treats them as part of the same run rather than as strays.
        local replacement = {}
        for i = 2, #sequence do
            local glyph = GlyphInfo.new(self.font, sequence[i], nil, features)
            glyph.shaperInfo = curGlyph.shaperInfo
            glyph.isLigated = curGlyph.isLigated
            glyph.ligatureComponent = i - 1
            glyph.substituted = true
            glyph.isMultiplied = true
            replacement[#replacement + 1] = glyph
        end

        for i = #replacement, 1, -1 do
            table.insert(self.glyphs, self.glyphIterator.index + 1, replacement[i])
        end

        return true

    elseif lookupType == 3 then
        -- Alternate Substitution
        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        -- Which alternate to take is the caller's choice in principle; fontkit
        -- has never exposed it and always takes the first, as noted by its own
        -- `TODO` here.
        local USER_INDEX = 1
        self.glyphIterator:cur():setId(table_.alternateSet:at(index)[USER_INDEX])
        return true

    elseif lookupType == 4 then
        -- Ligature Substitution
        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        for _, ligature in ipairs(table_.ligatureSets:at(index)) do
            -- The indices, not just a yes or no: the components may not be
            -- adjacent, and the glyphs between them have to be dealt with below.
            local matched = self:sequenceMatchIndices(1, ligature.components)
            if matched then
                local curGlyph = self.glyphIterator:cur()

                -- Concatenate all of the characters the new ligature will represent
                local characters = {}
                for _, cp in ipairs(curGlyph.codePoints) do
                    characters[#characters + 1] = cp
                end
                for _, i in ipairs(matched) do
                    for _, cp in ipairs(self.glyphs[i].codePoints) do
                        characters[#characters + 1] = cp
                    end
                end

                -- Create the replacement ligature glyph
                local ligatureGlyph = GlyphInfo.new(
                    self.font, ligature.glyph, characters, curGlyph.features)
                ligatureGlyph.shaperInfo = curGlyph.shaperInfo
                ligatureGlyph.isLigated = true
                ligatureGlyph.substituted = true

                -- From Harfbuzz:
                -- - If it *is* a mark ligature, we don't allocate a new ligature id, and leave
                --   the ligature to keep its old ligature id.  This will allow it to attach to
                --   a base ligature in GPOS.  Eg. if the sequence is: LAM,LAM,SHADDA,FATHA,HEH,
                --   and LAM,LAM,HEH for a ligature, they will leave SHADDA and FATHA with a
                --   ligature id and component value of 2.  Then if SHADDA,FATHA form a ligature
                --   later, we don't want them to lose their ligature id/component, otherwise
                --   GPOS will fail to correctly position the mark ligature on top of the
                --   LAM,LAM,HEH ligature. See https://bugzilla.gnome.org/show_bug.cgi?id=676343
                --
                -- - If a ligature is formed of components that some of which are also ligatures
                --   themselves, and those ligature components had marks attached to *their*
                --   components, we have to attach the marks to the new ligature component
                --   positions!  Now *that*'s tricky!  And these marks may be following the
                --   last component of the whole sequence, so we should loop forward looking
                --   for them and update them.
                --
                --   Eg. the sequence is LAM,LAM,SHADDA,FATHA,HEH, and the font first forms a
                --   'calt' ligature of LAM,HEH, leaving the SHADDA and FATHA with a ligature
                --   id and component == 1.  Now, during 'liga', the LAM and the LAM-HEH ligature
                --   form a LAM-LAM-HEH ligature.  We need to reassign the SHADDA and FATHA to
                --   the new ligature with a component value of 2.
                --
                --   This in fact happened to a font...  See https://bugzilla.gnome.org/show_bug.cgi?id=437633
                local isMarkLigature = curGlyph.isMark
                for i = 1, #matched do
                    if not isMarkLigature then
                        break
                    end
                    isMarkLigature = self.glyphs[matched[i]].isMark
                end

                if isMarkLigature then
                    ligatureGlyph.ligatureID = nil
                else
                    ligatureGlyph.ligatureID = self.ligatureID
                    self.ligatureID = self.ligatureID + 1
                end

                local lastLigID = curGlyph.ligatureID
                local lastNumComps = #curGlyph.codePoints
                local curComps = lastNumComps
                local idx = self.glyphIterator.index + 1

                -- Set ligatureID and ligatureComponent on glyphs that were skipped in the matched sequence.
                -- This allows GPOS to attach marks to the correct ligature components.
                for _, matchIndex in ipairs(matched) do
                    -- Don't assign new ligature components for mark ligatures (see above)
                    if isMarkLigature then
                        idx = matchIndex
                    else
                        while idx < matchIndex do
                            local ligatureComponent = curComps - lastNumComps
                                + math.min(component_or_1(self.glyphs[idx].ligatureComponent), lastNumComps)
                            self.glyphs[idx].ligatureID = ligatureGlyph.ligatureID
                            self.glyphs[idx].ligatureComponent = ligatureComponent
                            idx = idx + 1
                        end
                    end

                    lastLigID = self.glyphs[idx].ligatureID
                    lastNumComps = #self.glyphs[idx].codePoints
                    curComps = curComps + lastNumComps
                    idx = idx + 1 -- skip base glyph
                end

                -- Adjust ligature components for any marks following
                if lastLigID and not isMarkLigature then
                    for i = idx, #self.glyphs do
                        if self.glyphs[i].ligatureID ~= lastLigID then
                            break
                        end
                        local ligatureComponent = curComps - lastNumComps
                            + math.min(component_or_1(self.glyphs[i].ligatureComponent), lastNumComps)
                        self.glyphs[i].ligatureComponent = ligatureComponent
                    end
                end

                -- Delete the matched glyphs, and replace the current glyph with
                -- the ligature glyph. Back to front, so the earlier indices are
                -- still valid as the later ones are removed.
                for i = #matched, 1, -1 do
                    table.remove(self.glyphs, matched[i])
                end

                self.glyphs[self.glyphIterator.index] = ligatureGlyph
                return true
            end
        end

        return false

    elseif lookupType == 5 then
        -- Contextual Substitution
        return self:applyContext(table_)

    elseif lookupType == 6 then
        -- Chaining Contextual Substitution
        return self:applyChainingContext(table_)

    elseif lookupType == 7 then
        -- Extension Substitution
        return self:applyLookup(table_.lookupType, table_.extension)
    end

    error(string.format("GSUB lookupType %s is not supported", tostring(lookupType)))
end

return GSUBProcessor
