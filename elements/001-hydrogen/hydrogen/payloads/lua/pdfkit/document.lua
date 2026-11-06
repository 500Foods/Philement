-- document.lua -- port of pdfkit/lib/document.js
--
-- The document owns the object store, the byte counter and the output buffer.
-- Almost everything else in the library is a mixin hung off it.
--
-- Two things in here decide whether the whole file lines up:
--
-- OBJECT NUMBERING. Objects are numbered in creation order, starting at 1, and
-- the constructor creates Pages, Names and Catalog in that order before any
-- mixin runs. So every PDFKit document, however simple, has 1 = Pages,
-- 2 = Names, 3 = Catalog.
--
-- EMISSION ORDER. Objects are *written* in a completely different order, set by
-- finish() below, and each one records the byte offset at which it landed. Get
-- the order wrong and every offset in the xref table is wrong even though every
-- object is individually correct. The order is: the pages (each page writing
-- its dictionary, resources, then content), then the info entries, then the
-- info dictionary, then fonts, then the outline, then the catalog, then Pages,
-- then Names.
--
-- The JS version is a Node readable stream that emits bytes as it goes. Lua has
-- no equivalent, so this buffers the whole document and writes it out when
-- finish() completes. The bytes are identical either way.

local object = require("pdfkit.object")
local utils = require("pdfkit.utils")
local md5 = require("pdfkit.md5")
local Reference = require("pdfkit.reference")
local Page = require("pdfkit.page")
local NameTree = require("pdfkit.name_tree")

local Document = {}
Document.__index = Document

--- The Lua spelling of `x instanceof PDFDocument`. A table's structParent can
--- be either a structure element or the document itself, and the two are
--- attached to the tree by different methods, so the table code has to tell
--- them apart. PDFStructureElement carries the matching `is`.
function Document.is(v)
    return type(v) == "table" and getmetatable(v) == Document
end

--------------------------------------------------------------------------------
-- File ID
--------------------------------------------------------------------------------
--
-- Port of PDFSecurity.generateFileID. The ID is an MD5 over a text rendering of
-- the info dictionary, beginning with the creation timestamp in milliseconds.
-- Because the timestamp is in there, two runs a millisecond apart produce
-- different IDs -- which is exactly why comparisons against PDFKit output
-- normalise this field. Given a fixed CreationDate it is fully reproducible.

local function value_of(v)
    if type(v) == "string" then
        return v
    elseif type(v) == "number" then
        return utils.js_tostring(v)
    elseif type(v) == "table" and getmetatable(v) == object.DATE then
        return tostring(v.ms)
    elseif type(v) == "table" and getmetatable(v) == object.STR then
        return v.value
    end
    return tostring(v)
end

