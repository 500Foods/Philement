-- mixins/attachments.lua -- port of pdfkit/lib/mixins/attachments.js
--
-- Embedding a file inside the PDF. Two objects come out of it: the embedded
-- file stream, and a filespec that names and describes it. The same file
-- attached twice under the same name reuses the stream and only adds a second
-- filespec.
--
-- ONE DELIBERATE DIVERGENCE, and it is the only one in this file. The original
-- reads the source file's creation and modification times off the filesystem
-- and writes them into /Params. Lua has no way to stat a file, so those two
-- keys are omitted unless the caller supplies creationDate or modifiedDate --
-- or unless LuaFileSystem happens to be installed, in which case they are read
-- from it. Making the dates up would be worse than leaving them out: they are
-- optional in the spec, and a fabricated timestamp is a lie that survives in
-- the file forever.

local object = require("pdfkit.object")
local md5 = require("pdfkit.md5")
local base64 = require("pdfkit.base64")

-- Optional. Present it if the host has it; carry on without it if not.
local has_lfs, lfs = pcall(require, "lfs")

local M = {}

-- Two embedded-file bodies describe the same file when their checksums, sizes,
-- subtypes and timestamps all agree.
local function is_equal(a, b)
    local function date_ms(v)
        return v and v.ms or nil
    end

    return a.Subtype == b.Subtype
        and a.Params.CheckSum.value == b.Params.CheckSum.value
        and a.Params.Size == b.Params.Size
        and date_ms(a.Params.CreationDate) == date_ms(b.Params.CreationDate)
        and date_ms(a.Params.ModDate) == date_ms(b.Params.ModDate)
end

--- Embed the contents of `src` in the PDF.
---
--- src      a path to a file, a data: URI, or the bytes themselves
--- options  name          filename shown in the PDF; defaults to src
---          type          MIME type shown in the PDF
---          description   description shown in the PDF
---          hidden        keep it out of the attachments panel -- used by file
---                        annotations, which present it themselves
---          creationDate  object.date, overriding what is on disk
---          modifiedDate  object.date, overriding what is on disk
---          relationship  how the file relates to the document: Alternative,
---                        Data, Source, Supplement or Unspecified
---
--- Returns the filespec reference.
function M.file(doc, src, options)
    options = options or {}
    options.name = options.name or src
    options.relationship = options.relationship or "Unspecified"

    local params = object.dict()
    local refBody = object.dict({
        { "Type",   "EmbeddedFile" },
        { "Params", params },
    })

    local data

    if not src then
        error("No src specified")
    end

    -- A data: URI carries its own bytes and, usually, its own MIME type.
    local mime, payload = src:match("^data:(.-);base64,(.*)$")

    -- Otherwise, anything that opens as a file is a path and anything that does
    -- not is the bytes themselves. pcall because io.open rejects a string
    -- containing a NUL outright, which arbitrary file contents easily will.
    local handle
    if not mime then
        local ok, result = pcall(io.open, src, "rb")
        if ok then handle = result end
    end

    if mime then
        if mime ~= "" then
            refBody.Subtype = object.escape_name(mime)
        end
        data = base64.decode(payload)
    elseif handle then
        data = handle:read("a")
        handle:close()
        if not data then
            error("Could not read contents of file at filepath " .. src)
        end

        if has_lfs then
            local attrs = lfs.attributes(src)
            if attrs then
                params.CreationDate = object.date(attrs.change)
                params.ModDate = object.date(attrs.modification)
            end
        end
    else
        data = src
    end

    if options.creationDate then
        params.CreationDate = options.creationDate
    end
    if options.modifiedDate then
        params.ModDate = options.modifiedDate
    end

    if options.type then
        refBody.Subtype = object.escape_name(options.type)
    end

    params.CheckSum = object.str(md5.hex(data))
    params.Size = #data

    -- Attaching the same file twice under the same name should not embed it
    -- twice.
    doc._fileRegistry = doc._fileRegistry or {}
    local existing = doc._fileRegistry[options.name]

    local ref
    if existing and is_equal(refBody, existing.body) then
        ref = existing.ref
    else
        ref = doc:ref(refBody)
        ref:finish(data, true) -- raw: file bytes go in untouched
        doc._fileRegistry[options.name] = { body = refBody, ref = ref }
    end

    local fileSpecBody = object.dict({
        { "Type",           "Filespec" },
        { "AFRelationship", options.relationship },
        { "F",              object.str(options.name) },
        { "EF",             object.dict({ { "F", ref } }) },
        { "UF",             object.str(options.name) },
    })
    if options.description then
        fileSpecBody.Desc = object.str(options.description)
    end

    local filespec = doc:ref(fileSpecBody)
    filespec:finish()

    if not options.hidden then
        doc:addNamedEmbeddedFile(options.name, filespec)
    end

    -- Also listed on the catalog, which PDF/A-3 requires.
    if doc._root.data.AF then
        local AF = doc._root.data.AF
        AF[#AF + 1] = filespec
    else
        doc._root.data.AF = object.array({ filespec })
    end

    return filespec
end

return M
