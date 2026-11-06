-- mixins/subsets.lua -- port of pdfkit/lib/mixins/subsets.js
--
-- Conformance subsets are opt-in and mutually exclusive, so rather than always
-- carrying both sets of methods the chosen one is copied onto the document at
-- construction time. Note the consequence: endSubset exists only when a subset
-- was requested, which is why the caller checks before invoking it.

local PDFA = require("pdfkit.mixins.pdfa")
local PDFUA = require("pdfkit.mixins.pdfua")

local M = {}

local PDFA_SUBSETS = {
    ["PDF/A-1"] = true, ["PDF/A-1a"] = true, ["PDF/A-1b"] = true,
    ["PDF/A-2"] = true, ["PDF/A-2a"] = true, ["PDF/A-2b"] = true,
    ["PDF/A-3"] = true, ["PDF/A-3a"] = true, ["PDF/A-3b"] = true,
}

-- Copied onto the instance, not the class: two documents in one program may
-- claim different subsets.
function M._importSubset(doc, subset)
    for k, v in pairs(subset) do
        doc[k] = v
    end
end

function M.initSubset(doc, options)
    local subset = options and options.subset

    if PDFA_SUBSETS[subset] then
        doc:_importSubset(PDFA)
        doc:initPDFA(subset)
    elseif subset == "PDF/UA" then
        doc:_importSubset(PDFUA)
        doc:initPDFUA()
    end
end

return M
