-- fontkit/tables/opentype.lua -- port of fontkit/src/tables/opentype.js
--
-- The parts of GSUB, GPOS and GDEF that are the same in all three. Nothing in
-- here is a table you can look up by tag; it is the vocabulary the three real
-- tables are written in.
--
-- The shape every OpenType layout table has:
--
--   ScriptList    which scripts the font has rules for, and inside each, which
--                 languages. A script/language pair names a set of features.
--   FeatureList   a feature is a four-letter tag -- `liga`, `kern`, `frac` --
--                 and a list of lookup indices.
--   LookupList    a lookup is the actual rule: a type, some flags, and one or
--                 more subtables. GSUB and GPOS number the types differently,
--                 which is why the subtable decoder is passed in.
--
-- Substitution and positioning then share two more pieces, both here because
-- both tables use them with identical layout: Context and ChainingContext, the
-- "apply these other lookups when this sequence of glyphs appears" rules.
--
--------------------------------------------------------------------------------
-- Two conventions worth reading before the code
--------------------------------------------------------------------------------
--
-- **A null pointer decodes to `false`, not `nil`.** An offset of zero means "no
-- table" and restructure yields `null` for it. `nil` would be the obvious Lua
-- spelling, but a `nil` in the middle of an array truncates it as far as `#` and
-- `ipairs` are concerned, and these arrays are full of optional pointers --
-- chainClassSet in particular is routinely half empty. `false` is falsy in the
-- same places `null` is (`if not rules then`) and keeps the array dense.
--
-- **Arrays are one-based; indices that come out of the font are not.** Every
-- array here is a normal Lua array. But a featureIndex or a lookupListIndex read
-- from the font is zero-based, so those get an explicit `+ 1` at the point of
-- use. The lazy arrays keep both accessors for exactly this reason: `:get(i)` is
-- zero-based, for indices the font supplied, and `:at(i)` is one-based, for
-- indices this port computed.

local M = {}

--------------------------------------------------------------------------------
-- Primitives
--------------------------------------------------------------------------------

--- Follow a uint16 offset measured from `base` and decode what is there.
---
--- restructure's Pointer. The cursor is left just past the offset field, so a
--- struct can be read straight through with pointers in the middle of it.
---
--- @return whatever `decode` returned, or `false` for a null (zero) offset
function M.ptr16(r, base, decode)
    local offset = r:uint16()
    if offset == 0 then
        return false
    end

    local pos = r.pos
    r:seek(base + offset)
    local value = decode(r)
    r:seek(pos)
    return value
end

--- The same, for the 32-bit offsets the extension lookups use.
function M.ptr32(r, base, decode)
    local offset = r:uint32()
    if offset == 0 then
        return false
    end

    local pos = r.pos
    r:seek(base + offset)
    local value = decode(r)
    r:seek(pos)
    return value
end

--- restructure's Bitfield: a number, spread into named booleans.
---
--- `names[1]` is the *low* bit. The lookup flag word is the reason this matters:
--- it is a uint16 read as two uint8s, so the flags byte is the low half and
--- `rightToLeft` -- named first -- is bit 0 of it.
function M.bitfield(value, names)
    local out = {}
    for i, name in ipairs(names) do
        out[name] = (value & (1 << (i - 1))) ~= 0
    end
    return out
end

--------------------------------------------------------------------------------
-- Scripts and languages
--------------------------------------------------------------------------------

--- A language system: which features are on for one language in one script.
---
--- `reqFeatureIndex` names a feature that is always on, and 0xFFFF means there
--- is none. fontkit reads it and never uses it, so neither does this.
local function decode_lang_sys(r)
    local s = {}
    r:skip(2)                                       -- reserved, always zero
    s.reqFeatureIndex = r:uint16()
    s.featureCount = r:uint16()
    s.featureIndexes = r:array(s.featureCount, r.uint16)
    return s
end

--- One script's rules: a default language system, plus any language-specific
--- ones. `langSys` offsets are measured from the start of the Script table.
local function decode_script(r)
    local base = r.pos
    local s = {}
    s.defaultLangSys = M.ptr16(r, base, decode_lang_sys)
    s.count = r:uint16()
    s.langSysRecords = {}
    for i = 1, s.count do
        s.langSysRecords[i] = {
            tag = r:tag(),
            langSys = M.ptr16(r, base, decode_lang_sys),
        }
    end
    return s
end

