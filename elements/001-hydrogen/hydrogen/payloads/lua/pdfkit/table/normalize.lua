-- table/normalize.lua -- port of pdfkit/lib/table/normalize.js
--
-- Turns the loose config a caller writes into the strict shape the sizing and
-- rendering code assumes. Three levels, applied outward in:
--
--   normalizeTable   once, at construction -- position, width, and the two
--                    style lookups the rest of the table will call
--   normalizeRow     once per row -- walks the cells, skipping the columns
--                    already claimed by a span from an earlier row
--   normalizeCell    once per cell -- runs the whole style cascade and lands on
--                    a config with every field present and every size in points
--
-- ONE-BASED LISTS. Upstream indexes `columnStyles` and `rowStyles` from zero,
-- because they are JavaScript arrays. A Lua caller writing a list gets 1, 2, 3,
-- so the list form is indexed from one here -- `columnStyles[1]` styles the
-- first column. The callback form is handed the same number the list form would
-- have used, which keeps the two interchangeable: passing `function(i) return
-- styles[i] end` behaves exactly like passing `styles`.
--
-- The internal column and row indices stay zero-based, because they are not
-- list positions -- they are part of the generated accessibility ids
-- (`table-0-1-2`), and those have to match what PDFKit emits.

local utils = require("pdfkit.utils")
local object = require("pdfkit.object")
local table_utils = require("pdfkit.table.utils")
local style = require("pdfkit.table.style")

local normalizeSides = utils.normalizeSides
local deepMerge = table_utils.deepMerge

local M = {}

--- Pull the text out of a wrapped PDF string, if that is what this is.
--- Upstream gets this from `new String(x)` coercing its argument; the `_id` is
--- a wrapped string and it gets interpolated into every cell id.
local function unwrap(v)
    if type(v) == "table" and getmetatable(v) == object.STR then
        return v.value
    end
    return v
end

--------------------------------------------------------------------------------
-- normalizeTable
--------------------------------------------------------------------------------

function M.normalizeTable(self)
    local doc = self.document
    local opts = self.opts

    local index = doc._tableIndex
    doc._tableIndex = index + 1

    -- A PDF literal string, not a name: it ends up as /ID in the structure
    -- tree, where it has to render as (table-0) rather than /table-0.
    self._id = object.str(opts.id ~= nil and unwrap(opts.id) or ("table-" .. index))

    local position = opts.position
    self._position = {
        x = doc:sizeToPoint(position and position.x, doc.x),
        y = doc:sizeToPoint(position and position.y, doc.y),
    }

    -- Default width is whatever is left between the table's left edge and the
    -- right margin.
    self._maxWidth = doc:sizeToPoint(
        opts.maxWidth,
        doc.page.width - doc.page.margins.right - self._position.x
    )

    local defaultStyle, defaultRowStyle, defaultColStyle =
        style.normalizedDefaultStyle(opts.defaultStyle)
    self._defaultStyle = defaultStyle

    -- The three accepted forms of columnStyles collapse into one function of a
    -- column index. A list picks per column, a function computes per column,
    -- and a single style object applies to every column alike.
    local colStyle
    if opts.columnStyles ~= nil then
        if type(opts.columnStyles) == "function" then
            -- Memoized so a caller's function is asked about each column once.
            colStyle = table_utils.memoize(function(i)
                return opts.columnStyles(i)
            end, math.huge)
        elseif object.is_arrayish(opts.columnStyles) and #opts.columnStyles > 0 then
            colStyle = function(i) return opts.columnStyles[i + 1] end
        elseif type(opts.columnStyles) == "table" then
            colStyle = function() return opts.columnStyles end
        end
    end
    if not colStyle then colStyle = function() return {} end end
    self._colStyle = function(i)
        return style.normalizedColumnStyle(self, defaultColStyle, colStyle, i)
    end

    local rowStyle
    if opts.rowStyles ~= nil then
        if type(opts.rowStyles) == "function" then
            -- Upstream asks for a rolling buffer of ten here and does not get
            -- one; see memoize. Nothing caches more than the whole table.
            rowStyle = table_utils.memoize(function(i)
                return opts.rowStyles(i)
            end, 10)
        elseif object.is_arrayish(opts.rowStyles) and #opts.rowStyles > 0 then
            rowStyle = function(i) return opts.rowStyles[i + 1] end
        elseif type(opts.rowStyles) == "table" then
            rowStyle = function() return opts.rowStyles end
        end
    end
    if not rowStyle then rowStyle = function() return {} end end
    self._rowStyle = function(i)
        return style.normalizedRowStyle(self, defaultRowStyle, rowStyle, i)
    end
end

--------------------------------------------------------------------------------
-- normalizeText
--------------------------------------------------------------------------------

--- nil stays nil, because a cell with no text is not the same as a cell with
--- empty text -- the first draws nothing, the second still runs the renderer.
--- Everything else is stringified the way JavaScript would, which for numbers
--- means 1 rather than 1.0.
function M.normalizeText(text)
    if text == nil then return nil end
    if type(text) == "string" then return text end
    if type(text) == "number" then return utils.js_tostring(text) end
    if type(text) == "boolean" then return text and "true" or "false" end
    return tostring(text)
end

