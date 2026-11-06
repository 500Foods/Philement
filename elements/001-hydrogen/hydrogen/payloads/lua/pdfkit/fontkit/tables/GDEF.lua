-- fontkit/tables/GDEF.lua -- port of fontkit/src/tables/GDEF.js
--
-- The Glyph Definition table: what kind of thing each glyph is. Not rules, just
-- facts the rules in GSUB and GPOS lean on.
--
-- One field carries nearly all of the weight. `glyphClassDef` sorts every glyph
-- into base, ligature, mark or component, and that is what a lookup's
-- `ignoreMarks` flag is testing when it steps over an accent to find the letter
-- on the other side of it. Without GDEF, fontkit guesses from the Unicode
-- properties of the characters a glyph came from, which is right for Latin text
-- and unreliable for anything else.
--
-- The rest is read and unused, here as upstream: attachment points and ligature
-- caret positions are for editing and hit-testing rather than for layout, and
-- mark glyph sets serve the `useMarkFilteringSet` lookup flag, which fontkit
-- decodes but never acts on.

local ot = require("pdfkit.fontkit.tables.opentype")

local M = {}

--------------------------------------------------------------------------------
-- Attachment points
--------------------------------------------------------------------------------

--- Contour points on a base glyph where marks may be attached, for an editor
--- that wants to show them. A list of point numbers per covered glyph.
local function decode_attach_list(r)
    local base = r.pos
    local t = {}
    t.coverage = ot.ptr16(r, base, ot.decode_coverage)
    t.glyphCount = r:uint16()
    t.attachPoints = {}
    for i = 1, t.glyphCount do
        t.attachPoints[i] = ot.ptr16(r, base, function(rr)
            local count = rr:uint16()
            return rr:array(count, rr.uint16)
        end)
    end
    return t
end

--------------------------------------------------------------------------------
-- Ligature carets
--------------------------------------------------------------------------------

--- Where the text cursor goes inside a ligature: an `fi` is one glyph but two
--- characters, and clicking between them has to land somewhere.
local function decode_caret_value(r)
    local base = r.pos
    local c = { version = r:uint16() }

    if c.version == 1 then
        c.coordinate = r:int16()
    elseif c.version == 2 then
        c.caretValuePoint = r:uint16()
    elseif c.version == 3 then
        c.coordinate = r:int16()
        c.deviceTable = ot.ptr16(r, base, ot.decode_device)
    else
        error("Unknown version " .. c.version)
    end

    return c
end

local function decode_lig_glyph(r)
    local base = r.pos
    local count = r:uint16()
    local carets = {}
    for i = 1, count do
        carets[i] = ot.ptr16(r, base, decode_caret_value)
    end
    return carets
end

local function decode_lig_caret_list(r)
    local base = r.pos
    local t = {}
    t.coverage = ot.ptr16(r, base, ot.decode_coverage)
    t.ligGlyphCount = r:uint16()
    t.ligGlyphs = {}
    for i = 1, t.ligGlyphCount do
        t.ligGlyphs[i] = ot.ptr16(r, base, decode_lig_glyph)
    end
    return t
end

--------------------------------------------------------------------------------
-- Mark glyph sets
--------------------------------------------------------------------------------

--- Named sets of marks, so a lookup can say "ignore every mark except these".
--- Note the 32-bit offsets: this is the one part of GDEF that expected to grow.
local function decode_mark_glyph_sets(r)
    local base = r.pos
    local t = {}
    t.markSetTableFormat = r:uint16()
    t.markSetCount = r:uint16()
    t.coverage = {}
    for i = 1, t.markSetCount do
        t.coverage[i] = ot.ptr32(r, base, ot.decode_coverage)
    end
    return t
end

--------------------------------------------------------------------------------
-- The table itself
--------------------------------------------------------------------------------

function M.decode(stream)
    local base = stream.pos
    local t = {}
    t.version = stream:uint32()

    t.glyphClassDef = ot.ptr16(stream, base, ot.decode_class_def)
    t.attachList = ot.ptr16(stream, base, decode_attach_list)
    t.ligCaretList = ot.ptr16(stream, base, decode_lig_caret_list)
    t.markAttachClassDef = ot.ptr16(stream, base, ot.decode_class_def)

    if t.version == 0x00010000 then
        return t
    elseif t.version == 0x00010002 then
        t.markGlyphSetsDef = ot.ptr16(stream, base, decode_mark_glyph_sets)
        return t
    elseif t.version == 0x00010003 then
        t.markGlyphSetsDef = ot.ptr16(stream, base, decode_mark_glyph_sets)

        -- Version 1.3 adds an item variation store, for a variable font whose
        -- attachment points move as the axes move. Its offset is read past and
        -- not followed, and `itemVariationStore` is left unset -- exactly what a
        -- caller without a variation processor sees, since both places fontkit
        -- reads this field are guarded by one.
        stream:uint32()
        return t
    end

    error(string.format("Unknown version %d", t.version))
end

return M
