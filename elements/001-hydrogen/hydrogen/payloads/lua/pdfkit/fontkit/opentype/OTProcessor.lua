-- fontkit/opentype/OTProcessor.lua
--   -- port of fontkit/src/opentype/OTProcessor.js
--
-- Everything GSUB and GPOS processing have in common, which is most of it. The
-- two subclasses differ only in what a lookup *does* once it has matched; how a
-- script is chosen, how features become lookups, how a lookup finds its glyphs
-- and how a contextual rule chains into other lookups all live here.
--
-- The shape of the work:
--
--   selectScript      pick a script and language, and from them build the map
--                     of feature tag -> feature that the rest of it reads.
--   lookupsForFeatures  turn a list of requested feature tags into the lookups
--                     that implement them, in font order.
--   applyLookups      walk the glyph run once per lookup, offering each glyph
--                     to each of the lookup's subtables until one matches.
--
-- The rest is matching machinery: coverage tables, class definitions, and the
-- sequence matchers the contextual rules are built out of.
--
--------------------------------------------------------------------------------
-- Two numbering notes
--------------------------------------------------------------------------------
--
-- **`coverageIndex` returns a one-based index, or nil.** Upstream returns -1 for
-- "not covered". A -1 that escapes a check is a silent read of the last element
-- in JS and a silent nil in Lua; `nil` used as an index is at least visible at
-- the point of use, and it composes with the one-based arrays the coverage index
-- is immediately fed into.
--
-- **`getClassID` returns the class number, unchanged.** Class 0 is a real class
-- -- "every glyph not otherwise mentioned" -- so there is no absent value to map
-- to nil, and any array indexed *by a class* needs an explicit `+ 1`.

local GlyphIterator = require("pdfkit.fontkit.opentype.GlyphIterator")
local Script = require("pdfkit.fontkit.layout.Script")

-- Tried in order when the requested script is not in the font. `DFLT` is the
-- spec's spelling, `dflt` is what a good many fonts actually wrote, and `latn`
-- is the fallback of last resort because a font with any layout at all usually
-- has it.
local DEFAULT_SCRIPTS = { "DFLT", "dflt", "latn" }

--- Is `key` present in `t`, even if its value is false?
---
--- Stands in for JS `in`, which the feature checks below need rather than a
--- truth test: a user who passed `{liga = false}` has a `liga` key, and upstream
--- treats that as "this feature applies to this glyph". What stops it being
--- applied is that ShapingPlan dropped it from the stages, not this test.
local function has(t, key)
    return t[key] ~= nil
end

local OTProcessor = {}
OTProcessor.__index = OTProcessor

function OTProcessor.new(font, table_)
    local self = setmetatable({}, OTProcessor)
    OTProcessor.init(self, font, table_)
    return self
end

--- The constructor body, factored out so the two subclasses can call it.
function OTProcessor.init(self, font, table_)
    self.font = font
    self.table = table_

    self.script = nil
    self.scriptTag = nil

    self.language = nil
    self.languageTag = nil

    self.features = {}
    self.lookups = {}

    -- Feature variations are for variable fonts, which this port does not
    -- support; upstream computes an index into the variations table here and
    -- everything downstream is guarded by it being -1. Held as nil for the
    -- same purpose.
    self.variationsIndex = nil

    -- initialize to default script + language
    self:selectScript()

    -- current context (set by applyFeatures)
    self.glyphs = {}
    self.positions = {}     -- only used by GPOS
    self.ligatureID = 1
    self.currentFeature = nil
end

--------------------------------------------------------------------------------
-- Script and language selection
--------------------------------------------------------------------------------

--- Find a script record by tag, or by any of several tags.
---
--- A script can have two OpenType tags -- `deva` and `dev2` for Devanagari, the
--- second marking a font built to the newer shaping rules -- so `script` may be
--- a list, tried in order.
function OTProcessor:findScript(script)
    if not self.table.scriptList then
        return nil
    end

    if type(script) ~= "table" then
        script = { script }
    end

    for _, s in ipairs(script) do
        for _, entry in ipairs(self.table.scriptList) do
            if entry.tag == s then
                return entry
            end
        end
    end

    return nil
end

