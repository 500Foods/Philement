-- fontkit/base.lua -- port of fontkit/src/base.js
--
-- Two module-wide settings, kept in their own file so the modules that read them
-- do not have to reach into the one that owns the public API. Upstream's base.js
-- also holds the format registry and the open/create entry points; those live in
-- fontkit/init.lua here, because a registry that the formats register themselves
-- into needs to be loaded after them and Lua has no hoisting to hide that with.

local M = {}

--- Whether a table that fails to decode should say so on stderr.
---
--- False upstream and false here. A font with one broken table is still a usable
--- font, so TTFFont swallows the error and leaves the table undefined; this only
--- controls whether anyone hears about it.
M.logErrors = false

--- The language getName falls back to when no better match is available.
M.defaultLanguage = "en"

function M.setDefaultLanguage(lang)
    M.defaultLanguage = lang or "en"
end

return M
