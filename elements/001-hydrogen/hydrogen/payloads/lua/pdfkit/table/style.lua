-- table/style.lua -- port of pdfkit/lib/table/style.js
--
-- Three normalizers, one per level of the style cascade: the table-wide
-- default, a row, and a column. Each takes whatever loose form the caller used
-- and returns the strict form the rest of the table code assumes -- sides
-- always four-keyed, alignment always {x, y}, sizes always points.
--
-- All three mutate the object they were handed before merging it. That is
-- upstream's behaviour and it is visible to a caller who reuses a style object
-- between tables, so it is reproduced rather than tidied away: the second table
-- sees the first table's normalized form. It is harmless because every
-- normalizer here is idempotent -- normalizing an already-normalized side
-- definition or alignment returns it unchanged.

local utils = require("pdfkit.utils")
local table_utils = require("pdfkit.table.utils")

local normalizeSides = utils.normalizeSides
local deepMerge = table_utils.deepMerge

local M = {}

--- A single string means both axes; a table is already the expanded form.
---
--- `nil` expands to a pair of nils, which is an empty table in Lua rather than
--- upstream's `{x: undefined, y: undefined}`. The difference never surfaces:
--- every consumer reads `.x` and `.y` by name, and both readings answer nil.
function M.normalizeAlignment(align)
    if align == nil or type(align) == "string" then
        return { x = align, y = align }
    end
    return align
end

--- Split the table-wide default style into the parts that belong to a row, the
--- parts that belong to a column, and everything else.
---
--- The split happens before the sides are normalized, so `defaultRowStyle` and
--- `defaultColStyle` carry only the raw height/width fields -- which is all
--- they are ever asked for.
---
--- @return table defaultStyle, table defaultRowStyle, table defaultColStyle
function M.normalizedDefaultStyle(defaultStyleInternal)
    local defaultStyle = defaultStyleInternal

    -- Force object form: a bare value is the cell's text.
    if type(defaultStyle) ~= "table" then
        defaultStyle = { text = defaultStyle }
    end

    local defaultRowStyle = {}
    local defaultColStyle = {}
    for k, v in pairs(defaultStyle) do
        if table_utils.ROW_FIELD_SET[k] then defaultRowStyle[k] = v end
        if table_utils.COLUMN_FIELD_SET[k] then defaultColStyle[k] = v end
    end

    defaultStyle.padding = normalizeSides(defaultStyle.padding)
    defaultStyle.border = normalizeSides(defaultStyle.border)
    defaultStyle.borderColor = normalizeSides(defaultStyle.borderColor)
    defaultStyle.align = M.normalizeAlignment(defaultStyle.align)

    return defaultStyle, defaultRowStyle, defaultColStyle
end

--- Normalize the style of row `i`.
---
--- `self` is the table, not the cell, because the row height needs the page.
--- Percentages here are of the page's content height -- a row asking for `50%`
--- wants half the writable page, not half of anything table-shaped.
function M.normalizedRowStyle(self, defaultRowStyle, rowStyleInternal, i)
    local rowStyle = rowStyleInternal(i)

    -- Force object form: a bare value is the row's height.
    if rowStyle == nil or type(rowStyle) ~= "table" then
        rowStyle = { height = rowStyle }
    end

    rowStyle.padding = normalizeSides(rowStyle.padding)
    rowStyle.border = normalizeSides(rowStyle.border)
    rowStyle.borderColor = normalizeSides(rowStyle.borderColor)
    rowStyle.align = M.normalizeAlignment(rowStyle.align)

    rowStyle = deepMerge(defaultRowStyle, rowStyle)

    local document = self.document
    local page = document.page
    local contentHeight = page:contentHeight()

    -- `auto` is not a size, it is an instruction to measure the text, so it is
    -- passed through as the string and resolved later by measure().
    if rowStyle.height == nil or rowStyle.height == "auto" then
        rowStyle.height = "auto"
    else
        rowStyle.height = document:sizeToPoint(rowStyle.height, 0, page, contentHeight)
    end
    rowStyle.minHeight = document:sizeToPoint(rowStyle.minHeight, 0, page, contentHeight)
    rowStyle.maxHeight = document:sizeToPoint(rowStyle.maxHeight, 0, page, contentHeight)

    return rowStyle
end

--- Normalize the style of column `i`.
---
--- Percentages here are of the table's own max width rather than the page's,
--- which is the asymmetry with rows: a column asking for `50%` wants half the
--- table. Upstream flags this with a comment on all three call sites.
function M.normalizedColumnStyle(self, defaultColStyle, colStyleInternal, i)
    local colStyle = colStyleInternal(i)

    -- Force object form: a bare value is the column's width.
    if colStyle == nil or type(colStyle) ~= "table" then
        colStyle = { width = colStyle }
    end

    colStyle.padding = normalizeSides(colStyle.padding)
    colStyle.border = normalizeSides(colStyle.border)
    colStyle.borderColor = normalizeSides(colStyle.borderColor)
    colStyle.align = M.normalizeAlignment(colStyle.align)

    colStyle = deepMerge(defaultColStyle, colStyle)

    local document = self.document

    -- `*` means "share what is left", resolved by ensureColumnWidths.
    if colStyle.width == nil or colStyle.width == "*" then
        colStyle.width = "*"
    else
        colStyle.width = document:sizeToPoint(
            colStyle.width, 0, document.page, self._maxWidth)
    end
    colStyle.minWidth = document:sizeToPoint(
        colStyle.minWidth, 0, document.page, self._maxWidth)
    colStyle.maxWidth = document:sizeToPoint(
        colStyle.maxWidth, 0, document.page, self._maxWidth)

    return colStyle
end

return M