--- The ScriptList: `{tag = "latn", script = ...}` records, in tag order.
---
--- Searched linearly by OTProcessor rather than by binary search, which the
--- sorted order would allow -- there are rarely more than a handful.
function M.decode_script_list(r)
    local base = r.pos
    local count = r:uint16()
    local list = {}
    for i = 1, count do
        list[i] = {
            tag = r:tag(),
            script = M.ptr16(r, base, decode_script),
        }
    end
    return list
end

--------------------------------------------------------------------------------
-- Features and lookups
--------------------------------------------------------------------------------

--- Parameters some features carry. `size` uses them; nothing here does.
local function decode_feature_params(r)
    return { version = r:uint16(), nameID = r:uint16() }
end

--- A feature: the lookups that make it up, by index into the LookupList.
---
--- The indices are zero-based and stay that way -- see the note at the top.
local function decode_feature(r)
    local base = r.pos
    local f = {}
    f.featureParams = M.ptr16(r, base, decode_feature_params)
    f.lookupCount = r:uint16()
    f.lookupListIndexes = r:array(f.lookupCount, r.uint16)
    return f
end

--- The FeatureList: `{tag = "liga", feature = ...}` records.
---
--- Indexed by the featureIndexes a language system holds, so the order is the
--- font's and must not be disturbed.
function M.decode_feature_list(r)
    local base = r.pos
    local count = r:uint16()
    local list = {}
    for i = 1, count do
        list[i] = {
            tag = r:tag(),
            feature = M.ptr16(r, base, decode_feature),
        }
    end
    return list
end

-- The five lookup flags, low bit first. `ignoreMarks` and friends tell the
-- glyph iterator which glyphs to step over while applying this lookup, which is
-- how a kern pair still finds its partner across an intervening accent.
local LOOKUP_FLAGS = {
    "rightToLeft",
    "ignoreBaseGlyphs",
    "ignoreLigatures",
    "ignoreMarks",
    "useMarkFilteringSet",
}

--- One lookup: a type, flags, and the subtables that implement it.
---
--- The subtable decoder is a parameter because the type numbers mean different
--- things in GSUB and GPOS -- type 2 is multiple substitution in one and pair
--- positioning in the other. It is handed the type so it can dispatch.
local function decode_lookup(r, decode_subtable)
    local base = r.pos
    local l = {}
    l.lookupType = r:uint16()

    -- One uint16 read as two bytes: the high byte is a mark attachment class,
    -- the low byte is the flag bits. Big-endian, so the high byte comes first.
    l.flags = {
        markAttachmentType = r:uint8(),
        flags = M.bitfield(r:uint8(), LOOKUP_FLAGS),
    }

    l.subTableCount = r:uint16()
    l.subTables = {}
    for i = 1, l.subTableCount do
        l.subTables[i] = M.ptr16(r, base, function(rr)
            return decode_subtable(rr, l.lookupType)
        end)
    end

    -- Present only when the flag says so, and read after the subtable offsets.
    if l.flags.flags.useMarkFilteringSet then
        l.markFilteringSet = r:uint16()
    end

    return l
end

--- The LookupList, as a lazy array.
---
--- Lazy because this is where the bulk of a layout table lives and only the
--- lookups belonging to features actually in play are ever touched. A class pair
--- adjustment subtable alone can be a class1Count x class2Count matrix of value
--- records; decoding every lookup in DejaVuSans up front to lay out one word
--- would be most of the work of the whole port for nothing.
---
--- Returns a decoder, not a table: GSUB and GPOS call it with their own subtable
--- decoder.
function M.lookup_list(decode_subtable)
    return function(r)
        local base = r.pos
        local count = r:uint16()

        -- Each element is a uint16 offset from the start of the LookupList.
        return r:lazy(count, 2, function(rr)
            local offset = rr:uint16()
            if offset == 0 then
                return false
            end

            rr:seek(base + offset)
            return decode_lookup(rr, decode_subtable)
        end)
    end
end

--------------------------------------------------------------------------------
-- Coverage and class definition tables
--------------------------------------------------------------------------------

--- Coverage: the set of glyphs a lookup applies to, and each one's position in
--- that set -- the index that then selects the rule to use.
---
--- Two encodings, a sorted glyph list and a list of ranges, chosen by whichever
--- is smaller for the font. `end` is a Lua keyword, so the range records spell
--- that field `["end"]`; it is the only place in the port that has to.
function M.decode_coverage(r)
    local c = { version = r:uint16() }

    if c.version == 1 then
        c.glyphCount = r:uint16()
        c.glyphs = r:array(c.glyphCount, r.uint16)
    elseif c.version == 2 then
        c.rangeCount = r:uint16()
        c.rangeRecords = r:array(c.rangeCount, function(rr)
            return {
                start = rr:uint16(),
                ["end"] = rr:uint16(),
                startCoverageIndex = rr:uint16(),
            }
        end)
    else
        error("Unknown version " .. c.version)
    end

    return c
