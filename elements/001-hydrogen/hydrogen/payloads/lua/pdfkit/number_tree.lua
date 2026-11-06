-- number_tree.lua -- port of pdfkit/lib/number_tree.js
--
-- A number tree maps integers to objects. The structure tree's parent tree is
-- one: it maps each page's StructParents key to the list of structure elements
-- that claim marked content on that page.
--
-- Keys arrive as numbers and compare numerically, not as text -- so 10 sorts
-- after 9, where a name tree would put it before.

local Tree = require("pdfkit.tree")

local NumberTree = Tree.subclass()

function NumberTree.new(options)
    return Tree.init(setmetatable({}, NumberTree), options)
end

function NumberTree:_compareKeys(a, b)
    local na, nb = tonumber(a) or 0, tonumber(b) or 0
    if na == nb then return 0 end
    return na < nb and -1 or 1
end

function NumberTree:_keysName()
    return "Nums"
end

function NumberTree:_dataForKey(k)
    return math.tointeger(tonumber(k)) or tonumber(k)
end

return NumberTree
