-- outline.lua -- port of pdfkit/lib/outline.js
--
-- The bookmarks panel. One class serves both the invisible root and every entry
-- under it -- the root is simply the one constructed with no parent, no title
-- and no destination, which leaves its dictionary empty.
--
-- That empty root is not a curiosity: every document builds one during
-- construction whether or not any bookmark is ever added, and it takes an
-- object number when it does. In a default one-page document it is object 4,
-- and it is emitted as a bare `<<>>`. Skip it and every object number after it
-- moves.
--
-- Sibling links are wired up at the end rather than as items are added, because
-- an item cannot know its own successor until the list is closed.

local object = require("pdfkit.object")

local DEFAULT_OPTIONS = {
    top = 0,
    left = 0,
    zoom = 0,
    fit = true, -- Fit by default, for backward compatibility
    pageNumber = nil,
    expanded = false,
}

local PDFOutline = {}
PDFOutline.__index = PDFOutline

function PDFOutline.new(document, parent, title, dest, options)
    local self = setmetatable({}, PDFOutline)

    self.document = document
    self.options = options or DEFAULT_OPTIONS
    self.outlineData = object.dict()

    if dest ~= nil then
        local destWidth = dest.data.MediaBox[3]
        local destHeight = dest.data.MediaBox[4]
        local top = destHeight - (self.options.top or 0)
        local left = destWidth - (self.options.left or 0)
        local zoom = self.options.zoom or 0

        -- Fit scales the whole page into the window; XYZ jumps to a point at a
        -- given magnification. Note that `left` is measured from the RIGHT edge
        -- here, which reads like a bug and is what the original computes.
        if self.options.fit then
            self.outlineData.Dest = object.array({ dest, "Fit" })
        else
            self.outlineData.Dest = object.array({ dest, "XYZ", left, top, zoom })
        end
    end

    if parent ~= nil then
        self.outlineData.Parent = parent
    end

    if title ~= nil then
        self.outlineData.Title = object.str(title)
    end

    self.dictionary = self.document:ref(self.outlineData)
    self.children = {}

    return self
end

function PDFOutline:addItem(title, options)
    options = options or DEFAULT_OPTIONS

    local pages = self.document._root.data.Pages.data.Kids
    local dest
    if options.pageNumber ~= nil then
        -- Page numbers are zero-based on the caller's side.
        dest = pages[options.pageNumber + 1]
    else
        dest = self.document.page.dictionary
    end

    local result = PDFOutline.new(self.document, self.dictionary, title, dest, options)
    self.children[#self.children + 1] = result

    return result
end

-- Named endOutline in the original; kept as such because `end` is a Lua
-- keyword and this one is called by name from the mixin.
function PDFOutline:endOutline()
    if #self.children > 0 then
        if self.options.expanded then
            self.outlineData.Count = #self.children
        end

        self.outlineData.First = self.children[1].dictionary
        self.outlineData.Last = self.children[#self.children].dictionary

        for i, child in ipairs(self.children) do
            if i > 1 then
                child.outlineData.Prev = self.children[i - 1].dictionary
            end
            if i < #self.children then
                child.outlineData.Next = self.children[i + 1].dictionary
            end
            child:endOutline()
        end
    end

    return self.dictionary:finish()
end

return PDFOutline