--------------------------------------------------------------------------------
-- normalizeCell
--------------------------------------------------------------------------------

--- Run the style cascade for one cell and return its finished config.
---
--- The cascade is cell over row over column over table default, and it is a
--- deep merge at every step, so a cell can override just the top border and
--- inherit the other three from its column.
---
--- The font dance in the middle is the subtle part. A cell with its own font
--- has to be measured in that font, and sizes given in `em` resolve against
--- whatever font is current -- so the font is installed on the document before
--- the row style is re-read, and rolled back at the end. Without the re-read a
--- row height of `2em` would mean two of the *previous* cell's ems.
function M.normalizeCell(self, cell, rowIndex, colIndex)
    local colStyle = self._colStyle(colIndex)
    local rowStyle = self._rowStyle(rowIndex)

    -- Upstream counts the non-null values in the merged font; deepMerge never
    -- writes a nil, so any key at all means one of the three levels asked for a
    -- font of its own.
    local font = deepMerge({}, colStyle.font, rowStyle.font, cell.font)
    local customFont = next(font) ~= nil

    local doc = self.document

    local rollbackFont = doc._fontSource
    local rollbackFontSize = doc._fontSize
    local rollbackFontFamily = doc._fontFamily
    if customFont then
        if font.src then doc:font(font.src, font.family) end
        if font.size then doc:fontSize(font.size) end

        -- Refetch rowStyle to reflect font changes
        rowStyle = self._rowStyle(rowIndex)
    end

    cell.padding = normalizeSides(cell.padding)
    cell.border = normalizeSides(cell.border)
    cell.borderColor = normalizeSides(cell.borderColor)

    -- Cell takes highest priority, then row, then column, then defaultConfig
    local config = deepMerge(self._defaultStyle, colStyle, rowStyle, cell)
    config.rowIndex = rowIndex
    config.colIndex = colIndex
    config.font = font
    config.customFont = customFont

    config.text = M.normalizeText(config.text)
    if config.rowSpan == nil then config.rowSpan = 1 end
    if config.colSpan == nil then config.colSpan = 1 end
    config.padding = normalizeSides(config.padding, "0.25em", function(x)
        return doc:sizeToPoint(x, "0.25em")
    end)
    config.border = normalizeSides(config.border, 1, function(x)
        return doc:sizeToPoint(x, 1)
    end)
    config.borderColor = normalizeSides(config.borderColor, "black", function(x)
        if x == nil then return "black" end
        return x
    end)
    config.align = style.normalizeAlignment(config.align)
    if config.align.x == nil then config.align.x = "left" end
    if config.align.y == nil then config.align.y = "top" end
    config.textStroke = doc:sizeToPoint(config.textStroke, 0)
    if config.textStrokeColor == nil then config.textStrokeColor = "black" end
    if config.textColor == nil then config.textColor = "black" end
    if config.textOptions == nil then config.textOptions = {} end

    -- Accessibility settings
    config.id = object.str(
        config.id ~= nil and unwrap(config.id)
        or (self._id.value .. "-" .. rowIndex .. "-" .. colIndex)
    )
    config.type = (type(config.type) == "string" and config.type:upper() == "TH")
        and "TH" or "TD"
    if config.scope ~= nil and config.scope ~= "" then
        -- Lowercased first so that any spelling of the three known scopes is
        -- accepted, then title-cased back to what the PDF spec wants. Anything
        -- unrecognised is left lowercased, exactly as upstream leaves it.
        config.scope = config.scope:lower()
        if config.scope == "row" then config.scope = "Row"
        elseif config.scope == "both" then config.scope = "Both"
        elseif config.scope == "column" then config.scope = "Column" end
    end

    -- A table-wide debug flag overrides whatever the cascade decided.
    if type(self.opts.debug) == "boolean" then config.debug = self.opts.debug end

    if customFont then
        doc:font(rollbackFont, rollbackFontFamily, rollbackFontSize)
    end

    return config
end

--------------------------------------------------------------------------------
-- normalizeRow
--------------------------------------------------------------------------------

--- Normalize every cell in a row, assigning each one its column.
---
--- Columns are not simply counted off, because a rowSpan from an earlier row is
--- still occupying its column on this one. Every cell claims the whole
--- rectangle it spans when it is normalized, and later rows walk past anything
--- claimed. So a caller writing a row of three cells under a 2-row-tall first
--- column really does write three cells -- the skipping is automatic.
function M.normalizeRow(self, row, rowIndex)
    if not self._cellClaim then self._cellClaim = {} end

    local colIndex = 0
    local out = {}

    for n = 1, #row do
        local cell = row[n]

        -- Ensure TableCell: a bare value is the cell's text.
        if cell == nil or type(cell) ~= "table" then cell = { text = cell } end

        while self._cellClaim[rowIndex .. "," .. colIndex] do
            colIndex = colIndex + 1
        end

        cell = M.normalizeCell(self, cell, rowIndex, colIndex)

        for i = 0, cell.rowSpan - 1 do
            for j = 0, cell.colSpan - 1 do
                self._cellClaim[(rowIndex + i) .. "," .. (colIndex + j)] = true
            end
        end

        colIndex = colIndex + cell.colSpan
        out[n] = cell
    end

    return out
end

return M
