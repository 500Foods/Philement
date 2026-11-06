-- table/index.lua -- port of pdfkit/lib/table/index.js
--
-- The table object itself, which is mostly a sequencer: for each row, normalize
-- it, measure it, break the page if it will not fit, draw it, and leave the
-- document's cursor at the bottom of what was drawn. All the thinking lives in
-- the other five files.
--
-- Two ways to use it. Hand the whole thing over at once:
--
--     doc:table({ data = { { "a", "b" }, { "c", "d" } } })
--
-- which draws everything and gives you the document back, ready to chain. Or
-- drive it a row at a time:
--
--     local t = doc:table({ columnStyles = { { width = 100 } } })
--     t:row({ "a", "b" })
--     t:row({ "c", "d" }, true)   -- true marks the last row
--
-- which is what you want when the rows come from somewhere you are streaming.
-- Either way the document is what you end up holding.
--
-- One rename: upstream's `end()` is `finish()` here, because `end` is a Lua
-- keyword and `t:end()` will not parse.

local normalize = require("pdfkit.table.normalize")
local size = require("pdfkit.table.size")
local render = require("pdfkit.table.render")
local accessibility = require("pdfkit.table.accessibility")

local PDFTable = {}
PDFTable.__index = PDFTable

--- Construct a table and, if `opts.data` was given, draw all of it.
---
--- Returns the document rather than the table in the `data` case. Upstream gets
--- that from a JavaScript constructor's right to return something other than
--- the object being built; here it is just an early return, but the observable
--- behaviour is the same and it is what makes `doc:table{data=...}:text(...)`
--- chain.
function PDFTable.new(document, opts)
    local self = setmetatable({}, PDFTable)

    self.document = document
    -- Upstream freezes opts. Lua has no cheap equivalent, and nothing in here
    -- writes to it, so it is simply held as given.
    self.opts = opts or {}

    normalize.normalizeTable(self)
    accessibility.accommodateTable(self)

    self._currRowIndex = 0
    self._ended = false

    if self.opts.data then
        for _, row in ipairs(self.opts.data) do self:row(row) end
        return self:finish()
    end

    return self
end

--- Draw one row.
---
--- Column widths are settled on the first row only -- `ensure` runs once, and
--- from then on every row is fitted to the geometry row zero established.
---
--- @return table self, or the document when `lastRow` is true
function PDFTable:row(row, lastRow)
    if self._ended then
        error(("Table was marked as ended on row %d"):format(self._currRowIndex), 0)
    end

    -- Transform row
    row = normalize.normalizeRow(self, row, self._currRowIndex)
    if self._currRowIndex == 0 then size.ensure(self, row) end
    local newPage, toRender = size.measure(self, row, self._currRowIndex)
    if newPage then self.document:continueOnNewPage() end
    local yPos = render.renderRow(self, toRender, self._currRowIndex)

    -- Position document at base of new row
    self.document.x = self._position.x
    self.document.y = yPos

    if lastRow then return self:finish() end

    self._currRowIndex = self._currRowIndex + 1
    return self
end

--- Mark the table finished and return the document.
---
--- The loop drains the buffer. It is empty unless a rowSpan is still open, in
--- which case the cell is waiting for a row that the caller never wrote -- so
--- empty rows are fed in until it comes out. Each one costs a row of height,
--- which is what a half-finished span should cost.
---
--- @return table the document
function PDFTable:finish()
    while self._rowBuffer and self._rowBuffer:size() > 0 do
        self:row({})
    end
    self._ended = true
    accessibility.accommodateCleanup(self)
    return self.document
end

return PDFTable
