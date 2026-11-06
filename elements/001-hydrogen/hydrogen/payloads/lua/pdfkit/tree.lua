-- tree.lua -- port of pdfkit/lib/tree.js
--
-- The shared machinery behind name trees and number trees: a sorted key/value
-- map that renders itself as a dictionary. Subclasses supply three things --
-- how keys compare, what the key array is called, and how a key is written.
--
-- The rendering is worth spelling out, because it looks like a mistake and is
-- not. Lines are built with hardcoded two- and four-space indents and joined
-- with newlines, and the closing bracket and dictionary end are NOT indented.
-- An empty tree therefore renders as:
--
--     <<
--       /Names [
--     ]
--     >>
--
-- Every document carries a name tree whether or not anything was added to it,
-- so that ragged shape lands in the bytes of even the simplest one-page file.
-- Tidying it up would be wrong.

local object = require("pdfkit.object")

local Tree = {}
Tree.__index = Tree

-- Subclasses call this for a metatable that inherits the shared behaviour.
function Tree.subclass()
    local cls = {}
    cls.__index = cls
    setmetatable(cls, { __index = Tree })
    return cls
end

-- Initialise the shared fields on an already-constructed subclass instance.
function Tree.init(self, options)
    options = options or {}
    self._items = {}
    self._keys = {} -- insertion order; sorted at render time
    -- /Limits output can be disabled per tree. Attachments do this, because
    -- some readers will not display an embedded-files tree that has them.
    if type(options.limits) == "boolean" then
        self.limits = options.limits
    else
        self.limits = true
    end
    return self
end

function Tree:add(key, val)
    if self._items[key] == nil then
        self._keys[#self._keys + 1] = key
    end
    self._items[key] = val
    return val
end

function Tree:get(key)
    return self._items[key]
end

function Tree:_compareKeys()
    error("must be implemented by subclasses")
end

function Tree:_keysName()
    error("must be implemented by subclasses")
end

function Tree:_dataForKey()
    error("must be implemented by subclasses")
end

function Tree:to_pdf_string()
    local sorted = {}
    for i, k in ipairs(self._keys) do
        sorted[i] = k
    end

    local cls = self
    table.sort(sorted, function(a, b)
        return cls:_compareKeys(a, b) < 0
    end)

    local out = { "<<" }

    if self.limits and #sorted > 1 then
        local first, last = sorted[1], sorted[#sorted]
        out[#out + 1] = "  /Limits " .. object.convert(
            object.array({ self:_dataForKey(first), self:_dataForKey(last) })
        )
    end

    out[#out + 1] = "  /" .. self:_keysName() .. " ["
    for _, key in ipairs(sorted) do
        out[#out + 1] = "    "
            .. object.convert(self:_dataForKey(key))
            .. " "
            .. object.convert(self._items[key])
    end
    out[#out + 1] = "]"
    out[#out + 1] = ">>"

    return table.concat(out, "\n")
end

return Tree
