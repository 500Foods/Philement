-- structure_content.lua -- port of pdfkit/lib/structure_content.js
--
-- A handle on marked content in a page's content stream. It is a list rather
-- than a single reference because one logical element -- a paragraph, say --
-- can break across pages and end up as several marked sequences that all belong
-- to the same structure element.

local PDFStructureContent = {}
PDFStructureContent.__index = PDFStructureContent

function PDFStructureContent.new(pageRef, mcid)
    local self = setmetatable({}, PDFStructureContent)
    self.refs = { { pageRef = pageRef, mcid = mcid } }
    return self
end

-- Absorb another handle's references into this one. Used when content
-- continues onto a new page.
function PDFStructureContent:push(structContent)
    for _, ref in ipairs(structContent.refs) do
        self.refs[#self.refs + 1] = ref
    end
end

function PDFStructureContent.is(v)
    return type(v) == "table" and getmetatable(v) == PDFStructureContent
end

return PDFStructureContent
