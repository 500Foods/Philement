-- structure_element.lua -- port of pdfkit/lib/structure_element.js
--
-- One node in the logical structure tree: the thing that makes a PDF say "this
-- is a heading, this is a paragraph, this is a table cell" rather than just
-- "here are some glyphs at these coordinates". Screen readers walk this tree.
--
-- The tricky part is WHEN a node can be written out. An element is flushable
-- only once it has a parent, has been ended, and every one of its children is
-- itself flushable. Any of those can happen last, so _flush() is attempted
-- after each of them and does nothing until all three hold. Flushing early
-- would write a dictionary that is still missing children; flushing late --
-- never -- would leave a dangling reference in the file.
--
-- A child may be four different things, and the difference decides what ends up
-- in /K:
--
--   another element    its dictionary
--   marked content     an MCID integer, or an /MCR dictionary if it sits on a
--                      page other than the element's own /Pg
--   an annotation      an /OBJR dictionary
--   a function         a closure that draws content; called at attach time and
--                      replaced by the marked content it produced

local object = require("pdfkit.object")
local PDFStructureContent = require("pdfkit.structure_content")
local PDFAnnotationReference = require("pdfkit.structure_annotation")

local PDFStructureElement = {}
PDFStructureElement.__index = PDFStructureElement

function PDFStructureElement.is(v)
    return type(v) == "table" and getmetatable(v) == PDFStructureElement
end

local function is_valid_child(child)
    return PDFStructureElement.is(child)
        or PDFStructureContent.is(child)
        or PDFAnnotationReference.is(child)
        or type(child) == "function"
end

-- A list of children, as opposed to an options table. Anything with a first
-- element counts, so both object.array{...} and a plain {a, b} are accepted.
local function is_list(v)
    return type(v) == "table" and not is_valid_child(v) and v[1] ~= nil
end

function PDFStructureElement.new(document, type_, options, children)
    local self = setmetatable({}, PDFStructureElement)

    options = options or {}

    self.document = document
    self._attached = false
    self._ended = false
    self._flushed = false

    self.dictionary = document:ref(object.dict({
        -- /Type is deliberately omitted; it is optional for structure
        -- elements and the original leaves it out.
        { "S", type_ },
    }))

    local data = self.dictionary.data

    -- The options argument doubles as the children argument when it is
    -- obviously children rather than options: either a list, or a single child.
    if is_list(options) or is_valid_child(options) then
        children = options
        options = {}
    end

    if options.title then data.T = object.str(options.title) end
    if options.lang then data.Lang = object.str(options.lang) end
    if options.alt then data.Alt = object.str(options.alt) end
    if options.expanded then data.E = object.str(options.expanded) end
    if options.actual then data.ActualText = object.str(options.actual) end

    local hasBbox = object.is_arrayish(options.bbox) and #options.bbox == 4
    local hasPlacement = type(options.placement) == "string"

    if hasBbox or hasPlacement then
        local attrs = object.dict({ { "O", "Layout" } })
        attrs.Placement = hasPlacement and options.placement or "Block"
        if hasBbox then
            -- The bbox arrives in the caller's top-down coordinates and has to
            -- be flipped into the PDF's bottom-up ones, which swaps the two y
            -- values as well as subtracting them.
            local height = document.page.height
            attrs.BBox = object.array({
                options.bbox[1],
                height - options.bbox[4],
                options.bbox[3],
                height - options.bbox[2],
            })
        end
        data.A = attrs
    end

    if options.scope then
        -- Merge onto whatever /A already holds, preserving its key order; the
        -- JS object spread behaves the same way.
        local merged = object.dict()
        if data.A then
            for k, v in object.dict_pairs(data.A) do
                merged[k] = v
            end
        end
        merged.O = "Table"
        merged.Scope = options.scope
        data.A = merged
    end

    self._children = {}

    if children then
        -- A lone child is accepted in place of a list of one.
        if is_valid_child(children) then
            children = { children }
        end
        for _, child in ipairs(children) do
            self:add(child)
        end
        self:finish()
    end

    return self
end

