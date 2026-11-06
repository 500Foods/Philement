-- fontkit/tables/GSUB.lua -- port of fontkit/src/tables/GSUB.js
--
-- The Glyph Substitution table: rules for replacing glyphs with other glyphs.
-- This is where `f` + `i` becomes the `fi` ligature, where a small-caps font
-- finds its small capitals, and where an Indic vowel sign gets reordered.
--
-- The header -- script list, feature list, lookup list -- is shared with GPOS
-- and lives in opentype.lua. What is here is the eight lookup types, which are
-- GSUB's alone:
--
--   1  Single         one glyph becomes one other glyph
--   2  Multiple       one glyph becomes several
--   3  Alternate      one glyph, a choice of replacements (the caller picks)
--   4  Ligature       several glyphs become one
--   5  Contextual     run other lookups when a sequence appears
--   6  Chaining       the same, with context before and after
--   7  Extension      an indirection, so a big table can use 32-bit offsets
--   8  Reverse chain  like 6, applied back to front -- see the note below
--
-- Types 5 and 6 are the shared Context and ChainingContext tables. Type 7 is a
-- pointer to another lookup of any other type, which fonts use once their
-- layout data outgrows the 16-bit offsets everything else is written with.

local ot = require("pdfkit.fontkit.tables.opentype")

local M = {}

--------------------------------------------------------------------------------
-- Pieces the substitution types are built from
--------------------------------------------------------------------------------

--- A run of glyph ids: `count`, then that many uint16s.
---
--- Serves as both the replacement list of a multiple substitution and the
--- choices of an alternate substitution -- identical layout, different meaning.
local function decode_sequence(r)
    local count = r:uint16()
    return r:array(count, r.uint16)
end

--- One ligature rule: the glyph to produce, and the glyphs that must follow the
--- covered one for it to apply.
---
--- `compCount` counts the whole sequence including the first glyph, which the
--- coverage table already matched, so one fewer component is stored.
local function decode_ligature(r)
    local t = {}
    t.glyph = r:uint16()
    t.compCount = r:uint16()
    t.components = r:array(t.compCount - 1, r.uint16)
    return t
end

--- All the ligatures starting with one covered glyph, longest match first --
--- the font orders them, and the processor takes the first that matches.
local function decode_ligature_set(r)
    local base = r.pos
    local count = r:uint16()
    local set = {}
    for i = 1, count do
        set[i] = ot.ptr16(r, base, decode_ligature)
    end
    return set
end

--------------------------------------------------------------------------------
-- The lookup types
--------------------------------------------------------------------------------

--- Decode one GSUB subtable, given the type from its enclosing lookup.
---
--- Declared as a local first because type 7 points at another subtable of any
--- type, including -- by way of the coverage of the other seven -- back into
--- this function.
local decode_subtable

--- Type 1: single substitution. Either every covered glyph shifts by the same
--- delta, or each has its own replacement.
local function decode_single(r)
    local base = r.pos
    local t = { version = r:uint16() }

    if t.version == 1 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.deltaGlyphID = r:int16()
    elseif t.version == 2 then
        t.coverage = ot.ptr16(r, base, ot.decode_coverage)
        t.glyphCount = r:uint16()
        t.substitute = r:lazy(t.glyphCount, 2, r.uint16)
    else
        error("Unknown version " .. t.version)
    end

    return t
end

--- Types 2, 3 and 4 have the same shape: a coverage table, a count, and one
--- lazily-decoded pointer per covered glyph. Only the target differs.
---
--- The pointers are measured from the start of the subtable, not from the start
--- of the array, so `base` is captured before the count is read.
local function decode_indexed(r, field, decode_one)
    local base = r.pos
    local t = {}
    t.substFormat = r:uint16()
    t.coverage = ot.ptr16(r, base, ot.decode_coverage)
    t.count = r:uint16()
    t[field] = r:lazy(t.count, 2, function(rr)
        local offset = rr:uint16()
        if offset == 0 then
            return false
        end

        rr:seek(base + offset)
        return decode_one(rr)
    end)
    return t
end

--- Type 7: extension substitution. A second lookup type and a 32-bit offset to
--- a subtable of that type, which by the spec is anything but 7 again.
local function decode_extension(r)
    local base = r.pos
    local t = {}
    t.substFormat = r:uint16()
    t.lookupType = r:uint16()
    t.extension = ot.ptr32(r, base, function(rr)
        return decode_subtable(rr, t.lookupType)
    end)
    return t
end

--- Type 8: reverse chaining contextual single substitution.
---
--- This raises, and it raises for the same reason fontkit does: fontkit's
--- declaration of the format is missing the `backtrackGlyphCount` field, so the
--- array that follows is asked for a length that was never read, restructure
--- finds `undefined` and throws "Not a fixed size". Reproduced rather than
--- fixed, because a font whose layout differs between the two libraries is a
--- worse outcome than one that fails identically in both.
---
--- Nothing is lost in practice: fontkit's GSUB processor has no case for type 8
--- either, so a font that reached this point would fail a few lines later
--- anyway. The type is used by Nastaliq Arabic and by a handful of fonts that
--- swap a glyph based on what follows it.
local function decode_reverse_chain(r)
    error("Not a fixed size")
end

function decode_subtable(r, lookupType)
    if lookupType == 1 then
        return decode_single(r)
    elseif lookupType == 2 then
        return decode_indexed(r, "sequences", decode_sequence)
    elseif lookupType == 3 then
        return decode_indexed(r, "alternateSet", decode_sequence)
    elseif lookupType == 4 then
        return decode_indexed(r, "ligatureSets", decode_ligature_set)
    elseif lookupType == 5 then
        return ot.decode_context(r)
    elseif lookupType == 6 then
        return ot.decode_chaining_context(r)
    elseif lookupType == 7 then
        return decode_extension(r)
    elseif lookupType == 8 then
        return decode_reverse_chain(r)
    end

    error("Unknown version " .. lookupType)
end

M.decode_subtable = decode_subtable

--------------------------------------------------------------------------------
-- The table itself
--------------------------------------------------------------------------------

function M.decode(stream)
    return ot.decode_layout_header(stream, decode_subtable)
end

return M
