-- mixins/outline.lua -- port of pdfkit/lib/mixins/outline.js
--
-- Four lines of wiring. The root outline is created unconditionally at
-- construction time -- and so takes an object number in every document -- but
-- only reaches the catalog if something was actually added to it.

local PDFOutline = require("pdfkit.outline")

local M = {}

function M.initOutline(doc)
    doc.outline = PDFOutline.new(doc, nil, nil, nil)
end

function M.endOutline(doc)
    doc.outline:endOutline()

    if #doc.outline.children > 0 then
        doc._root.data.Outlines = doc.outline.dictionary
        -- Opening on the bookmarks panel, unless the caller already chose a
        -- different page mode.
        doc._root.data.PageMode = doc._root.data.PageMode or "UseOutlines"
        return doc._root.data.PageMode
    end
end

return M