function PDFStructureElement:add(child)
    if self._ended then
        error("Cannot add child to already-ended structure element")
    end

    if not is_valid_child(child) then
        error("Invalid structure element child")
    end

    if PDFStructureElement.is(child) then
        child:setParent(self.dictionary)
        if self._attached then
            child:setAttached()
        end
    end

    if PDFStructureContent.is(child) then
        self:_addContentToParentTree(child)
    end

    if PDFAnnotationReference.is(child) then
        self:_addAnnotationToParentTree(child.annotationRef)
    end

    -- A closure can only be run once the element is attached to the tree, so an
    -- unattached one is stored as-is and converted later by setAttached.
    if type(child) == "function" and self._attached then
        child = self:_contentForClosure(child)
    end

    self._children[#self._children + 1] = child

    return self
end

function PDFStructureElement:_addContentToParentTree(content)
    for _, ref in ipairs(content.refs) do
        local pageStructParents =
            self.document:getStructParentTree():get(ref.pageRef.data.StructParents)
        -- MCIDs count from zero; Lua tables from one.
        pageStructParents[ref.mcid + 1] = self.dictionary
    end
end

function PDFStructureElement:_addAnnotationToParentTree(annotRef)
    local parentTreeKey = self.document:createStructParentTreeNextKey()
    annotRef.data.StructParent = parentTreeKey
    self.document:getStructParentTree():add(parentTreeKey, self.dictionary)
end

function PDFStructureElement:setParent(parentRef)
    if self.dictionary.data.P then
        error("Structure element added to more than one parent")
    end

    self.dictionary.data.P = parentRef

    self:_flush()
end

function PDFStructureElement:setAttached()
    if self._attached then
        return
    end

    for index, child in ipairs(self._children) do
        if PDFStructureElement.is(child) then
            child:setAttached()
        end
        if type(child) == "function" then
            self._children[index] = self:_contentForClosure(child)
        end
    end

    self._attached = true

    self:_flush()
end

-- Named finish() rather than end(), which is a Lua keyword.
function PDFStructureElement:finish()
    if self._ended then
        return
    end

    for _, child in ipairs(self._children) do
        if PDFStructureElement.is(child) then
            child:finish()
        end
    end

    self._ended = true

    self:_flush()
end

-- Run a closure with this element as the current one, wrapping whatever it
-- draws in a marked content sequence.
function PDFStructureElement:_contentForClosure(closure)
    local content = self.document:markStructureContent(self.dictionary.data.S)

    local prevStructElement = self.document._currentStructureElement
    self.document._currentStructureElement = self

    -- The closure is allowed to add children, so the ended flag is lifted for
    -- the duration and restored afterwards.
    local wasEnded = self._ended
    self._ended = false

    closure()

    self._ended = wasEnded

    self.document._currentStructureElement = prevStructElement
    self.document:endMarkedContent()

    self:_addContentToParentTree(content)

    return content
end

function PDFStructureElement:_isFlushable()
    if not self.dictionary.data.P or not self._ended then
        return false
    end

    for _, child in ipairs(self._children) do
        if type(child) == "function" then
            return false
        end
        if PDFStructureElement.is(child) and not child:_isFlushable() then
            return false
        end
    end

    return true
end

function PDFStructureElement:_flush()
    if self._flushed or not self:_isFlushable() then
        return
    end

    self.dictionary.data.K = object.array({})

    for _, child in ipairs(self._children) do
        self:_flushChild(child)
    end

    self.dictionary:finish()

    -- Release the children now that they are written. The dictionary may still
    -- be referenced by a parent, but the tree below it can go.
    self._children = {}
    self.dictionary.data.K = nil

    self._flushed = true
end

function PDFStructureElement:_flushChild(child)
    local K = self.dictionary.data.K

    if PDFStructureElement.is(child) then
        K[#K + 1] = child.dictionary
    end

    if PDFStructureContent.is(child) then
        for _, ref in ipairs(child.refs) do
            -- The first page seen becomes the element's /Pg. Content on that
            -- page is then just an MCID; content anywhere else has to name its
            -- page explicitly through an /MCR dictionary.
            if not self.dictionary.data.Pg then
                self.dictionary.data.Pg = ref.pageRef
            end

            if self.dictionary.data.Pg == ref.pageRef then
                K[#K + 1] = ref.mcid
            else
                K[#K + 1] = object.dict({
                    { "Type", "MCR" },
                    { "Pg",   ref.pageRef },
                    { "MCID", ref.mcid },
                })
            end
        end
    end

    if PDFAnnotationReference.is(child) then
        K[#K + 1] = object.dict({
            { "Type", "OBJR" },
            { "Obj",  child.annotationRef },
            { "Pg",   self.document.page.dictionary },
        })
    end
end

return PDFStructureElement
