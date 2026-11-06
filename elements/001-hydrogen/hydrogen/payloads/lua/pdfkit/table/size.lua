-- table/size.lua -- port of pdfkit/lib/table/size.js
--
-- Where every number in a table comes from. Widths are settled once, on the
-- first row, because a PDF has no reflow -- once column two starts at x=180 it
-- starts there for the whole table. Heights are settled row by row, because a
-- row's height depends on how tall its text turned out.
--
-- ZERO-BASED. `_columnWidths`, `_columnXPos`, `_rowHeights` and `_rowYPos` are
-- all indexed from zero, matching the column and row indices they are keyed by
-- -- which in turn match the ones baked into the accessibility ids. Lua tables
-- hold index 0 perfectly well; what they cannot do is report a length for one,
-- so `#` is never used on these and the column count is carried separately in
-- `_numColumns`.

local utils = require("pdfkit.utils")
local table_utils = require("pdfkit.table.utils")

local cosine = utils.cosine
local sine = utils.sine

local M = {}

--------------------------------------------------------------------------------
-- Column widths
--------------------------------------------------------------------------------

--- Resolve the `*` columns against whatever width the fixed ones left over.
---
--- Star columns are handed out one at a time rather than all at once, and the
--- remaining budget is recomputed after each. That matters when a column is
--- clamped: if the first of three stars is capped by its maxWidth at less than
--- its even share, the leftover is not wasted -- the two after it divide a
--- larger remainder. Handing out `unclaimedWidth / starColCount` up front would
--- leave a gap at the right edge instead.
local function ensureColumnWidths(self, numCols)
    -- Star columns, in ascending column order, each remembering its index.
    local starColumns = {}
    local starMinAcc = 0
    local unclaimedWidth = self._maxWidth

    for i = 0, numCols - 1 do
        local col = self._colStyle(i)
        if col.width == "*" then
            starColumns[#starColumns + 1] = { index = i, col = col }
            starMinAcc = starMinAcc + col.minWidth
        else
            unclaimedWidth = unclaimedWidth - col.width
            self._columnWidths[i] = col.width
        end
    end

    local starColCount = #starColumns

    if starMinAcc >= unclaimedWidth then
        -- Case 1: the columns cannot all fit. A PDF has no horizontal scroll,
        -- so every star column takes its minimum and the table overflows.
        for _, entry in ipairs(starColumns) do
            self._columnWidths[entry.index] = entry.col.minWidth
        end
    elseif starColCount > 0 then
        for _, entry in ipairs(starColumns) do
            local col = entry.col
            local starSize = unclaimedWidth / starColCount
            local width = math.max(starSize, col.minWidth)
            if col.maxWidth > 0 then
                width = math.min(width, col.maxWidth)
            end
            self._columnWidths[entry.index] = width
            unclaimedWidth = unclaimedWidth - width
            starColCount = starColCount - 1
        end
    end

    -- Running left edge of each column.
    local tempX = self._position.x
    self._columnXPos = {}
    for i = 0, numCols - 1 do
        self._columnXPos[i] = tempX
        tempX = tempX + self._columnWidths[i]
    end
end

--- Settle the geometry that lasts for the whole table. Called once, from the
--- first row, because that is the row that tells us how many columns there are.
function M.ensure(self, row)
    local numCols = 0
    for _, cell in ipairs(row) do numCols = numCols + cell.colSpan end

    self._columnWidths = {}
    self._numColumns = numCols
    ensureColumnWidths(self, numCols)

    self._rowHeights = {}
    self._rowYPos = { [0] = self._position.y }
    self._rowBuffer = table_utils.ordered_set()
end

--------------------------------------------------------------------------------
-- Cell geometry
--------------------------------------------------------------------------------

--- Measure one cell against a proposed row height.
---
--- Called twice per cell per row, and the two calls are not independent. The
--- first runs before the row height is known and passes `auto`, which measures
--- the text against a whole page's worth of vertical space; the second runs
--- with the height that measurement produced. In between, the first call has
--- written the text's own measured width and height back into `cell.textOptions`
--- -- and the second call spreads `cell.textOptions` over its freshly computed
--- bounds, so those written-back values win.
---
--- That looks like a bug and behaves like a feature: it is what stops a
--- centred, rotated or right-aligned string from being re-laid-out against the
--- second, smaller box and shifting. Whatever it is, it decides coordinates, so
--- the write-back and the ordering of the spread are both reproduced exactly.
local function measureCell(self, cell, rowHeight)
    -- Cell width: the sum of the columns it spans.
    local cellWidth = 0
    for i = 0, cell.colSpan - 1 do
        cellWidth = cellWidth + self._columnWidths[cell.colIndex + i]
    end

    -- Cell height: the row's, plus the rows above it that it spans back into.
    local cellHeight = rowHeight
    if cellHeight == "auto" then
        -- Effectively infinite, clamped to something a page could hold.
        cellHeight = self.document.page:contentHeight()
    else
        for i = 0, cell.rowSpan - 2 do
            cellHeight = cellHeight + self._rowHeights[cell.rowIndex + i]
        end
    end

    local textAllocatedWidth = cellWidth - cell.padding.left - cell.padding.right
    local textAllocatedHeight = cellHeight - cell.padding.top - cell.padding.bottom

    local rotation = cell.textOptions.rotation or 0
    local textMaxWidth, textMaxHeight =
        M.computeBounds(rotation, textAllocatedWidth, textAllocatedHeight)

    local textOptions = {
        -- Alignment is handled internally, by offsetting where the text starts.
        align = cell.align.x,
        ellipsis = true, -- overflowing text is cut off with an ellipsis
        stroke = cell.textStroke > 0,
        fill = true,     -- to fix the stroke issue
        width = textMaxWidth,
        height = textMaxHeight,
        rotation = rotation,
    }
    -- Anything the caller set overrides all of the above.
    for k, v in pairs(cell.textOptions) do textOptions[k] = v end

    -- Rendered bounds of the text within the cell's constraints.
    local textBounds = { x = 0, y = 0, width = 0, height = 0 }
    if cell.text ~= nil and cell.text ~= "" then
        local doc = self.document
        local rollbackFont = doc._fontSource
        local rollbackFontSize = doc._fontSize
        local rollbackFontFamily = doc._fontFamily
        if cell.font and cell.font.src then doc:font(cell.font.src, cell.font.family) end
        if cell.font and cell.font.size then doc:fontSize(cell.font.size) end

        -- The un-rotated bounds first, to learn how wide the text really is.
        local unrotatedOptions = {}
        for k, v in pairs(textOptions) do unrotatedOptions[k] = v end
        unrotatedOptions.rotation = 0
        local unRotatedTextBounds = doc:boundsOfString(cell.text, 0, 0, unrotatedOptions)

        textOptions.width = unRotatedTextBounds.width
        textOptions.height = unRotatedTextBounds.height

        -- Then the rendered bounds, which for a rotated cell is the horizon-
        -- locked box the rotated text sweeps out.
        textBounds = doc:boundsOfString(cell.text, 0, 0, textOptions)

        doc:font(rollbackFont, rollbackFontFamily, rollbackFontSize)
    end

    local sized = {}
    for k, v in pairs(cell) do sized[k] = v end
    sized.textOptions = textOptions
    sized.x = self._columnXPos[cell.colIndex]
    sized.y = self._rowYPos[cell.rowIndex]
    sized.textX = self._columnXPos[cell.colIndex] + cell.padding.left
    sized.textY = self._rowYPos[cell.rowIndex] + cell.padding.top
    sized.width = cellWidth
    sized.height = cellHeight
    sized.textAllocatedHeight = textAllocatedHeight
    sized.textAllocatedWidth = textAllocatedWidth
    sized.textBounds = textBounds
    return sized
end

--------------------------------------------------------------------------------
-- measure
--------------------------------------------------------------------------------

--- Work out the height of row `rowIndex` and the geometry of every cell that
--- finishes on it.
---
--- The buffer is what makes rowSpan work. Every cell of every row goes in, and
--- comes out again on the row where it ends -- so a cell spanning three rows is
--- buffered by row 0 and measured by row 2, at which point the heights of rows
--- 0 and 1 are known and can be subtracted from what it still needs.
---
--- @return boolean newPage, table toRender
function M.measure(self, row, rowIndex)
    for _, cell in ipairs(row) do self._rowBuffer:add(cell) end

    if rowIndex > 0 then
        self._rowYPos[rowIndex] =
            self._rowYPos[rowIndex - 1] + self._rowHeights[rowIndex - 1]
    end

    local rowStyle = self._rowStyle(rowIndex)

    -- Cells that are to finish rendering on this row.
    local toRender = {}
    self._rowBuffer:each(function(cell)
        if cell.rowIndex + cell.rowSpan - 1 == rowIndex then
            toRender[#toRender + 1] = measureCell(self, cell, rowStyle.height)
            self._rowBuffer:delete(cell)
        end
    end)

    -- The shared height of the row: the tallest of its cells' demands.
    local rowHeight = rowStyle.height
    if rowHeight == "auto" then
        rowHeight = 0
        for _, cell in ipairs(toRender) do
            local minHeight =
                cell.textBounds.height + cell.padding.top + cell.padding.bottom
            -- A spanning cell has already been given the rows above it.
            for i = 0, cell.rowSpan - 2 do
                minHeight = minHeight - self._rowHeights[cell.rowIndex + i]
            end
            rowHeight = math.max(rowHeight, minHeight)
        end
    end

    rowHeight = math.max(rowHeight, rowStyle.minHeight)
    if rowStyle.maxHeight > 0 then
        rowHeight = math.min(rowHeight, rowStyle.maxHeight)
    end
    self._rowHeights[rowIndex] = rowHeight

    local newPage = false
    if rowHeight > self.document.page:contentHeight() then
        -- Taller than a page: there is nowhere to put it, so it is clamped to
        -- what is left and allowed to overflow rather than looping forever
        -- across new pages.
        -- Upstream warns with `new Error(...).stack.slice(7)`, where the slice
        -- strips the leading "Error: " and leaves the message and a trace.
        io.stderr:write(("Row %d requested more than the safe page height, " ..
            "row has been clamped\n"):format(rowIndex))
        self._rowHeights[rowIndex] =
            self.document.page:maxY() - self._rowYPos[rowIndex]
    elseif self._rowYPos[rowIndex] + rowHeight >= self.document.page:maxY() then
        -- It fits on a page, just not on what is left of this one.
        self._rowYPos[rowIndex] = self.document.page.margins.top
        newPage = true
    end

    -- Re-measure now that the height is known.
    local remeasured = {}
    for i, cell in ipairs(toRender) do
        remeasured[i] = measureCell(self, cell, rowHeight)
    end

    return newPage, remeasured
end

--------------------------------------------------------------------------------
-- computeBounds
--------------------------------------------------------------------------------

--- The largest rectangle at angle `rotation` that fits inside an
--- `allocWidth` x `allocHeight` box.
---
--- Rotated text still has to live inside the cell, so the text is not given the
--- cell's width -- it is given the width of the biggest tilted rectangle the
--- cell can contain. Upstream derives it with a large ASCII diagram; the result
--- is that for a box of fixed width AB, the inscribed rectangle of maximum area
--- at angle T has sides
---
---     EF = AB / (2 cos T)      FG = AB / (2 sin T)
---
--- which is the first half below. That derivation only constrains the width, so
--- the answer can be taller than the box; when it is, the second half re-solves
--- with both sides constrained:
---
---     EF = (AB cos T - BC sin T) / (cos^2 T - sin^2 T)
---     FG = (BC cos T - AB sin T) / (cos^2 T - sin^2 T)
---
--- The right angles are special-cased rather than computed, because cosine and
--- sine return exact zeros and ones there and dividing by an exact zero would
--- give infinity where the answer is simply the other side.
---
--- @return number width, number height
function M.computeBounds(rotation, allocWidth, allocHeight)
    local textMaxWidth, textMaxHeight

    local cos = cosine(rotation)
    local sin = sine(rotation)

    if rotation == 0 or rotation == 180 then
        textMaxWidth = allocWidth
        textMaxHeight = allocHeight
    elseif rotation == 90 or rotation == 270 then
        textMaxWidth = allocHeight
        textMaxHeight = allocWidth
    elseif rotation < 90 or (rotation > 180 and rotation < 270) then
        textMaxWidth = allocWidth / (2 * cos)
        textMaxHeight = allocWidth / (2 * sin)
    else
        textMaxHeight = allocWidth / (2 * cos)
        textMaxWidth = allocWidth / (2 * sin)
    end

    -- If the bounding box of the text is beyond allocHeight, clamp it and
    -- recompute against the outer box.
    local EF = sin * textMaxWidth
    local FG = cos * textMaxHeight
    if EF + FG > allocHeight then
        local denominator = cos * cos - sin * sin

        if rotation == 0 or rotation == 180 then
            textMaxWidth = allocWidth
            textMaxHeight = allocHeight
        elseif rotation == 90 or rotation == 270 then
            textMaxWidth = allocHeight
            textMaxHeight = allocWidth
        elseif rotation < 90 or (rotation > 180 and rotation < 270) then
            textMaxWidth = (allocWidth * cos - allocHeight * sin) / denominator
            textMaxHeight = (allocHeight * cos - allocWidth * sin) / denominator
        else
            textMaxHeight = (allocWidth * cos - allocHeight * sin) / denominator
            textMaxWidth = (allocHeight * cos - allocWidth * sin) / denominator
        end
    end

    return math.abs(textMaxWidth), math.abs(textMaxHeight)
end

return M
