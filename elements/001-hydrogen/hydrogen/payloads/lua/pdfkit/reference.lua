-- reference.lua -- port of pdfkit/lib/reference.js
--
-- An indirect object: "N 0 obj ... endobj". A reference is created up front
-- (which reserves its object number) and finalised later, at which point it
-- records its own byte offset in the file for the xref table and writes itself
-- out.
--
-- The newline handling here is fiddly and it decides byte offsets, so it is
-- worth being precise about. The document's writer appends a newline to
-- anything it is given as text, but writes byte payloads through untouched.
-- A stream object therefore finalises as:
--
--     5 0 obj\n              <- text, newline appended
--     << /Length 138 >>\n    <- text, newline appended
--     stream\n               <- text, newline appended
--     ...content bytes...    <- raw, NO newline appended
--     \nendstream\n          <- text with a LEADING newline, newline appended
--     endobj\n
--
-- Because the content already ends with its own newline, the leading newline on
-- "\nendstream" produces a blank line before endstream. That blank line is real
-- and it is in PDFKit's output, so it has to be in ours.

local object = require("pdfkit.object")
local flate = require("pdfkit.flate")

local Reference = {}
Reference.__index = Reference

function Reference.new(document, id, data)
    local self = setmetatable({}, Reference)
    self.document = document
    self.id = id
    self.data = data or object.dict()
    self.gen = 0
    -- A stream that already declares a Filter is left alone; otherwise it
    -- inherits the document's compression setting.
    self.compress = document.compress and (self.data.Filter == nil)
    self.uncompressedLength = 0
    self.buffer = {}
    return self
end

-- Append content to this object's stream. Text chunks gain a trailing newline;
-- pass raw = true for byte payloads (image data, font files) that must go in
-- untouched.
function Reference:write(chunk, raw)
    if not raw then
        chunk = chunk .. "\n"
    end

    self.uncompressedLength = self.uncompressedLength + #chunk

    if self.data.Length == nil then
        self.data.Length = 0
    end

    self.buffer[#self.buffer + 1] = chunk
    self.data.Length = self.data.Length + #chunk

    if self.compress then
        self.data.Filter = "FlateDecode"
    end
end

function Reference:finish(chunk, raw)
    if chunk then
        self:write(chunk, raw)
    end
    self:finalize()
end

function Reference:finalize()
    -- Where this object starts in the file. The xref table is built from these.
    self.offset = self.document._offset

    local encrypt_fn = nil
    if self.document._security then
        encrypt_fn = self.document._security:get_encrypt_fn(self.id, self.gen)
    end

    local body = nil
    if #self.buffer > 0 then
        body = table.concat(self.buffer)

        if self.compress then
            -- PDFKit calls zlib.deflateSync with no options, so this does too:
            -- level 6, 32 KB window, memLevel 8, default strategy.
            body = flate.deflate(body)
        end

        if encrypt_fn then
            body = encrypt_fn(body)
        end

        self.data.Length = #body
    end

    self.document:_write(self.id .. " " .. self.gen .. " obj")
    self.document:_write(object.convert(self.data, encrypt_fn))

    if body then
        self.document:_write("stream")
        self.document:_write(body, true) -- raw: no trailing newline
        self.buffer = {}
        self.document:_write("\nendstream")
    end

    self.document:_write("endobj")
    self.document:_ref_end(self)
end

-- How a reference renders when it appears inside another object.
function Reference:to_pdf_string()
    return self.id .. " " .. self.gen .. " R"
end

return Reference
