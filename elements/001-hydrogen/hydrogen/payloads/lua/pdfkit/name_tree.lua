-- name_tree.lua -- port of pdfkit/lib/name_tree.js
--
-- A name tree maps strings to objects: named destinations, embedded files,
-- JavaScript actions. Keys are PDF literal strings, and they are sorted by
-- collation rather than by byte value -- see collate.lua for why that
-- distinction is not academic.

local Tree = require("pdfkit.tree")
local object = require("pdfkit.object")
local collate = require("pdfkit.collate")

local NameTree = Tree.subclass()

function NameTree.new(options)
    return Tree.init(setmetatable({}, NameTree), options)
end

function NameTree:_compareKeys(a, b)
    return collate.compare(a, b)
end

function NameTree:_keysName()
    return "Names"
end

-- Keys are emitted as PDF literal strings, not names.
function NameTree:_dataForKey(k)
    return object.str(k)
end

return NameTree