--- Choose a script, a language and a direction, and build the feature map.
---
--- Returns the tag actually selected, which may not be the one asked for: a
--- font with no rules for the script falls back to a default, and the caller
--- needs to know that happened.
function OTProcessor:selectScript(script, language, direction)
    local changed = false
    local entry

    if not self.script or script ~= self.scriptTag then
        entry = self:findScript(script)
        if not entry then
            entry = self:findScript(DEFAULT_SCRIPTS)
        end

        -- No script at all, not even a default. Nothing can be applied, and the
        -- previously selected script -- if any -- stays.
        if not entry then
            return self.scriptTag
        end

        self.scriptTag = entry.tag
        self.script = entry.script
        self.language = nil
        self.languageTag = nil
        changed = true
    end

    if not direction or direction ~= self.direction then
        self.direction = direction or Script.direction(script)
    end

    -- Language tags are four bytes, space-padded. `DEU` in the font is `DEU `.
    if language and #language < 4 then
        language = language .. string.rep(" ", 4 - #language)
    end

    if not language or language ~= self.languageTag then
        self.language = nil

        for _, lang in ipairs(self.script.langSysRecords) do
            if lang.tag == language then
                self.language = lang.langSys
                self.languageTag = lang.tag
                break
            end
        end

        if not self.language then
            self.language = self.script.defaultLangSys
            self.languageTag = nil
        end

        changed = true
    end

    -- Build a feature lookup table
    if changed then
        self.features = {}
        if self.language then
            for _, featureIndex in ipairs(self.language.featureIndexes) do
                -- The index is the font's, so zero-based.
                local record = self.table.featureList[featureIndex + 1]
                self.features[record.tag] = record.feature
            end
        end
    end

    return self.scriptTag
end

--------------------------------------------------------------------------------
-- Features to lookups
--------------------------------------------------------------------------------

--- The lookups implementing `userFeatures`, in the order the font lists them.
---
--- Order is the font's, not the caller's: a feature's number in the LookupList
--- is its priority, and two features sharing a lookup must not run it twice in
--- different places. `exclude` drops lookups already applied in an earlier pass.
---
--- The sort has to be stable, because two lookups with the same index -- which
--- happens when two requested features name the same lookup -- must stay in the
--- order they were collected. `table.sort` is not stable, so position is used to
--- break ties, which is the same thing done explicitly.
function OTProcessor:lookupsForFeatures(userFeatures, exclude)
    userFeatures = userFeatures or {}
    local lookups = {}

    for _, tag in ipairs(userFeatures) do
        local feature = self.features[tag]
        if feature then
            for _, lookupIndex in ipairs(feature.lookupListIndexes) do
                local excluded = false
                if exclude then
                    for _, e in ipairs(exclude) do
                        if e == lookupIndex then
                            excluded = true
                            break
                        end
                    end
                end

                if not excluded then
                    lookups[#lookups + 1] = {
                        feature = tag,
                        index = lookupIndex,
                        -- Zero-based, straight from the font.
                        lookup = self.table.lookupList:get(lookupIndex),
                        order = #lookups + 1,
                    }
                end
            end
        end
    end

    table.sort(lookups, function(a, b)
        if a.index ~= b.index then
            return a.index < b.index
        end
        return a.order < b.order
    end)

    return lookups
end

--------------------------------------------------------------------------------
-- Applying lookups
--------------------------------------------------------------------------------

function OTProcessor:applyFeatures(userFeatures, glyphs, advances)
    local lookups = self:lookupsForFeatures(userFeatures)
    self:applyLookups(lookups, glyphs, advances)
end

--- Run each lookup over the whole run, in turn.
---
--- One pass per lookup rather than one pass per glyph, and the order matters:
--- `ccmp` has to have finished composing before `liga` looks for ligatures.
---
--- Within a pass, a glyph is skipped unless the feature that brought this lookup
--- in is switched on for it -- that is what makes `frac` apply to four digits
--- and not to the rest of the line.
function OTProcessor:applyLookups(lookups, glyphs, positions)
    self.glyphs = glyphs
    self.positions = positions
    self.glyphIterator = GlyphIterator.new(glyphs)

    for _, entry in ipairs(lookups) do
        local feature, lookup = entry.feature, entry.lookup
        self.currentFeature = feature
        self.glyphIterator:reset(lookup.flags)

        while self.glyphIterator.index <= #glyphs do
            if not has(self.glyphIterator:cur().features, feature) then
                self.glyphIterator:next()
            else
                -- Subtables of one lookup are alternatives: the first that
                -- matches wins and the rest are not tried.
                for _, subTable in ipairs(lookup.subTables) do
                    if self:applyLookup(lookup.lookupType, subTable) then
                        break
                    end
                end

                self.glyphIterator:next()
            end
        end
    end
end

function OTProcessor:applyLookup(lookup, table_)
    error("applyLookup must be implemented by subclasses")
end

--- Run the lookups a contextual rule asked for.
---
--- Each record says "apply lookup N at sequence position M". The position is
--- counted the way the iterator counts, so a rule that matched across an ignored
--- mark still lands on the right glyph. Flags are swapped in and out around each
--- one, because the nested lookup has its own and the outer rule's must be
--- restored before the next record.
---
--- Always returns true: a contextual rule that matched has done its work whether
--- or not the lookups it invoked changed anything.
function OTProcessor:applyLookupList(lookupRecords)
    local options = self.glyphIterator.options
    local glyphIndex = self.glyphIterator.index

    for _, lookupRecord in ipairs(lookupRecords) do
        -- Reset flags and find glyph index for this lookup record
        self.glyphIterator:reset(options, glyphIndex)
        self.glyphIterator:increment(lookupRecord.sequenceIndex)

        -- Get the lookup and setup flags for subtables
        local lookup = self.table.lookupList:get(lookupRecord.lookupListIndex)
        self.glyphIterator:reset(lookup.flags, self.glyphIterator.index)

        -- Apply lookup subtables until one matches
        for _, subTable in ipairs(lookup.subTables) do
            if self:applyLookup(lookup.lookupType, subTable) then
                break
            end
        end
    end

    self.glyphIterator:reset(options, glyphIndex)
    return true
end

--------------------------------------------------------------------------------
-- Coverage and classes
--------------------------------------------------------------------------------

--- Where `glyph` sits in a coverage table, one-based, or nil if not covered.
---
--- The position is not incidental -- it is the index into whatever parallel
--- array of rules the subtable carries, so "covered" and "which rule" are the
--- same question.
function OTProcessor:coverageIndex(coverage, glyph)
    if glyph == nil then
        glyph = self.glyphIterator:cur().id
    end

    if coverage.version == 1 then
        -- A sorted list of glyph ids; position in the list is the index.
        for i, g in ipairs(coverage.glyphs) do
            if g == glyph then
                return i
            end
        end
    elseif coverage.version == 2 then
        -- Ranges, each carrying the coverage index its first glyph has.
        for _, range in ipairs(coverage.rangeRecords) do
            if range.start <= glyph and glyph <= range["end"] then
                return range.startCoverageIndex + glyph - range.start + 1
            end
        end
    end

    return nil
end

--- The class `glyph` belongs to in `classDef`. Zero for anything not listed.
---
--- A free function rather than a method because GlyphInfo needs it before it has
--- a processor to call it on -- upstream reaches for
--- `OTProcessor.prototype.getClassID` there, with a `// TODO: clean this up`
--- next to it. This is that cleanup, as far as it goes.
function OTProcessor.class_id(glyph, classDef)
    if classDef.version == 1 then
        -- A run of consecutive glyphs starting at startGlyph.
        local i = glyph - classDef.startGlyph
        if i >= 0 and i < #classDef.classValueArray then
            return classDef.classValueArray[i + 1]
        end
    elseif classDef.version == 2 then
        for _, range in ipairs(classDef.classRangeRecord) do
            if range.start <= glyph and glyph <= range["end"] then
                return range.class
            end
        end
    end

    return 0
end

function OTProcessor:getClassID(glyph, classDef)
    return OTProcessor.class_id(glyph, classDef)
end

--------------------------------------------------------------------------------
-- Sequence matching
--------------------------------------------------------------------------------

--- Match `sequence` against the run, starting `sequenceIndex` glyphs away.
---
--- The comparison is `fn`, which is what separates matching literal glyph ids
--- from matching classes from matching coverage tables -- the walk is the same
--- in all three cases.
---
--- A negative `sequenceIndex` walks backwards, which is how a chaining rule
--- matches its backtrack: the backtrack is stored nearest-glyph-first, so
--- reading it forwards while stepping backwards lines the two up.
---
--- The cursor is always put back where it started. Returns false if the sequence
--- ran out of glyphs before it ran out of components; otherwise `matched` if one
--- was passed, and true if not.
function OTProcessor:match(sequenceIndex, sequence, fn, matched)
    local pos = self.glyphIterator.index
    local glyph = self.glyphIterator:increment(sequenceIndex)
    local idx = 1

    while idx <= #sequence and glyph and fn(sequence[idx], glyph) do
        if matched then
            matched[#matched + 1] = self.glyphIterator.index
        end

        idx = idx + 1
        glyph = self.glyphIterator:next()
    end

    self.glyphIterator.index = pos
    if idx <= #sequence then
        return false
    end

    return matched or true
end

--- Match a sequence of literal glyph ids.
function OTProcessor:sequenceMatches(sequenceIndex, sequence)
    return self:match(sequenceIndex, sequence, function(component, glyph)
        return component == glyph.id
    end)
end

--- The same, collecting the index of each glyph matched.
---
--- Ligature substitution needs the indices, because the glyphs it consumed are
--- not necessarily consecutive -- an ignored mark can sit between two components
--- and has to survive the ligation.
---
--- Unlike the others this one also requires the feature to be on for every glyph
--- in the sequence, not just the first.
function OTProcessor:sequenceMatchIndices(sequenceIndex, sequence)
    return self:match(sequenceIndex, sequence, function(component, glyph)
        -- If the current feature doesn't apply to this glyph,
        if not has(glyph.features, self.currentFeature) then
            return false
        end

        return component == glyph.id
    end, {})
end

--- Match a sequence of coverage tables -- each position is a set of glyphs.
function OTProcessor:coverageSequenceMatches(sequenceIndex, sequence)
    return self:match(sequenceIndex, sequence, function(coverage, glyph)
        return self:coverageIndex(coverage, glyph.id) ~= nil
    end)
end

--- Match a sequence of class numbers.
function OTProcessor:classSequenceMatches(sequenceIndex, sequence, classDef)
    return self:match(sequenceIndex, sequence, function(classID, glyph)
        return classID == OTProcessor.class_id(glyph.id, classDef)
    end)
end

--------------------------------------------------------------------------------
-- Contextual rules
--------------------------------------------------------------------------------

--- "When this sequence appears, run these other lookups on it."
---
--- The three formats say the same thing three ways: literal glyph ids, glyph
--- classes, or a coverage table per position. Formats 1 and 2 select a set of
--- rules by the first glyph and then try each rule in turn; format 3 is a single
--- rule with no selection step.
function OTProcessor:applyContext(table_)
    local index, set

    if table_.version == 1 then
        index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        set = table_.ruleSets[index]
        for _, rule in ipairs(set) do
            -- 1, not 0: the covered glyph is the first of the sequence and is
            -- already matched, so the stored input starts one glyph along.
            if self:sequenceMatches(1, rule.input) then
                return self:applyLookupList(rule.lookupRecords)
            end
        end
    elseif table_.version == 2 then
        if self:coverageIndex(table_.coverage) == nil then
            return false
        end

        -- Indexed by class number, so the array position is class + 1.
        index = OTProcessor.class_id(self.glyphIterator:cur().id, table_.classDef)
        set = table_.classSet[index + 1]
        for _, rule in ipairs(set) do
            if self:classSequenceMatches(1, rule.classes, table_.classDef) then
                return self:applyLookupList(rule.lookupRecords)
            end
        end
    elseif table_.version == 3 then
        if self:coverageSequenceMatches(0, table_.coverages) then
            return self:applyLookupList(table_.lookupRecords)
        end
    end

    return false
end

--- The same, with context on either side that is matched but not rewritten.
---
--- All three parts have to match together: backtrack walking left from the
--- current glyph, input walking right, lookahead beyond the input. Only the
--- input positions are handed to the nested lookups.
function OTProcessor:applyChainingContext(table_)
    local index

    if table_.version == 1 then
        index = self:coverageIndex(table_.coverage)
        if index == nil then
            return false
        end

        local set = table_.chainRuleSets[index]
        for _, rule in ipairs(set) do
            if self:sequenceMatches(-#rule.backtrack, rule.backtrack)
                and self:sequenceMatches(1, rule.input)
                and self:sequenceMatches(1 + #rule.input, rule.lookahead) then
                return self:applyLookupList(rule.lookupRecords)
            end
        end
    elseif table_.version == 2 then
        if self:coverageIndex(table_.coverage) == nil then
            return false
        end

        index = OTProcessor.class_id(self.glyphIterator:cur().id, table_.inputClassDef)
        local rules = table_.chainClassSet[index + 1]

        -- Unlike the context table above, this one is expected to be sparse:
        -- most classes have no chaining rules, and their entries are null.
        if not rules then
            return false
        end

        for _, rule in ipairs(rules) do
            if self:classSequenceMatches(-#rule.backtrack, rule.backtrack, table_.backtrackClassDef)
                and self:classSequenceMatches(1, rule.input, table_.inputClassDef)
                and self:classSequenceMatches(1 + #rule.input, rule.lookahead, table_.lookaheadClassDef) then
                return self:applyLookupList(rule.lookupRecords)
            end
        end
    elseif table_.version == 3 then
        if self:coverageSequenceMatches(-table_.backtrackGlyphCount, table_.backtrackCoverage)
            and self:coverageSequenceMatches(0, table_.inputCoverage)
            and self:coverageSequenceMatches(table_.inputGlyphCount, table_.lookaheadCoverage) then
            return self:applyLookupList(table_.lookupRecords)
        end
    end

    return false
end

return OTProcessor
