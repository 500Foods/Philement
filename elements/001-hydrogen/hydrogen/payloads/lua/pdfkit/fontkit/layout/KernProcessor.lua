-- fontkit/layout/KernProcessor.lua
--   -- port of fontkit/src/layout/KernProcessor.js
--
-- Applies the `kern` table to a positioned run: for each adjacent pair of
-- glyphs, look up the pair and add whatever it says to the left glyph's advance.
--
-- Used only when the font has no GPOS `kern` feature. A font with both would be
-- kerned twice.
--
-- A kern table can hold several subtables, and their results add up -- one might
-- carry the bulk of the pairs and another a set of corrections. The `override`
-- flag on a Microsoft subtable discards the running total instead of adding to
-- it, so subtable order matters.

local utils = require("pdfkit.fontkit.utils")

local KernProcessor = {}
KernProcessor.__index = KernProcessor

function KernProcessor.new(font)
    return setmetatable({ kern = font.kern }, KernProcessor)
end

--- Kern every adjacent pair in the run.
---
--- The last glyph has nothing after it, so the loop stops one short: kerning
--- adjusts the gap *between* two glyphs and is charged to the one on the left.
function KernProcessor:process(glyphs, positions)
    for glyphIndex = 1, #glyphs - 1 do
        local left = glyphs[glyphIndex].id
        local right = glyphs[glyphIndex + 1].id
        positions[glyphIndex].xAdvance =
            positions[glyphIndex].xAdvance + self:getKerning(left, right)
    end
end

--- The kerning between two glyph ids, in font units. Zero if the pair is not
--- listed, which is nearly all pairs.
---
--- Glyph ids are zero-based, as they are everywhere in a font; the arrays they
--- index are one-based Lua arrays, hence the `+ 1`s below.
function KernProcessor:getKerning(left, right)
    local res = 0

    for _, table_ in ipairs(self.kern.tables) do
        -- Cross-stream kerning moves the pen sideways to the text direction --
        -- for cursive scripts where the join between letters rises or falls.
        -- Not applied here in either version.
        if not table_.coverage.crossStream then
            local skip = false

            if table_.version == 0 then
                -- Microsoft: one bit says horizontal, and its absence says
                -- vertical.
                skip = not table_.coverage.horizontal
            elseif table_.version == 1 then
                -- Apple: two separate bits, either of which disqualifies. A
                -- variation subtable holds deltas for a variable font's axes
                -- rather than kerning values.
                skip = table_.coverage.vertical or table_.coverage.variation
            else
                error("Unsupported kerning table version " .. tostring(table_.version))
            end

            if not skip then
                local val = 0
                local s = table_.subtable

                if table_.format == 0 then
                    -- The pairs are sorted by (left, right) as a single key, so
                    -- one binary search finds both. The comparator returns the
                    -- left difference unless it is zero, in which case the pairs
                    -- share a left glyph and the right one decides.
                    local pairIdx = utils.binary_search(s.pairs, function(pair)
                        local d = left - pair.left
                        if d == 0 then
                            d = right - pair.right
                        end
                        return d
                    end)

                    if pairIdx then
                        val = s.pairs[pairIdx].value
                    end

                elseif table_.format == 2 then
                    -- Two byte offsets that add up to a position in the value
                    -- array: the left table's offset selects a row, the right
                    -- table's a column, and both are pre-scaled so no
                    -- multiplication is needed. A glyph outside the left table
                    -- falls back to the array's own offset, which is row zero;
                    -- one outside the right table takes column zero.
                    local leftOffset, rightOffset = 0, 0

                    if left >= s.leftTable.firstGlyph
                        and left < s.leftTable.firstGlyph + s.leftTable.nGlyphs then
                        leftOffset = s.leftTable.offsets[left - s.leftTable.firstGlyph + 1]
                    else
                        leftOffset = s.array.off
                    end

                    if right >= s.rightTable.firstGlyph
                        and right < s.rightTable.firstGlyph + s.rightTable.nGlyphs then
                        rightOffset = s.rightTable.offsets[right - s.rightTable.firstGlyph + 1]
                    end

                    local index = (leftOffset + rightOffset - s.array.off) / 2

                    -- The lazy array yields nothing past its end, where upstream
                    -- would go on to add a null and get its numeric value.
                    val = s.array.values:get(index) or 0

                elseif table_.format == 3 then
                    if left >= s.glyphCount or right >= s.glyphCount then
                        -- Upstream abandons the whole lookup here rather than
                        -- skipping this one subtable, so anything already
                        -- accumulated is discarded.
                        return 0
                    end

                    val = s.kernValue[
                        s.kernIndex[
                            s.leftClass[left + 1] * s.rightClassCount + s.rightClass[right + 1] + 1
                        ] + 1
                    ]

                else
                    error("Unsupported kerning sub-table format " .. tostring(table_.format))
                end

                -- Microsoft's override flag resets the result; otherwise the
                -- subtables' values are summed.
                if table_.coverage.override then
                    res = val
                else
                    res = res + val
                end
            end
        end
    end

    return res
end

return KernProcessor
