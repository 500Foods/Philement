-- fontkit/opentype/GPOSProcessor.lua
--   -- port of fontkit/src/opentype/GPOSProcessor.js
--
-- Applying a positioning rule. Unlike GSUB, nothing here changes which glyphs
-- are in the run -- only where they sit and how far the pen moves afterwards.
--
-- The nine types are three ideas:
--
--   1, 2   shift a glyph, or a pair of glyphs. Type 2 is kerning, and it is the
--          overwhelming majority of what any Latin font does with GPOS.
--   3      cursive attachment: chain each glyph's exit point onto the next
--          one's entry point, which is how a joining script stays joined.
--   4, 5, 6  anchor attachment: put a mark where the base glyph says it goes.
--          Base, ligature component, or another mark.
--
-- Types 7 and 8 are the shared contextual machinery and 9 is an indirection, as
-- in GSUB.
--
-- Two passes run after all the lookups, in `applyFeatures`. Attachments are
-- recorded during the lookups as an index -- "this glyph hangs off that one" --
-- and resolved afterwards, because the glyph being attached to may itself move
-- later in the same pass. Resolving eagerly would attach to a position that no
-- longer exists by the time the run is finished.

local OTProcessor = require("pdfkit.fontkit.opentype.OTProcessor")

local GPOSProcessor = setmetatable({}, { __index = OTProcessor })
GPOSProcessor.__index = GPOSProcessor

function GPOSProcessor.new(font, table_)
    local self = setmetatable({}, GPOSProcessor)
    OTProcessor.init(self, font, table_)
    return self
end

--------------------------------------------------------------------------------
-- Value records and anchors
--------------------------------------------------------------------------------

--- Add a value record's adjustments to a glyph's position.
---
--- `sequenceIndex` is counted the iterator's way, so 0 is the current glyph and
--- 1 is the next one this lookup can see -- which may not be the next one in the
--- run, if there is an ignored mark between them.
---
--- The device table fields are read and not applied. They mean two different
--- things depending on the font: a hinting adjustment per pixel size, which is
--- irrelevant to a PDF and which fontkit also ignores, or a variation delta,
--- which needs a variation processor this port does not have.
function GPOSProcessor:applyPositionValue(sequenceIndex, value)
    local position = self.positions[self.glyphIterator:peekIndex(sequenceIndex)]

    if value.xAdvance ~= nil then
        position.xAdvance = position.xAdvance + value.xAdvance
    end

    if value.yAdvance ~= nil then
        position.yAdvance = position.yAdvance + value.yAdvance
    end

    if value.xPlacement ~= nil then
        position.xOffset = position.xOffset + value.xPlacement
    end

    if value.yPlacement ~= nil then
        position.yOffset = position.yOffset + value.yPlacement
    end
end

--- An anchor's coordinates.
---
--- Upstream notes two things it does not do -- contour points, where the anchor
--- follows a point on the outline rather than a fixed coordinate, and device
--- tables. Neither is done here either.
function GPOSProcessor:getAnchor(anchor)
    return { x = anchor.xCoordinate, y = anchor.yCoordinate }
end

--- Put the current glyph's mark anchor onto a base glyph's anchor.
---
--- The offset is recorded relative to the base glyph, and which base it was is
--- remembered on the glyph; `fixMarkAttachment` turns that pair into an absolute
--- offset once every lookup has run.
function GPOSProcessor:applyAnchor(markRecord, baseAnchor, baseGlyphIndex)
    local baseCoords = self:getAnchor(baseAnchor)
    local markCoords = self:getAnchor(markRecord.markAnchor)

    local markPos = self.positions[self.glyphIterator.index]

    markPos.xOffset = baseCoords.x - markCoords.x
    markPos.yOffset = baseCoords.y - markCoords.y
    self.glyphIterator:cur().markAttachment = baseGlyphIndex
end

--------------------------------------------------------------------------------
-- The lookup types
--------------------------------------------------------------------------------