end

--- Class definitions: glyphs grouped into numbered classes.
---
--- Class 0 is every glyph not named here, and is a real class -- "everything
--- else" is what a class-based kern pair usually matches against. So a lookup
--- that finds nothing returns 0, not "absent".
---
--- Again two encodings: a run of consecutive glyphs with a class each, or a list
--- of ranges.
function M.decode_class_def(r)
    local c = { version = r:uint16() }

    if c.version == 1 then
        c.startGlyph = r:uint16()
        c.glyphCount = r:uint16()
        c.classValueArray = r:array(c.glyphCount, r.uint16)
    elseif c.version == 2 then
        c.classRangeCount = r:uint16()
        c.classRangeRecord = r:array(c.classRangeCount, function(rr)
            return {
                start = rr:uint16(),
                ["end"] = rr:uint16(),
                class = rr:uint16(),
            }
        end)
    else
        error("Unknown version " .. c.version)
    end

    return c
end

--------------------------------------------------------------------------------
-- Device table
--------------------------------------------------------------------------------

--- A device table, read but not interpreted.
---
--- Two different things share this layout. For a hinting device table `a` and
--- `b` are a start and end ppem size with per-size deltas following; for a
--- variation index they are an outer and inner index into a variation store.
--- fontkit reads the three fields and only ever uses `a` and `b`, and only for
--- variations -- which is why the fields are named after their positions rather
--- than after either meaning.
function M.decode_device(r)
    return { a = r:uint16(), b = r:uint16(), deltaFormat = r:uint16() }
end

--------------------------------------------------------------------------------
-- Contextual substitution and positioning
--------------------------------------------------------------------------------

--- Which other lookups to run, and at which glyph of the matched sequence.
local function decode_lookup_record(r)
    return { sequenceIndex = r:uint16(), lookupListIndex = r:uint16() }
end

local function decode_lookup_records(r, n)
    return r:array(n, decode_lookup_record)
end

--- A rule matched against literal glyph ids. The first glyph of the sequence is
--- the one covered by the coverage table, so only the rest is stored.
local function decode_rule(r)
    local t = {}
    t.glyphCount = r:uint16()
    t.lookupCount = r:uint16()
    t.input = r:array(t.glyphCount - 1, r.uint16)
    t.lookupRecords = decode_lookup_records(r, t.lookupCount)
    return t
end

--- A rule matched against glyph classes instead of glyph ids.
local function decode_class_rule(r)
    local t = {}
    t.glyphCount = r:uint16()
    t.lookupCount = r:uint16()
    t.classes = r:array(t.glyphCount - 1, r.uint16)
    t.lookupRecords = decode_lookup_records(r, t.lookupCount)
    return t
end

--- Rules are grouped into sets, one set per coverage index or class.
local function rule_set(decode_one)
    return function(r)
        local base = r.pos
        local count = r:uint16()
        local set = {}
        for i = 1, count do
            set[i] = M.ptr16(r, base, decode_one)
        end
        return set
    end
end

local decode_rule_set = rule_set(decode_rule)
local decode_class_set = rule_set(decode_class_rule)

--- Contextual substitution or positioning: "when this sequence appears, run
--- these other lookups on it".
---
--- Three formats, which differ only in how the sequence is written down --
--- literal glyphs, classes, or a coverage table per position.
function M.decode_context(r)
    local base = r.pos
    local t = { version = r:uint16() }

    if t.version == 1 then
        t.coverage = M.ptr16(r, base, M.decode_coverage)
        t.ruleSetCount = r:uint16()
        t.ruleSets = {}
        for i = 1, t.ruleSetCount do
            t.ruleSets[i] = M.ptr16(r, base, decode_rule_set)
        end
    elseif t.version == 2 then
        t.coverage = M.ptr16(r, base, M.decode_coverage)
        t.classDef = M.ptr16(r, base, M.decode_class_def)
        t.classSetCnt = r:uint16()
        t.classSet = {}
        for i = 1, t.classSetCnt do
            t.classSet[i] = M.ptr16(r, base, decode_class_set)
        end
    elseif t.version == 3 then
        t.glyphCount = r:uint16()
        t.lookupCount = r:uint16()
        t.coverages = {}
        for i = 1, t.glyphCount do
            t.coverages[i] = M.ptr16(r, base, M.decode_coverage)
        end
        t.lookupRecords = decode_lookup_records(r, t.lookupCount)
    else
        error("Unknown version " .. t.version)
    end

    return t
