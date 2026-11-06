-- table/accessibility.lua -- port of pdfkit/lib/table/accessibility.js
--
-- The other half of a table: the structure tree that tells a screen reader what
-- the drawn rectangles mean. A visual table is lines and text; a tagged table
-- is a Table element holding TR elements holding TH and TD elements, each
-- carrying an /A dictionary of layout attributes and, for data cells, a list of
-- which header cells apply to them.
--
-- None of this runs unless the caller passed `structParent`. Without it
-- `_tableStruct` is never created and render.lua takes its plain path.

local object = require("pdfkit.object")
local PDFStructureElement = require("pdfkit.structure_element")
local PDFDocument = require("pdfkit.document")

local M = {}

--- Create the Table element and mount it wherever the caller asked.
---
--- `structParent` is either a structure element to nest under or the document
--- to sit at the top of the tree, and those attach by different methods.
function M.accommodateTable(self)
    local structParent = self.opts.structParent
    if structParent then
        self._tableStruct = self.document:struct("Table")
        self._tableStruct.dictionary.data.ID = self._id

        if PDFStructureElement.is(structParent) then
            structParent:add(self._tableStruct)
        elseif PDFDocument.is(structParent) then
            structParent:addStructure(self._tableStruct)
        end

        -- Which cells are headers for which rows and columns. Filled in as the
        -- header cells are rendered, read by every cell after them -- so a
        -- header only covers the cells that come after it, which is why a
        -- header row has to be the first row to work.
        self._headerRowLookup = {}
        self._headerColumnLookup = {}
    end
end

function M.accommodateCleanup(self)
    if self._tableStruct then self._tableStruct:finish() end
end

--- Wrap a row's cells in a TR element.
function M.accessibleRow(self, row, rowIndex, renderCell)
    local rowStruct = self.document:struct("TR")
    rowStruct.dictionary.data.ID = object.str(self._id.value .. "-" .. rowIndex)
    self._tableStruct:add(rowStruct)
    for _, cell in ipairs(row) do
        renderCell(cell, rowStruct)
    end
    rowStruct:finish()
end

--- Wrap one cell in a TH or TD element and draw it inside that element.
---
--- The /A dictionary's key order is the order they are written below, because
--- that is the order the JavaScript object literal lists them and a PDF
--- dictionary keeps whatever order it was built in. Keys whose value would be
--- undefined are skipped rather than written and deleted, which lands in the
--- same place: RowSpan and ColSpan sit between Padding and BorderThickness when
--- they are present and leave no gap when they are not.
function M.accessibleCell(self, cell, rowStruct, callback)
    local doc = self.document

    local cellStruct = doc:struct(cell.type, { title = cell.title })
    cellStruct.dictionary.data.ID = cell.id

    rowStruct:add(cellStruct)

    local padding = cell.padding
    local border = cell.border

    local attributes = object.dict()
    attributes.O = "Table"
    attributes.Width = cell.width
    attributes.Height = cell.height
    attributes.Padding = object.array({
        padding.top, padding.bottom, padding.left, padding.right,
    })
    if cell.rowSpan > 1 then attributes.RowSpan = cell.rowSpan end
    if cell.colSpan > 1 then attributes.ColSpan = cell.colSpan end
    attributes.BorderThickness = object.array({
        border.top, border.bottom, border.left, border.right,
    })

    -- A header cell claims the rows or columns it scopes over, so that the
    -- data cells beneath and beside it can name it as their header.
    if cell.type == "TH" then
        if cell.scope == "Row" or cell.scope == "Both" then
            for i = 0, cell.rowSpan - 1 do
                local key = cell.rowIndex + i
                if not self._headerRowLookup[key] then
                    self._headerRowLookup[key] = {}
                end
                local list = self._headerRowLookup[key]
                list[#list + 1] = cell.id
            end
            attributes.Scope = cell.scope
        end
        if cell.scope == "Column" or cell.scope == "Both" then
            for i = 0, cell.colSpan - 1 do
                local key = cell.colIndex + i
                if not self._headerColumnLookup[key] then
                    self._headerColumnLookup[key] = {}
                end
                local list = self._headerColumnLookup[key]
                list[#list + 1] = cell.id
            end
            attributes.Scope = cell.scope
        end
    end

    -- Any cells marked as headers for this one. Column headers first, then row
    -- headers, de-duplicated by identity -- a cell scoped to Both is registered
    -- in each lookup as the same object and must be named only once.
    local headers = {}
    local seen = {}
    local function collect(lookup, base, span)
        for i = 0, span - 1 do
            local list = lookup[base + i]
            if list then
                for _, id in ipairs(list) do
                    if not seen[id] then
                        seen[id] = true
                        headers[#headers + 1] = id
                    end
                end
            end
        end
    end
    collect(self._headerColumnLookup, cell.colIndex, cell.colSpan)
    collect(self._headerRowLookup, cell.rowIndex, cell.rowSpan)
    if #headers > 0 then
        attributes.Headers = object.array(headers)
    end

    -- Upstream pulls _normalizeColor off the document and calls it detached, so
    -- inside it `this` is undefined. Two of its three string branches never
    -- touch `this`, so hex and named colours work; a registered spot colour
    -- name reaches `this.spotColors[color]` and throws. Passing nil for the
    -- document here reproduces that exactly, error and all -- binding it
    -- properly would make this port accept input PDFKit rejects.
    local normalizeColor = doc._normalizeColor

    -- A value that is not a colour normalizes to nothing, and the attribute is
    -- still written -- as an explicit null, since assigning nil to an ordered
    -- dictionary removes the key instead of setting it.
    local function color_value(color)
        local c = normalizeColor(nil, color)
        if c == nil then return object.NULL end
        return object.array(c)
    end

    if cell.backgroundColor ~= nil then
        attributes.BackgroundColor = color_value(cell.backgroundColor)
    end

    local hasBorder = { border.top, border.bottom, border.left, border.right }
    local anyBorder = false
    for _, v in ipairs(hasBorder) do
        if v and v ~= 0 then anyBorder = true end
    end
    if anyBorder then
        local borderColor = cell.borderColor
        local sides = { borderColor.top, borderColor.bottom,
                        borderColor.left, borderColor.right }
        local out = {}
        for i = 1, 4 do
            -- A side with no border gets an explicit null rather than a colour,
            -- so the four entries still line up with the four sides.
            if hasBorder[i] and hasBorder[i] ~= 0 then
                out[i] = color_value(sides[i])
            else
                out[i] = object.NULL
            end
        end
        attributes.BorderColor = object.array(out)
    end

    cellStruct.dictionary.data.A = doc:ref(attributes)
    cellStruct:add(callback)
    cellStruct:finish()
    cellStruct.dictionary.data.A:finish()
end

return M