function GPOSProcessor:applyLookup(lookupType, table_)
    if lookupType == 1 then
        -- Single positioning value
        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        if table_.version == 1 then
            self:applyPositionValue(0, table_.value)
        elseif table_.version == 2 then
            self:applyPositionValue(0, table_.values:at(index))
        end

        return true

    elseif lookupType == 2 then
        -- Pair Adjustment Positioning -- kerning
        local nextGlyph = self.glyphIterator:peek()
        if not nextGlyph then
            return false
        end

        local index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        if table_.version == 1 then
            -- Adjustments for glyph pairs, listed one by one.
            local set = table_.pairSets:at(index)

            for _, pair in ipairs(set) do
                if pair.secondGlyph == nextGlyph.id then
                    self:applyPositionValue(0, pair.value1)
                    self:applyPositionValue(1, pair.value2)
                    return true
                end
            end

            return false
        elseif table_.version == 2 then
            -- Class pair adjustment: a matrix indexed by the two glyphs'
            -- classes. Both lazy arrays are the font's own indexing, so the
            -- class numbers go in unchanged.
            local class1 = OTProcessor.class_id(self.glyphIterator:cur().id, table_.classDef1)
            local class2 = OTProcessor.class_id(nextGlyph.id, table_.classDef2)

            local pair = table_.classRecords:get(class1):get(class2)
            self:applyPositionValue(0, pair.value1)
            self:applyPositionValue(1, pair.value2)
            return true
        end

        return false

    elseif lookupType == 3 then
        -- Cursive Attachment Positioning
        local nextIndex = self.glyphIterator:peekIndex()
        local nextGlyph = self.glyphs[nextIndex]
        if not nextGlyph then
            return false
        end

        local curRecord = table_.entryExitRecords[self:coverageIndex(table_.coverage)]
        if not curRecord or not curRecord.exitAnchor then
            return false
        end

        local nextRecord = table_.entryExitRecords[
            self:coverageIndex(table_.coverage, nextGlyph.id)]
        if not nextRecord or not nextRecord.entryAnchor then
            return false
        end

        local entry = self:getAnchor(nextRecord.entryAnchor)
        local exit = self:getAnchor(curRecord.exitAnchor)

        local cur = self.positions[self.glyphIterator.index]
        local next_ = self.positions[nextIndex]
        local d

        if self.direction == "ltr" then
            -- The pen stops where this glyph's exit point is, and the next
            -- glyph is pulled back so its entry point lands there.
            cur.xAdvance = exit.x + cur.xOffset

            d = entry.x + next_.xOffset
            next_.xAdvance = next_.xAdvance - d
            next_.xOffset = next_.xOffset - d
        elseif self.direction == "rtl" then
            d = exit.x + cur.xOffset
            cur.xAdvance = cur.xAdvance - d
            cur.xOffset = cur.xOffset - d
            next_.xAdvance = entry.x + next_.xOffset
        end

        -- The vertical join is deferred like a mark attachment, because the
        -- glyph joined to may itself be joined to another further along.
        if self.glyphIterator.flags.rightToLeft then
            self.glyphIterator:cur().cursiveAttachment = nextIndex
            cur.yOffset = entry.y - exit.y
        else
            nextGlyph.cursiveAttachment = self.glyphIterator.index
            cur.yOffset = exit.y - entry.y
        end

        return true

    elseif lookupType == 4 then
        -- Mark to base positioning
        local markIndex = self:coverageIndex(table_.markCoverage)
        if markIndex == nil then
            return false
        end

        -- search backward for a base glyph
        local baseGlyphIndex = self.glyphIterator.index
        repeat
            baseGlyphIndex = baseGlyphIndex - 1
        until baseGlyphIndex < 1
            or not (self.glyphs[baseGlyphIndex].isMark
                or (self.glyphs[baseGlyphIndex].ligatureComponent or 0) > 0)

        if baseGlyphIndex < 1 then
            return false
        end

        local baseIndex = self:coverageIndex(
            table_.baseCoverage, self.glyphs[baseGlyphIndex].id)
        if baseIndex == nil then
            return false
        end

        local markRecord = table_.markArray[markIndex]
        -- The mark's class picks which of the base's anchors to use, and it is
        -- the font's own numbering, so it starts at zero.
        local baseAnchor = table_.baseArray[baseIndex][markRecord.class + 1]
        self:applyAnchor(markRecord, baseAnchor, baseGlyphIndex)
        return true

    elseif lookupType == 5 then
        -- Mark to ligature positioning
        local markIndex = self:coverageIndex(table_.markCoverage)
        if markIndex == nil then
            return false
        end

        -- search backward for a base glyph
        local baseGlyphIndex = self.glyphIterator.index
        repeat
            baseGlyphIndex = baseGlyphIndex - 1
        until baseGlyphIndex < 1 or not self.glyphs[baseGlyphIndex].isMark

        if baseGlyphIndex < 1 then
            return false
        end

        local ligIndex = self:coverageIndex(
            table_.ligatureCoverage, self.glyphs[baseGlyphIndex].id)
        if ligIndex == nil then
            return false
        end

        local ligAttach = table_.ligatureArray[ligIndex]
        local markGlyph = self.glyphIterator:cur()
        local ligGlyph = self.glyphs[baseGlyphIndex]

        -- Which letter of the ligature this mark belongs over. The ligature
        -- bookkeeping GSUB did is what makes this answerable; without a matching
        -- ligature id the mark goes over the last component.
        local compIndex
        if ligGlyph.ligatureID
            and ligGlyph.ligatureID == markGlyph.ligatureID
            and (markGlyph.ligatureComponent or 0) > 0 then
            compIndex = math.min(markGlyph.ligatureComponent, #ligGlyph.codePoints)
        else
            compIndex = #ligGlyph.codePoints
        end

        local markRecord = table_.markArray[markIndex]
        local baseAnchor = ligAttach[compIndex][markRecord.class + 1]
        self:applyAnchor(markRecord, baseAnchor, baseGlyphIndex)
        return true

    elseif lookupType == 6 then
        -- Mark to mark positioning
        local mark1Index = self:coverageIndex(table_.mark1Coverage)
        if mark1Index == nil then
            return false
        end

        -- get the previous mark to attach to
        local prevIndex = self.glyphIterator:peekIndex(-1)
        local prev = self.glyphs[prevIndex]
        if not prev or not prev.isMark then
            return false
        end

        local cur = self.glyphIterator:cur()

        -- The following logic was borrowed from Harfbuzz
        local good = false
        if cur.ligatureID == prev.ligatureID then
            if not cur.ligatureID then
                -- Marks belonging to the same base
                good = true
            elseif cur.ligatureComponent == prev.ligatureComponent then
                -- Marks belonging to the same ligature component
                good = true
            end
        else
            -- If ligature ids don't match, it may be the case that one of the marks
            -- itself is a ligature, in which case match.
            if (cur.ligatureID and not cur.ligatureComponent)
                or (prev.ligatureID and not prev.ligatureComponent) then
                good = true
            end
        end

        if not good then
            return false
        end

        local mark2Index = self:coverageIndex(table_.mark2Coverage, prev.id)
        if mark2Index == nil then
            return false
        end

        local markRecord = table_.mark1Array[mark1Index]
        local baseAnchor = table_.mark2Array[mark2Index][markRecord.class + 1]
        self:applyAnchor(markRecord, baseAnchor, prevIndex)
        return true

    elseif lookupType == 7 then
        -- Contextual positioning
        return self:applyContext(table_)

    elseif lookupType == 8 then
        -- Chaining contextual positioning
        return self:applyChainingContext(table_)

    elseif lookupType == 9 then
        -- Extension positioning
        return self:applyLookup(table_.lookupType, table_.extension)
    end

    error(string.format("Unsupported GPOS table: %s", tostring(lookupType)))
end

--------------------------------------------------------------------------------
-- Resolving deferred attachments
--------------------------------------------------------------------------------

function GPOSProcessor:applyFeatures(userFeatures, glyphs, advances)
    OTProcessor.applyFeatures(self, userFeatures, glyphs, advances)

    for i = 1, #self.glyphs do
        self:fixCursiveAttachment(i)
    end

    self:fixMarkAttachment()
end

--- Roll a chain of cursive joins up into absolute vertical offsets.
---
--- Recursive because a glyph joined to a glyph that is itself joined has to wait
--- for the far end of the chain to be resolved first. Clearing the attachment
--- before recursing is what stops a cycle spinning; it also means each glyph is
--- resolved exactly once however many times it is reached.
function GPOSProcessor:fixCursiveAttachment(i)
    local glyph = self.glyphs[i]
    if glyph.cursiveAttachment ~= nil then
        local j = glyph.cursiveAttachment

        glyph.cursiveAttachment = nil
        self:fixCursiveAttachment(j)

        self.positions[i].yOffset = self.positions[i].yOffset + self.positions[j].yOffset
    end
end

--- Turn each mark's offset-relative-to-its-base into an absolute offset.
---
--- Two corrections. The base's own offset is added, so a mark on a mark on a
--- letter ends up in the right place. And the advances between the base and the
--- mark are subtracted, because the pen has moved on since the base was drawn
--- and the mark's offset is measured from where the pen is now.
function GPOSProcessor:fixMarkAttachment()
    for i = 1, #self.glyphs do
        local glyph = self.glyphs[i]
        if glyph.markAttachment ~= nil then
            local j = glyph.markAttachment

            self.positions[i].xOffset = self.positions[i].xOffset + self.positions[j].xOffset
            self.positions[i].yOffset = self.positions[i].yOffset + self.positions[j].yOffset

            if self.direction == "ltr" then
                for k = j, i - 1 do
                    self.positions[i].xOffset = self.positions[i].xOffset - self.positions[k].xAdvance
                    self.positions[i].yOffset = self.positions[i].yOffset - self.positions[k].yAdvance
                end
            else
                for k = j + 1, i do
                    self.positions[i].xOffset = self.positions[i].xOffset + self.positions[k].xAdvance
                    self.positions[i].yOffset = self.positions[i].yOffset + self.positions[k].yAdvance
                end
            end
        end
    end
end

return GPOSProcessor