end

--------------------------------------------------------------------------------
-- Chaining contextual substitution and positioning
--------------------------------------------------------------------------------

--- The same idea with context on both sides: match `backtrack` before the
--- input and `lookahead` after it, but only rewrite the input.
---
--- The backtrack sequence is stored in reverse -- nearest glyph first -- which
--- is why the processor matches it with a negative sequence index and walks
--- backwards from the current glyph.
local function decode_chain_rule(r)
    local t = {}
    t.backtrackGlyphCount = r:uint16()
    t.backtrack = r:array(t.backtrackGlyphCount, r.uint16)
    t.inputGlyphCount = r:uint16()
    t.input = r:array(t.inputGlyphCount - 1, r.uint16)
    t.lookaheadGlyphCount = r:uint16()
    t.lookahead = r:array(t.lookaheadGlyphCount, r.uint16)
    t.lookupCount = r:uint16()
    t.lookupRecords = decode_lookup_records(r, t.lookupCount)
    return t
end

local decode_chain_rule_set = rule_set(decode_chain_rule)

function M.decode_chaining_context(r)
    local base = r.pos
    local t = { version = r:uint16() }

    if t.version == 1 then
        t.coverage = M.ptr16(r, base, M.decode_coverage)
        t.chainCount = r:uint16()
        t.chainRuleSets = {}
        for i = 1, t.chainCount do
            t.chainRuleSets[i] = M.ptr16(r, base, decode_chain_rule_set)
        end
    elseif t.version == 2 then
        t.coverage = M.ptr16(r, base, M.decode_coverage)
        t.backtrackClassDef = M.ptr16(r, base, M.decode_class_def)
        t.inputClassDef = M.ptr16(r, base, M.decode_class_def)
        t.lookaheadClassDef = M.ptr16(r, base, M.decode_class_def)
        t.chainCount = r:uint16()
        t.chainClassSet = {}
        for i = 1, t.chainCount do
            t.chainClassSet[i] = M.ptr16(r, base, decode_chain_rule_set)
        end
    elseif t.version == 3 then
        t.backtrackGlyphCount = r:uint16()
        t.backtrackCoverage = {}
        for i = 1, t.backtrackGlyphCount do
            t.backtrackCoverage[i] = M.ptr16(r, base, M.decode_coverage)
        end
        t.inputGlyphCount = r:uint16()
        t.inputCoverage = {}
        for i = 1, t.inputGlyphCount do
            t.inputCoverage[i] = M.ptr16(r, base, M.decode_coverage)
        end
        t.lookaheadGlyphCount = r:uint16()
        t.lookaheadCoverage = {}
        for i = 1, t.lookaheadGlyphCount do
            t.lookaheadCoverage[i] = M.ptr16(r, base, M.decode_coverage)
        end
        t.lookupCount = r:uint16()
        t.lookupRecords = decode_lookup_records(r, t.lookupCount)
    else
        error("Unknown version " .. t.version)
    end

    return t
end

--------------------------------------------------------------------------------
-- The header GSUB, GPOS and GDEF share
--------------------------------------------------------------------------------

--- Read the three pointers at the top of GSUB and GPOS.
---
--- Both tables start with a uint32 version and three offsets, all measured from
--- the start of the table. Version 1.1 adds a fourth, to a feature variations
--- table, which this port reads the offset of and does not follow -- see the
--- comment in GSUB.lua.
function M.decode_layout_header(r, decode_subtable)
    local base = r.pos
    local t = {}
    t.version = r:uint32()
    t.scriptList = M.ptr16(r, base, M.decode_script_list)
    t.featureList = M.ptr16(r, base, M.decode_feature_list)
    t.lookupList = M.ptr16(r, base, M.lookup_list(decode_subtable))

    if t.version == 0x00010000 then
        return t
    elseif t.version == 0x00010001 then
        -- Read past the offset without following it. The table it points at is
        -- only ever consulted through a variation processor, which this port
        -- does not have, so decoding it would build a structure nothing reads
        -- and would drag in the whole item-variation-store format to do it.
        r:uint32()
        t.featureVariations = nil
        return t
    end

    error(string.format("Unknown version %d", t.version))
end

return M