local function generate_file_id(info)
    local parts = { tostring(info.CreationDate.ms), "\n" }
    for key, val in object.dict_pairs(info) do
        parts[#parts + 1] = key .. ": " .. value_of(val) .. "\n"
    end
    return md5.digest(table.concat(parts))
end

--------------------------------------------------------------------------------
-- Construction
--------------------------------------------------------------------------------

local VERSIONS = {
    ["1.4"] = 1.4,
    ["1.5"] = 1.5,
    ["1.6"] = 1.6,
    ["1.7"] = 1.7,
    ["1.7ext3"] = 1.7,
}

function Document.new(options)
    options = options or {}

    local self = setmetatable({}, Document)
    self.options = options

    self.version = VERSIONS[options.pdfVersion] or 1.3

    if options.compress ~= nil then
        self.compress = options.compress
    else
        self.compress = true
    end

    self._pageBuffer = {}
    self._pageBufferStart = 0

    -- The object store. _offsets is indexed by object number and holds the byte
    -- offset each object was written at; false means "created but not yet
    -- written". _waiting counts how many are still outstanding.
    self._offsets = {}
    self._waiting = 0
    self._ended = false
    self._offset = 0
    self._out = {}
    self._stream = nil

    -- Work that PDFKit does on a later tick of the event loop -- see
    -- _defer() and _drain_deferred() below.
    self._deferred = {}

    -- Object 1, 2 and 3, in that order, before anything else can claim a number.
    local Pages = self:ref(object.dict({
        { "Type",  "Pages" },
        { "Count", 0 },
        { "Kids",  object.array() },
    }))

    local Names = self:ref(object.dict({
        { "Dests", NameTree.new() },
    }))

    self._root = self:ref(object.dict({
        { "Type",  "Catalog" },
        { "Pages", Pages },
        { "Names", Names },
    }))

    if options.lang then
        self._root.data.Lang = object.str(options.lang)
    end

    if options.pageLayout then
        local layout = options.pageLayout
        self._root.data.PageLayout = layout:sub(1, 1):upper() .. layout:sub(2)
    end

    self.page = nil

    -- Mixin initialisers, in the order the original runs them. Order matters:
    -- any of these could claim an object number.
    self:initMetadata()
    self:initColor()
    self:initVector()
    self:initFonts(options.font)
    self:initText()
    self:initImages()
    self:initOutline()
    self:initMarkings(options)
    self:initTables()
    self:initSubset(options)

    -- The info dictionary. Insertion order is preserved because it is written
    -- out key by key, and because the file ID hashes it in this order.
    local now = options._now or os.time()
    self.info = object.dict({
        { "Producer",     "PDFKit" },
        { "Creator",      "PDFKit" },
        { "CreationDate", object.date(now, options._nowMs) },
    })

    if options.info then
        -- options.info may be an ordered dict or a plain table; both are
        -- accepted, but only the ordered form gives reproducible key order.
        if object.is_dict(options.info) then
            for key, val in object.dict_pairs(options.info) do
                self.info[key] = val
            end
        else
            for key, val in pairs(options.info) do
                self.info[key] = val
            end
        end
    end

    if options.displayTitle then
        self._root.data.ViewerPreferences = self:ref(object.dict({
            { "DisplayDocTitle", true },
        }))
    end

    self._id = generate_file_id(self.info)

    -- Encryption is only set up when a password is supplied; without one this
    -- stays nil and every encrypt hook downstream is skipped.
    self._security = nil
    if options.ownerPassword or options.userPassword then
        error("pdfkit: encrypted documents are not supported")
    end

    -- The header: version, then four high bytes that mark the file as binary.
    self:_write("%PDF-" .. utils.js_tostring(self.version))
    self:_write("%\255\255\255\255")

    if options.autoFirstPage ~= false then
        self:addPage()
    end

    return self
end

--------------------------------------------------------------------------------
-- Output
--------------------------------------------------------------------------------

-- Where the finished document should be written. `handle` is anything with a
-- :write() method -- normally an open file handle from io.open(path, "wb").
function Document:pipe(handle)
    self._stream = handle
    return self
end

-- Append to the output. Text gains a trailing newline; byte payloads (pass
-- raw = true) go through untouched. Every call advances the byte counter that
-- the xref table is built from, so nothing may bypass this.
function Document:_write(data, raw)
    if not raw then
        data = data .. "\n"
    end
    self._out[#self._out + 1] = data
    self._offset = self._offset + #data
end

-- The finished document as a string, also available after finish() for callers
-- that would rather have the bytes than a file.
function Document:output()
    return table.concat(self._out)
end

function Document:addContent(data)
    self.page:write(data)
    return self
end

--------------------------------------------------------------------------------
-- The object store
--------------------------------------------------------------------------------

function Document:ref(data)
    local ref = Reference.new(self, #self._offsets + 1, data)
    self._offsets[#self._offsets + 1] = false -- placeholder until finalised
    self._waiting = self._waiting + 1
    return ref
end

function Document:_ref_end(ref)
    self._offsets[ref.id] = ref.offset
    self._waiting = self._waiting - 1
    if self._waiting == 0 and self._ended then
        self:_finalize()
        self._ended = false
    end
end

--- Queue work for after the synchronous part of the document is written.
---
--- PDFKit inflates PNG pixel data with zlib's *asynchronous* API, so any image
--- that has to be decoded -- alpha split out, indexed transparency turned into
--- a soft mask, Adam7 undone -- finishes on a later tick of the event loop.
--- Everything written synchronously therefore lands in the file first, and the
--- image objects arrive at the end, just before the xref table. The reference
--- numbers follow the same rule: the XObject's number is claimed up front,
--- during the synchronous call, but any object the deferred half creates (a
--- soft mask, say) claims its number after every synchronous object has.
---
--- Lua's inflate is a plain function call, so that ordering would otherwise be
--- lost. This queue puts it back.
---
--- `cost` is the number of bytes the inflate produced. It decides the order,
--- because Node does not deliver these callbacks in the order they were queued
--- -- see _drain_deferred.
function Document:_defer(fn, cost)
    self._deferred[#self._deferred + 1] = { fn = fn, cost = cost or 0 }
end

--- Run the deferred work in the order Node's event loop would have run it.
---
--- A document with two or more decoded PNGs has two or more inflates in flight
--- at once, and their callbacks do not come back in the order they were
--- started. Node's one-shot zlib.inflate does not decompress in a single
--- threadpool task: it loops, handing back one 16 KiB output chunk per task and
--- re-queueing for the next, so several inflates running together interleave
--- round-robin. The one needing the fewest chunks finishes first, whatever
--- order it was started in, and equal chunk counts keep their original order.
---
--- The chunk count is floor(bytes / 16384) + 1. The `+ 1` is not a rounding-up:
--- a stream whose output is exactly 16384 bytes fills the buffer, cannot tell
--- that it is finished, and costs a second round -- which is measurable, and
--- measured. It falls out of the sort as a constant, but it is why the boundary
--- sits where it does rather than one byte later.
---
--- Two things this deliberately does not model, both outside what PDFKit can
--- produce: an inflate started after the others have already finished (every
--- image embeds during the synchronous run, so they all overlap), and work
--- deferred from inside deferred work (nothing here inflates twice). The drain
--- loop below still picks up anything queued mid-drain, and puts it last.
function Document:_drain_deferred()
    local queue = self._deferred
    self._deferred = {}

    -- Sort by chunk count, ties keeping the order they were queued in.
    -- table.sort is not stable, so the queue position is part of the key.
    local order = {}
    for i = 1, #queue do order[i] = i end
    table.sort(order, function(a, b)
        local ca = queue[a].cost // 16384
        local cb = queue[b].cost // 16384
        if ca ~= cb then return ca < cb end
        return a < b
    end)

    for i = 1, #order do
        queue[order[i]].fn()
    end

    -- Anything queued while draining runs after, on its own turn of the loop.
    if #self._deferred > 0 then
        self:_drain_deferred()
    end
end

--------------------------------------------------------------------------------
-- Pages
--------------------------------------------------------------------------------

function Document:addPage(options)
    if options == nil then
        options = self.options
    end

    -- Unless pages are being buffered, finishing a page happens as soon as the
    -- next one starts.
    if not self.options.bufferPages then
        self:flushPages()
    end

    self.page = Page.new(self, options)
    self._pageBuffer[#self._pageBuffer + 1] = self.page

    local pages = self._root.data.Pages.data
    pages.Kids[#pages.Kids + 1] = self.page.dictionary
    pages.Count = pages.Count + 1

    self.x = self.page.margins.left
    self.y = self.page.margins.top

    -- Flip the coordinate system so the origin is the top left rather than the
    -- bottom left. This is why every content stream opens with a "cm".
    self._ctm = { 1, 0, 0, 1, 0, 0 }
    self:transform(1, 0, 0, -1, 0, self.page.height)

    return self
end

function Document:continueOnNewPage(options)
    local pageMarkings = self:endPageMarkings(self.page)
    self:addPage(options or self.page._options)
    self:initPageMarkings(pageMarkings)
    return self
end

function Document:bufferedPageRange()
    return { start = self._pageBufferStart, count = #self._pageBuffer }
end

function Document:switchToPage(n)
    local page = self._pageBuffer[n - self._pageBufferStart + 1]
    if not page then
        error(("switchToPage(%d) out of bounds, current buffer covers pages %d to %d")
            :format(n, self._pageBufferStart, self._pageBufferStart + #self._pageBuffer - 1))
    end
    self.page = page
    return page
end

function Document:flushPages()
    -- Copied out first so that a reentrant call cannot see a half-drained list.
    local pages = self._pageBuffer
    self._pageBuffer = {}
    self._pageBufferStart = self._pageBufferStart + #pages
    for _, page in ipairs(pages) do
        self:endPageMarkings(page)
        page:finish()
    end
end

--------------------------------------------------------------------------------
-- Named destinations
--------------------------------------------------------------------------------

function Document:addNamedDestination(name, ...)
    -- table.pack rather than `{...}`: the argument list may legitimately hold a
    -- nil, and only `.n` survives that. `#args` would stop at the hole and make
    -- a four-argument call look like a one-argument call.
    local args = table.pack(...)

    if args.n == 0 then
        -- `['XYZ', null, null, null]` -- "this page, keep the current position
        -- and zoom". The three nulls have to be the explicit NULL value, not
        -- Lua nils: a nil here would simply not be in the table, and the
        -- destination would come out as `[page /XYZ]`, which is malformed.
        args = table.pack("XYZ", object.NULL, object.NULL, object.NULL)
    end

    -- A null top coordinate means "leave the vertical position alone"; anything
    -- else is flipped from PDFKit's top-left origin to the PDF's bottom-left.
    if args[1] == "XYZ" and args[3] ~= object.NULL then
        args[3] = self.page.height - args[3]
    end

    -- args.unshift(this.page.dictionary)
    local dest = object.array({ self.page.dictionary })
    for i = 1, args.n do
        dest[i + 1] = args[i]
    end

    self._root.data.Names.data.Dests:add(name, dest)
end

function Document:addNamedEmbeddedFile(name, ref)
    local names = self._root.data.Names.data
    if not names.EmbeddedFiles then
        -- /Limits is disabled here; leaving it in stops attachments showing up
        -- in Adobe Reader.
        names.EmbeddedFiles = NameTree.new({ limits = false })
    end
    names.EmbeddedFiles:add(name, ref)
end

function Document:addNamedJavaScript(name, js)
    local names = self._root.data.Names.data
    if not names.JavaScript then
        names.JavaScript = NameTree.new()
    end
    names.JavaScript:add(name, object.dict({
        { "JS", object.str(js) },
        { "S",  "JavaScript" },
    }))
end

--------------------------------------------------------------------------------
-- Finishing
--------------------------------------------------------------------------------
--
-- Named finish() rather than end() because `end` is a reserved word in Lua.

function Document:finish()
    self:flushPages()

    -- The info dictionary is written as a set of indirect objects, one per key,
    -- each finalised immediately -- so they appear in the file *before* the
    -- dictionary that refers to them.
    self._info = self:ref()
    for key, val in object.dict_pairs(self.info) do
        if type(val) == "string" then
            val = object.str(val)
        end
        local entry = self:ref(val)
        entry:finish()
        self._info.data[key] = entry
    end
    self._info:finish()

    -- Every font that was loaded gets a chance to embed itself. The same font
    -- can be registered under more than one key (its cache key and its
    -- PostScript name), so finalize() is guarded against running twice.
    for _, font in object.dict_pairs(self._fontFamilies) do
        font:finalize()
    end

    self:endOutline()
    self:endMarkings()

    if self.subset then
        self:endSubset()
    end

    self:endMetadata()

    self._root:finish()
    self._root.data.Pages:finish()
    self._root.data.Names:finish()
    self:endAcroForm()

    if self._root.data.ViewerPreferences then
        self._root.data.ViewerPreferences:finish()
    end

    -- Now the event loop would get a turn: the deferred image work runs here,
    -- after every synchronous object has been written.
    self:_drain_deferred()

    if self._waiting == 0 then
        self:_finalize()
    else
        self._ended = true
    end

    return self
end

function Document:_finalize()
    -- The cross-reference table: one fixed-width entry per object, listing the
    -- byte offset it was written at.
    local xRefOffset = self._offset

    self:_write("xref")
    self:_write("0 " .. (#self._offsets + 1))
    self:_write("0000000000 65535 f ")

    for _, offset in ipairs(self._offsets) do
        local padded = ("0000000000" .. tostring(offset or 0)):sub(-10)
        self:_write(padded .. " 00000 n ")
    end

    local trailer = object.dict({
        { "Size", #self._offsets + 1 },
        { "Root", self._root },
        { "Info", self._info },
        { "ID",   object.array({ object.hex(self._id), object.hex(self._id) }) },
    })

    self:_write("trailer")
    self:_write(object.convert(trailer))

    self:_write("startxref")
    self:_write(tostring(xRefOffset))
    self:_write("%%EOF")

    if self._stream then
        self._stream:write(table.concat(self._out))
    end
end

--------------------------------------------------------------------------------
-- Mixins
--------------------------------------------------------------------------------
--
-- The JS does Object.assign(PDFDocument.prototype, methods) for each mixin, so
-- every mixin method becomes a document method taking `this`. Lua's colon call
-- passes the receiver as the first argument, which means a mixin function
-- written as f(doc, ...) drops straight in.

local function mixin(module)
    for k, v in pairs(module) do
        Document[k] = v
    end
end

mixin(require("pdfkit.mixins.metadata"))
mixin(require("pdfkit.mixins.color"))
mixin(require("pdfkit.mixins.vector"))
mixin(require("pdfkit.mixins.fonts"))
mixin(require("pdfkit.mixins.text"))
mixin(require("pdfkit.mixins.images"))
mixin(require("pdfkit.mixins.annotations"))
mixin(require("pdfkit.mixins.outline"))
mixin(require("pdfkit.mixins.markings"))
mixin(require("pdfkit.mixins.acroform"))
mixin(require("pdfkit.mixins.attachments"))
mixin(require("pdfkit.mixins.subsets"))
mixin(require("pdfkit.mixins.table"))

return Document
