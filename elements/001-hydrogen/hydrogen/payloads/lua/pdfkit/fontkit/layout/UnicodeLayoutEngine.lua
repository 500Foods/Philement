-- fontkit/layout/UnicodeLayoutEngine.lua
--   -- port of fontkit/src/layout/UnicodeLayoutEngine.js
--
-- Mark positioning for fonts that cannot do it themselves.
--
-- A well-equipped OpenType font carries `mark` and `mkmk` features in GPOS
-- saying exactly where each accent belongs on each letter. Most fonts do not.
-- This is the fallback: stack the marks on the base glyph using nothing but
-- their bounding boxes and their Unicode combining class, which says roughly
-- where a mark goes -- above, below, left, right, attached or not.
--
-- The results are approximate by nature. An accent centred on a letter's
-- bounding box is not where a type designer would have put it, but it is
-- legible, and it is what you get when the font does not say otherwise.
--
-- Based on Harfbuzz's hb-ot-shape-fallback.cc.

local unicode = require("pdfkit.fontkit.unicode")

local UnicodeLayoutEngine = {}
UnicodeLayoutEngine.__index = UnicodeLayoutEngine

function UnicodeLayoutEngine.new(font)
    return setmetatable({ font = font }, UnicodeLayoutEngine)
end

--- Position every mark in the run against the base glyph it follows.
---
--- Walks the run splitting it into clusters, where a cluster is one non-mark
--- glyph followed by every mark up to the next non-mark. Clusters of one -- a
--- letter with no accents, which is nearly all of them -- are skipped.
---
--- Indices here are one-based and inclusive at both ends, unlike upstream's
--- zero-based half-open pair, so `clusterStart == clusterEnd` still means "no
--- marks" and the comparisons carry over unchanged.
function UnicodeLayoutEngine:positionGlyphs(glyphs, positions)
    local clusterStart = 1
    local clusterEnd = 1

    for index = 1, #glyphs do
        if glyphs[index].isMark then
            -- TODO upstream: ligatures are not handled here.
            clusterEnd = index
        else
            if clusterStart ~= clusterEnd then
                self:positionCluster(glyphs, positions, clusterStart, clusterEnd)
            end
            clusterStart = index
            clusterEnd = index
        end
    end

    -- A cluster that runs to the end of the text never meets a following base
    -- glyph, so it is flushed here.
    if clusterStart ~= clusterEnd then
        self:positionCluster(glyphs, positions, clusterStart, clusterEnd)
    end

    return positions
end

