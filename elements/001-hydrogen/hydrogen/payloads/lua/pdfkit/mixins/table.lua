-- mixins/table.lua -- port of pdfkit/lib/mixins/table.js
--
-- Two lines of glue. Everything a table actually does lives in the table/
-- directory; this exists so that `doc:table(...)` reads the same as every other
-- document method.
--
-- The require is deliberately deferred to the call. document.lua mixes this in
-- while it is still being loaded, and the table code needs a finished document
-- module -- so requiring at the top of this file would close a cycle. vector.lua
-- defers its path require for the same reason.

local M = {}

function M.initTables(doc)
    doc._tableIndex = 0
end

--- Start a table. Returns the table object, so rows can be added one at a time
--- -- unless `opts.data` was given, in which case the whole thing is rendered
--- immediately and the document is returned instead.
function M.table(doc, opts)
    local PDFTable = require("pdfkit.table.index")
    return PDFTable.new(doc, opts)
end

return M
