-- mixins/pdfua.lua -- port of pdfkit/lib/mixins/pdfua.js
--
-- PDF/UA is the accessibility conformance standard. Declaring it is a single
-- metadata claim; actually meeting it is the caller's job -- tagging content,
-- setting a language, giving images alt text.

local M = {}

function M.initPDFUA(doc)
    doc.subset = 1
end

function M.endSubset(doc)
    doc:_addPdfuaMetadata()
end

function M._addPdfuaMetadata(doc)
    doc:appendXML(doc:_getPdfuaid())
end

function M._getPdfuaid(doc)
    return table.concat({
        "\n",
        '        <rdf:Description xmlns:pdfuaid="http://www.aiim.org/pdfua/ns/id/" rdf:about="">\n',
        "            <pdfuaid:part>", tostring(doc.subset), "</pdfuaid:part>\n",
        "        </rdf:Description>\n",
        "        ",
    })
end

return M