--- Stack the marks of one cluster onto its base.
---
--- The base's box is a running total rather than a constant: each mark placed
--- above pushes the box's top up by its own height, so the next mark stacks on
--- top of it instead of colliding. Same downwards.
function UnicodeLayoutEngine:positionCluster(glyphs, positions, clusterStart, clusterEnd)
    local base = glyphs[clusterStart]
    local baseBox = base:cbox():copy()

    -- If the base is a ligature, the marks belong over its last component, so
    -- the box is narrowed to that component's share of the width. An even split
    -- is a guess -- the font knows the real division and is not being asked.
    if #base.codePoints > 1 then
        -- LTR only. TODO upstream: RTL.
        baseBox.minX = baseBox.minX
            + ((#base.codePoints - 1) * baseBox:width()) / #base.codePoints
    end

    -- Marks are drawn at the pen position of the glyph *after* the base, so
    -- every offset has to be walked back by the advances in between. xOffset
    -- starts at minus the base's advance and grows as unreordered marks with
    -- advances of their own are passed over.
    local xOffset = -positions[clusterStart].xAdvance
    local yOffset = 0
    local yGap = self.font:unitsPerEm() / 16

    for index = clusterStart + 1, clusterEnd do
        local mark = glyphs[index]
        local markBox = mark:cbox()
        local position = positions[index]

        local combiningClass = self:getCombiningClass(mark.codePoints[1])

        if combiningClass ~= "Not_Reordered" then
            position.xOffset = 0
            position.yOffset = 0

            -- Horizontal placement.
            if combiningClass == "Double_Above" or combiningClass == "Double_Below" then
                -- A double mark spans two letters, so it is centred on the
                -- *boundary* between them -- the base's left edge. LTR only.
                position.xOffset = position.xOffset
                    + baseBox.minX - markBox:width() / 2 - markBox.minX
            elseif combiningClass == "Attached_Below_Left"
                or combiningClass == "Below_Left"
                or combiningClass == "Above_Left" then
                position.xOffset = position.xOffset + baseBox.minX - markBox.minX
            elseif combiningClass == "Attached_Above_Right"
                or combiningClass == "Below_Right"
                or combiningClass == "Above_Right" then
                position.xOffset = position.xOffset
                    + baseBox.maxX - markBox:width() - markBox.minX
            else
                -- Attached_Below, Attached_Above, Below, Above, and anything
                -- unrecognised: centre it.
                position.xOffset = position.xOffset
                    + baseBox.minX + (baseBox:width() - markBox:width()) / 2 - markBox.minX
            end

            -- Vertical placement. Marks that are "attached" touch the letter;
            -- the rest get a gap of a sixteenth of an em first.
            if combiningClass == "Double_Below"
                or combiningClass == "Below_Left"
                or combiningClass == "Below"
                or combiningClass == "Below_Right"
                or combiningClass == "Attached_Below_Left"
                or combiningClass == "Attached_Below" then
                if combiningClass == "Attached_Below_Left"
                    or combiningClass == "Attached_Below" then
                    baseBox.minY = baseBox.minY + yGap
                end

                position.yOffset = -baseBox.minY - markBox.maxY
                baseBox.minY = baseBox.minY + markBox:height()
            elseif combiningClass == "Double_Above"
                or combiningClass == "Above_Left"
                or combiningClass == "Above"
                or combiningClass == "Above_Right"
                or combiningClass == "Attached_Above"
                or combiningClass == "Attached_Above_Right" then
                if combiningClass == "Attached_Above"
                    or combiningClass == "Attached_Above_Right" then
                    baseBox.maxY = baseBox.maxY + yGap
                end

                position.yOffset = baseBox.maxY - markBox.minY
                baseBox.maxY = baseBox.maxY + markBox:height()
            end

            -- A positioned mark occupies no width of its own.
            position.xAdvance = 0
            position.yAdvance = 0
            position.xOffset = position.xOffset + xOffset
            position.yOffset = position.yOffset + yOffset
        else
            -- Not_Reordered: a mark that advances the pen like a letter. It is
            -- left where it is, and the marks after it have that much further
            -- to walk back.
            xOffset = xOffset - position.xAdvance
            yOffset = yOffset - position.yAdvance
        end
    end
end

--- The combining class of a code point, with the exceptions the raw Unicode
--- property gets wrong for typesetting purposes.
---
--- Three groups of them, all inherited from Harfbuzz:
---
---   * Thai and Lao vowels and tone marks are Not_Reordered in the database but
---     are visibly above or below the consonant.
---   * The Hebrew points are numbered classes (CCC10 upwards) that encode
---     canonical ordering, not position; the numbers have to be mapped by hand.
---   * Arabic, Syriac, Thai, Lao and Tibetan add more of the same.
function UnicodeLayoutEngine:getCombiningClass(codePoint)
    local combiningClass = unicode.get_combining_class(codePoint)

    -- Thai and Lao: the U+0E00 block.
    if (codePoint & ~0xff) == 0x0e00 then
        if combiningClass == "Not_Reordered" then
            if codePoint == 0x0e31 or codePoint == 0x0e34 or codePoint == 0x0e35
                or codePoint == 0x0e36 or codePoint == 0x0e37 or codePoint == 0x0e47
                or codePoint == 0x0e4c or codePoint == 0x0e3d or codePoint == 0x0e4e then
                return "Above_Right"
            elseif codePoint == 0x0eb1 or codePoint == 0x0eb4 or codePoint == 0x0eb5
                or codePoint == 0x0eb6 or codePoint == 0x0eb7 or codePoint == 0x0ebb
                or codePoint == 0x0ecc or codePoint == 0x0ecd then
                return "Above"
            elseif codePoint == 0x0ebc then
                return "Below"
            end
        elseif codePoint == 0x0e3a then  -- phinthu, a virama
            return "Below_Right"
        end
    end

    -- Hebrew points.
    if combiningClass == "CCC10"        -- sheva
        or combiningClass == "CCC11"    -- hataf segol
        or combiningClass == "CCC12"    -- hataf patah
        or combiningClass == "CCC13"    -- hataf qamats
        or combiningClass == "CCC14"    -- hiriq
        or combiningClass == "CCC15"    -- tsere
        or combiningClass == "CCC16"    -- segol
        or combiningClass == "CCC17"    -- patah
        or combiningClass == "CCC18"    -- qamats
        or combiningClass == "CCC20"    -- qubuts
        or combiningClass == "CCC22" then  -- meteg
        return "Below"
    elseif combiningClass == "CCC23" then  -- rafe
        return "Attached_Above"
    elseif combiningClass == "CCC24" then  -- shin dot
        return "Above_Right"
    elseif combiningClass == "CCC25"       -- sin dot
        or combiningClass == "CCC19" then  -- holam
        return "Above_Left"
    elseif combiningClass == "CCC26" then  -- point varika
        return "Above"
    elseif combiningClass == "CCC21" then  -- dagesh
        -- Falls through to the class as given: a dagesh sits *inside* the
        -- letter and none of the eight positions describes that.
        return combiningClass

    -- Arabic and Syriac.
    elseif combiningClass == "CCC27"    -- fathatan
        or combiningClass == "CCC28"    -- dammatan
        or combiningClass == "CCC30"    -- fatha
        or combiningClass == "CCC31"    -- damma
        or combiningClass == "CCC33"    -- shadda
        or combiningClass == "CCC34"    -- sukun
        or combiningClass == "CCC35"    -- superscript alef
        or combiningClass == "CCC36" then  -- superscript alaph
        return "Above"
    elseif combiningClass == "CCC29"    -- kasratan
        or combiningClass == "CCC32" then  -- kasra
        return "Below"

    -- Thai.
    elseif combiningClass == "CCC103" then  -- sara u / sara uu
        return "Below_Right"
    elseif combiningClass == "CCC107" then  -- mai
        return "Above_Right"

    -- Lao.
    elseif combiningClass == "CCC118" then  -- sign u / sign uu
        return "Below"
    elseif combiningClass == "CCC122" then  -- mai
        return "Above"

    -- Tibetan.
    elseif combiningClass == "CCC129"       -- sign aa
        or combiningClass == "CCC132" then  -- sign u
        return "Below"
    elseif combiningClass == "CCC130" then  -- sign i
        return "Above"
    end

    return combiningClass
end

return UnicodeLayoutEngine
