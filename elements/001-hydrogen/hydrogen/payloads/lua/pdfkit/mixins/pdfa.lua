-- mixins/pdfa.lua -- port of pdfkit/lib/mixins/pdfa.js
--
-- PDF/A is the archival conformance standard: a file that will still render the
-- same way in fifty years. Two things are required of us here -- say which part
-- and conformance level is claimed, and embed a colour profile so that colours
-- mean something absolute rather than "whatever this monitor does".
--
-- The sRGB profile is a Lua module for the same reason the font metrics are.
-- A path beside this file works in a checkout and disappears under luarocks
-- make, which installs the listed modules and nothing next to them.

local object = require("pdfkit.object")
local srgb_profile = require("pdfkit.mixins.srgb_profile")

local M = {}

-- "PDF/A-2b" -> part 2, conformance B. "PDF/A-2" -> part 2, conformance B by
-- default. The test is whether the third character from the end is the hyphen,
-- which distinguishes "-2b" from "-2".
function M.initPDFA(doc, pSubset)
    if pSubset:sub(#pSubset - 2, #pSubset - 2) == "-" then
        doc.subset_conformance = pSubset:sub(#pSubset):upper()
        doc.subset = math.tointeger(tonumber(pSubset:sub(#pSubset - 1, #pSubset - 1)))
    else
        -- Basic conformance when the caller did not say.
        doc.subset_conformance = "B"
        doc.subset = math.tointeger(tonumber(pSubset:sub(#pSubset)))
    end
end

function M.endSubset(doc)
    doc:_addPdfaMetadata()
    doc:_addColorOutputIntent()
end

function M._addColorOutputIntent(doc)
    local iccProfile = srgb_profile

    local colorProfileRef = doc:ref(object.dict({
        { "Length", #iccProfile },
        { "N",      3 }, -- three components: it is an RGB profile
    }))
    colorProfileRef:write(iccProfile, true) -- raw: profile bytes go in untouched
    colorProfileRef:finish()

    local intentRef = doc:ref(object.dict({
        { "Type",                      "OutputIntent" },
        { "S",                         "GTS_PDFA1" },
        { "Info",                      object.str("sRGB IEC61966-2.1") },
        { "OutputConditionIdentifier", object.str("sRGB IEC61966-2.1") },
        { "DestOutputProfile",         colorProfileRef },
    }))
    intentRef:finish()

    doc._root.data.OutputIntents = object.array({ intentRef })
end

function M._getPdfaid(doc)
    return table.concat({
        "\n",
        '        <rdf:Description xmlns:pdfaid="http://www.aiim.org/pdfa/ns/id/" rdf:about="">\n',
        "            <pdfaid:part>", tostring(doc.subset), "</pdfaid:part>\n",
        "            <pdfaid:conformance>", tostring(doc.subset_conformance), "</pdfaid:conformance>\n",
        "        </rdf:Description>\n",
        "        ",
    })
end

function M._addPdfaMetadata(doc)
    doc:appendXML(doc:_getPdfaid())
end

return M
