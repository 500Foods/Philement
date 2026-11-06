-- table/render.lua -- port of pdfkit/lib/table/render.js
--
-- Everything here draws; nothing here decides. By the time a cell arrives it
-- already knows its x, y, width, height and where its text starts, so this file
-- is background, then border, then text, in that order, for each cell in turn.
--
-- The one thing it does compute is where inside its allocated box the text
-- sits, which cannot be settled earlier because it depends on how large the
-- text turned out to be.

local accessibility = require("pdfkit.table.accessibility")

local M = {}

local renderCell

--- Draw a row, wrapping it in structure elements if the table is tagged.
--- Returns the y coordinate of the bottom of the row, which is where the
--- document's cursor is left.
function M.renderRow(self, row, rowIndex)
    if self._tableStruct then
        accessibility.accessibleRow(self, row, rowIndex, function(cell, rowStruct)
            renderCell(self, cell, rowStruct)
        end)
    else
        for _, cell in ipairs(row) do
            renderCell(self, cell)
        end
    end

    return self._rowYPos[rowIndex] + self._rowHeights[rowIndex]
end

--- Draw one cell. When the table is tagged the drawing is handed to the
--- structure element as a closure, so the content stream operators it emits are
--- wrapped in the marked-content sequence that element owns.
function renderCell(self, cell, rowStruct)
    local function cellRenderer()
        if cell.backgroundColor ~= nil then
            self.document
                :save()
                :fillColor(cell.backgroundColor)
                :rect(cell.x, cell.y, cell.width, cell.height)
                :fill()
                :restore()
        end

        M.renderBorder(self, cell.border, cell.borderColor,
            cell.x, cell.y, cell.width, cell.height)

        if cell.debug then
            self.document:save()
            self.document:dash(1, { space = 1 }):lineWidth(1):strokeOpacity(0.3)

            -- Cell bounds
            self.document
                :rect(cell.x, cell.y, cell.width, cell.height)
                :stroke("green")

            self.document:restore()
        end

        if cell.text ~= nil and cell.text ~= "" then
            M.renderCellText(self, cell)
        end
    end

    if rowStruct then
        accessibility.accessibleCell(self, cell, rowStruct, cellRenderer)
    else
        cellRenderer()
    end
end

--- Place and draw a cell's text.
---
--- The offsets are the whole job. `A` is the box the text was allocated, `C` is
--- the box the text actually occupies, and `O` is where the text's own origin
--- sits relative to its bounds -- non-zero whenever the text is rotated, since
--- rotating about the start point swings the box away from it.
---
---   P = (A - C) * scale     how far in to push, 0 / 0.5 / 1 by alignment
---   d = P + O               plus the correction for the text's own origin
---
--- The clip is set to the allocated box rather than the cell, so text that
--- overflows is cut off at the padding and not at the border. Upstream flags
--- that as deliberate.
function M.renderCellText(self, cell)
    local doc = self.document

    local rollbackFont = doc._fontSource
    local rollbackFontSize = doc._fontSize
    local rollbackFontFamily = doc._fontFamily
    if cell.customFont then
        if cell.font.src then doc:font(cell.font.src, cell.font.family) end
        if cell.font.size then doc:fontSize(cell.font.size) end
    end

    local x = cell.textX
    local y = cell.textY
    local Ah = cell.textAllocatedHeight
    local Aw = cell.textAllocatedWidth
    local Cw = cell.textBounds.width
    local Ch = cell.textBounds.height
    local Ox = -cell.textBounds.x
    local Oy = -cell.textBounds.y

    local PxScale = 0
    if cell.align.x == "right" then PxScale = 1
    elseif cell.align.x == "center" then PxScale = 0.5 end
    local Px = (Aw - Cw) * PxScale

    local PyScale = 0
    if cell.align.y == "bottom" then PyScale = 1
    elseif cell.align.y == "center" then PyScale = 0.5 end
    local Py = (Ah - Ch) * PyScale

    local dx = Px + Ox
    local dy = Py + Oy

    if cell.debug then
        doc:save()
        doc:dash(1, { space = 1 }):lineWidth(1):strokeOpacity(0.3)

        -- Actual text bounds
        if cell.text ~= nil and cell.text ~= "" then
            doc:moveTo(x + Px, y)
                :lineTo(x + Px, y + Ah)
                :moveTo(x + Px + Cw, y)
                :lineTo(x + Px + Cw, y + Ah)
                :stroke("blue")
                :moveTo(x, y + Py)
                :lineTo(x + Aw, y + Py)
                :moveTo(x, y + Py + Ch)
                :lineTo(x + Aw, y + Py + Ch)
                :stroke("green")
        end
        -- Allocated text bounds
        doc:rect(x, y, Aw, Ah):stroke("orange")

        doc:restore()
    end

    -- Mask off anything that overflows.
    doc:save():rect(x, y, Aw, Ah):clip()

    doc:fillColor(cell.textColor):strokeColor(cell.textStrokeColor)
    if cell.textStroke > 0 then doc:lineWidth(cell.textStroke) end

    doc:text(cell.text, x + dx, y + dy, cell.textOptions)

    doc:restore()
    if cell.font then doc:font(rollbackFont, rollbackFontFamily, rollbackFontSize) end
end

--- Draw a cell's border.
---
--- Four equal sides become one `rect` and one `stroke`, which is both shorter
--- in the content stream and mitred at the corners. Anything else is drawn side
--- by side, clockwise from the top, each with its own width and colour.
---
--- `mask` zeroes out sides that a neighbour has already drawn. Nothing passes
--- it today -- upstream threads the parameter through without ever supplying it
--- -- but it is part of the signature and costs nothing to honour.
function M.renderBorder(self, border, borderColor, x, y, width, height, mask)
    local sides = { "top", "right", "bottom", "left" }
    local b = {}
    for _, side in ipairs(sides) do
        if mask and not mask[side] then
            b[side] = 0
        else
            b[side] = border[side]
        end
    end

    local doc = self.document

    if b.right == b.top and b.bottom == b.top and b.left == b.top then
        if b.top > 0 then
            doc:save()
                :lineWidth(b.top)
                :strokeColor(borderColor.top)
                :rect(x, y, width, height)
                :stroke()
                :restore()
        end
    else
        if b.top > 0 then
            doc:save()
                :lineWidth(b.top)
                :moveTo(x, y)
                :strokeColor(borderColor.top)
                :lineTo(x + width, y)
                :stroke()
                :restore()
        end
        if b.right > 0 then
            doc:save()
                :lineWidth(b.right)
                :moveTo(x + width, y)
                :strokeColor(borderColor.right)
                :lineTo(x + width, y + height)
                :stroke()
                :restore()
        end
        if b.bottom > 0 then
            doc:save()
                :lineWidth(b.bottom)
                :moveTo(x + width, y + height)
                :strokeColor(borderColor.bottom)
                :lineTo(x, y + height)
                :stroke()
                :restore()
        end
        if b.left > 0 then
            doc:save()
                :lineWidth(b.left)
                :moveTo(x, y + height)
                :strokeColor(borderColor.left)
                :lineTo(x, y)
                :stroke()
                :restore()
        end
    end
end

return M
