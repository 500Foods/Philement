-- fontkit/tables/glyf.lua -- port of fontkit/src/tables/glyf.js
--
-- The outlines. Write-only, as it is upstream, where the whole file is three
-- lines under the comment "only used for encoding".
--
-- Reading a glyph does not come through here. `loca` says where each outline
-- starts and glyph/TTFGlyph.lua decodes it from there, one glyph at a time,
-- which is the only sane way to treat a table that is most of the font. What is
-- left for this module is the subset writer's need to name `glyf` in a
-- directory and put a value in it: a list of outlines, already encoded, each
-- either copied verbatim out of the source font or rebuilt with the component
-- glyph ids renumbered.
--
-- The absence of a `decode` is what keeps TTFFont from exposing a `glyf` field.

local M = {}

--- The outlines back to back. Each carries whatever padding the source font
--- gave it, which is why the loca offsets built alongside them stay even.
function M.size(list)
    local n = 0
    for i = 1, #list do
        n = n + #list[i]
    end
    return n
end

function M.encode(w, list)
    for i = 1, #list do
        w:bytes(list[i])
    end
end

return M
