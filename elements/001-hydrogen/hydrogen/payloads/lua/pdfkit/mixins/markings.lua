-- mixins/markings.lua -- port of pdfkit/lib/mixins/markings.js
--
-- Marked content: the operators that tag regions of a content stream so the
-- logical structure tree can point at them. BMC/BDC open a sequence, EMC closes
-- one, and every open sequence has to be closed before the page ends.
--
-- Note what initMarkings does NOT do for an untagged document: nothing at all
-- beyond creating an empty list. No MarkInfo dictionary, no structure tree
-- root, no indirect object. That absence is load bearing -- it is part of why a
-- default document has the object count it has.

local object = require("pdfkit.object")
local PDFStructureElement = require("pdfkit.structure_element")
local PDFStructureContent = require("pdfkit.structure_content")
local PDFNumberTree = require("pdfkit.number_tree")

local M = {}

function M.initMarkings(doc, options)
    doc.structChildren = {}

    if options and options.tagged then
        doc:getMarkInfoDictionary().data.Marked = true
        doc:getStructTreeRoot()
    end
end

function M.markContent(doc, tag, options)
    -- An artifact, or anything carrying an MCID, cannot nest inside another
    -- marked sequence -- so any that are open get closed first.
    if tag == "Artifact" or (options and options.mcid) then
        local toClose = 0
        for _, marking in ipairs(doc.page.markings) do
            if toClose > 0 or marking.structContent or marking.tag == "Artifact" then
                toClose = toClose + 1
            end
        end
        while toClose > 0 do
            doc:endMarkedContent()
            toClose = toClose - 1
        end
    end

    if not options then
        doc.page.markings[#doc.page.markings + 1] = { tag = tag }
        doc:addContent("/" .. tag .. " BMC")
        return doc
    end

    doc.page.markings[#doc.page.markings + 1] = { tag = tag, options = options }

    local dictionary = object.dict()

    if options.mcid ~= nil then
        dictionary.MCID = options.mcid
    end

    if tag == "Artifact" then
        if type(options.type) == "string" then
            dictionary.Type = options.type
        end
        if object.is_arrayish(options.bbox) then
            -- Flipped into PDF coordinates, which swaps the y pair.
            dictionary.BBox = object.array({
                options.bbox[1],
                doc.page.height - options.bbox[4],
                options.bbox[3],
                doc.page.height - options.bbox[2],
            })
        end
        if object.is_arrayish(options.attached) then
            local allStrings = true
            for _, val in ipairs(options.attached) do
                if type(val) ~= "string" then
                    allStrings = false
                    break
                end
            end
            if allStrings then
                dictionary.Attached = options.attached
            end
        end
    end

    if tag == "Span" then
        if options.lang then dictionary.Lang = object.str(options.lang) end
        if options.alt then dictionary.Alt = object.str(options.alt) end
        if options.expanded then dictionary.E = object.str(options.expanded) end
        if options.actual then dictionary.ActualText = object.str(options.actual) end
    end

    doc:addContent("/" .. tag .. " " .. object.convert(dictionary) .. " BDC")
    return doc
end

function M.markStructureContent(doc, tag, options)
    options = options or {}

    local pageStructParents =
        doc:getStructParentTree():get(doc.page:structParentTreeKey())

    -- MCIDs number from zero and must be contiguous, so the next one is however
    -- many entries the page already has. The slot is reserved immediately; the
    -- structure element that claims it fills it in later.
    local mcid = #pageStructParents
    pageStructParents[mcid + 1] = object.NULL

    local withMcid = { mcid = mcid }
    for k, v in pairs(options) do
        if k ~= "mcid" then withMcid[k] = v end
    end

    doc:markContent(tag, withMcid)

    local structContent = PDFStructureContent.new(doc.page.dictionary, mcid)
    doc.page.markings[#doc.page.markings].structContent = structContent
    return structContent
end

function M.endMarkedContent(doc)
    table.remove(doc.page.markings)
    doc:addContent("EMC")

    -- Options that only make sense inside the sequence just closed.
    if doc._textOptions then
        doc._textOptions.link = nil
        doc._textOptions.goTo = nil
        doc._textOptions.destination = nil
        doc._textOptions.underline = nil
        doc._textOptions.strike = nil
    end

    return doc
end

function M.struct(doc, type_, options, children)
    return PDFStructureElement.new(doc, type_, options or {}, children)
end

function M.addStructure(doc, structElem)
    local structTreeRoot = doc:getStructTreeRoot()
    structElem:setParent(structTreeRoot)
    structElem:setAttached()
    doc.structChildren[#doc.structChildren + 1] = structElem

    if not structTreeRoot.data.K then
        structTreeRoot.data.K = object.array({})
    end
    local K = structTreeRoot.data.K
    K[#K + 1] = structElem.dictionary

    return doc
end

-- Re-open, on a new page, the marked sequences that were open when the previous
-- page ended. Structure content is re-marked and folded back into the same
-- handle, so a paragraph split across pages stays one logical element.
function M.initPageMarkings(doc, pageMarkings)
    for _, marking in ipairs(pageMarkings) do
        if marking.structContent then
            local structContent = marking.structContent
            local newStructContent = doc:markStructureContent(marking.tag, marking.options)
            structContent:push(newStructContent)
            doc.page.markings[#doc.page.markings].structContent = structContent
        else
            doc:markContent(marking.tag, marking.options)
        end
    end
end

-- Close every sequence still open on the page, and hand the list back so the
-- next page can re-open them.
function M.endPageMarkings(doc, page)
    local pageMarkings = page.markings
    for _ = 1, #pageMarkings do
        page:write("EMC")
    end
    page.markings = {}
    return pageMarkings
end

function M.getMarkInfoDictionary(doc)
    if not doc._root.data.MarkInfo then
        doc._root.data.MarkInfo = doc:ref(object.dict())
    end
    return doc._root.data.MarkInfo
end

function M.hasMarkInfoDictionary(doc)
    return doc._root.data.MarkInfo ~= nil
end

function M.getStructTreeRoot(doc)
    if not doc._root.data.StructTreeRoot then
        doc._root.data.StructTreeRoot = doc:ref(object.dict({
            { "Type",              "StructTreeRoot" },
            { "ParentTree",        PDFNumberTree.new() },
            { "ParentTreeNextKey", 0 },
        }))
    end
    return doc._root.data.StructTreeRoot
end

function M.getStructParentTree(doc)
    return doc:getStructTreeRoot().data.ParentTree
end

function M.createStructParentTreeNextKey(doc)
    -- Touching the MarkInfo dictionary here creates it if it does not exist.
    doc:getMarkInfoDictionary()

    local structTreeRoot = doc:getStructTreeRoot()
    local key = structTreeRoot.data.ParentTreeNextKey
    structTreeRoot.data.ParentTreeNextKey = key + 1
    structTreeRoot.data.ParentTree:add(key, object.array({}))
    return key
end

function M.endMarkings(doc)
    local structTreeRoot = doc._root.data.StructTreeRoot
    if structTreeRoot then
        structTreeRoot:finish()
        for _, structElem in ipairs(doc.structChildren) do
            structElem:finish()
        end
    end
    if doc._root.data.MarkInfo then
        doc._root.data.MarkInfo:finish()
    end
end

return M
