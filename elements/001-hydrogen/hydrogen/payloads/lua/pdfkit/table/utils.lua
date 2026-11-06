-- table/utils.lua -- port of pdfkit/lib/table/utils.js
--
-- Upstream's utils.js is mostly JSDoc typedefs describing the shape of a table
-- config. Those are documentation, not code, so what survives the port is the
-- four things at the bottom of the file: two field lists, a memoizer and a deep
-- merge. Plus one thing JavaScript gave upstream for free and Lua does not --
-- an insertion-ordered set -- which lives here because the table code is the
-- only place that needs one.
--
-- Function names follow upstream rather than this port's usual snake_case. The
-- table/ directory mirrors pdfkit/lib/table/ file for file, and keeping the
-- names means the two can be read side by side.

local object = require("pdfkit.object")

local M = {}

--------------------------------------------------------------------------------
-- Field lists
--------------------------------------------------------------------------------
--
-- Which style keys belong to a row and which to a column. A default style can
-- carry both, and normalizedDefaultStyle splits it along these two lists.

M.ROW_FIELDS = { "height", "minHeight", "maxHeight" }
M.COLUMN_FIELDS = { "width", "minWidth", "maxWidth" }

-- Membership sets built from the lists above, so the split is a lookup rather
-- than a scan. The lists stay in their array form because that is how they read
-- upstream, and because their order is easier to check against it that way.
local function to_set(list)
    local set = {}
    for _, k in ipairs(list) do set[k] = true end
    return set
end

M.ROW_FIELD_SET = to_set(M.ROW_FIELDS)
M.COLUMN_FIELD_SET = to_set(M.COLUMN_FIELDS)

--------------------------------------------------------------------------------
-- memoize
--------------------------------------------------------------------------------

--- Cache `fn` by its first argument.
---
--- `maxSize` is accepted and ignored, which is not an oversight -- it is what
--- upstream does. Its eviction line reads
---
---     if (cache.size > maxSize) cache.delete(cache.keys().next());
---
--- and `cache.keys().next()` is an iterator *result* -- `{value, done}` -- not
--- the key inside it. Map.delete of a value that is not a key returns false and
--- removes nothing, so the cache never evicts. That matters here: it means
--- `rowStyles(i)` is called exactly once per row for the whole table, however
--- long the table is. A working eviction would re-call it for rows more than
--- ten back, and a caller whose rowStyles function counts its own invocations,
--- or returns something different the second time, would see different output.
--- So the bug is load bearing, and this reproduces it by never evicting at all.
---
--- The nil sentinel is a Lua problem rather than a ported one. `opts.rowStyles`
--- is often an array shorter than the table, so `rowStyles(i)` legitimately
--- returns nothing -- and a Lua table cannot tell "stored nothing" from "never
--- stored". Upstream's Map can: `has` is true where `get` returns undefined.
local NIL = setmetatable({}, { __name = "table.utils.nil" })

function M.memoize(fn, _maxSize)
    local cache = {}
    return function(...)
        local key = ...
        local hit = cache[key]
        if hit == nil then
            local value = fn(...)
            if value == nil then value = NIL end
            cache[key] = value
            hit = value
        end
        if hit == NIL then return nil end
        return hit
    end
end

--------------------------------------------------------------------------------
-- deepMerge
--------------------------------------------------------------------------------

--- Upstream's `isObject`: truthy, of type object, and not an array.
---
--- JavaScript can tell `{}` from `[]`; Lua cannot, and does not need to. An
--- empty table behaves identically under both readings -- merging from it
--- iterates nothing, and cloning it produces an empty table either way -- so
--- the only tables that have to be classified are the non-empty ones, and there
--- a positive length settles it. A tagged PDF array counts as an array too,
--- since a caller may well have written one as a colour.
local function is_object(v)
    if type(v) ~= "table" then return false end
    if object.is_array(v) then return false end
    return #v == 0
end

local function deep_clone(obj)
    if type(obj) ~= "table" then return obj end

    local result = {}
    for k, v in pairs(obj) do
        result[k] = deep_clone(v)
    end
    -- A tagged array stays tagged. Upstream's clone drops the prototype, but
    -- the only thing riding on ours is "render this as a PDF array", and losing
    -- that would turn a caller's colour into something convert() rejects.
    if object.is_array(obj) then
        return object.array(result)
    end
    return result
end

M.deep_clone = deep_clone

--- Merge `sources` onto a clone of `target`, recursing into nested objects.
---
--- Three behaviours here look accidental and are not:
---
---   * A non-object target is returned untouched, sources ignored. Cell styles
---     rely on this -- `deepMerge(1, {...})` is 1.
---   * Arrays are replaced wholesale rather than merged element by element,
---     because `isObject` excludes them. A row style's four-sided border does
---     not half-inherit the column's.
---   * A scalar already in the target beats an object in the source, because
---     the recursive call gets a non-object target and bails at the first line.
---     So `deepMerge({border = 2}, {border = {top = 1}})` keeps 2.
---
--- nil values in a source are skipped, which is upstream's `!== undefined`
--- guard: a style that does not mention a key must not erase the key it is
--- being merged onto.
function M.deepMerge(target, ...)
    if not is_object(target) then return target end
    target = deep_clone(target)

    local sources = table.pack(...)
    for i = 1, sources.n do
        local source = sources[i]
        if is_object(source) then
            for key, value in pairs(source) do
                if is_object(value) then
                    if target[key] == nil then target[key] = {} end
                    target[key] = M.deepMerge(target[key], value)
                elseif value ~= nil then
                    target[key] = deep_clone(value)
                end
            end
        end
    end

    return target
end

--------------------------------------------------------------------------------
-- Ordered set
--------------------------------------------------------------------------------
--
-- The row buffer holds cells that span into later rows, and measure() walks it
-- looking for the ones that finish on the row being drawn. Upstream uses a Set,
-- whose iteration order is insertion order -- and that order decides which cell
-- is measured first, which decides the row height, which decides where every
-- later row starts. A Lua table keyed by cell would iterate in hash order and
-- lay the table out differently on a different run.
--
-- Deleting during iteration is the whole point of the buffer, so `each` walks a
-- snapshot: removals inside the callback take effect on the set but do not
-- disturb the walk. That is what Set.forEach guarantees too.

local OrderedSet = {}
OrderedSet.__index = OrderedSet

function M.ordered_set()
    return setmetatable({ _items = {}, _present = {} }, OrderedSet)
end

function OrderedSet:add(item)
    if self._present[item] then return end
    self._present[item] = true
    self._items[#self._items + 1] = item
end

function OrderedSet:delete(item)
    if not self._present[item] then return end
    self._present[item] = nil
    for i = 1, #self._items do
        if self._items[i] == item then
            table.remove(self._items, i)
            break
        end
    end
end

function OrderedSet:size()
    return #self._items
end

function OrderedSet:each(fn)
    local snapshot = table.move(self._items, 1, #self._items, 1, {})
    for _, item in ipairs(snapshot) do
        fn(item)
    end
end

return M
